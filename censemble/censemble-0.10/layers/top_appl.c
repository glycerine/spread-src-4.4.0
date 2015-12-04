/**************************************************************/
/* TOP_APPL.C */
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
#include "infr/appl_intf.h"

static string_t name = "TOP_APPL" ;

typedef enum { NOHDR, DATA, MAX } header_type_t ;

typedef enum {
    UNBLOCKED,
    DN_BLOCKING,
    DN_BLOCKED,
    UP_BLOCKING,
    UP_BLOCKED,
} blocking_t ;

#if 0
static string_t string_of_blocking(blocking_t blocking) {
    string_t ret ;
    switch(blocking) {
    case UNBLOCKED:
	ret = "unblocked" ;
	break ;
    case DN_BLOCKING:
	ret = "dn_blocking" ;
	break ;
    case DN_BLOCKED:
	ret = "dn_blocked" ;
	break ;
    case UP_BLOCKING:
	ret = "up_blocking" ;
	break ;
    case UP_BLOCKED:
	ret = "up_blocked" ;
	break ;
    OTHERWISE_ABORT() ;
    }
    return ret ;
}
#endif
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;

    /*recv_cast             : (Iovecl.t -> Iovecl.t naction array) array ;*/
    /*recv_send             : (Iovecl.t -> Iovecl.t naction array) array ;*/
    appl_intf_t interface ;
    env_t main_env ;
    env_t view_env ;
    seqno_array_t send_xmit ;
    seqno_array_t send_recv ;
    seqno_array_t cast_recv ;
    seqno_t cast_xmit ;
    bool_t got_expect ;
    seqno_array_t cast_expect ;
    seqno_array_t send_expect ;
    bool_t leaving ;
    etime_t next_sweep ;
    bool_array_t failed ;
    blocking_t blocking ;

    bool_t appl_block ;
    equeue_t xmit ;
} *state_t ;

#include "infr/layer_supp.h"

static bool_t unblocked(state_t s) {
    return (s->blocking == UNBLOCKED ||
	    s->blocking == DN_BLOCKING) ;
}

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "got_expect=%b blocking=%s\n" s.got_expect (string_of_blocking s.blocking) ;
    sprintf "failed=%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "cast_xmit=%d\n" s.cast_xmit ;
    sprintf "cast_recv  =%s\n" (string_of_int_array s.cast_recv) ;
    sprintf "cast_expect=%s\n" (Arrayf.int_to_string s.cast_expect) ;
    sprintf "send_xmit  =%s\n" (string_of_int_array s.send_xmit) ;
    sprintf "send_recv  =%s\n" (string_of_int_array s.send_recv) ;
    sprintf "send_expect=%s\n" (Arrayf.int_to_string s.send_expect)
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
    s->interface = state->appl_intf ;
    s->main_env = state->appl_intf_env ;
    s->view_env = NULL ;
    s->cast_xmit = 0 ;
    s->cast_recv = seqno_array_create_init(vs->nmembers, 0) ;
    s->send_recv = seqno_array_create_init(vs->nmembers, 0) ;
    s->send_xmit = seqno_array_create_init(vs->nmembers, 0) ;
    s->send_expect = seqno_array_create_init(vs->nmembers, 0) ;
    s->cast_expect = seqno_array_create_init(vs->nmembers, 0) ;
    s->leaving = FALSE ;
    s->next_sweep = time_zero() ;
    s->got_expect = FALSE ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->blocking = UNBLOCKED ;

    s->appl_block = FALSE ;
    s->xmit = equeue_create(name, sizeof(appl_action_t)) ;
}

static void check_block_ok(state_t s) {
    rank_t rank ;

    if (s->blocking != UP_BLOCKING ||
	!s->got_expect) {
	log_sync(("check_block_ok:%d", s->got_expect)) ;
	return ;
    }
    
    for (rank=0;rank<s->vs->nmembers;rank++) {
	if (rank == s->ls->rank) {
	    continue ;
	}
	if (array_get(s->cast_recv, rank) <
		array_get(s->cast_expect, rank)) {
	    break ;
	}
	if (!array_get(s->failed, rank) &&
	    (array_get(s->send_recv, rank) <
	     array_get(s->send_expect, rank))) {
	    break ;
	}
    }
    if (rank == s->vs->nmembers) {
	log_sync(("release EBlockOk (cast=%s)", seqno_array_to_string(s->cast_recv, s->vs->nmembers))) ;
	s->blocking = UP_BLOCKED ;
	    upnm(s, event_create(EVENT_BLOCK_OK)) ;
    }
}

