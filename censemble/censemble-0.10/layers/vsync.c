/**************************************************************/
/* VSYNC.C */
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

static string_t name = "VSYNC" ;

typedef enum { NOHDR, GOSSIP, MAX } header_type_t ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;

    const_bool_array_t failed ;
    bool_matrix_t fails ;
    seqno_matrix_t casts ;
    seqno_matrix_t sends ;
    bool_t donesync ;
    bool_t blocked ;

    /* Following fields added by TC for new protocol, 12/12/97 */
    rank_t coord ;	  /* who do I think is coord? */
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "blocked=%b coord=%d donesync=%b\n" s.blocked s.coord s.donesync ;
    sprintf "failed=%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "heard=%s\n" (string_of_bool_array s.heard) ;
    //|];(Array.mapi (fun i c ->
    //sprintf "casts(%02d)=%s\n" i (Arrayf.int_to_string c)
    //) s.casts);(Array.mapi (fun i s ->
    sprintf "sends(%02d)=%s\n" i (Arrayf.int_to_string s) ;
    //) s.sends)])
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
    rank_t i ;
    s->ls = ls ; 
    s->vs = vs ;
    s->layer = layer ;

    s->blocked = FALSE ;
    s->donesync = FALSE ;
    s->coord = 0 ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->fails = bool_matrix_create(vs->nmembers) ;
    s->sends = seqno_matrix_create(vs->nmembers) ;
    s->casts = seqno_matrix_create(vs->nmembers) ;
    for (i=0;i<s->vs->nmembers;i++) {
	array_set(s->fails, i, NULL) ;
	array_set(s->sends, i, NULL) ;
	array_set(s->casts, i, NULL) ;
    }
}

static void check_ready(state_t s) {
    seqno_array_t sends ;
    seqno_array_t casts ;
    event_t e ;
    rank_t i ;
    rank_t j ;

#if 1
    log_sync(("donesync=%d failed=%s", s->donesync,
	 bool_array_to_string(s->failed, s->vs->nmembers))) ;
#endif
	
    if (s->donesync) return ;

    for (i=0;i<s->vs->nmembers;i++) {
	if (!array_get(s->failed, i) &&
	    !array_get(s->fails, i)) {
	    break ;
	}
    }
    if (i < s->vs->nmembers) {
	return ;
    }

    s->donesync = TRUE ;

    sends = seqno_array_create_init(s->vs->nmembers, 0) ;
    for (i=0;i<s->vs->nmembers;i++) {
	if (array_get(s->failed, i)) {
	    continue ;
	}
	array_set(sends, i, matrix_get(s->sends, i, s->ls->rank)) ;
    }
    
    casts = seqno_array_create_init(s->vs->nmembers, 0) ;
    for (i=0;i<s->vs->nmembers;i++) {
	if (array_get(s->failed, i)) {
	    continue ;
	}
	for (j=0;j<s->vs->nmembers;j++) {
	    array_set(casts, j, seqno_max(array_get(casts, j), matrix_get(s->casts, i, j))) ;
	}
    }

    log_sync(("delivering ESyncInfo casts=%s sends=%s", 
	      seqno_array_to_string(casts, s->vs->nmembers),
	      seqno_array_to_string(sends, s->vs->nmembers))) ;
    e = event_create(EVENT_SYNC_INFO) ;
    e = event_set_appl_casts(e, casts) ;
    e = event_set_appl_sends(e, sends) ;
    dnnm(s, e) ;

    if (s->ls->rank == s->coord) {
	/* Create an array with info from all non-failed members.
	 */
	marsh_t msg = marsh_create(NULL) ;
	rank_t i ;
    
	for (i=0;i<s->vs->nmembers;i++) {
	    if (!array_get(s->failed, i)) {
		marsh_seqno_array(msg, array_get(s->sends, i), s->vs->nmembers) ;
		marsh_seqno_array(msg, array_get(s->casts, i), s->vs->nmembers) ;
		marsh_bool_array(msg, array_get(s->fails, i), s->vs->nmembers) ;
		marsh_bool(msg, TRUE) ;
	    } else {
		marsh_bool(msg, FALSE) ;
	    }
	}
	
        /* Send it out.
         */
	log(("COORD casting info")) ;
	marsh_enum(msg, GOSSIP, MAX) ;
	dn(s, event_cast(), msg) ;
    }
}

static void got_info(
	state_t s,
        rank_t origin,
	const_bool_array_t fails,
	const_seqno_array_t casts,
	const_seqno_array_t sends
) {
    const_bool_array_t old_fails = array_get(s->fails, origin) ;

    /* If the failures do not include at least as much
     * as what I've heard so far, then ignore them.
     */
    if (!bool_array_super(fails, s->failed, s->vs->nmembers) ||
	(old_fails && bool_array_super(old_fails, fails, s->vs->nmembers))) {
	return ;
    }

    if (array_get(s->fails, origin)) {
	array_free(array_get(s->fails, origin)) ;
    }
    if (array_get(s->sends, origin)) {
	array_free(array_get(s->sends, origin)) ;
    }
    if (array_get(s->casts, origin)) {
	array_free(array_get(s->casts, origin)) ;
    }
    array_set(s->fails, origin, bool_array_copy(fails, s->vs->nmembers)) ;
    array_set(s->casts, origin, seqno_array_copy(casts, s->vs->nmembers)) ;
    array_set(s->sends, origin, seqno_array_copy(sends, s->vs->nmembers)) ;
    
    log(("got_info from %d", origin));
    check_ready(s) ;
}


