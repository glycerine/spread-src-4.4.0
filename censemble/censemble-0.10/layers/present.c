/**************************************************************/
/* PRESENT.C */
/* Author: Mark Hayden, 5/00 */
/* Based on code by: Mark Hayden, Zhen Xiao, 9/97 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
/* Note: this layer generates an EPresent event (which contains
 * a bool array) whenever a member reports its presence in the 
 * view. This could be quite inefficient if the group size is 
 * large.  We can either reduce the frequency of generating 
 * such events or not use an array.
 */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "PRESENT" ;
	
typedef enum { NOHDR, PRESENT, MAX } header_type_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_array_t in_view ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "in_view=%s\n" (string_of_bool_array s.in_view)
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
    s->in_view = bool_array_create_init(vs->nmembers, FALSE) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (type) {
    case NOHDR:
	up(s, e, abv) ;
	break ;

	/* ECast|ESend(InView): other member is telling me
	 * that he is in the view.
	 */
    case PRESENT: {
	rank_t origin = event_peer(e) ;
	array_set(s->in_view, origin, TRUE) ;
	/*log (fun () -> sprintf "present=%s" (Arrayf.bool_to_string in_view)) ;*/
	dnnm(s, event_set_presence(event_create(EVENT_PRESENT),
				   bool_array_copy(s->in_view, s->vs->nmembers))) ;
	up_free(e, abv) ;
    } break ;

    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
	
	/* Send a message to the coordinator saying that I'm
	 * here.  
	 */
    case EVENT_INIT:
	upnm(s, e) ;
	if (s->ls->am_coord) {
	    array_set(s->in_view, s->ls->rank, TRUE) ;
	    dnnm(s, event_set_presence(event_create(EVENT_PRESENT),
				       bool_array_copy(s->in_view, s->vs->nmembers))) ;
	} else {
	    marsh_t msg = marsh_create(NULL) ;
	    marsh_enum(msg, PRESENT, MAX) ;
	    dn(s, event_send_peer(s->vs->coord), msg) ;
	}
	break ;
	
    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    default:
	marsh_enum(abv, NOHDR, MAX) ;
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
    array_free(s->in_view) ;
}

LAYER_REGISTER(present) ;
