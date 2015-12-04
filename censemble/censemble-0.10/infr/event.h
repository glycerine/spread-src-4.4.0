/**************************************************************/
/* EVENT.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef EVENT_H
#define EVENT_H

#include "infr/addr.h"
#include "infr/etime.h"
#include "infr/view.h"

typedef struct event_t *event_t ;

typedef enum {
    /* These events have messages associated with them. */
    EVENT_CAST,			/* Multicast message */
    EVENT_SEND,			/* Pt2pt message */
    EVENT_SUBCAST,		/* Multi-destination message */
    EVENT_CAST_UNREL,		/* Unreliable multicast message */
    EVENT_SEND_UNREL,		/* Unreliable pt2pt message */
    EVENT_MERGE_REQUEST,	/* Request a merge */
    EVENT_MERGE_GRANTED,	/* Grant a merge request */
    EVENT_ORPHAN,		/* Message was orphaned */
    
    /* These do not have messages. */
    EVENT_ACCOUNT,		/* Output accounting information */
    EVENT_ASYNC,		/* Asynchronous application event */
    EVENT_BLOCK,		/* Block the group */
    EVENT_BLOCK_OK,		/* Acknowledge blocking of group */
    EVENT_DUMP,			/* Dump your state (debugging) */
    EVENT_ELECT,		/* I am now the coordinator */
    EVENT_EXIT,			/* Disable this stack */
    EVENT_FAIL,			/* Fail some members */
    EVENT_GOSSIP_EXT,		/* Gossip message */
    EVENT_INIT,			/* First event delivered */
    EVENT_LEAVE,		/* A member wants to leave */
    EVENT_LOST_MESSAGE,		/* Member doesn't have a message */
    EVENT_MERGE_DENIED,		/* Deny a merge request */
    EVENT_MERGE_FAILED,		/* Merge request failed */
    EVENT_MIGRATE,		/* Change my location */
    EVENT_PRESENT,		/* Members present in this view */
    EVENT_PROMPT,		/* Prompt a new view */
    EVENT_PROTO,		/* Request a protocol switch */
    EVENT_REKEY,		/* Request a rekeying of the group */
    EVENT_REKEYPRCL,		/* The rekey protocol events */
    EVENT_STABLE,		/* Deliver stability down */
    EVENT_STABLE_REQ,		/* Request for stability information */
    EVENT_SUSPECT,		/* Member is suspected to be faulty */
    EVENT_SYSTEM_ERROR,		/* Something serious has happened */
    EVENT_TIMER,		/* Request a timer */
    EVENT_VIEW,			/* Notify that a new view is ready */
    EVENT_XFER_DONE,		/* Notify that a state transfer is complete */
    EVENT_SYNC_INFO
} event_type_t ;

typedef struct event_heal_gossip_t {
    view_contact_t contact ;
    proto_id_t proto ;
    endpt_id_array_t view ;
    nmembers_t nmembers ;
} const *event_heal_gossip_t ;

event_heal_gossip_t
event_heal_gossip_create(
        view_contact_t contact,
	proto_id_t proto,
	endpt_id_array_t view,
	nmembers_t nmembers
) ;

typedef struct {
    view_id_t view_id ;
    proto_id_t proto ;
    etime_t time ;
} const *event_switch_gossip_t ;

typedef struct {
    string_t cipher_data ;
} const *event_exchange_gossip_t ;

/**************************************************************/

  /* Constructor */
event_t event_create(event_type_t) ;

/**************************************************************/

  /* Copier */ 
event_t event_copy(event_t, nmembers_t) ;

/**************************************************************/

  /* Destructor */
void event_free(event_t) ;

/**************************************************************/

  /* Sanity checkers */
void event_check(event_t) ;

/**************************************************************/

  /* Pretty printer */
string_t event_to_string(event_t) ;
string_t event_type_to_string(event_type_t) ;

/**************************************************************/

  /* Special constructors */
