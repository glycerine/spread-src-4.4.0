/**************************************************************/
/* CHK_SYNC.C */
/* Author: Mark Hayden, 11/99 */
/* Tests that all members saw the same number of broadcasts
 * messages in each view, and that all pt2pt sends were
 * received. */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "CHK_SYNC" ;

typedef enum { NOHDR, APPL, GOSSIP, VIEW_STATE, MAX } header_type_t ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_array_t failed ;
    seqno_t cast_xmit ;
    seqno_array_t cast_recv ;
    seqno_array_t send_xmit ;
    seqno_array_t send_recv ;
    bool_t got_dn_block ;
    view_state_t next_vs ;
    const_seqno_array_t view_casts ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "failed=%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "cast_xmit=%d\n" s.cast_xmit ;
    sprintf "cast_recv=%s\n" (string_of_int_array s.cast_recv) ;
    sprintf "send_xmit=%s\n" (string_of_int_array s.send_xmit) ;
    sprintf "send_recv=%s\n" (string_of_int_array s.send_recv)
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
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->cast_xmit = 0 ;
    s->cast_recv = seqno_array_create_init(vs->nmembers, 0) ;
    s->send_xmit = seqno_array_create_init(vs->nmembers, 0) ;
    s->send_recv = seqno_array_create_init(vs->nmembers, 0) ;
    s->got_dn_block = FALSE ;
    s->next_vs = NULL ;
    s->view_casts = NULL ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {
    case EVENT_CAST:
    case EVENT_SEND:
	switch (type) {
	case NOHDR:
	    up(s, e, abv) ;
	    break ;

	case APPL:	    
	    if (s->next_vs) {
		/*
		  eprintf("CHK_SYNC:received message after Up(EView) (dropping msg)\n") ;
		  eprintf("  event=%s\n", event_to_string(e)) ;
		  dnnm (create name EDump[]) ;
		  event_free(e) ;
		*/
		sys_abort() ;
	    } else {
		rank_t origin = event_peer(e) ;
		if (event_type(e) == EVENT_CAST) {
		    seqno_array_incr(s->cast_recv, origin) ;
		} else {
		    seqno_array_incr(s->send_recv, origin) ;
		}
		up(s, e, abv) ;
	    }
	    break ;

	case VIEW_STATE: {
	    /* Check whether all members have the same view states.
	     */
	    view_state_t vs ;
	    assert(event_type(e) == EVENT_CAST) ;
	    unmarsh_view_state(abv, &vs) ;
	    if (!view_state_eq(s->vs, vs)) {
		sys_panic(("mismatched view state")) ;
	    }
	    view_state_free(vs) ;
	    up_free(e, abv) ;
	} break; 
	    
	case GOSSIP: {
	    view_id_t view_id ;
	    view_id_t next_view_id ;
	    const_seqno_array_t sends ;
	    const_seqno_array_t casts ;
	    rank_t origin = event_peer(e) ;
	    assert(event_type(e) == EVENT_CAST) ;
	    unmarsh_view_id(abv, &view_id) ;
	    unmarsh_seqno_array(abv, &sends, s->vs->nmembers) ;
	    unmarsh_seqno_array(abv, &casts, s->vs->nmembers) ;
	    
	    if (s->next_vs) {
		next_view_id = view_id_of_state(s->next_vs) ;
		if (view_id_eq(&next_view_id, &view_id) &&
		    ((!seqno_array_eq(s->view_casts, casts, s->vs->nmembers) ||
		      (array_get(sends, s->ls->rank) !=
		     array_get(s->send_recv, origin))))) {
		    eprintf("CHK_SYNC:virtual synchrony failure{%s}\n", s->ls->name) ;
		    eprintf("  rank=%d rmt_rank=%d\n", s->ls->rank, origin) ;
		    eprintf("  view_id=%s\n", view_id_to_string(&s->ls->view_id)) ;
		    eprintf("   my failed=%s\n", bool_array_to_string(s->failed, s->vs->nmembers));
		    eprintf("     my cast=%s\n", seqno_array_to_string(s->cast_recv, s->vs->nmembers)) ;
		    eprintf("    rmt cast=%s\n", seqno_array_to_string(casts, s->vs->nmembers)) ;
		    eprintf("    rmt_send_xmt=%llu my_send_xmit=%llu\n", 
			    array_get(sends, s->ls->rank),
			    array_get(s->send_recv, origin)) ;
		    sys_abort() ;
		}
	    }
	    array_free(sends) ;
	    array_free(casts) ;
	    up_free(e, abv) ;
	} break; 
	OTHERWISE_ABORT() ;
	}
	break ;

    default:
	up(s, e, abv) ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_INIT:
	/* If I'm coordinator then tell the others what my view
	 * state is.  
	 */
	if (s->ls->am_coord) {
	    marsh_t msg = marsh_create(NULL) ;
	    marsh_view_state(msg, s->vs) ;
	    marsh_enum(msg, VIEW_STATE, MAX) ;
	    dn(s, event_cast(), msg) ;
	}
	upnm(s, e) ;
	break ;

    case EVENT_VIEW: {
	marsh_t msg = marsh_create(NULL) ;
	view_id_t view_id ;
	assert(!s->next_vs) ;
	s->next_vs = view_state_copy(event_view_state(e)) ;
	view_id = view_id_of_state(s->next_vs) ;
	array_set(s->cast_recv, s->ls->rank, s->cast_xmit) ;
	assert(s->view_casts == NULL) ;
	s->view_casts = seqno_array_copy(s->cast_recv, s->vs->nmembers) ;
	marsh_seqno_array(msg, s->cast_recv, s->vs->nmembers) ;
	marsh_seqno_array(msg, s->send_xmit, s->vs->nmembers) ;
	marsh_view_id(msg, &view_id) ;
	marsh_enum(msg, GOSSIP, MAX) ;
	dn(s, event_set_no_total(event_cast(), TRUE), msg) ;
	upnm(s, e) ;
    } break ;

    case EVENT_FAIL:
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	upnm(s, e) ;
	break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    case EVENT_CAST: 
    case EVENT_SEND:
	if (event_appl_msg(e)) {
	    assert(!s->got_dn_block) ;
	    /*	    
		    if Once.isset s.got_dn_block then (
		    eprintf "CHK_SYNC:sending message after Dn(Block)\n" ;
		    dnnm (create name EDump[])
		    ) ;
	    */
	    if (event_type(e) == EVENT_CAST) {
		s->cast_xmit ++ ;
	    } else {
		seqno_array_incr(s->send_xmit, event_peer(e)) ;
	    }
	    marsh_enum(abv, APPL, MAX) ;
	    dn(s, e, abv) ;
	} else {
	    marsh_enum(abv, NOHDR, MAX) ;
	    dn(s, e, abv) ;
	}
	break; 

    default:
	marsh_enum(abv, NOHDR, MAX) ;
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_BLOCK:
	assert(!s->got_dn_block) ;
	s->got_dn_block = TRUE ;
	dnnm(s, e) ;
	break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->failed) ;
    array_free(s->cast_recv) ;
    array_free(s->send_xmit) ;
    array_free(s->send_recv) ;
    if (s->view_casts) {
	array_free(s->view_casts) ;
    }
    if (s->next_vs) {
	view_state_free(s->next_vs) ;
    }
}

LAYER_REGISTER(chk_sync) ;
