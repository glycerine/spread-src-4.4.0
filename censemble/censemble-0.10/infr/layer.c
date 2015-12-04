/**************************************************************/
/* LAYER.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/event.h"
#include "infr/sched.h"
#include "infr/layer.h"
#include "infr/config.h"
#ifdef LAYER_DYNLINK
#include <dlfcn.h>
#include <stdlib.h>
#include <ctype.h>
#endif

static string_t name UNUSED() = "LAYER" ;

typedef struct layer_def_t {
    string_t name ;
    int state_size ;
    layer_init_t init ;
    layer_free_t free ;
    layer_dump_t dump ;
    layer_up_handler_t up ;
    layer_upnm_handler_t upnm ;
    layer_dn_handler_t dn ;
    layer_dnnm_handler_t dnnm ;
} *layer_def_t ;

struct layer_t {
    int count ;
    sched_t sched ;
    layer_def_t def ;
    layer_t up ;
    layer_t dn ;
} ;

typedef struct state_t {
    layer_array_t layers ;
    len_t nlayers ;
    layer_t wrapper ;
    view_local_t ls ;
    view_state_t vs ;
} *state_t ;

layer_array_t layer_array_create(len_t) ;

static inline void *layer_state(layer_t l) {
    return l + 1 ;
}

#define MAX_LAYERS 30
layer_def_t layers[MAX_LAYERS] ;

static layer_def_t layer_of_string(string_t name) {
    int i ;
    /*eprintf("layer_of_string:%s\n", name) ;*/
    for (i=0;i<MAX_LAYERS;i++) {
	if (layers[i] &&
	    string_eq(name, layers[i]->name)) {
	    break ;
	}
    }
    if (i < MAX_LAYERS) {
	return layers[i] ;
    } else {
	return NULL ;
    }
}

static inline void layer_check(layer_t l) {
    assert(l) ;
    assert(layer_state(l)) ;
    assert(l->def) ;
    assert(l->def->up) ;
    assert(l->def->upnm) ;
    assert(l->def->dn) ;
    assert(l->def->dnnm) ;
}

void
layer_register(
        string_t name,
	int state_size,
	layer_init_t init,
	layer_up_handler_t up,
	layer_upnm_handler_t upnm,
	layer_dn_handler_t dn,
	layer_dnnm_handler_t dnnm,
	layer_free_t free,
	layer_dump_t dump
) {
    int i ;
    layer_def_t l ;
    for(i=0;i<MAX_LAYERS;i++) {
	if (!layers[i]) break ;
    }
    assert(i < MAX_LAYERS) ;
    l = record_create(layer_def_t, l) ;
    l->name = name ;
    l->state_size = state_size ;
    l->init = init ;
    l->free = free ;
    l->dump = dump ;
    l->up = up ;
    l->upnm = upnm ;
    l->dn = dn ;
    l->dnnm = dnnm ;
    layers[i] = l ;
}

static void cleanup(env_t env) {
    state_t s = env ;
    ofs_t ofs ;
    assert_array_length(s->layers, s->nlayers) ;
    for (ofs=0;ofs<s->nlayers;ofs++) {
	layer_t l = array_get(s->layers, ofs) ;
	l->def->free(layer_state(l)) ;
	sys_free(l) ;
    }
    s->wrapper->def->free(layer_state(s->wrapper)) ;
    sys_free(s->wrapper) ;
    view_state_free(s->vs) ;
    view_local_free(s->ls) ;
    array_free(s->layers) ;
    sys_free(s) ;
}

