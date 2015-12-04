/**************************************************************/
/* BOTTOM.C : bottom protocol layer */
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
#include "infr/alarm.h"
#include "infr/stack.h"

static string_t name = "BOTTOM" ;

typedef enum {
    NOHDR,
    MERGE_REQUEST,
    MERGE_GRANTED,
    MERGE_DENIED,
    MAX
} header_type_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_t all_alive ;
    bool_t block ;
    bool_array_t failed ;
    bool_t enabled ;
} *state_t ;

#include "infr/layer_supp.h"

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
    s->block = 0 ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->all_alive = TRUE ;
    s->enabled = TRUE ;
}

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "rank=%d nmembers=%d\n" ls.rank ls.nmembers ;
    sprintf "enabled=%b\n" s.enabled ;
    sprintf "all_alive=%b\n" s.all_alive ;
    sprintf "failed=%s\n" (Arrayf.bool_to_string s.failed) ;
#endif
}
#endif

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type ;
    type = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {
    case EVENT_CAST:
    case EVENT_SEND:
	if (s->all_alive ||
	    !array_get(s->failed, event_peer(e))) {

	    /* Common case: origin was alive.
	     */
	    up(s, e, abv) ;
	} else {
	    up_free(e, abv) ;
	}
	break ;

#if 0
    case EVENT_PRIVATE_ENCRYPTED:
    case EVENT_PRIVATE_DECRYPTED:
	if (s->enabled) {
	    up(s, e, abv) ;
	} else {
	    up_free(e, abv) ;
	}
	break ;
#endif

    case EVENT_MERGE_REQUEST:	
    case EVENT_MERGE_GRANTED:	
    case EVENT_MERGE_DENIED: {
	if (s->enabled) {
	    switch (type) {
	    case MERGE_REQUEST:
		e = event_set_type(e, EVENT_MERGE_REQUEST) ;
		break ;
	    case MERGE_GRANTED:
		e = event_set_type(e, EVENT_MERGE_GRANTED) ;
		break ;
	    case MERGE_DENIED:
		e = event_set_type(e, EVENT_MERGE_DENIED) ;
		break ;
	    OTHERWISE_ABORT() ;
	    }

	    up(s, e, abv) ;
	} else {
	    up_free(e, abv) ;
	}
    } break ;

    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_INIT:
    case EVENT_TIMER:
    case EVENT_ASYNC:
    case EVENT_GOSSIP_EXT:
	if (s->enabled) {
	    upnm(s, e) ;
	} else {
	    event_free(e) ;
	}
	break ;

    EVENT_DUMP_HANDLE() ;

    OTHERWISE_ABORT() ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    if (s->enabled) {
	switch(event_type(e)) {
#if 0
	//case EVENT_PRIVATE_DEC:
	//case EVENT_PRIVATE_ENC:
#endif

	case EVENT_CAST:
	case EVENT_SEND:
	case EVENT_CAST_UNREL:
	case EVENT_SEND_UNREL:
	    /* Note the ESendUnrel/CastUnrel are delivered as ESend
	     * and ECast.  
	     */
	    /*eprintf("BOTTOM:dn type=%s\n", event_type_to_string(event_type(e))) ;*/
	    marsh_enum(abv, NOHDR, MAX) ;
	    dn(s, e, abv) ;
	    break ;

	case EVENT_MERGE_REQUEST:
	    marsh_enum(abv, MERGE_REQUEST, MAX) ;
	    dn(s, e, abv) ;
	    break ;

	case EVENT_MERGE_GRANTED:
	    marsh_enum(abv, MERGE_GRANTED, MAX) ;
	    dn(s, e, abv) ;
	    break ;
	    
	case EVENT_MERGE_DENIED: 
	    marsh_enum(abv, MERGE_DENIED, MAX) ;
	    dn(s, e, abv) ;
	    break ;

	OTHERWISE_ABORT() ;
	}
    } else {
        dn_free(e, abv) ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    if (!s->enabled) {
	event_free(e) ;
	return ;
    }

    /*eprintf("%s:dnnm:%s\n", name, event_type_to_string(event_type(e))) ;*/
    
    switch(event_type(e)) {
    case EVENT_GOSSIP_EXT:
	dnnm(s, e) ;
	break ;

    case EVENT_TIMER:
	dnnm(s, e) ;

#if 0
	/* ETimer: request a timeout callback from the transport.
	 */
	if (time_is_zero(time)) {
	    upnm (create name ETimer[Time (Alarm.gettime s.alarm)]) ;
	    free(name, e)
      	}
#endif
        break ;

    case EVENT_FAIL:
	/* EFail: mark the members as failed and bounce the event.
	 */
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	s->all_alive = !bool_array_exists(s->failed, s->vs->nmembers) ;
	upnm(s, e) ;
	break ;
	
    case EVENT_EXIT:
	/* EExit: disable the transport and send up an EExit event.
	 * Mark myself and all members as disabled.
	 */
	s->enabled = FALSE ;
	s->all_alive = FALSE ;
	array_free(s->failed) ;
	s->failed = bool_array_create_init(s->vs->nmembers, TRUE) ;

	/*let time = Alarm.gettime s.alarm in
	  upnm (create name EExit[Time time]) ;*/
	upnm(s, e) ;
	
	/* Pass event down.
	 */
	dnnm(s, event_create(EVENT_EXIT)) ;
	break ;
	
	/* All of these we bounce up as-is.
	 */
    case EVENT_BLOCK:
    case EVENT_SUSPECT:
    case EVENT_XFER_DONE:
    case EVENT_REKEY:
    case EVENT_PROMPT:
    case EVENT_PROTO:
    case EVENT_MIGRATE:
    case EVENT_VIEW:
    case EVENT_ELECT:
    case EVENT_STABLE:
    case EVENT_LEAVE:
    case EVENT_BLOCK_OK:
    case EVENT_ACCOUNT:
    case EVENT_SYNC_INFO:
    case EVENT_STABLE_REQ:
    case EVENT_PRESENT:		/* BUG? */
	upnm(s, e) ;
	break ;

    OTHERWISE_ABORT() ;
    }
}

static void free_handler(state_t s) {
    array_free(s->failed) ;
}
 
LAYER_REGISTER(bottom) ;
