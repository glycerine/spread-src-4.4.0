/**************************************************************/
/* PT2PT.C */
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
#include "infr/iq.h"

static string_t name = "PT2PT" ;

/* Data(seqno): sent with message number 'seqno'

 * Ack(seqno): acks first seqno pt2pt messages.

 * Nak(lo,hi): request for retransmission of messages number
 * 'lo' to 'hi'.
 * BUG: comment should say, inclusive/exclusive
 */

typedef enum { NOHDR, DATA, ACK, DATA_ACK, NAK, MAX } header_type_t ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    etime_t sweep ;
    etime_t next_sweep ;
    bool_array_t failed ;
    iq_array_t sends ;
    iq_array_t recvs ;
    seqno_array_t naked ;
    seqno_array_t acked	;
    seqno_array_t retrans0 ;
    seqno_array_t retrans1 ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "failed =%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "send_lo=%s\n" (Arraye.int_to_string (Arraye.map Iq.lo s.sends)) ;
    sprintf "send_hi=%s\n" (Arraye.int_to_string (Arraye.map Iq.hi s.sends)) ;
    sprintf "recv_lo=%s\n" (Arraye.int_to_string (Arraye.map Iq.lo s.recvs)) ;
    sprintf "recv_hi=%s\n" (Arraye.int_to_string (Arraye.map Iq.hi s.recvs)) ;
    sprintf "recv_size=%s\n" (Arraye.int_to_string (Arraye.map iq_size s.recvs)) ;
    sprintf "send_size=%s\n" (Arraye.int_to_string (Arraye.map iq_size s.sends)) ;
    sprintf "retrans=%s\n" (string_of_int_array s.retrans)
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
    s->sweep    = time_of_secs(1) ;
    s->next_sweep = time_zero() ;
    s->acked	= seqno_array_create_init(s->vs->nmembers, 0) ;
    s->naked	= seqno_array_create_init(s->vs->nmembers, 0) ;
    s->retrans0 = seqno_array_create_init(s->vs->nmembers, 0) ;
    s->retrans1 = seqno_array_create_init(s->vs->nmembers, 0) ;
    s->failed	= bool_array_create_init(vs->nmembers, FALSE) ;
    s->sends    = iq_array_create(s->vs->nmembers) ;
    s->recvs	= iq_array_create(s->vs->nmembers) ;
    for (i=0;i<s->vs->nmembers;i++) {
	if (i == s->ls->rank) {
	    array_set(s->sends, i, NULL) ;
	    array_set(s->recvs, i, NULL) ;
	} else {
	    array_set(s->sends, i, iq_create(name, (iq_free_t)marsh_buf_free)) ;
	    array_set(s->recvs, i, iq_create(name, (iq_free_t)marsh_buf_free)) ;
	}
    }
}

/* Code for handling acks and data.  Note that handle_ack
 * does not modify the event, but handle_data does.
 */
static void handle_ack(state_t s, rank_t origin, seqno_t ack) {
    iq_t sends = array_get(s->sends, origin) ;
    iq_set_lo(sends, ack) ;
    log(("recv ack(%llu) from %d (new lo=%llu)", ack, origin, iq_lo(sends))) ;
}

