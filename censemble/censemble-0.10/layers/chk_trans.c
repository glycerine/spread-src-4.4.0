/**************************************************************/
/* CHK_TRANS.C */
/* Author: Mark Hayden, 11/99 */
/* Tests for FIFO ordering properties on casts and sends */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#define MAGIC (0xFEEDFACEUL)
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "CHK_TRANS" ;

typedef enum { NOHDR, CAST, SEND, MAX } header_type_t ;

static seqno_t unique ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    debug_t debug ;
    bool_array_t failed ;
} *state_t ;

#include "infr/layer_supp.h"

static void dump(state_t s) {
    eprintf("CHK_TRANS\n") ;
    eprintf("  failed=%s\n", bool_array_to_string(s->failed, s->vs->nmembers)) ;
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
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type UNUSED() = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {

    case EVENT_CAST_UNREL:
    case EVENT_CAST: {
	rank_t origin = event_peer(e) ;
	rank_t chk_origin ;
	seqno_t id ;
	bool_t err0 ;
	assert(type == CAST) ;
	unmarsh_rank(abv, &chk_origin) ;
	unmarsh_seqno(abv, &id) ;
	
	err0 = chk_origin != origin ;
	
	if (err0) {
	    dump(s) ;
	    eprintf("CHK_TRANS:event=%s\n", event_to_string(e)) ;
	    
	    if (err0) {
		eprintf("CHK_TRANS:bad-origin=%d\n", err0) ;
		eprintf("  msg_origin=%d\n", chk_origin) ;
		eprintf("  ev_origin=%d my_rank=%d\n", origin, s->ls->rank) ;
	    }
	    sys_abort() ;
	}
	up(s, e, abv) ;
    } break ;

    case EVENT_SEND_UNREL:
    case EVENT_SEND: {
	rank_t origin = event_peer(e) ;
	rank_t chk_origin ;
	rank_t dest ;
	bool_t err0, err1 ;
	uint32_t magic ;
	assert(type == SEND) ;
	unmarsh_mux(abv, &magic) ;
	assert(magic == MAGIC) ;
	unmarsh_rank(abv, &chk_origin) ;
	unmarsh_rank(abv, &dest) ;
	
	if (array_get(s->failed, origin)) {
	    sys_panic(("recd send from failed member")) ;
	}
	
	err0 = chk_origin != origin ;
	err1 = dest != s->ls->rank ;
	
	if (err0 || err1) {
	    dump(s) ;
	    eprintf("CHK_TRANS:ESend:sanity check failed\n") ;
	    eprintf("  bad-origin=%d bad-destination=%d\n", err0, err1) ;
	    eprintf("  msg_origin=%d msg_dest=%d\n", chk_origin, dest) ;
	    eprintf("  ev_origin=%d my_rank=%d\n", origin, s->ls->rank) ;
	    sys_abort() ;
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

    case EVENT_CAST_UNREL:
    case EVENT_CAST:
	marsh_seqno(abv, unique++) ;
	marsh_rank(abv, s->ls->rank) ;
	marsh_enum(abv, CAST, MAX) ;
	dn(s, e, abv) ;
	break ;

    case EVENT_SEND_UNREL:
    case EVENT_SEND: {
	rank_t dest = event_peer(e) ;
	marsh_rank(abv, dest) ;
	marsh_rank(abv, s->ls->rank) ;
	marsh_mux(abv, MAGIC) ;
	marsh_enum(abv, SEND, MAX) ;
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
}

LAYER_REGISTER(chk_trans) ;
