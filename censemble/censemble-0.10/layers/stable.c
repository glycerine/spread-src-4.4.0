/**************************************************************/
/* STABLE.C : stability detection protocol */
/* Author: Mark Hayden, 11/99 */
/* Note: Sequence numbers begin at 0 in each epoch.
 * Acknowlegdements give the number of messages acknowledged
 * so far in the epoch.  Thus the first messge is
 * acknowledged with '1'.  Similarly, the stability numbers
 * correspond to the number of messages that are stable in
 * the current epoch. */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "STABLE" ;

typedef enum { NOHDR, GOSSIP, MAX } header_type_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    etime_t sweep ;
    seqno_matrix_t acks ;
    bool_array_t failed ;
    etime_t next_gossip ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    eprintf "  failed  =%s\n" (Arrayf.bool_to_string s.failed) ;
  //|] (Array.mapi (fun i a ->
  //	sprintf "acks(%02d)=%s\n" i (string_of_int_array a))) ;
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
    s->sweep = time_of_secs(1) ;
    s->next_gossip = time_zero() ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->acks = seqno_matrix_create(vs->nmembers) ;
    for (i=0;i<vs->nmembers;i++) {
	array_set(s->acks, i, seqno_array_create_init(vs->nmembers, 0)) ;
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (type) {
    case NOHDR:
	up(s, e, abv) ;
	break ;
	
    case GOSSIP: {
	/* Gossip Message: if from a live member, copy into
	 * the origins row in my acknowledgement matrix.
	 */
	const_bool_array_t failed ;
	const_seqno_array_t seqnos ;
	rank_t origin ;
	rank_t i ; 
	assert(event_type(e) == EVENT_CAST) ;
	unmarsh_bool_array(abv, &failed, s->vs->nmembers) ;
	unmarsh_seqno_array(abv, &seqnos, s->vs->nmembers) ;
	origin = event_peer(e) ;

	if (!array_get(failed, s->ls->rank) && /* BUG: could auto-fail him */
	    !array_get(s->failed, origin)) {
	    seqno_array_t local = array_get(s->acks, origin) ;
	    for (i=0;i<s->vs->nmembers;i++) {
		array_set(local, i, seqno_max(array_get(local, i),
					      array_get(seqnos, i))) ;
	    }
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

    /* EInit: request a first timer alarm to bootstrap
     * things.
     */
    case EVENT_INIT:
	dnnm(s, event_timer_time(time_zero())) ;
	upnm(s, e) ;
	break ;

    /* EFail: mark the failed members and check if any
     * messages are now stable.
     */
    case EVENT_FAIL:
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	dnnm(s, event_create(EVENT_STABLE_REQ)) ;
	upnm(s, e) ;
	break ;

    /* ETimer: every so often:
     *   1. recalculate stability and deliver EStable event
     *   2. gossip about my row in the stability matrix
     */
    case EVENT_TIMER: {
	etime_t time = event_time(e) ;
	if (time_ge(time, s->next_gossip)) {
	    etime_t old = s->next_gossip ;
	    s->next_gossip = time_add(time, s->sweep) ;
	    dnnm(s, event_timer_time(s->next_gossip)) ; /* request next gossip */
	
	    /* This check prevents immediate broadcasts after a view.
	     */
	    if (!time_is_zero(old)) {
		dnnm(s, event_create(EVENT_STABLE_REQ)) ;
	    }
	}
	upnm(s, e) ;
	break ;
    }

    /* Got reply to my stability request.
     */
    case EVENT_STABLE_REQ: {
	const_seqno_array_t casts = event_num_casts(e) ;
	seqno_array_t mins ;
	seqno_array_t maxs ;
	rank_t i, j ;
	for (i=0;i<s->vs->nmembers;i++) {
	    array_set(array_get(s->acks, s->ls->rank), i, array_get(casts, i)) ;
	}

	mins = seqno_array_create_init(s->vs->nmembers, 0) ;
	maxs = seqno_array_create_init(s->vs->nmembers, 0) ;
	for (i=0;i<s->vs->nmembers;i++) {
	    array_set(mins, i, array_get(array_get(s->acks, s->ls->rank), i)) ;
	}

	/* For each member, update ack entry and find new stability. 
	 */
	for (i=0;i<s->vs->nmembers;i++) {
	    seqno_array_t row ;
	    if (array_get(s->failed, i)) {
		continue ;
	    }
	    row = array_get(s->acks, i) ;
	    for (j=0;j<s->vs->nmembers;j++) {
		seqno_t seqno = array_get(row, j) ;
		array_set(maxs, j, seqno_max(seqno, array_get(maxs, j))) ;
		array_set(mins, j, seqno_min(seqno, array_get(mins, j))) ;
	    }
	}

	/* Generate the stability event.
	 */
	{
	    event_t e ;
	    e = event_create(EVENT_STABLE) ;
	    e = event_set_stability(e, mins) ;
	    e = event_set_num_casts(e, maxs) ;
	    dnnm(s, e) ;
	}

	{
	    marsh_t msg = marsh_create(NULL) ;
	    marsh_seqno_array(msg, array_get(s->acks, s->ls->rank), s->vs->nmembers) ;
	    marsh_bool_array(msg, s->failed, s->vs->nmembers) ;
	    marsh_enum(msg, GOSSIP, MAX) ;
	    dn(s, event_create(EVENT_CAST_UNREL), msg) ;
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
    rank_t i ;
    for (i=0;i<s->vs->nmembers;i++) {
	array_free(array_get(s->acks, i)) ;
    }
    array_free(s->acks) ;
    array_free(s->failed) ;
}

LAYER_REGISTER(stable) ;