static void handle_data(state_t s, event_t e, seqno_t seqno, unmarsh_t abv) {
    rank_t origin = event_peer(e) ;
    iq_t recvs = array_get(s->recvs, origin) ;

    /* Check for fast-path.
     */
    if (iq_opt_update_check(recvs, seqno)) {
	/*log(("recv %d from %d hi=%d (fast path)", seqno, origin, iq_hi(recvs))) ;*/
	iq_opt_update_update(recvs, seqno) ;
	up(s, e, abv) ;
    } else {
	marsh_buf_t abv_buf = unmarsh_to_buf(abv) ;
	unmarsh_free(abv) ;
	log(("recv %llu from %d hi=%llu (slow path)", seqno, origin, iq_hi(recvs))) ;
	if (iq_assign(recvs, seqno, abv_buf)) {
	    iq_item_t abv ;
	    seqno_t seqno ;
	    while (iq_get_prefix(recvs, &seqno, &abv)) {
		up(s, event_send_peer(origin), unmarsh_of_buf(abv)) ;
		marsh_buf_free(abv) ;
	    }
	} else {
	    log(("slow path, redundant trans origin=%d seqno=%llu lo=%llu hi=%llu",
		 origin, seqno, iq_lo(recvs), iq_hi(recvs))) ;
	    marsh_buf_free(abv_buf) ;
	}

	/* If seqno seems old, then maybe a previous ack did
	 * not get through.  Shift ack seqno back.  This will
	 * cause an ack to be sent on next timeout.
	 */
	if (seqno < array_get(s->acked, origin)) {
	    log(("setting acked=%llu back to %llu",
		 array_get(s->acked, origin), seqno)) ;
	    array_set(s->acked, origin, seqno) ;
	}

	/* If there are holes out of order
	 * then send a nak.
	 */
	if (seqno > array_get(s->naked, origin)) {
	    seqno_t lo, hi ;
	    if (iq_hole(recvs, &lo, &hi) &&
		seqno >= lo) {
		/* Keep track of highest msg # we've naked.
		 */
		marsh_t msg = marsh_create(NULL) ;
		marsh_seqno(msg, hi) ;
		marsh_seqno(msg, lo) ;
		marsh_enum(msg, NAK, MAX) ;
		log(("xmit NAK(%llu..%llu) to %d", lo, hi, origin)) ;
		array_set(s->naked, origin, seqno_max(array_get(s->naked, origin), hi)) ;
		dn(s, event_send_peer(origin), msg) ;
	    }
	}
	
	event_free(e) ;
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {
    case EVENT_SEND: {
	rank_t origin = event_peer(e) ;
	switch(type) {
	case DATA: {
	    /* ESend:Data: Got a data message from other
	     * member.
	     */
	    seqno_t data ;
	    unmarsh_seqno(abv, &data) ;
	    handle_data(s, e, data, abv) ;
	} break ;
	
	case DATA_ACK: {
	    /* ESend:DataAck: Got a data message from other
	     * member as well as an acknowledgement.
	     */
	    seqno_t data ;
	    seqno_t ack ;
	    unmarsh_seqno(abv, &data) ;
	    unmarsh_seqno(abv, &ack) ;
	    handle_ack(s, event_peer(e), ack) ;
	    handle_data(s, e, data, abv) ;
	} break ;

	case NAK: {
	    /* Nak: got a request for retransmission.  Send any
	     * messages I have in the requested interval, lo..hi. 
	     */
	    iq_t sends = array_get(s->sends, origin) ;
	    seqno_t lo ;
	    seqno_t hi ;
	    seqno_t seqno ;

	    unmarsh_seqno(abv, &lo) ;
	    unmarsh_seqno(abv, &hi) ;
	    log(("recv NAK(%llu..%llu) from %d", lo, hi, origin)) ;

	    /* Ack: Unbuffer any acked messages.
	     */
	    iq_set_lo(sends, lo) ;

	    /* Retransmit any of the messages asked for that I have.
	     */
	    for (seqno=lo;seqno<hi;seqno++) {
		marsh_buf_t buf ;
		marsh_t abv ;
		if (iq_get(sends, seqno, (void**)&buf) != IQ_DATA) {
		    continue ;
		}
		abv = marsh_of_buf(buf) ;
		marsh_seqno(abv, seqno) ;
		marsh_enum(abv, DATA, MAX) ;
		dn(s, event_send_peer(origin), abv) ;
	    }
	    up_free(e, abv) ;
	} break ;

	case ACK: {
	    /* Ack: Unbuffer any acked messages.
	     */
	    seqno_t ack ;
	    unmarsh_seqno(abv, &ack) ;
	    handle_ack(s, event_peer(e), ack) ;
	    up_free(e, abv) ;
	} break ;

	OTHERWISE_ABORT() ;
	}
    } break ;
    default:
	up(s, e, abv) ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_FAIL:
	/* EFail: Mark failed members, and pass on up.
	 */
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	upnm(s, e) ;
	break ;

    case EVENT_TIMER: {
	/* ETimer: check for any messages that need to be
	 * retransmitted.  Acknowledge any unacknowledged
	 * messages.  
	 */
	etime_t time = event_time(e) ;
	if (time_ge(time, s->next_sweep)) {
	    rank_t rank ;
	    /*log(("timer %s", time_to_string(time))) ;*/

	    s->next_sweep = time_add(time, s->sweep) ;
	    dnnm(s, event_timer_time(s->next_sweep)) ;

	    /* Go through all live members other than myself.
	     */
	    for (rank=0;rank<s->vs->nmembers;rank++) {
		seqno_t seqno ;
		seqno_t lo ;
		seqno_t hi ;
		iq_t recvs ;
		iq_t sends ;
		if (rank == s->ls->rank ||
		    array_get(s->failed, rank)) {
		    continue ;
		}

		recvs = array_get(s->recvs, rank) ;
		sends = array_get(s->sends, rank) ;

		/* Send out acknowledgements.
		 */
		lo = iq_lo(recvs) ;
		if (lo > array_get(s->acked, rank)) {
		    marsh_t msg = marsh_create(NULL) ;
		    marsh_seqno(msg, lo) ;
		    marsh_enum(msg, ACK, MAX) ;
		    log(("xmit ack(%llu) to %d last was %llu", 
			 lo, rank, array_get(s->acked, rank))) ;
		    marsh_seqno(msg, lo) ;
		    marsh_enum(msg, ACK, MAX) ;
		    dn(s, event_send_peer(rank), msg) ;
		    array_set(s->acked, rank, lo) ;
		}
#if 1
		/* Retransmit any unacknowledged messages I've sent
		 * up to the previous rounds.  This gives the other
		 * endpoints a chance to acknowledge my messages.  
		 */
		lo = iq_lo(sends) ;
		hi = array_get(s->retrans1, rank) ;
		array_set(s->retrans1, rank, array_get(s->retrans0, rank)) ;
		array_set(s->retrans0, rank, iq_hi(sends)) ;

		for (seqno=lo;seqno<hi;seqno++) {
		    marsh_buf_t abv ;
		    marsh_t msg ;
		    if (iq_get(sends, seqno, (void**)&abv) != IQ_DATA) {
			continue ;
		    }
		    log(("retransmitting %llu to %d", seqno, rank)) ;
		    msg = marsh_of_buf(abv) ;
		    marsh_seqno(msg, seqno) ;
		    marsh_enum(msg, DATA, MAX) ;
		    dn(s, event_send_peer(rank), msg) ;
		}
#else
		/* DOESN'T work because doesn't actually send NAKS */
		hi = array_get(s->retrans, rank) ;
		array_set(s->retrans, rank, iq_hi(sends)) ;
		if (hi != iq_hi(sends)) {
		    layer_msg_t abv ;
		    seqno = hi - 1 ;

		    if (iq_get(sends, seqno, &abv) != IQ_DATA) {
			continue ;
		    }
		    log(("retransmitting %d to %d", seqno, rank) ;
		    dn(s, event_send_peer(rank), header_data(seqno), abv) ;
		}
#endif
	    }
	}
	upnm(s, e) ;
    } break ;

    case EVENT_ACCOUNT:
	/* Dump buffering information if requested.
	 */
/*
	logb("recv(bytes)=%s", seqno_array_to_string(Arraye.map iq_size s.recvs))) ;
	logb("send(bytes)=%s", seqno_array_to_string(Arraye.map iq_size s.sends))) ;
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
	/* ESend: buffer a copy and send it out.
	 */
	rank_t dest = event_peer(e) ;
	iq_t sends = array_get(s->sends, dest) ;
	seqno_t seqno ;

	if (dest == s->ls->rank) {
	    sys_panic(("send to myself:%s:%s", view_state_to_string(s->vs), event_to_string(e))) ;
	}
	seqno = iq_hi(sends) ;
	/*log(("xmit %d to %d", seqno, dest)) ;*/

	iq_add(sends, marsh_to_buf(abv)) ;

	/* Non-application messages piggyback an acknowledgement.  The key
	 * here is for the Pt2ptw ack messages to cause an ack to be piggy
	 * backed by this layer.
	 */
	if (event_appl_msg(e)) {
	    marsh_seqno(abv, seqno) ;
	    marsh_enum(abv, DATA, MAX) ;
	} else {
	    iq_t recvs = array_get(s->recvs, dest) ;
	    seqno_t head = iq_lo(recvs) ;
	    if (head <= array_get(s->acked, dest)) {
		marsh_seqno(abv, seqno) ;
		marsh_enum(abv, DATA, MAX) ;
	    } else {
		log(("sending ack(%llu) to %d last was %llu",
		     head, dest, array_get(s->acked, dest))) ;
		array_set(s->acked, dest, head) ;
		marsh_seqno(abv, head) ;
		marsh_seqno(abv, seqno) ;
		marsh_enum(abv, DATA_ACK, MAX) ;
	    }
	}
	dn(s, e, abv) ;
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
	if (rank == s->ls->rank) {
	    continue ;
	}
	iq_free(array_get(s->sends, rank)) ;
	iq_free(array_get(s->recvs, rank)) ;
    }
    array_free(s->sends) ;
    array_free(s->recvs) ;
    array_free(s->failed) ;
    array_free(s->naked) ;
    array_free(s->acked) ;
    array_free(s->retrans0) ;
    array_free(s->retrans1) ;
}

LAYER_REGISTER(pt2pt) ;