static void handle_action(state_t s, appl_action_t a) {
    assert(a) ;
    switch(a->type) {
	
    case APPL_CAST: {
	marsh_t msg = marsh_create(a->u.cast) ;
	assert(unblocked(s)) ;
	s->cast_xmit ++ ;
	assert(a->u.cast) ;
	marsh_enum(msg, DATA, MAX) ;
	dn(s, event_cast_appl(), msg) ;
    } break ;

    case APPL_SEND: {
	ofs_t ofs ;
	assert(unblocked(s)) ;
	assert_array_length(a->u.send.dests, a->u.send.ndests) ;
	for(ofs=0;ofs<a->u.send.ndests;ofs++) {
	    marsh_t msg = marsh_create(a->u.send.msg) ;
	    rank_t dest = array_get(a->u.send.dests, ofs) ;
	    /*assert(dest != s->ls->rank) ;*/
	    assert(dest >= 0 && dest < s->vs->nmembers) ;
	    seqno_array_incr(s->send_xmit, dest) ;
	    assert(a->u.send.msg) ;
	    marsh_enum(msg, DATA, MAX) ;
	    dn(s, event_send_peer_appl(dest), msg) ;
	}
    } break ;

    case APPL_SEND1: {
	marsh_t msg = marsh_create(a->u.send1.msg) ;
	rank_t dest = a->u.send1.dest ;
	assert(unblocked(s)) ;
	/*assert(dest != s->ls->rank) ;*/
	assert(dest >= 0 && dest < s->vs->nmembers) ;
	seqno_array_incr(s->send_xmit, dest) ;
	assert(a->u.send1.msg) ;
	marsh_enum(msg, DATA, MAX) ;
	dn(s, event_send_peer_appl(dest), msg) ;
    } break ;

    case APPL_LEAVE:
	s->leaving = TRUE ;
	dnnm(s, event_create(EVENT_LEAVE)) ;
	break ;
    
    case APPL_XFER_DONE:
	dnnm(s, event_create(EVENT_XFER_DONE)) ;
	break ;

    case APPL_PROTO:
	dnnm(s, event_set_proto(event_create(EVENT_PROTO), a->u.proto)) ;
	break ;

    case APPL_MIGRATE:
	dnnm(s, event_set_address(event_create(EVENT_MIGRATE), a->u.migrate)) ;
	break ;

    case APPL_REKEY:
	dnnm(s, event_create(EVENT_REKEY)) ;
	break ;

    case APPL_DUMP:
	dnnm(s, event_create(EVENT_DUMP)) ;
	break ;

    case APPL_PROMPT:
	dnnm(s, event_create(EVENT_PROMPT)) ;
	break ;

    case APPL_SUSPECT:
	assert_array_length(a->u.suspect, s->vs->nmembers) ; 
	assert(!array_get(a->u.suspect, s->ls->rank)) ; 
	dnnm(s, event_suspect_reason_create(a->u.suspect, name)) ;
	break ;

    case APPL_TIMEOUT:
      	sys_panic(("Timeout not supported")) ;

    case APPL_BLOCK:
	if (a->u.block) {
	    if (s->blocking == DN_BLOCKING) {
		s->blocking = DN_BLOCKED ;
		dnnm(s, event_create(EVENT_BLOCK)) ;
		check_block_ok(s) ;
	    }
	} else {
	    s->appl_block = TRUE ;
	}
	break ;

    case APPL_NO_OP:
	break ;

    OTHERWISE_ABORT() ;
    }

    record_free(a) ;
}