void layer_compose(
	layer_state_t s,
	const endpt_id_t *endpt,
	view_state_t vs
) {
    proto_layer_array_t protos ;
    layer_array_t layers ;
    view_local_t ls ;
    layer_t wrapper ;
    len_t len ;
    ofs_t i ;
    alarm_t alarm = s->alarm ;

    {
	/* We take the given reference, but use a copy
	 * so that bugs will be caught quickly.
	 */
	view_state_t tmp = vs ;
	vs = view_state_copy(vs) ;
	view_state_free(tmp) ;
    }
    ls = view_local(endpt, vs) ;

    protos = proto_layers_of_id(vs->proto, &len) ;
    assert(protos) ;
    assert(len > 0) ;
    layers = layer_array_create(len) ;

    for (i=0;i<len;i++) {
	layer_def_t def ;
	layer_t l ;
	string_t name = proto_string_of_layer(array_get(protos, i)) ;
	def = layer_of_string(name) ;

#ifdef LAYER_DYNLINK
	if (!def) {
	    void *handle ;
	    char *pathname ;
	    char basename[100] ;
	    string_t fullpathname ;
	    string_t registerfun ;
	    int i ;
	    
	    pathname = getenv("CENS_ROOT") ;
	    if (!pathname) {
		sys_panic("LAYER:CENS_ROOT environment variable undefined") ;
	    }
	    
	    strncpy(basename, name, sizeof(basename)) ;
	    basename[sizeof(basename)-1] = '\0' ;
	    for (i=0;basename[i];i++) {
		basename[i] = tolower(basename[i]) ;
	    }

	    fullpathname = util_sprintf("%s/layers/%s.so", pathname, basename) ;
	    registerfun = util_sprintf("%s_register", basename) ;
	    //eprintf("fullpathname='%s'\n", fullpathname) ;
	    
	    handle = dlopen(fullpathname, RTLD_NOW) ;
	    if (!handle) {
		eprintf("LAYER:problem loading layer module '%s':%s\n", 
			name, dlerror()) ;
	    } else {
		void (*do_register)(void) ;
		do_register = dlsym(handle, registerfun) ;
		if (!do_register) {
		    eprintf("LAYER:problem loading layer module '%s':%s\n", 
			    name, dlerror()) ;
		    sys_panic("LAYER:missing layer register function") ;
		}
		do_register() ;
	    }

	    string_free(registerfun) ;
	    string_free(fullpathname) ;

	    /* Try again.
	     */
	    def = layer_of_string(name) ;
	}
#endif

	if (!def) {
	    sys_panic(("no such layer '%s'", name)) ;
	}
	assert(def) ;
	l = sys_alloc(sizeof(*l) + def->state_size) ;
	array_set(layers, i, l) ;
	l->sched = alarm_sched(alarm) ;
	l->count = 0 ;
	l->def = def ;
	l->def->init(layer_state(l), l, s, ls, vs) ;
    }

    for (i=0;i<len;i++) {
	sys_free(array_get(protos, i)) ;
    }
    array_free(protos) ;
    protos = NULL ;

    {
	layer_def_t def ;
	def = layer_of_string("WRAPPER") ;
	assert(def) ;
	wrapper = sys_alloc(sizeof(*wrapper) + def->state_size) ;
	wrapper->sched = alarm_sched(alarm) ;
	wrapper->def = def ;
	wrapper->def->init(layer_state(wrapper), wrapper, s, ls, vs) ;
	wrapper->up = array_get(layers, 0) ;
	wrapper->dn = array_get(layers, len - 1) ;
	wrapper->count = 0 ;
    }

    {
	extern void wrapper_quiesce(env_t env, void (*fun)(void *), void *arg) ;
	state_t s = record_create(state_t, s) ;
	s->ls = ls ;
	s->vs = vs ;
	s->layers = layers ;
	s->nlayers = len ;
	s->wrapper = wrapper ;
	assert_array_length(s->layers, s->nlayers) ;
	wrapper_quiesce(layer_state(wrapper), cleanup, s) ;
    }

    for (i=0;i<len;i++) {
	layer_t l = array_get(layers, i) ;
	if (i == 0) {
	    l->dn = wrapper ;
	} else {
	    l->dn = array_get(layers, i - 1) ;
	}
	if (i == len-1) {
	    l->up = wrapper ;
	} else {
	    l->up = array_get(layers, i + 1) ;
	}
    }

    {
	layer_t l = array_get(layers, 0) ;
	event_t e ;
	e = event_create(EVENT_INIT) ;
	e = event_set_time(e, alarm_gettime(alarm)) ;
	sched_enqueue_2arg(l->sched, (void*)l->def->upnm, layer_state(l), e) ;
    }
}

void
up_free(
	event_t e,
        unmarsh_t m
) {
    event_free(e) ;
    unmarsh_free(m) ;
}

void
dn_free(
	event_t e,
	marsh_t m
) {
    event_free(e) ;
    marsh_free(m) ;
}

static inline void count_decr(layer_t l) {
    assert(l->count > 0) ;
    l->count -- ;
}

static inline void count_incr(layer_t l) {
    assert(l->count >= 0) ;
    l->count ++ ;
}

static inline void
up_handler(
	layer_t l,
	event_t e,
	unmarsh_t m
) {
    l->def->up(layer_state(l), e, m) ;
    count_decr(l) ;
}

void
layer_up(
	layer_t l,
	event_t e,
        unmarsh_t m
) {
    layer_t d = l->up ;
    layer_check(d) ;
    count_incr(d) ;
#ifdef LAYER_SCHED_FAST
    if (d->count > 1) {
	sched_enqueue_3arg(d->sched, (void*)up_handler, d, e, m) ;
    } else {
	up_handler(d, e, m) ;
    }
#else
    sched_enqueue_3arg(d->sched, (void*)up_handler, d, e, m) ;
#endif
}

static inline void
upnm_handler(
	layer_t l,
	event_t e
) {
    l->def->upnm(layer_state(l), e) ;
    count_decr(l) ;
}

void
layer_upnm(
	layer_t l,
	event_t e
) {
    layer_t d = l->up ;
    layer_check(d) ;
    count_incr(d) ;
#ifdef LAYER_SCHED_FAST
    if (d->count > 1) {
	sched_enqueue_2arg(d->sched, (void*)upnm_handler, d, e) ;
    } else {
	upnm_handler(d, e) ;
    }
#else
    sched_enqueue_2arg(d->sched, (void*)upnm_handler, d, e) ;
#endif
}

static inline void
dn_handler(
	layer_t l,
	event_t e,
	marsh_t a
) {
    l->def->dn(layer_state(l), e, a) ;
    count_decr(l) ;
}

void
layer_dn(
	layer_t l,
	event_t e,
	marsh_t a
) {
    layer_t d = l->dn ;
    layer_check(d) ;
    count_incr(d) ;
#ifdef LAYER_SCHED_FAST
    if (d->count > 1) {
	sched_enqueue_3arg(d->sched, (void*)dn_handler, d, e, a) ;
    } else {
	dn_handler(d, e, a) ;
    }
#else
    sched_enqueue_3arg(d->sched, (void*)dn_handler, d, e, a) ;
#endif
}

static inline void
dnnm_handler(
	layer_t l,
	event_t e
) {
    l->def->dnnm(layer_state(l), e) ;
    count_decr(l) ;
}

void
layer_dnnm(
	layer_t l,
	event_t e
) {
    layer_t d = l->dn ;
    layer_check(d) ;
    count_incr(d) ;
#ifdef LAYER_SCHED_FAST
    if (d->count > 1) {
	sched_enqueue_2arg(d->sched, (void*)dnnm_handler, d, e) ;
    } else {
	dnnm_handler(d, e) ;
    }
#else
    sched_enqueue_2arg(d->sched, (void*)dnnm_handler, d, e) ;
#endif
}

#include "infr/array_supp.h"
ARRAY_CREATE(layer)
