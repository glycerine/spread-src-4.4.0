/**************************************************************/
/* LOCAL.C */
/* Author: Mark Hayden, 6/2000 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "LOCAL" ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
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
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    switch (event_type(e)) {
    default:
	up(s, e, abv) ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    case EVENT_CAST: {
	unmarsh_t abv_up = marsh_to_unmarsh_copy(abv) ;
	dn(s, e, abv) ;
	up(s, event_cast_peer(s->ls->rank), abv_up) ;
    } break; 
#if 0
    case EVENT_CAST_UNREL: {
	unmarsh_t abv_up = marsh_to_unmarsh_copy(name, abv) ;
	dn(s, e, abv) ;
	up(s, event_cast_unrel_peer(name, s->ls->rank), abv_up) ;
    } break ;
#endif
    case EVENT_SEND: {
	rank_t peer = event_peer(e) ;
	if (peer == s->ls->rank) {
	    up(s, e, marsh_to_unmarsh(abv)) ;
	} else {
	    dn(s, e, abv) ;
	}
    } break; 
#if 0
    case EVENT_SEND_UNREL: {
	rank_t peer = event_peer(e) ;
	if (peer == s->ls->rank) {
	    up(s, e, marsh_to_unmarsh(name, abv)) ;
	} else {
	    dn(s, e, abv) ;
	}
    } break; 
#endif
    default:
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
}

LAYER_REGISTER(local) ;