static void handle_actions(state_t s) {
    ofs_t ofs ;
    len_t len ;
    len = equeue_length(s->xmit) ;
    for (ofs=0;ofs<len;ofs++) {
	appl_action_t *actionp ;
	actionp = equeue_take(s->xmit) ;
	handle_action(s, *actionp) ;
    }
    assert(equeue_empty(s->xmit)) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t msg) {
    header_type_t type = unmarsh_enum_ret(msg, MAX) ;
    switch (type) {

    case NOHDR:
	up(s, e, msg) ;
	break ;

    case DATA:
	switch (event_type(e)) {
	case EVENT_CAST: {
	    rank_t origin = event_peer(e) ;
	    seqno_array_incr(s->cast_recv, origin) ;
	    if (s->got_expect &&
		(array_get(s->cast_recv, origin) >
		 array_get(s->cast_expect, origin))) {
		sys_panic(("bad cast")) ;
	    }

	    s->interface->receive(s->view_env, origin, 
				  FALSE, (s->blocking != UNBLOCKED),
				  unmarsh_to_iovec(msg)) ; 
	    handle_actions(s) ;
	    if (s->blocking == UP_BLOCKING) {
		check_block_ok(s) ;
	    }
	    event_free(e) ;
	} break ;

	case EVENT_SEND: {
	    rank_t origin = event_peer(e) ;
	    seqno_array_incr(s->send_recv, origin) ;
	    if (s->got_expect &&
		(array_get(s->send_recv, origin) >
		 array_get(s->send_expect, origin))) {
		sys_panic(("bad send")) ;
	    }

	    s->interface->receive(s->view_env, origin, 
				  TRUE, (s->blocking != UNBLOCKED), 
				  unmarsh_to_iovec(msg)) ;
	    handle_actions(s) ;
	    if (s->blocking == UP_BLOCKING) {
		check_block_ok(s) ;
	    }
	    event_free(e) ;
	} break ;

	OTHERWISE_ABORT() ;
	}
	break ;

    OTHERWISE_ABORT() ;
    }    
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_INIT: {
	/* Send the event on.
	 */
	upnm(s, e) ;

	{
	    /* Copy the view state and pass it up.
	     * The application owns the reference.
	     */
	    view_state_t vs ;
	    view_local_t ls ;
	    vs = view_state_copy(s->vs) ;
	    ls = view_local(&s->ls->endpt, s->vs) ;
	    s->view_env = s->interface->install(s->main_env, ls, vs, s->xmit) ;
	}

	handle_actions(s) ;

	/* Request first alarm.
	 */
	dnnm(s, event_timer_time(time_zero())) ; 
    } break ;

    /* Drain application actions.
     */
    case EVENT_ASYNC:
	if (unblocked(s)) {
	    handle_actions(s) ;
	}
	event_free(e) ;
	break ;

    case EVENT_TIMER: {
	etime_t time = event_time(e) ;
	if (time_ge(time, s->next_sweep) &&
	    unblocked(s)) {
	    /* Schedule a new heartbeat.
	     */
	    s->next_sweep = time_add(time, s->interface->heartbeat_rate) ;

	    dnnm(s, event_timer_time(s->next_sweep)) ;
	    
	    /* Wake up the application.
	     */
	    s->interface->heartbeat(s->view_env, time) ;
	    handle_actions(s) ;
	}
	upnm(s, e) ;
    } break ;

    case EVENT_EXIT:
	s->interface->disable(s->view_env) ;

	if (s->leaving) {
	    s->interface->exit(s->main_env) ;
	}
	
	upnm(s, e) ;
	break ;
	
    case EVENT_BLOCK:
    case EVENT_FAIL: {
	/* Add message count info into Block and Fail events.
	 * Other layers should not use this if we have not
	 * yet blocked.
	 */
	seqno_array_t cast_recv ;

	if (event_type(e) == EVENT_FAIL) {
	    bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	}
	
	assert(s->cast_xmit < 100000) ;
	log_sync(("EBlock|EFail cast_xmit=%llu", s->cast_xmit)) ;
	cast_recv = seqno_array_copy(s->cast_recv, s->vs->nmembers) ;
	array_set(cast_recv, s->ls->rank, s->cast_xmit) ;
	
	log_sync(("EBlock|EFail cast_recv=%s", seqno_array_to_string(cast_recv, s->vs->nmembers))) ;

	e = event_set_appl_casts(e, cast_recv) ;
	e = event_set_appl_sends(e, seqno_array_copy(s->send_xmit, s->vs->nmembers)) ;
	upnm(s, e) ;
	check_block_ok(s) ;
    } break ;

    case EVENT_SYNC_INFO: {
	rank_t i ;
	s->got_expect = TRUE ;
	seqno_array_copy_into(s->cast_expect, event_appl_casts(e), s->vs->nmembers) ;
	seqno_array_copy_into(s->send_expect, event_appl_sends(e), s->vs->nmembers) ;
	log_sync(("got ESyncInfo expect  cast=%s send=%s", 
		  seqno_array_to_string(s->cast_expect, s->vs->nmembers),
		  seqno_array_to_string(s->send_expect, s->vs->nmembers))) ;
	log_sync(("got ESyncInfo receive cast=%s send=%s", 
		  seqno_array_to_string(s->cast_recv, s->vs->nmembers),
		  seqno_array_to_string(s->send_recv, s->vs->nmembers))) ;
	
	for(i=0;i<s->vs->nmembers;i++) {
	    if (array_get(s->cast_recv, i) >
		array_get(s->cast_expect, i)) {
		sys_panic(("sanity")) ;
	    }
	}
	check_block_ok(s) ;
	upnm(s, e) ;
    } break ;
	
    case EVENT_BLOCK_OK: {
	assert(s->blocking == DN_BLOCKED) ;
	if (event_no_vsync(e)) {
	    upnm(s, e) ;
	} else {
	    log_sync(("capture EBlockOk")) ;
	    s->blocking = UP_BLOCKING ;
	    check_block_ok(s) ;
	    event_free(e) ;
	}
    } break ;

/*
    case EVENT_FLOWBLOCK: 
      let fb = getFlowBlock ev in
      s.handlers.flow_block fb ;
      free name ev
*/

    case EVENT_ACCOUNT:
/*
	logb (fun () -> [
  	sprintf "blocking=%s" (string_of_blocking s.blocking) ;
	sprintf "failed     =%s" (Arrayf.bool_to_string s.failed) ;
  	sprintf "cast_recv  =%s" (string_of_int_array s.cast_recv) ;
  	sprintf "cast_expect=%s" (Arrayf.int_to_string s.cast_expect) ;
  	sprintf "send_recv  =%s" (string_of_int_array s.send_recv) ;
  	sprintf "send_expect=%s" (Arrayf.int_to_string s.send_expect)
      ]) ;
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
    case EVENT_CAST:
	/* Hack!!! Add NoTotal flag to all messages from above.
	 */
	e = event_set_no_total(e, TRUE) ;
	marsh_enum(abv, NOHDR, MAX) ;
	dn(s, e, abv) ;
	break ;
	
    default:
	marsh_enum(abv, NOHDR, MAX) ;
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_BLOCK: {
	assert(s->blocking == UNBLOCKED) ;

	/* Only deliver this if we are not leaving.
	 */
	if (!s->leaving) {
	    s->interface->block(s->view_env) ;
	    handle_actions(s) ;
	} else {
	    /* I hate the hassles with leaving.  Perhaps we should
	     * deliver blocks and handle actions even when leaving?
	     */
	    assert(equeue_empty(s->xmit)) ;
	}
      
	/* This is set after the above statements so
	 * that Block(true) in the actions will not
	 * generate an EBlock event.
	 */
	s->blocking = DN_BLOCKING ;

	if (!s->appl_block) {
	    s->blocking = DN_BLOCKED ;
	    dnnm(s, event_create(EVENT_BLOCK)) ;
	}
	event_free(e) ;
    } break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {

    array_free(s->send_xmit) ;
    array_free(s->send_recv) ;
    array_free(s->cast_recv) ;
    array_free(s->cast_expect) ;
    array_free(s->send_expect) ;
    array_free(s->failed) ;

    if (!equeue_empty(s->xmit)) {
	sys_panic(("actions added after block")) ;
    }
#if 0
    {
	ofs_t ofs ;
	len_t len ;
	len = equeue_length(s->xmit) ;
	for (ofs=0;ofs<len;ofs++) {
	    appl_action_t *actionp ;
	    actionp = equeue_take(s->xmit) ;
	    appl_action_free(*actionp) ;
	}
	assert(equeue_empty(s->xmit)) ;
    }
#endif
    equeue_free(s->xmit) ;
}

LAYER_REGISTER(top_appl) ;
