/**************************************************************/
/* SUSPECT.C */
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

static string_t name = "SUSPECT" ;

typedef enum { NOHDR, GOSSIP, MAX } header_type_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    int max_idle ;
    etime_t sweep ;
    seqno_array_t send ;
    seqno_array_t recv ;
    bool_array_t failed ;
    etime_t next_sweep ;
} *state_t ;

#include "infr/layer_supp.h"

#if 0
let conv rank ofs idles =
  let idles = Array.map (fun idle -> string_of_int (ofs - idle)) idles in
  idles.(rank) <- "_" ;
string_of_array ident idles ;
#endif

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "failed=%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "send=%s\n" (string_of_int_array s.send) ;
    sprintf "recv=%s\n" (string_of_int_array s.recv) ;
    sprintf "idle=%s\n" (conv ls.rank s.send.(ls.rank) s.recv) ;
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
    s->max_idle   = 4 /*Param.int vs.params "suspect_max_idle"*/ ;
    s->sweep      = time_of_secs(1) /*Param.time vs.params "suspect_sweep"*/ ;
    s->failed     = bool_array_create_init(vs->nmembers, FALSE) ;
    s->send       = seqno_array_create_init(vs->nmembers, 0) ;
    s->recv       = seqno_array_create_init(vs->nmembers, 0) ;
    s->next_sweep = time_zero() ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (type) {
    case NOHDR:
	up(s, e, abv) ;
	break ;

    case GOSSIP: {
	/* Ping Message.
	 */
	const_bool_array_t failed ;
	const_seqno_array_t seqnos ;
	rank_t origin = event_peer(e) ;
	unmarsh_bool_array(abv, &failed, s->vs->nmembers) ;
	unmarsh_seqno_array(abv, &seqnos, s->vs->nmembers) ;

	logp(("Ping from %d %s", origin, seqno_array_to_string(seqnos, s->vs->nmembers))) ;
	if (origin != s->ls->rank && /* Check just in case. */
	    !array_get(failed, s->ls->rank) && /* BUG: could auto-fail him */
	    !array_get(s->failed, origin)) {
	    array_set(s->recv, origin,
		      seqno_max(array_get(s->recv, origin),
				array_get(seqnos, s->ls->rank))) ;
	    array_set(s->send, origin,
		      seqno_max(array_get(s->send, origin),
				array_get(seqnos, origin))) ;
	} else {
	    logp(("dropping ping")) ;
	}
	array_free(failed) ;
	array_free(seqnos) ;
	up_free(e, abv) ;
    } break ;
    
    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_FAIL:
	/* EFail: mark the failed members.
	 */
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	upnm(s, e) ;
	break ;

    case EVENT_INIT:
	/* EInit: request a first timer alarm to bootstrap things.
	 */
	dnnm(s, event_timer_time(time_zero())) ;
	upnm(s, e) ;
	break ;

    case EVENT_TIMER: {
	/* ETimer: every so often:
	 *   1. check for suspicions
	 *   2. ping the other members
	 */
	etime_t time = event_time(e) ;
	if (time_ge(time, s->next_sweep)) {
	    bool_array_t suspicions ;
	    bool_t any ;
	    rank_t i ;
	    marsh_t msg ;
	    s->next_sweep = time_add(time, s->sweep) ;
	    dnnm(s, event_timer_time(s->next_sweep)) ; /* request next sweep */
	    logp(("Pinging:%s", time_to_string(time))) ;

	    seqno_array_incr(s->send, s->ls->rank) ;

	    msg = marsh_create(NULL) ;
	    marsh_seqno_array(msg, s->send, s->vs->nmembers) ;
	    marsh_bool_array(msg, s->failed, s->vs->nmembers) ;
	    marsh_enum(msg, GOSSIP, MAX) ;
	    dn(s, event_cast_unrel(), msg) ;
	    
	    suspicions = bool_array_create_init(s->vs->nmembers, FALSE) ;
	    any = FALSE ;
	    for(i=0;i<s->vs->nmembers;i++) {
		if (i != s->ls->rank &&
		    !array_get(s->failed, i) &&
		    array_get(s->send, s->ls->rank) > array_get(s->recv, i) + s->max_idle) {
		    array_set(suspicions, i, TRUE) ;
		    any = TRUE ;
		}
	    }
	    /*logp("Idles:%s", (conv ls.rank s.send.(ls.rank) s.recv)) ;*/
	    if (any) {
		log(("Suspect:%s", bool_array_to_string(suspicions, s->vs->nmembers))) ;
		dnnm(s, event_suspect_reason_create(suspicions, name)) ;
	    } else {
		array_free(suspicions) ;
	    }
	}
	upnm(s, e) ;
    } break ;
    
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
    dnnm(s, e) ;
}

static void free_handler(state_t s) {
    array_free(s->send) ;
    array_free(s->recv) ;
    array_free(s->failed) ;
}

LAYER_REGISTER(suspect) ;
