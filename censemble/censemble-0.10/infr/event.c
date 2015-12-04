/**************************************************************/
/* EVENT.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/event.h"
#include "infr/config.h"

static string_t name UNUSED() = "EVENT" ;

typedef enum {
    FIELD_ADDRESS,
    FIELD_APPL_CASTS,
    FIELD_APPL_SENDS,
    FIELD_CONTACT,	
    FIELD_DEBUG_NAME, 
    FIELD_EXCHANGE_GOSSIP,	
    FIELD_FAILURES,
    FIELD_HEAL_GOSSIP,	
    FIELD_MERGERS,	
    FIELD_NUM_CASTS,	
    FIELD_PRESENCE,
    FIELD_PROTO,	
    FIELD_STABILITY,	
    FIELD_SUSPECTS,	
    FIELD_SUSPECT_REASON,
    FIELD_SWITCH_GOSSIP,	
    FIELD_TIME,     
    FIELD_VIEW_STATE
} event_field_t ;

typedef struct event_extend_t {
    event_field_t field ;
    union {
	addr_id_t address ;
	const_seqno_array_t appl_casts ;
	const_seqno_array_t appl_sends ;
	view_contact_t contact ;
	string_t debug_name ;
	event_exchange_gossip_t exchange_gossip ;
	const_bool_array_t failures ;
	event_heal_gossip_t heal_gossip ;
	view_state_t mergers ;
	const_seqno_array_t num_casts ;
	const_bool_array_t presence ;
	proto_id_t proto ;
	const_seqno_array_t stability ;
	string_t suspect_reason ;
	event_switch_gossip_t switch_gossip ;
	const_bool_array_t suspects ;
	etime_t time ;
	view_state_t view_state ;
    } u ;
    struct event_extend_t *next ;
} *event_extend_t ;

struct event_t {
    event_type_t type ;
    rank_t peer ;
    bool_t no_total : 1 ;
    bool_t server_only : 1 ;
    bool_t client_only : 1 ;
    bool_t no_vsync : 1 ;
    bool_t force_vsync : 1 ;
    bool_t fragment : 1 ;
    bool_t appl_msg : 1 ;
    bool_t live : 1 ;
    event_extend_t extend ;
} ;

event_t event_create(event_type_t type) {
    event_t ev = sys_alloc(sizeof(*ev)) ;
    ev->type = type ;
    ev->extend = NULL ;
    ev->appl_msg = FALSE ;
    ev->peer = 0 ;
    ev->no_total = 0 ;
    ev->server_only = 0 ;
    ev->client_only = 0 ;
    ev->no_vsync = 0 ;
    ev->force_vsync = 0 ;
    ev->fragment = 0 ;
    ev->appl_msg = 0 ;
    ev->live = TRUE ;
    return ev ;
}

static void extend_free(event_extend_t ex) {
    event_extend_t next ;
    if (!ex) {
	return ;
    } 
    next = ex->next ;
    switch (ex->field) {
    case FIELD_ADDRESS:
	break ;
    case FIELD_APPL_CASTS:
	array_free(ex->u.appl_casts) ;
	break ;
    case FIELD_APPL_SENDS:
	array_free(ex->u.appl_sends) ;
	break ;
    case FIELD_CONTACT:
	break ;
    case FIELD_DEBUG_NAME:
	break ;
    case FIELD_EXCHANGE_GOSSIP:
	break ;
    case FIELD_FAILURES:
	array_free(ex->u.failures) ;
	break ;
    case FIELD_HEAL_GOSSIP:
	array_free(ex->u.heal_gossip->view) ;
	proto_id_free(ex->u.heal_gossip->proto) ;
	record_free(ex->u.heal_gossip) ;
	break ;
    case FIELD_MERGERS:
	view_state_free(ex->u.mergers) ;
	break ;
    case FIELD_NUM_CASTS:
	array_free(ex->u.num_casts) ;
	break ;
    case FIELD_PRESENCE:
	array_free(ex->u.presence) ;
	break ;
    case FIELD_PROTO:
	proto_id_free(ex->u.proto) ;
	break ;
    case FIELD_STABILITY:
	array_free(ex->u.stability) ;
	break ;
    case FIELD_SUSPECTS:
	array_free(ex->u.suspects) ;
	break ;
    case FIELD_SUSPECT_REASON:
	/*BUG*/
	break ;
    case FIELD_SWITCH_GOSSIP:
	break ;
    case FIELD_TIME:
	break ;
    case FIELD_VIEW_STATE:
	view_state_free(ex->u.view_state) ;
	break ;
    OTHERWISE_ABORT() ;
    }
    record_free(ex) ;
    extend_free(next) ;
}

