/**************************************************************/
/* PT2PTW.C */
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

static string_t name = "PT2PTW" ;

typedef enum { NOHDR, ACK, MAX } header_type_t ;
	
typedef enum { HI, LO } mark_t ;

typedef array_def(mark) mark_array_t ;

typedef struct item_t {
    event_t event ;
    marsh_t marsh ;
} *item_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    int window ;
    int ack_thresh ;
    int_array_t mark ;
    int hi_wmark ;
    int overhead ;
    equeue_array_t send_buf ;
    int_array_t send_credit ;
    int_array_t recv_credit ;
    bool_array_t failed ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "send_buf=%s\n" (string_of_array string_of_equeue_len s.send_buf) ;
    sprintf "send_credit=%s\n" (string_of_int_array s.send_credit) ;
    sprintf "recv_credit=%s\n" (string_of_int_array s.recv_credit)
#endif
}
#endif

#if 0
static len_t msg_len(state_t s, iovec_t iov) {
    return iovec_len(iov) + s->overhead ;
}
#endif

static void clean_buf(equeue_t q) {
    while (!equeue_empty(q)) {
	item_t item = equeue_take(q) ;
	dn_free(item->event, item->marsh) ;
    }
}

/* Check the water-mark. If it is low, tell the application to 
 * continue sending messagses; if it is high, tell it to block.
 */
static void check_wmark(state_t s, rank_t peer) {
/*
    if Queuee.length s.send_buf.(peer) >= s.hi_wmark && s.mark.(peer) = Low then (
      logc (fun () -> sprintf "Hi") ;
      s.mark.(peer) <- Hi ;
      let ev = create name EFlowBlock[FlowBlock (Some(peer),true)] in
      upnm ev 
    ) else if Queuee.length s.send_buf.(peer) = 0 && s.mark.(peer) = Hi then (
      logc (fun () -> sprintf "Low") ;
      s.mark.(peer) <- Low ;
      let ev = create name EFlowBlock[FlowBlock (Some(peer),false)] in
      upnm ev 
    )
*/
}

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
    s->window = 128 * 1024 ;
    s->overhead = 128 ;
    s->ack_thresh = 64 * 1024 ;
    s->hi_wmark = 100 ;
    s->mark = int_array_create(vs->nmembers) ;
    s->send_buf = equeue_array_create(vs->nmembers) ;
    s->send_credit = int_array_create_init(vs->nmembers, s->window) ;
    s->recv_credit = int_array_create_init(vs->nmembers, 0) ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    for (i=0;i<vs->nmembers;i++) {
	array_set(s->mark, i, LO) ;
	array_set(s->send_buf, i, equeue_create(name, sizeof(struct item_t))) ;
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {
    case EVENT_SEND:
	switch(type) {
	case NOHDR: {
	    /* Increase amount of credit to pass back to sender.
	     * If the amount of credit is beyond the threshhold,
	     * send an acknowledgement.  Finally, deliver the
	     * message.  
	     */
	    rank_t origin = event_peer(e) ;
	    int_array_add(s->recv_credit, origin, (int)unmarsh_length(abv) + s->overhead) ;
	    if (array_get(s->recv_credit, origin) > s->ack_thresh) {
		marsh_t msg = marsh_create(NULL) ;
		marsh_len(msg, array_get(s->recv_credit, origin)) ;
		marsh_enum(msg, ACK, MAX) ;
		dn(s, event_send_peer(origin), msg) ;
		array_set(s->recv_credit, origin, 0) ;
	    }
	    up(s, e, abv) ;
	} break ;
	
	case ACK: {
	    /* Some credit were sent back, send more data if it's waiting.
	     */
	    rank_t origin = event_peer(e) ;
	    len_t credit ;
	    equeue_t buf ;

	    assert(origin >= 0 && origin < s->vs->nmembers) ;
	    unmarsh_len(abv, &credit) ;
	    int_array_add(s->send_credit, origin, (int)credit) ;
	    buf = array_get(s->send_buf, origin) ;
	    log_flow(("recv from=%d credit=%d", origin, array_get(s->send_credit, origin))) ;
	    while (array_get(s->send_credit, origin) > 0 &&
		   !equeue_empty(buf)) {
		item_t item = equeue_take(buf) ;
		int_array_sub(s->send_credit, origin, (int)marsh_length(item->marsh) + s->overhead) ;
		marsh_enum(item->marsh, NOHDR, MAX) ;
		dn(s, item->event, item->marsh) ;
	    }
	    up_free(e, abv) ;
	} break ;

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

	/* EFail: Mark failed members, and pass on up.
	 */
    case EVENT_FAIL: {
	rank_t rank ;
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	for (rank=0;rank<s->vs->nmembers;rank++) {
	    if (!array_get(s->failed, rank)) {
		continue ;
	    }
	    clean_buf(array_get(s->send_buf, rank)) ;
	    array_set(s->send_credit, rank, 0) ;
	    array_set(s->recv_credit, rank, 0) ;
	}
	upnm(s, e) ;
    } break ;

    case EVENT_ACCOUNT:
/*
      logb (fun () -> sprintf "blocked(msgs):%s" 
	(string_of_array string_of_equeue_len s.send_buf)) ;
      logb (fun () -> sprintf "blocked(byte):%s" 
	(string_of_array (string_of_equeue_bytes s) s.send_buf)) ;
*/
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

    case EVENT_SEND: {
	/* Send a message to the destination.  If we don't have
	 * any credit, then buffer it.  
	 */
	rank_t dest = event_peer(e) ;
	if (array_get(s->failed, dest)) {
            log_flow(("send to failed member")) ;
	    dn_free(e, abv) ;
	} else if (array_get(s->send_credit, dest) > 0) {
	    /* Normal case. I have enough credit.
	     */
	    log(("send dest=%d credit=%d",
		 dest, array_get(s->send_credit, dest))) ;
	    int_array_sub(s->send_credit, dest, (int)marsh_length(abv) + s->overhead) ;
	    marsh_enum(abv, NOHDR, MAX) ;
	    dn(s, e, abv) ;
	} else {
	    item_t item ;
	    log_flow(("messages are buffered dest=%d credit=%d",
		      dest, array_get(s->send_credit, dest))) ;
	    item = equeue_add(array_get(s->send_buf, dest)) ;
	    item->event = e ;
	    item->marsh = abv ;
	    check_wmark(s, dest) ;
	}
    } break ;

    default:
	marsh_enum(abv, NOHDR, MAX) ;
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    dnnm(s, e) ;
}

static void free_handler(state_t s) {
    rank_t rank ;
    for (rank=0;rank<s->vs->nmembers;rank++) {
	clean_buf(array_get(s->send_buf, rank)) ;
	equeue_free(array_get(s->send_buf, rank)) ;
    }
    array_free(s->send_buf) ;
    array_free(s->mark) ;
    array_free(s->send_credit) ;
    array_free(s->recv_credit) ;
    array_free(s->failed) ;
}

LAYER_REGISTER(pt2ptw) ;
