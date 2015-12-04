/**************************************************************/
/* LEAVE.C */
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

static string_t name = "LEAVE" ;

typedef enum { NOHDR, LEAVE_LEAVE, LEAVE_GOT_VIEW, MAX } header_type_t ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_t exited ;
    bool_t leaving ;
    bool_array_t got_view ;
    bool_array_t failed ;
    rank_t coord ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "exited=%b leaving=%b\n" s.exited s.leaving ;
    sprintf "got_view=%s\n" (Arrayf.bool_to_string s.got_view)
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
    s->exited      = FALSE ;
    s->got_view    = bool_array_create_init(vs->nmembers, FALSE) ;
    s->failed      = bool_array_create_init(vs->nmembers, FALSE) ;
    s->leaving     = FALSE ;
    s->coord       = vs->coord ;
}

static void do_gossip(state_t s) {
    if (array_get(s->got_view, s->ls->rank) &&
	s->ls->rank != s->coord) {
	marsh_t msg = marsh_create(NULL) ;
	marsh_bool_array(msg, s->got_view, s->vs->nmembers) ;
	marsh_enum(msg, LEAVE_GOT_VIEW, MAX) ;
	dn(s, event_send_peer(s->coord), msg) ;
    }
}

static void check_exit(state_t s) {
    if (!s->exited &&
	array_get(s->got_view, s->ls->rank)) {
	rank_t i ;
	for (i=0;i<s->vs->nmembers;i++) {
	    if (!array_get(s->got_view, i) &&
		!array_get(s->failed, i)) {
		break ;
	    }
	}
	if (i == s->vs->nmembers) {
	    s->exited = TRUE ;

	    /* If I am the coordinator, then tell the others
	     * that we're done.  Be sure to broadcast the
	     * GotView before doing the exit (which will
	     * disable the transport).  
	     */
	    if (s->ls->rank == s->coord) {
		marsh_t msg = marsh_create(NULL) ;
		marsh_bool_array(msg, s->got_view, s->vs->nmembers) ;
		marsh_enum(msg, LEAVE_GOT_VIEW, MAX) ;
		dn(s, event_cast(), msg) ;
	    }

	    dnnm(s, event_create(EVENT_EXIT)) ;
        }
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {
    case EVENT_CAST:
    case EVENT_SEND:
    case EVENT_CAST_UNREL:
    case EVENT_SEND_UNREL:
	switch(type) {
	case LEAVE_LEAVE: {
	    rank_t origin = event_peer(e) ;
	    if (origin != s->ls->rank) {
		bool_array_t suspects = bool_array_setone(s->vs->nmembers, origin) ;
		dnnm(s, event_suspect_reason_create(suspects, name)) ;
	    }
	    up_free(e, abv) ;
	} break ;

	case LEAVE_GOT_VIEW: {
	    /* Watch out!  Be careful about these GotView messages.
	     * Because we may get them from leaving members, in
	     * which case they should not have set things they
	     * shouldn't have.
	     */
	    const_bool_array_t got_view ;
	    const_bool_array_t tmp ;
	    unmarsh_bool_array(abv, &got_view, s->vs->nmembers) ;
	    if (array_get(got_view, s->ls->rank) &&
		!array_get(s->got_view, s->ls->rank)) {
		sys_panic(("sanity")) ;
	    }
	    
	    tmp = s->got_view ;
	    s->got_view = bool_array_map_or(s->got_view, got_view, s->vs->nmembers) ;
	    array_free(tmp) ;
	    array_free(got_view) ;
	    check_exit(s) ;
	    up_free(e, abv) ;
	} break ;

	case NOHDR:
	    up(s, e, abv) ;
	    break ;

	OTHERWISE_ABORT() ;
	}
	break ;

    default:
	assert(type == NOHDR) ;
	up(s, e, abv) ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_VIEW:
	/* Only pass up views if we are not leaving.
	 */
	if (!s->leaving) {
	    upnm(s, e) ;
	} else {
	    event_free(e) ;
	}

	/* Check for exiting.
	 */
	array_set(s->got_view, s->ls->rank, TRUE) ;
	do_gossip(s) ;
	check_exit(s) ;
	break ;

    case EVENT_FAIL:
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;

	/* Recompute the rank of the coordinator. 
	 */
	s->coord = bool_array_min_false(s->failed, s->vs->nmembers) ;
	
	do_gossip(s) ;
	check_exit(s) ;
	upnm(s, e) ;
	break ;

    case EVENT_LEAVE:
	{
	    /* Cast a message to the group.
	     */
	    marsh_t msg = marsh_create(NULL) ;
	    marsh_enum(msg, LEAVE_LEAVE, MAX) ;
	    dn(s, event_cast(), msg) ;
	}

	/* Mark me as leaving.
	 */
	s->leaving = TRUE ;

	{
	    /* Suspect everyone else.
	     */
	    bool_array_t suspects ;
	    suspects = bool_array_create_init(s->vs->nmembers, TRUE) ;
	    array_set(suspects, s->ls->rank, FALSE) ;
	    dnnm(s, event_suspect_reason_create(suspects, name)) ;
	}
      
	upnm(s, e) ;
	break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    marsh_enum(abv, NOHDR, MAX) ;
    dn(s, e, abv) ;
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_EXIT:
	/* This should not happen.  Don't exit twice, and
	 * remember if someone else generates an exit.  
	 */
	if (s->exited) {
	    event_free(e) ;
	} else {
	    s->exited = TRUE ;
	    dnnm(s, e) ;
	}
	break ;

    case EVENT_BLOCK_OK:
	/* Why is this necessary?
	 */
	if (s->exited) {
	    event_free(e) ;
	} else {
	    dnnm(s, e) ;
	}
	break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->failed) ;
    array_free(s->got_view) ;
}

LAYER_REGISTER(leave) ;
