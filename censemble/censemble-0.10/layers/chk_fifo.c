/**************************************************************/
/* CHK_FIFO.C */
/* Author: Mark Hayden, 11/99 */
/* Tests for FIFO ordering properties on casts and sends */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef MINIMIZE_CODE

#define MAGIC (0xFEEDFACEUL)
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "CHK_FIFO" ;

typedef enum { NOHDR, CAST, SEND, MAX } header_type_t ;

static seqno_t unique ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    debug_t debug ;
    bool_array_t failed ;
    seqno_t cast_dn ;
    seqno_array_t cast_up ;
    seqno_array_t send_dn ;
    seqno_array_t send_up ;
} *state_t ;

#include "infr/layer_supp.h"

static void dump(state_t s) {
    eprintf("CHK_FIFO\n") ;
    eprintf("  failed=%s\n", bool_array_to_string(s->failed, s->vs->nmembers)) ;
    eprintf("  cast_dn=%llu\n", s->cast_dn) ;
    eprintf("  cast_up=%s\n", seqno_array_to_string(s->cast_up, s->vs->nmembers)) ;
    eprintf("  send_dn=%s\n", seqno_array_to_string(s->send_dn, s->vs->nmembers)) ;
    eprintf("  send_up=%s\n", seqno_array_to_string(s->send_up, s->vs->nmembers)) ;
}

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
    s->debug = "(?!?)" ;
    s->cast_dn = 0 ;
    s->cast_up = seqno_array_create_init(vs->nmembers, 0) ;
    s->send_dn = seqno_array_create_init(vs->nmembers, 0) ;
    s->send_up = seqno_array_create_init(vs->nmembers, 0) ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {
    case EVENT_CAST: {
	if (type != NOHDR) {
	    rank_t origin = event_peer(e) ;
	    rank_t chk_origin ;
	    seqno_t id ;
	    seqno_t seqno ;
	    bool_t err0 ;
	    bool_t err1 ;
	    assert(type == CAST) ;
	    unmarsh_rank(abv, &chk_origin) ;
	    unmarsh_seqno(abv, &id) ;
	    unmarsh_seqno(abv, &seqno) ;

	    /*eprintf "CHK_FIFO:%s:CAST+:%s:" name (Endpt.string_of_id ls.endpt) ; eprint_members members ;*/
	/*eprintf "CHK_FIFO:%s:%d->%d:receiving #%d (unique_id=%d,actual=%s,expect=%d)\n"
	  ls.name origin ls.rank seqno unique_id (Endpt.string_of_id chk_from) s.cast_up.(origin) ;*/

	    err0 = seqno != array_get(s->cast_up, origin) ;
	    err1 = chk_origin != origin ;
	    
	    if (err0 || err1) {
		eprintf("CHK_FIFO:event=%s\n", event_to_string(e)) ;

		if (err0) {
		    eprintf("CHK_FIFO:%s:multicast message out of order unique_id=%llu (origin=%d, my_rank=%d, seqno=%llu, expect=%llu)\n",
			    s->debug, id, origin, s->ls->rank, seqno, array_get(s->cast_up, origin)) ;
		}
		
		if (err1) {
		    if (err1) {
			eprintf("CHK_FIFO:***AND***\n") ;
		    }
		    eprintf("CHK_FIFO:incorrect origin msg=(%d) <> (%s,%d)=my_info\n",
			    chk_origin, endpt_id_to_string(&array_get(s->vs->view, origin)), origin) ;
		}
		dump(s) ;
		/*dnnm(s, dumpEv name);*/
		sys_abort() ;
	    }
	    
	    seqno_array_incr(s->cast_up, origin) ;
	}
	    
	up(s, e, abv) ;
    } break ;

    case EVENT_SEND: {
	if (type != NOHDR) {
	    rank_t origin = event_peer(e) ;
	    rank_t chk_origin ;
	    rank_t dest ;
	    seqno_t seqno ;
	    bool_t err0, err1 ;
	    uint32_t magic ;
	    assert(type == SEND) ;
	    unmarsh_mux(abv, &magic) ;
	    assert(magic == MAGIC) ;
	    unmarsh_rank(abv, &chk_origin) ;
	    unmarsh_rank(abv, &dest) ;
	    unmarsh_seqno(abv, &seqno) ;

	    if (dest != s->ls->rank) {
		eprintf("CHK_FIFO:got send message, but not destination msg=%d me=%d\n",
			dest, s->ls->rank /*, origin, chk_origin*/) ;
		eprintf("  ev_origin=%d msg_origin=%d seqno=%llu\n", origin, chk_origin, seqno) ;

		sys_abort() ;
		/*dnnm (dumpEv name)*/
	    }

	    if (array_get(s->failed, origin)) {
		sys_panic(("got send from failed member")) ;
	    }

	    err0 = seqno != array_get(s->send_up, origin) ;
	    err1 = chk_origin != origin ;
	    if (err0 || err1) {
		dump(s) ;
		eprintf("CHK_FIFO:ESend.Send:sanity check failed\n") ;
		eprintf("  out-of-order=%d, origin-incorrect=%d\n", err0, err1) ;
		eprintf("  my_info: msg %llu, %%s -> %s\n", array_get(s->send_up, origin),
			/*endpt_id_to_string origine),*/ endpt_id_to_string(&s->ls->endpt)) ;

		/*eprintf("  message: msg %d, %s -> %s\n", seqno, endpt_id_to_string(s->ls->from),  (Endpt.string_of_id dest) ;*/
		sys_abort() ;
/*
        dnnm (dumpEv name)
*/
	    }

	    seqno_array_incr(s->send_up, origin) ;
	}
	up(s, e, abv) ;
    } break ;
    
    default:
	up(s, e, abv) ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_LOST_MESSAGE:
	seqno_array_incr(s->cast_up, event_peer(e)) ;
	upnm(s, e) ;
	break ;

    case EVENT_FAIL:
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
	marsh_seqno(abv, s->cast_dn ++) ;
	marsh_seqno(abv, unique++) ;
	marsh_rank(abv, s->ls->rank) ;
	marsh_enum(abv, CAST, MAX) ;
	dn(s, e, abv) ;
	break ;

    case EVENT_SEND: {
	rank_t dest = event_peer(e) ;
	marsh_seqno(abv, array_get(s->send_dn, dest)) ;
	marsh_rank(abv, dest) ;
	marsh_rank(abv, s->ls->rank) ;
	marsh_mux(abv, MAGIC) ;
	marsh_enum(abv, SEND, MAX) ;
	seqno_array_incr(s->send_dn, dest) ;
	dn(s, e, abv) ;
    } break ;

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
    array_free(s->failed) ;
    array_free(s->cast_up) ;
    array_free(s->send_up) ;
    array_free(s->send_dn) ;
}

LAYER_REGISTER(chk_fifo) ;
#endif