/* Get the info out of the event, broadcast it
 * to the other members, update my state, and
 * pass it on.
 */
static void got_local(
        state_t s,
	event_t e
) {
    log_sync(("got_local:%s", event_to_string(e))) ;
    
    /* If not coord, send info pt2pt to coord.
     */
    if (s->ls->rank != s->coord) {
	rank_t i ;
	marsh_t msg = marsh_create(NULL) ;
	
	for (i=0;i<s->vs->nmembers;i++) {
	    if (i != s->ls->rank) {
		marsh_bool(msg, FALSE) ;
	    } else {
		marsh_seqno_array(msg, event_appl_sends(e), s->vs->nmembers) ;
		marsh_seqno_array(msg, event_appl_casts(e), s->vs->nmembers) ;
		marsh_bool_array(msg, s->failed, s->vs->nmembers) ;
		marsh_bool(msg, TRUE) ;
	    }
	}
	marsh_enum(msg, GOSSIP, MAX) ;
	dn(s, event_send_peer(s->coord), msg) ;

	log(("EFail NOT coord, sending cast/send info to coord")) ;
    } else {
	log(("EFail AM coord, not sending cast/send info!")) ;
    }

    /* Call receive function for this info.
     */
    got_info(s, s->ls->rank, s->failed,
	     event_appl_casts(e),
	     event_appl_sends(e)) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t msg) {
    header_type_t type = unmarsh_enum_ret(msg, MAX) ;
    switch (type) {
    case NOHDR:
	up(s, e, msg) ;
	break ;

    case GOSSIP: {
	/* Receive vector info in General hdr.
	 * Ignore messages from myself.
	 */
	if (event_peer(e) != s->ls->rank) {
	    rank_t i ;
	    for (i=0;i<s->vs->nmembers;i++) {
		bool_t bool ;
		const_seqno_array_t casts ;
		const_seqno_array_t sends ;
		const_bool_array_t fails ;
		unmarsh_bool(msg, &bool) ;
		if (!bool) {
		    continue ;
		}
		unmarsh_bool_array(msg, &fails, s->vs->nmembers) ; 
		unmarsh_seqno_array(msg, &casts, s->vs->nmembers) ;
		unmarsh_seqno_array(msg, &sends, s->vs->nmembers) ;
		log(("ECast|ESend - got info from %d", event_peer(e))) ;
		got_info(s, s->vs->nmembers - i - 1, fails, casts, sends) ;
		array_free(fails) ;
		array_free(casts) ;
		array_free(sends) ;
	    }
	}
	up_free(e, msg) ;
    } break ;

    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_FAIL: {
	const_bool_array_t old_failed = s->failed ;

	/* When my failure information changes, reset all info
	 * that is now out of date.
	 */
	s->failed = bool_array_map_or(event_failures(e), s->failed, s->vs->nmembers) ;
	if (bool_array_super(s->failed, old_failed, s->vs->nmembers)) {
	    rank_t i ;
	    s->donesync = FALSE ;
	    for (i=0;i<s->vs->nmembers;i++) {
		const_bool_array_t fails = array_get(s->fails, i) ;
		if (fails &&
		    bool_array_super(s->failed, fails, s->vs->nmembers)) {
		    array_free(fails) ;
		    array_set(s->fails, i, NULL) ;
		}
	    }
	}
	array_free(old_failed) ;

	/* Recompute the rank of the coordinator.  Hope to do
	 * this in elect layer later on.  TC 12/12/97.
	 */
	log(("EFail:recomputing coord")) ;
	s->coord = bool_array_min_false(s->failed, s->vs->nmembers) ;

	/* If we are not blocked then the data in the event
	 * is no good.  Only continue if we're blocked.
	 */
	if (s->blocked) {
	    got_local(s, e) ;
	}

	upnm(s, e) ;
    } break ;

    case EVENT_BLOCK:
	s->blocked = TRUE ;
	got_local(s, e) ;
	upnm(s, e) ;
	break ;

    case EVENT_ACCOUNT:
  	logb(("failed=%s", bool_array_to_string(s->failed, s->vs->nmembers))) ;
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
    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    rank_t rank ;
    for (rank=0;rank<s->vs->nmembers;rank++) {
	if (array_get(s->fails, rank)) {
	    array_free(array_get(s->fails, rank)) ;
	}
	if (array_get(s->sends, rank)) {
	    array_free(array_get(s->sends, rank)) ;
	}
	if (array_get(s->casts, rank)) {
	    array_free(array_get(s->casts, rank)) ;
	}
    }
    array_free(s->fails) ;
    array_free(s->sends) ;
    array_free(s->casts) ;
    array_free(s->failed) ;
}

LAYER_REGISTER(vsync) ;