static event_extend_t extend_copy(
	event_extend_t ex0,
	nmembers_t nmembers
) {
    event_extend_t ex1 ;
    if (!ex0) {
	return NULL ;
    } 
    ex1 = record_create(event_extend_t, ex1) ;
    *ex1 = *ex0 ;
    ex1->next = extend_copy(ex0->next, nmembers) ;
    switch (ex0->field) {
    case FIELD_ADDRESS:
	break ;
    case FIELD_APPL_CASTS:
	ex1->u.appl_casts = seqno_array_copy(ex0->u.appl_casts, nmembers) ;
	break ;
    case FIELD_APPL_SENDS:
	ex1->u.appl_sends = seqno_array_copy(ex0->u.appl_sends, nmembers) ;
	break ;
    case FIELD_CONTACT:
	ex1->u.contact = view_contact_copy(ex0->u.contact) ;
	break ;
    case FIELD_DEBUG_NAME:
	sys_abort() ;
	break ;
    case FIELD_EXCHANGE_GOSSIP:
	sys_abort() ;
	break ;
    case FIELD_FAILURES:
	ex1->u.failures = bool_array_copy(ex0->u.failures, nmembers) ;
	break ;
    case FIELD_HEAL_GOSSIP:
	sys_abort() ;
	/*ex1->u.heal_gossip = event_heal_gossip_copy(ex0->u.heal_gossip); */
	break ;
    case FIELD_MERGERS:
	ex1->u.mergers = view_state_copy(ex0->u.mergers) ;
	break ;
    case FIELD_NUM_CASTS:
	ex1->u.num_casts = seqno_array_copy(ex0->u.num_casts, nmembers) ;
	break ;
    case FIELD_PRESENCE:
	ex1->u.presence = bool_array_copy(ex0->u.presence, nmembers) ;
	break ;
    case FIELD_PROTO:
	ex1->u.proto = proto_id_copy(ex0->u.proto) ;
	break ;
    case FIELD_STABILITY:
	ex1->u.stability = seqno_array_copy(ex0->u.stability, nmembers) ;
	break ;
    case FIELD_SUSPECTS:
	ex1->u.suspects = bool_array_copy(ex0->u.suspects, nmembers) ;
	break ;
    case FIELD_SUSPECT_REASON:
	ex1->u.suspect_reason = string_copy(ex0->u.suspect_reason) ;
	break ;
    case FIELD_SWITCH_GOSSIP:
	sys_abort() ;
	break ;
    case FIELD_TIME:
	break ;
    case FIELD_VIEW_STATE:
	ex1->u.view_state = view_state_copy(ex0->u.view_state) ;
	break ;
    OTHERWISE_ABORT() ;
    }
    return ex1 ;
}
    
void event_free(event_t ev) {
    assert(ev->live) ;
    extend_free(ev->extend) ;
    record_free(ev) ;
}

event_t event_copy(event_t ev0, nmembers_t nmembers) {
    event_t ev1 = event_create(0) ;
    assert(ev0->live) ;
    *ev1 = *ev0 ;
    ev1->extend = extend_copy(ev0->extend, nmembers) ;
    return ev1 ;
}

event_heal_gossip_t
event_heal_gossip_create(
        view_contact_t contact,
	proto_id_t proto,
	endpt_id_array_t view,
	nmembers_t nmembers
) {
    struct event_heal_gossip_t *g =
	record_create(struct event_heal_gossip_t *, g) ;
    assert_array_length(view, nmembers) ;
    g->contact = contact ;
    g->proto = proto ;
    g->view = view ;
    g->nmembers = nmembers ;
    return g ;
}

#if 0
void
event_heal_gossip_free(
        event_heal_gossip_t g
) {
}
#endif

static event_extend_t
get_extend(event_t ev, event_field_t field) {
    event_extend_t ex ;
    for (ex=ev->extend;ex;ex=ex->next) {
	if (ex->field == field) {
	    break ;
	}
    }
    return ex ;
}

static event_extend_t
get_extend_fail(event_t ev, event_field_t field) {
    event_extend_t ex ;
    ex = get_extend(ev, field) ;
    assert(ex) ;
    return ex ;
}

static event_extend_t
set_extend(event_t ev, event_field_t field) {
    event_extend_t ex ;
    ex = sys_alloc(sizeof(*ex)) ;
    ex->next = ev->extend ;
    ex->field = field ;
    ev->extend = ex ;
    return ex ;
}

#define ACCESS_STRUCT(_name, _type) \
_type \
event_ ## _name(event_t ev) { \
    assert(ev->live) ; \
    return ev->_name ; \
} \
\
event_t \
event_set_ ## _name(event_t ev, _type val) { \
    assert(ev) ; \
    assert(ev->live) ; \
    ev->_name = val ; \
    return ev ; \
}

