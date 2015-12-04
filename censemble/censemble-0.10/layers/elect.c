/**************************************************************/
/* ELECT.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "ELECT" ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_t dn_elect ;
    bool_t up_elect ;
    const_bool_array_t suspects ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
  sprintf "dn_elect=%b, up_elect=%b, suspects=%s\n" 
    (Once.isset s.dn_elect) (Once.isset s.up_elect) 
    (Arrayf.bool_to_string s.suspects)
#endif
}
#endif

static void init(
        state_t s,
        layer_t layer,
	layer_state_t state,
	view_local_t ls,
	view_state_t vs
) {
    s->ls = ls ; 
    s->vs = vs ;
    s->layer = layer ;
    s->dn_elect = FALSE ;
    s->up_elect = FALSE ;
    s->suspects = bool_array_create_init(vs->nmembers, FALSE) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    up(s, e, abv) ;
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_FAIL: {
	const_bool_array_t tmp = s->suspects ;
	s->suspects = bool_array_map_or(s->suspects, event_failures(e), s->vs->nmembers) ;
	array_free(tmp) ;
	upnm(s, e) ;
    } break ;

    case EVENT_ELECT:
	s->up_elect = TRUE ;
	upnm(s, e) ;
	break ;

    case EVENT_INIT:
	/* EInit: If I'm marked as coord in the view state then
	 * I start out as coordinator.
	 */
	upnm(s, e) ;
	if (s->ls->rank == s->vs->coord) {
	    s->dn_elect = TRUE ;
	    dnnm(s, event_create(EVENT_ELECT)) ;
	}
	break ;

    case EVENT_SUSPECT: {
	/* ESuspect: Some members are suspected to be faulty.
	 * Check with the policy module about what to do.
	 */
	const_bool_array_t tmp = s->suspects ;
	s->suspects = bool_array_map_or(s->suspects, event_suspects(e), s->vs->nmembers) ;
	array_free(tmp) ;
	assert(!array_get(s->suspects, s->ls->rank)) ;
	    
	if (!s->dn_elect &&
	    bool_array_min_false(s->suspects, s->vs->nmembers) >= s->ls->rank) {
	    log(("reporting:'%s':%s", "loc", bool_array_to_string(s->suspects, s->vs->nmembers))) ;
	    s->dn_elect = TRUE ;
	    dnnm(s, event_create(EVENT_ELECT)) ;
	}

	e = event_set_suspects(e, bool_array_copy(s->suspects, s->vs->nmembers)) ;

	upnm(s, e) ;
    } break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    dn(s, e, abv) ;
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_ELECT:
	sys_panic(("got Elect event from above")) ;
	break ;
    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->suspects) ;
}

LAYER_REGISTER(elect) ;
