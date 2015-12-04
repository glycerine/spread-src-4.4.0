/**************************************************************/
/* SYNC.C */
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

static string_t name = "SYNC" ;

typedef enum { NOHDR, BLOCK, BLOCK_OK, MAX } header_type_t ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_array_t failed ;
    rank_t coord ;		/* who do I think is coord? hack */
    bool_t dn_block ;		/* have I passed down a EBlock? */
    bool_t req_up_block_ok ;	/* have I got a EBlock from above? */
    bool_t up_block_ok ;	/* have I passed up an EBlockOk? */
    bool_array_t block_ok ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "req_up_block_ok=%b rank=%d\n" s.req_up_block_ok ls.rank ;
    sprintf "dn_block=%b up_block_ok=%b\n" s.dn_block s.up_block_ok ;
    sprintf "failed  =%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "block_ok=%s\n" (string_of_bool_array s.block_ok)
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
    s->req_up_block_ok = FALSE ;
    s->coord = 0 ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->dn_block = FALSE ;
    s->up_block_ok = FALSE ;
    s->block_ok = bool_array_create_init(vs->nmembers, FALSE) ;
}

static void do_gossip(state_t s) {
    marsh_t msg ;

    if (s->ls->rank == s->coord ||
	!bool_array_exists(s->block_ok, s->vs->nmembers)) {
	return ;
    }

    log_sync(("sending BlockOk to coordinator coord=%d oks=%s",
	     s->coord, bool_array_to_string(s->block_ok, s->vs->nmembers))) ;
    msg = marsh_create(NULL) ;
    marsh_bool_array(msg, s->block_ok, s->vs->nmembers) ;
    marsh_enum(msg, BLOCK_OK, MAX) ;
    dn(s, event_send_peer(s->coord), msg) ;
}

static void check_ok(state_t s) {
    if (s->req_up_block_ok &&
	!s->up_block_ok) {
	const_bool_array_t tmp = bool_array_map_or(s->block_ok, s->failed, s->vs->nmembers) ;
	if (bool_array_forall(tmp, s->vs->nmembers)) {
	    log_sync(("releasing EBlockOk")) ;
	    s->up_block_ok = TRUE ;
	    upnm(s, event_create(EVENT_BLOCK_OK)) ;
	}
	array_free(tmp) ;
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type = unmarsh_enum_ret(abv, MAX) ;
    switch(type) {
    case NOHDR:
	up(s, e, abv) ;
	break ;

    case BLOCK:
	/* Block: cast from coordinator. 
	 */
	if (!s->dn_block) {
	    s->dn_block = TRUE ;
	    dnnm(s, event_create(EVENT_BLOCK)) ;
	}
	up_free(e, abv) ;
	break ;
	
    case BLOCK_OK: {
	/* BlockOk: Got block Ok from other members, mark him
	 * as OK and check whether we're done blocking.
	 */
	const_bool_array_t block_ok ;
	const_bool_array_t tmp ;
	unmarsh_bool_array(abv, &block_ok, s->vs->nmembers) ;
	tmp = s->block_ok ;
	s->block_ok = bool_array_map_or(s->block_ok, block_ok, s->vs->nmembers) ;
	array_free(tmp) ;
	log_sync(("coord got block_ok from %d is %s",
		  event_peer(e), bool_array_to_string(s->block_ok, s->vs->nmembers))) ;
	check_ok(s) ;
	array_free(block_ok) ;
	up_free(e, abv) ;
    } break ;

    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_FAIL:
	/* EFail: Mark some members as being failed.  Check if
	 * we're done blocking.
	 */
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;

	/* Hack!  Recompute the rank of the coordinator. 
	 */
	s->coord = bool_array_min_false(s->failed, s->vs->nmembers) ;
	do_gossip(s) ;
	upnm(s, e) ;
	check_ok(s) ;
	break ;

    case EVENT_BLOCK_OK:
	/* EBlockOk: Collect the EBlockOk.
	 */
	log_sync(("capturing EBlockOk nmembers=%d", s->vs->nmembers)) ;
	array_set(s->block_ok, s->ls->rank, TRUE) ;
	do_gossip(s) ;
	check_ok(s) ;
	event_free(e) ;
	break ;

    case EVENT_ACCOUNT:
/*
  	sprintf "req_up_block_ok=%b" s.req_up_block_ok ;
  	sprintf "dn_block=%b up_block_ok=%b" s.dn_block s.up_block_ok ;
  	sprintf "failed  =%s" (Arrayf.bool_to_string s.failed) ;
  	sprintf "block_ok=%s" (string_of_bool_array s.block_ok)
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
    marsh_enum(abv, NOHDR, MAX) ;
    dn(s, e, abv) ;
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_BLOCK:
	/* Layer above has started blocking group.  If blocking
	 * is already done then deliver EBlockOk.  
	 */
	assert(!s->req_up_block_ok) ;
	s->req_up_block_ok = TRUE ;
	if (!s->dn_block) {
	    marsh_t msg = marsh_create(NULL) ;
	    log_sync(("broadcasting Block")) ;
	    s->dn_block = TRUE ;
	    marsh_enum(msg, BLOCK, MAX) ;
	    dn(s, event_cast(), msg) ;
	    dnnm(s, e) ;
	} else {
	    event_free(e) ;
	}
	break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->failed) ;
    array_free(s->block_ok) ;
}

LAYER_REGISTER(sync) ;