#define ACCESS_EXTEND(_lower_name, _upper_name, _type) \
_type \
event_ ## _lower_name(event_t ev) { \
    assert(ev) ; \
    assert(ev->live) ; \
    return get_extend_fail(ev, FIELD_ ## _upper_name)->u._lower_name ; \
} \
\
event_t \
event_set_ ## _lower_name(event_t ev, _type val) { \
    assert(ev) ; \
    assert(ev->live) ; \
    set_extend(ev, FIELD_ ## _upper_name)->u._lower_name = val ; \
    return ev ; \
}

#define ACCESS_EXTEND_NO_FAIL(_lower_name, _upper_name, _type) \
_type \
event_ ## _lower_name(event_t ev) { \
    event_extend_t ex ; \
    assert(ev) ; \
    assert(ev->live) ; \
    ex = get_extend(ev, FIELD_ ## _upper_name) ; \
    if (!ex) return NULL ; \
    return ex->u._lower_name ; \
} \
\
event_t \
event_set_ ## _lower_name(event_t ev, _type val) { \
    assert(ev) ; \
    assert(ev->live) ; \
    set_extend(ev, FIELD_ ## _upper_name)->u._lower_name = val ; \
    return ev ; \
}

ACCESS_STRUCT(appl_msg, bool_t)
ACCESS_EXTEND(address, ADDRESS, addr_id_t)
ACCESS_EXTEND(appl_casts, APPL_CASTS, const_seqno_array_t)
ACCESS_EXTEND(appl_sends, APPL_SENDS, const_seqno_array_t)
ACCESS_STRUCT(client_only, bool_t)
ACCESS_EXTEND(contact, CONTACT, view_contact_t)
ACCESS_EXTEND_NO_FAIL(exchange_gossip, EXCHANGE_GOSSIP, event_exchange_gossip_t)
ACCESS_EXTEND(failures, FAILURES, const_bool_array_t)
ACCESS_STRUCT(fragment, bool_t)
ACCESS_EXTEND_NO_FAIL(heal_gossip, HEAL_GOSSIP, event_heal_gossip_t)
ACCESS_EXTEND(mergers, MERGERS, view_state_t)
ACCESS_STRUCT(no_total, bool_t)
ACCESS_STRUCT(no_vsync, bool_t)
ACCESS_EXTEND(num_casts, NUM_CASTS, const_seqno_array_t)
ACCESS_STRUCT(force_vsync, bool_t)
ACCESS_STRUCT(peer, rank_t)
ACCESS_EXTEND(debug_name, DEBUG_NAME, string_t)
ACCESS_EXTEND(presence, PRESENCE, const_bool_array_t)
ACCESS_EXTEND(proto, PROTO, proto_id_t)
ACCESS_STRUCT(server_only, bool_t)
ACCESS_EXTEND(stability, STABILITY, const_seqno_array_t)
ACCESS_EXTEND(suspect_reason, SUSPECT_REASON, string_t)
ACCESS_EXTEND(suspects, SUSPECTS, const_bool_array_t)
ACCESS_EXTEND_NO_FAIL(switch_gossip, SWITCH_GOSSIP, event_switch_gossip_t)
ACCESS_EXTEND(time, TIME, etime_t)
ACCESS_STRUCT(type, event_type_t)
ACCESS_EXTEND(view_state, VIEW_STATE, view_state_t)

bool_t
event_has_contact(event_t ev) {
    event_extend_t ex ;
    for (ex=ev->extend;ex;ex=ex->next) {
	if (ex->field == FIELD_CONTACT) {
	    break ;
	}
    }
    return ex != NULL  ;
}

#ifndef MINIMIZE_CODE
bool_t event_type_has_message(event_type_t type) {
    switch(type) {
    case EVENT_CAST:
    case EVENT_SEND:
    case EVENT_SUBCAST:
    case EVENT_CAST_UNREL:
    case EVENT_SEND_UNREL:
    case EVENT_MERGE_REQUEST:
    case EVENT_MERGE_GRANTED:
    case EVENT_ORPHAN:
	return TRUE ;
	break ;

    case EVENT_ACCOUNT:
    case EVENT_ASYNC:
    case EVENT_BLOCK:
    case EVENT_BLOCK_OK:
    case EVENT_DUMP:
    case EVENT_ELECT:
    case EVENT_EXIT:
    case EVENT_FAIL:
    case EVENT_GOSSIP_EXT:
    case EVENT_INIT:
    case EVENT_LEAVE:
    case EVENT_LOST_MESSAGE:
    case EVENT_MERGE_DENIED:
    case EVENT_MERGE_FAILED:
    case EVENT_MIGRATE:
    case EVENT_PRESENT:
    case EVENT_PROMPT:
    case EVENT_PROTO:
    case EVENT_REKEY:
    case EVENT_REKEYPRCL:
    case EVENT_STABLE:
    case EVENT_STABLE_REQ:
    case EVENT_SUSPECT:
    case EVENT_SYSTEM_ERROR:
    case EVENT_TIMER:
    case EVENT_VIEW:
    case EVENT_XFER_DONE:
    case EVENT_SYNC_INFO:
	return FALSE ;
	break ;
    OTHERWISE_ABORT() ;
    }
}

