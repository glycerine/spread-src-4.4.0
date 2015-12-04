/**************************************************************/
/* MNAK.C : FIFO, reliable multicast protocol */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/iq.h"
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "MNAK" ;

/* Data(seqno): sent with message number 'seqno'

 * Retrans(seqno): sent with the retransmission of member
 * 'rank's messsage number 'seqno.'

 * Nak(rank,lo,hi): request for retransmission of messages
 * number 'lo' to 'hi'. 
 * BUG: comment should say, inclusive/exclusive.
 */
typedef enum { NOHDR, MNAK_DATA, MNAK_RETRANS, MNAK_NAK, MAX } header_type_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    rank_t coord ;
    bool_array_t failed ;
    iq_array_t buf ;
    seqno_array_t naked ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "failed   =%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "cast_lo  =%s\n" (Arrayf.int_to_string (Arrayf.map Iq.lo s.buf)) ;
    sprintf "cast_hi  =%s\n" (Arrayf.int_to_string (Arrayf.map Iq.hi s.buf)) ;
    sprintf "cast_read=%s\n" (Arrayf.int_to_string (Arrayf.map Iq.read s.buf)) ;
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
    ofs_t i ;
    s->ls = ls ; 
    s->vs = vs ;
    s->layer = layer ;
    s->coord = 0 ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->naked = seqno_array_create_init(vs->nmembers, 0) ;
    s->buf = iq_array_create(vs->nmembers) ;
    for (i=0;i<vs->nmembers;i++) {
	array_set(s->buf, i, iq_create(name, (iq_free_t)marsh_buf_free)) ;
    }
}

/* CHECK_NAK: checks if missing a message, and sends appropriate
 * Nak. If there is a hole then send a Nak to (in this order): 1) to
 * original owner of message if he is not failed.  2) to coord if I'm
 * not the coord 3) to entire group.
 */
static void check_nak(state_t s, rank_t rank, bool_t is_stable) {
    iq_t buf = array_get(s->buf, rank) ;
    seqno_t lo, hi ;
    marsh_t msg ;

    if (!iq_read_hole(buf, &lo, &hi)) {
	return ;
    }

    if (!is_stable &&
	hi > array_get(s->naked, rank)) {
	return ;
    }

    /* Keep track of highest msg # we've naked.
     */
    array_set(s->naked, rank, seqno_max(array_get(s->naked, rank), hi)) ;
    logn(("send:Nak(%d,%llu..%llu)", rank, lo, hi)) ;
    
    msg = marsh_create(NULL) ;
    marsh_seqno(msg, hi) ;
    marsh_seqno(msg, lo) ;
    marsh_rank(msg, rank) ;
    marsh_enum(msg, MNAK_NAK, MAX) ;
    
    if (!array_get(s->failed, rank)) {
	dn(s, event_send_unrel_peer(rank), msg) ;
    } else if (s->coord != s->ls->rank) {
	dn(s, event_send_unrel_peer(s->coord), msg) ;
    } else {
	/* Don't forget to set the Unreliable option
	 * for the STABLE layer (see note in stable.ml).
	 */
	dn(s, event_cast_unrel(), msg) ;
    }
}


/* Arguments to two functions below:
 * - rank: rank of member who's cast I've just gotten (or heard about)
 * - seqno: seqno of message from member 'rank'
 */

/* READ_PREFIX: called to read messages from beginning of buffer.
 */
static void read_prefix(state_t s, rank_t rank) {
    iq_t buf = array_get(s->buf, rank) ;
    seqno_t seqno ;
    iq_item_t msg ;
    bool_t got_one = FALSE ;
    while (iq_read_prefix(buf, &seqno, &msg)) {
	event_t e ;
	got_one = TRUE ;
	log(("read_prefix:%d:Data(%llu)", rank, seqno)) ;
	e = event_cast_peer(rank) ;
	up(s, e, unmarsh_of_buf(msg)) ;
    }
    
    if (got_one) {
	check_nak(s, rank, FALSE) ;
    }
}

/* RECV_CAST: called when a cast message is recieved,
 * either as a data message or a retranmission.
 * This function is on the "slow path," the fast
 * path is in the up_hdlr for ECast.
 */