event_t event_cast(void) ;
event_t event_cast_unrel(void) ;
event_t event_cast_unrel_peer(rank_t) ;
event_t event_cast_appl(void) ;
event_t event_cast_peer(rank_t) ;
event_t event_send_peer(rank_t) ;
event_t event_send_peer_appl(rank_t) ;
event_t event_send_unrel_peer(rank_t) ;
event_t event_suspect_reason_create(const_bool_array_t, debug_t) ;
event_t event_timer_time(etime_t) ;

/**************************************************************/

bool_t                  event_has_contact(event_t) ;

/* Accessors.
 */

addr_id_t               event_address(event_t) ;
bool_t                  event_appl_msg(event_t) ;
const_seqno_array_t     event_appl_casts(event_t) ;
const_seqno_array_t     event_appl_sends(event_t) ;
bool_t                  event_client_only(event_t) ;
view_contact_t          event_contact(event_t) ;
event_exchange_gossip_t event_exchange_gossip(event_t) ;
const_bool_array_t      event_failures(event_t) ;
bool_t                  event_fragment(event_t) ;
event_heal_gossip_t     event_heal_gossip(event_t) ;
view_state_t            event_mergers(event_t) ;
bool_t                  event_no_total(event_t) ;
bool_t                  event_no_vsync(event_t) ;
bool_t                  event_force_vsync(event_t) ;
const_seqno_array_t     event_num_casts(event_t) ;
origin_t                event_peer(event_t) ;
string_t                event_debug_name(event_t) ;
const_bool_array_t      event_presence(event_t) ;
proto_id_t              event_proto(event_t) ;
bool_t                  event_server_only(event_t) ;
const_seqno_array_t     event_stability(event_t) ;
string_t                event_suspect_reason(event_t) ;
const_bool_array_t      event_suspects(event_t) ;
event_switch_gossip_t   event_switch_gossip(event_t) ;
etime_t                 event_time(event_t) ;
event_type_t            event_type(event_t) ;
view_state_t            event_view_state(event_t) ;

event_t event_set_address(event_t, addr_id_t) ;
event_t event_set_appl_msg(event_t, bool_t) ;
event_t event_set_appl_casts(event_t, const_seqno_array_t) ;
event_t event_set_appl_sends(event_t, const_seqno_array_t) ;
event_t event_set_client_only(event_t, bool_t) ;
event_t event_set_contact(event_t, view_contact_t) ;
event_t event_set_exchange_gossip(event_t, event_exchange_gossip_t) ;
event_t event_set_failures(event_t, const_bool_array_t) ;
event_t event_set_fragment(event_t, bool_t) ;
event_t event_set_heal_gossip(event_t, event_heal_gossip_t) ;
event_t event_set_mergers(event_t, view_state_t) ;
event_t event_set_no_total(event_t, bool_t) ;
event_t event_set_no_vsync(event_t, bool_t) ;
event_t event_set_force_vsync(event_t, bool_t) ;
event_t event_set_num_casts(event_t, const_seqno_array_t) ;
event_t event_set_peer(event_t, origin_t) ;
event_t event_set_debug_name(event_t, string_t) ;
event_t event_set_presence(event_t, const_bool_array_t) ;
event_t event_set_proto(event_t, proto_id_t) ;
event_t event_set_server_only(event_t, bool_t) ;
event_t event_set_stability(event_t, const_seqno_array_t) ;
event_t event_set_suspect_reason(event_t, string_t) ;
event_t event_set_suspects(event_t, const_bool_array_t) ;
event_t event_set_switch_gossip(event_t, event_switch_gossip_t) ;
event_t event_set_time(event_t, etime_t) ;
event_t event_set_type(event_t, event_type_t) ;
event_t event_set_view_state(event_t, view_state_t) ;

/**************************************************************/

/* Does this event type have a message associated with it?
 */
bool_t event_type_has_message(event_type_t type) ;

/**************************************************************/

#endif /* EVENT_H */