string_t event_type_to_string(event_type_t type) {
    switch(type) {
    case EVENT_CAST: return "cast" ; 
    case EVENT_SEND: return "send" ; 
    case EVENT_SUBCAST: return "subcast" ; 
    case EVENT_CAST_UNREL: return "cast_unrel" ; 	
    case EVENT_SEND_UNREL: return "send_unrel" ; 	
    case EVENT_MERGE_REQUEST: return "merge_request" ; 
    case EVENT_MERGE_GRANTED: return "merge_granted" ; 
    case EVENT_ORPHAN: return "orphan" ; 	
    case EVENT_ACCOUNT: return "account" ; 	
    case EVENT_ASYNC: return "async" ; 	
    case EVENT_BLOCK: return "block" ; 	
    case EVENT_BLOCK_OK: return "block_ok" ; 	
    case EVENT_DUMP: return "dump" ; 		
    case EVENT_ELECT: return "elect" ; 	
    case EVENT_EXIT: return "exit" ; 		
    case EVENT_FAIL: return "fail" ; 		
    case EVENT_GOSSIP_EXT: return "gossip_ext" ; 	
    case EVENT_INIT:return "init" ; 		
    case EVENT_LEAVE: return "leave" ; 	
    case EVENT_LOST_MESSAGE: return "lost_message" ; 	
    case EVENT_MERGE_DENIED: return "merge_denied" ; 	
    case EVENT_MERGE_FAILED: return "merge_failed" ; 	
    case EVENT_MIGRATE:	return "migrate" ; 	
    case EVENT_PRESENT: return "present" ; 	
    case EVENT_PROMPT: return "prompt" ; 	
    case EVENT_PROTO: return "proto" ; 	
    case EVENT_REKEY: return "rekey" ; 	
    case EVENT_REKEYPRCL: return "rekeyprcl" ; 	
    case EVENT_STABLE: return "stable" ; 	
    case EVENT_STABLE_REQ: return "stable_req" ;
    case EVENT_SUSPECT: return "suspect" ;
    case EVENT_SYSTEM_ERROR: return "system_error" ;
    case EVENT_TIMER: return "timer" ;
    case EVENT_VIEW: return "view" ;
    case EVENT_XFER_DONE: return "xfer_done" ;
    case EVENT_SYNC_INFO: return "sync_info" ;
    }
    sys_abort() ;
}

string_t event_to_string(event_t e) {
    return sys_sprintf("Event{%s;peer=%d}",
		       event_type_to_string(e->type), e->peer) ;
}
#endif

/* Special constructors.
 */

event_t event_cast(void) {
    return event_create(EVENT_CAST) ;
}

event_t event_cast_unrel(void) {
    return event_create(EVENT_CAST_UNREL) ;
}

event_t event_cast_unrel_peer(rank_t peer) {
    event_t e = event_create(EVENT_CAST_UNREL) ;
    e->peer = peer ;
    return e ;
}

event_t event_cast_appl(void) {
    event_t e = event_create(EVENT_CAST) ;
    e->appl_msg = TRUE ;
    return e ;
}

event_t event_cast_peer(rank_t peer) {
    event_t e = event_create(EVENT_CAST) ;
    e->peer = peer ;
    return e ;
}

event_t event_send_peer(rank_t peer) {
    event_t e = event_create(EVENT_SEND) ;
    e->peer = peer ;
    return e ;
}

event_t event_send_peer_appl(rank_t peer) {
    event_t e = event_create(EVENT_SEND) ;
    e->appl_msg = TRUE ;
    e->peer = peer ;
    return e ;
}

event_t event_send_unrel_peer(rank_t peer) {
    event_t e = event_create(EVENT_SEND_UNREL) ;
    e->peer = peer ;
    return e ;
}

event_t event_suspect_reason_create(const_bool_array_t suspects, debug_t suspect_reason) {
    event_t e = event_create(EVENT_SUSPECT) ;
    e = event_set_suspects(e, suspects) ;
    e = event_set_suspect_reason(e, suspect_reason) ;
    return e ;
}

event_t event_timer_time(etime_t time) {
    event_t e = event_create(EVENT_TIMER) ;
    e = event_set_time(e, time) ;
    return e ;
}