static void recv_cast(
        state_t s,
	rank_t rank,
	seqno_t seqno,
	unmarsh_t msg
) {
    /* Add to buffer and check if events are now in order.
     */
    iq_t buf = array_get(s->buf, rank) ;
    marsh_buf_t msg_buf = unmarsh_to_buf(msg) ;
    unmarsh_free(msg) ;
    if (iq_assign(buf, seqno, msg_buf)) {
	log(("recv_cast:%d:assign=%llu", rank, seqno)) ;
	read_prefix(s, rank) ;
    } else {
	log(("recv_cast:%d:dropping redundant msg:seqno=%llu", rank, seqno)) ;
	marsh_buf_free(msg_buf) ;
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (type) {
    case NOHDR:
	up(s, e, abv) ;
	break ;
	
    case MNAK_DATA: {
	/* ECast:Data: Got a data message from other
	 * member.  Check for fast path or call recv_cast.
	 */
	seqno_t seqno ;
	rank_t origin ;
	iq_t buf ;
	
	unmarsh_seqno(abv, &seqno) ;
	origin = event_peer(e) ;
	buf = array_get(s->buf, origin) ;
	assert(!array_get(s->failed, origin)) ;
	
	/* Check for fast-path.
	 */
	if (iq_opt_insert_check(buf, seqno)) {
	    /* Fast-path.
	     */
	    iq_opt_insert_doread(buf, seqno, unmarsh_to_buf(abv)) ;
	    up(s, e, abv) ;
	} else {
	    recv_cast(s, origin, seqno, abv) ;
	    event_free(e) ;
	}
    } break ;
    
    case MNAK_NAK: {
	rank_t rank ;
	seqno_t lo, hi ;
	seqno_t seqno ;
	iq_t buf ;
	rank_t origin ;
	
	/* Nak: got a request for retransmission.  Send any
	 * messages I have in the requested interval, lo..hi.
	 */
	assert(type == MNAK_NAK) ;
	unmarsh_rank(abv, &rank) ;
	unmarsh_seqno(abv, &lo) ;
	unmarsh_seqno(abv, &hi) ;
	
	/* Retransmit any of the messages asked for that I have.
	 */
	/* TODO: check if request is for message from failed member
	 * and I'm coordinator and I don't have what is being asked for
	 */
	origin = event_peer(e) ;
        logn(("recd:origin=%d:original=%d:Nak(%llu..%llu)", origin, rank, lo, hi)) ;
	buf = array_get(s->buf, rank) ;
	
	/* We do not retransmit messages that we have not been
	 * able to read.  See the notes for Iq.clean_unread in
	 * the Efail handler.  Note: this should probably be
	 * [pred (Iq.read buf)], but then again there should
	 * be no messages in that last slot.
	 */
	hi = seqno_min(hi, iq_read(buf)) ;
	
	for(seqno=lo;seqno<=hi;seqno++) {
	    iq_status_t status ;
	    marsh_buf_t mbuf ;
	    status = iq_get(buf, seqno, (void*)&mbuf) ;
	    if (status == IQ_DATA) {
		marsh_t msg = marsh_of_buf(mbuf) ;
		marsh_seqno(msg, seqno) ;
		marsh_rank(msg, rank) ;
		marsh_enum(msg, MNAK_RETRANS, MAX) ;
		dn(s, event_send_unrel_peer(origin), msg) ;
	    } else {
		/* Do nothing...
		 */
		log(("Nak from %d for message I think is %s", origin, iq_string_of_status(status))) ;
	    }
	}
	up_free(e, abv) ;
    } break ;
    
    case MNAK_RETRANS: {
	/* Retrans: Got a retransmission.  Always use the slow
	 * path.  
	 */
	rank_t rank ;
	seqno_t seqno ;
	assert(type == MNAK_RETRANS) ;
	unmarsh_rank(abv, &rank) ;
	unmarsh_seqno(abv, &seqno) ;
	
	if (rank != s->ls->rank) {
	    log(("retrans:%d->%d->me:seqno=%llu", rank, event_peer(e), seqno)) ;
	    recv_cast(s, rank, seqno, abv) ;
	    event_free(e) ;
	} else {
	    up_free(e, abv) ;
	}
    } break ;
    
    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    /* EFail: Mark failed members, find out who is the new
     * coordinator, and pass on up.  
     */
    case EVENT_FAIL: {
	rank_t i ;

	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	
	/* Find out who's the new coordinator.
	 */
	s->coord = bool_array_min_false(s->failed, s->vs->nmembers) ;

	/* Clear all messages in our buffers from failed
	 * processes that we haven't read yet.  If all
	 * processes do this in a consistent cut, we guarantee
	 * that any message from a failed process that can be
	 * delivered has been delivered at some process.  This
	 * is necessary for the Vsync layer to function
	 * properly.  
	 */
	for(i=0;i<s->vs->nmembers;i++) {
	    if (array_get(s->failed, i)) {
		iq_t buf = array_get(s->buf, i) ;
		log(("EFail:%d:cleaning %llu", i, iq_read(buf))) ;
		iq_clear_unread(buf) ;
	    }
	}

	upnm(s, e) ;
	break ;
    }

    case EVENT_STABLE_REQ: {
	/* Stability protocol is requesting information about my
	 * buffers.  I pass up the seqno of messages that I've
	 * read so far.  Note that I could tell about the
	 * messages I've received but not delivered yet (because
	 * they are out of order), but I suspect that would
	 * introduce liveness problems.  
	 */
	seqno_array_t casts ;
	rank_t i ;
	casts = seqno_array_create(s->vs->nmembers) ;
	for (i=0;i<s->vs->nmembers;i++) {
	    array_set(casts, i, iq_read(array_get(s->buf, i))) ;
	}
	upnm(s, event_set_num_casts(e, casts)) ;
    } break ;

    case EVENT_STABLE: {
	/* EStable: got stability and num casts information.
	 * 1. garbage collect now-stable messages.
	 * 2. check for any messages I'm missing.
	 */
	/* GC any messages that are now stable.
	 */
	const_seqno_array_t mins ;
	const_seqno_array_t maxs ;
	rank_t rank ;
	
	mins = event_stability(e) ;
	for (rank=0;rank<s->vs->nmembers;rank++) {
	    iq_t buf = array_get(s->buf, rank) ;
	    seqno_t min = array_get(mins, rank) ;
	    assert(iq_read(buf) >= min) ;
	    iq_set_lo(buf, min) ;
	}

	/* Check if there are some messages that we haven't
	 * gotten yet.  
	 */
	/* TODO: when max=stable then we must zap the iq (you remember why...) */
	maxs = event_num_casts(e) ;
	for (rank=0;rank<s->vs->nmembers;rank++) {
	    if (rank != s->ls->rank) {
		iq_t buf = array_get(s->buf, rank) ;
		iq_set_hi(buf, array_get(maxs, rank)) ; /*BUG?*/
		check_nak(s, rank, TRUE) ;
	    }
	}
	
	upnm(s, e) ;
    } break ;

    case EVENT_ACCOUNT: {
	/* Dump information about status of buffers.
	 */
/*
        logb (fun () -> sprintf "total bytes=%d" s.acct_size) ;
	logb("msgs=%s", (Arrayf.int_to_string (Arrayf.map (fun c -> Iq.read c - Iq.lo c) s.buf))) ;
*/
	upnm(s, e) ;
    } break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t msg) {
    switch(event_type(e)) {
    case EVENT_CAST: {
	/* ECast: buffer a copy and send it on.
	 */
	iq_t buf = array_get(s->buf, s->ls->rank) ;
	seqno_t seqno ;
	seqno = iq_read(buf) ;
	assert(iq_opt_insert_check(buf, seqno)) ;

	iq_opt_insert_doread(buf, seqno, marsh_to_buf(msg)) ;
/*
        s.acct_size <- s.acct_size + getIovLen ev ;
*/
	marsh_seqno(msg, seqno) ;
	marsh_enum(msg, MNAK_DATA, MAX) ;
	dn(s, e, msg) ;
    } break ;

    default:
	marsh_enum(msg, NOHDR, MAX) ;
	dn(s, e, msg) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    dnnm(s, e) ;
}

static void free_handler(state_t s) {
    rank_t rank ;
    for (rank=0;rank<s->vs->nmembers;rank++) {
	iq_free(array_get(s->buf, rank)) ;
    }
    array_free(s->buf) ;
    array_free(s->naked) ;
    array_free(s->failed) ;
}

LAYER_REGISTER(mnak) ;
