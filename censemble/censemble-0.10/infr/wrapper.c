/**************************************************************/
/* WRAPPER.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"
#include "infr/alarm.h"
#include "infr/marsh.h"
#include "infr/transport.h"

static string_t name = "WRAPPER" ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;

    layer_state_t state ;

    transport_t primary_trans ;
    transport_xmit_t cast ;
    transport_xmit_array_t send ;

    transport_t gossip_trans ;
    transport_xmit_t gossip ;

    alarm_t alarm ;
    alarm_handle_t alarm_handle ;

    struct {
	void (*fun)(void*) ;
	env_t env ;
    } on_exit ;
} *state_t ;

#include "infr/layer_supp.h"

static
void alarm_handler(env_t env, etime_t time) {
    state_t s = env ;
    event_t e ;
    e = event_create(EVENT_TIMER) ;
    e = event_set_time(e, time) ;
    upnm(s, e) ;
}

static
void gossip_handler(
        env_t env,
	conn_kind_t kind,
	rank_t rank,
	/*bool_t secure,*/
	iovec_t iov
) {
    state_t s = env ;
    event_t e ;
    view_contact_t contact ;
    proto_id_t proto ;
    endpt_id_array_t view ;
    nmembers_t nmembers ;
    unmarsh_t m ;

    m = unmarsh_of_iovec(iov) ;
    unmarsh_view_contact(m, &contact) ;
    unmarsh_proto_id(m, &proto) ;
    unmarsh_rank(m, &nmembers) ;
    unmarsh_endpt_id_array(m, &view, nmembers) ;
    unmarsh_free(m) ;
    
    /*assert(secure) ;*/
    assert(kind == CONN_OTHER) ;
    e = event_create(EVENT_GOSSIP_EXT) ;
    e = event_set_heal_gossip(e, event_heal_gossip_create(contact, proto, view, nmembers)) ;
    upnm(s, e) ;
}

static
void transport_handler(
        env_t env,
	conn_kind_t kind,
	rank_t rank,
	/*bool_t secure,*/
	iovec_t iov
) {
    state_t s = env ;
    unmarsh_t m ;
    event_t e ;
    /*assert(secure) ;*/
    switch(kind) {
    case CONN_CAST:
	assert(rank >= 0 && rank < s->vs->nmembers) ;
	e = event_cast_peer(rank) ;
	break ;
    case CONN_SEND:
	assert(rank >= 0 && rank < s->vs->nmembers) ;
	e = event_send_peer(rank) ;
	break ;
    case CONN_ASYNC:
	e = event_create(EVENT_ASYNC) ;
	assert(!iov) ;
	upnm(s, e) ;
	return ;		/* HACK */
	break ;
    case CONN_OTHER:
	e = event_create(EVENT_MERGE_REQUEST) ;
	break ;
    OTHERWISE_ABORT() ;
    }

    m = unmarsh_of_iovec(iov) ;
    up(s, e, m) ;
}

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
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

    assert(state->alarm) ;
    s->alarm = state->alarm ;
    s->state = state ;

    s->on_exit.fun = NULL ;
    s->on_exit.env = NULL ;

    s->alarm_handle = alarm_alarm(alarm_handler, s) ;
    s->primary_trans = alarm_transport(state->alarm,
				       state->domain,
				       ls, vs,
				       STACK_PRIMARY,
				       transport_handler, s) ;
    s->cast = transport_cast(s->primary_trans) ;
    s->send = transport_xmit_array_create(vs->nmembers) ;
    for (i=0;i<vs->nmembers;i++) {
	if (i == ls->rank) {
	    array_set(s->send, i, NULL) ;
	} else {
	    array_set(s->send, i, transport_send(s->primary_trans, i)) ;
	}
    }

    s->gossip_trans = alarm_transport(state->alarm,
				      state->domain,
				      ls, vs,
				      STACK_GOSSIP,
				      gossip_handler, s) ;
    s->gossip = transport_gossip(s->gossip_trans) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    sys_abort() ;
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_VIEW: {
	view_state_t vs ;
	vs = view_state_copy(event_view_state(e)) ;
	/*eprintf("TOP:view:ltime=%d nmembers=%d rank=%d\n",
	  vs->ltime, vs->nmembers, ls->rank) ;*/
	layer_compose(s->state, &s->ls->endpt, vs) ;
	event_free(e) ;
    } break ;

    case EVENT_EXIT:
	event_free(e) ;
	break ;

    OTHERWISE_ABORT() ;
    }
}

static void do_xmit(transport_xmit_t xmit, marsh_t m) {
    transport_xmit(xmit, m) ;
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    case EVENT_CAST:
    case EVENT_CAST_UNREL: {
	if (s->vs->nmembers > 1) {
	    do_xmit(s->cast, abv) ;
	} else {
	    marsh_free(abv) ;
	}
    } break ; 
	
    case EVENT_SEND:
    case EVENT_SEND_UNREL:
	do_xmit(array_get(s->send, event_peer(e)), abv) ;
	break; 

    case EVENT_MERGE_REQUEST:
    case EVENT_MERGE_GRANTED:
    case EVENT_MERGE_DENIED: {
	transport_xmit_t xmit ;
	xmit = transport_merge(s->primary_trans, event_contact(e)) ;
	do_xmit(xmit, abv) ;
	transport_xmit_free(xmit) ;
    } break ;
	
    OTHERWISE_ABORT() ;
    }
    event_free(e) ;	
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_GOSSIP_EXT: {
	event_heal_gossip_t gossip = event_heal_gossip(e) ;
	if (gossip) {
	    marsh_t m = marsh_create(NULL) ;
	    assert_array_length(gossip->view, gossip->nmembers) ;
	    marsh_endpt_id_array(m, gossip->view, gossip->nmembers) ;
	    marsh_rank(m, gossip->nmembers) ;
	    marsh_proto_id(m, gossip->proto) ;
	    marsh_view_contact(m, &gossip->contact) ;
	    do_xmit(s->gossip, m) ;
        }
    } break ; 

    case EVENT_TIMER: {
	/* ETimer: request a timeout callback from the transport.
	 */
	alarm_schedule(s->alarm, s->alarm_handle, event_time(e)) ;
    } break ;

    case EVENT_EXIT: {
	ofs_t i ;
	transport_xmit_free(s->cast) ; s->cast = NULL ;
	transport_xmit_free(s->gossip) ; s->gossip = NULL ;
	for (i=0;i<s->vs->nmembers;i++) {
	    if (i == s->ls->rank) {
		continue ;
	    }
	    transport_xmit_free(array_get(s->send, i)) ;
	    array_set(s->send, i, NULL) ;
	}
	array_free(s->send) ; s->send = NULL ;
	transport_disable(s->primary_trans) ;
	s->primary_trans = NULL ;
	transport_disable(s->gossip_trans) ;
	s->gossip_trans = NULL ;
	assert(s->on_exit.fun) ;
	sched_quiesce(alarm_sched(s->state->alarm),
		      s->on_exit.fun,
		      s->on_exit.env) ;
    } break ;

    OTHERWISE_ABORT() ;
    }

    event_free(e) ;
}

static void free_handler(state_t s) {
    alarm_alarm_free(s->alarm_handle) ;
    s->alarm_handle = NULL ;
}

void wrapper_quiesce(env_t env, void (*fun)(void *), void *arg) {
    state_t s = env ;
    s->on_exit.fun = fun ;
    s->on_exit.env = arg ;
}

LAYER_REGISTER(wrapper)
