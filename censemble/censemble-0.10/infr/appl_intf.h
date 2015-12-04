/**************************************************************/
/* APPL_INTF.H: application interface */
/* Author: Mark Hayden, 11/99 */
/* See documentation for a description of this interface */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#ifndef APPL_INTF_H
#define APPL_INTF_H

#include "infr/trans.h"
#include "infr/util.h"
#include "infr/proto.h"
#include "infr/addr.h"
#include "infr/etime.h"
#include "infr/view.h"
#include "infr/equeue.h"

/* ACTION: The type of actions an application can take.
 * These are typically returned from callbacks as lists of
 * actions to be taken.

 * [NOTE: XferDone, Protocol, Rekey, and Migrate are not
 * implemented by all protocol stacks.  Dump and Block are
 * undocumented actions used for internal Ensemble
 * services.]

 * Cast(msg): Broadcast a message to entire group

 * Send(dests,msg): Send a message to subset of group.

 * Leave: Leave the group.

 * Prompt: Prompt for a view change.

 * Suspect: Tell the protocol stack about suspected failures
 * in the group.

 * XferDone: Mark end of a state transfer.

 * Proto(protocol): Request a protocol switch.  Will
 * cause a view change to new protocol stack.

 * Migrate(addresses): Change address list for this member.
 * Will cause a view change.

 * Rekey: Request that the group be rekeyed.

 * Timeout(time): Request a timeout at a particular time.
 * The timeout takes the form of a heartbeat callback.
 * Currently not supported.

 * Dump: Undefined (for debugging purposes)

 * Block: Controls synchronization of view changes.  Not
 * for casual use.
 */
typedef struct appl_action_t {
    enum {
	APPL_CAST,
	APPL_SEND,
	APPL_SEND1,
	APPL_LEAVE,
	APPL_PROMPT,
	APPL_SUSPECT,
	APPL_XFER_DONE,
	APPL_REKEY,
	APPL_PROTO,
	APPL_MIGRATE,
	APPL_TIMEOUT,
	APPL_DUMP,
	APPL_BLOCK,
	APPL_NO_OP
    } type ;
    union {
	iovec_t cast ;
	struct {
	    rank_array_t dests ;
	    len_t ndests ;
	    iovec_t msg ;
	} send ;
	struct {
	    rank_t dest ;
	    iovec_t msg ;
	} send1 ;
	bool_array_t suspect ;
	proto_id_t proto ;
	addr_id_t migrate ;
	etime_t timeout ;
	bool_t block ;
    } u ;
} *appl_action_t ;

appl_action_t appl_cast(void *msg) ;
appl_action_t appl_send(rank_array_t dest, len_t ndests, void *msg) ;
appl_action_t appl_send1(rank_t dest, void *msg) ;
appl_action_t appl_leave(void) ;
appl_action_t appl_xfer_done(void) ;
appl_action_t appl_suspect(bool_array_t suspects) ;
	    
void appl_action_free(appl_action_t) ;

string_t string_of_action(appl_action_t) ;

typedef bool_t is_send_t ;
typedef bool_t blocked_t ;

typedef void (*appl_receive_t)(env_t env, rank_t, is_send_t, blocked_t, iovec_t) ;
typedef void (*appl_block_t)(env_t env) ;
typedef void (*appl_heartbeat_t)(env_t env, etime_t) ;
typedef void (*appl_disable_t)(env_t env) ;

typedef env_t (*appl_install_t)(env_t env, view_local_t ls, view_state_t vs, equeue_t xmit) ;
typedef void (*appl_exit_t)(env_t) ;

typedef struct appl_intf_t {
    etime_t heartbeat_rate ;
    appl_exit_t exit ;

    /* Note that the view state and local state records passed up are
     * "owned" by the application.
     */
    appl_install_t install ;

    /* These use the state returned by install.
     */
    appl_block_t block ;

    /* The iovec's passed up with the receive call are owned
     * by the application.
     */
    appl_receive_t receive ;

    appl_heartbeat_t heartbeat ;
    appl_disable_t disable ;
} *appl_intf_t ;

#endif /* APPL_INTF */


