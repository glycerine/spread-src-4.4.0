/**************************************************************/
/* TRANSPORT.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "infr/trans.h"
#include "infr/domain.h"
#include "infr/conn.h"
#include "infr/endpt.h"
#include "infr/stack.h"
#include "infr/sched.h"
#include "infr/iovec.h"

typedef struct transport_root_t *transport_root_t ;

typedef struct transport_t *transport_t ;

typedef struct transport_xmit_t *transport_xmit_t ;

typedef array_def(transport_xmit) transport_xmit_array_t ;

typedef void (*transport_deliver_t)(
        env_t,				 
        conn_kind_t,
	rank_t,
	/*bool_t,*/
	iovec_t
) ;

transport_root_t transport_root(sched_t sched) ;

/* Construct and enable a new transport instance.
 * None of the arguments are consumed by the callee.
 */
transport_t transport_create(
	transport_root_t,
	domain_t,
        view_local_t,
        view_state_t,
	stack_id_t,
	transport_deliver_t,
	env_t
) ;
	
/* Deliver an ASYNC upcall.
 */
void transport_async(
        transport_root_t, 
	view_state_t,
	const endpt_id_t *
) ;

/* Disable the transport. 
 */
void transport_disable(transport_t) ;

/* Prepare to cast on the transport.
 */
transport_xmit_t transport_cast(transport_t) ;

/* Prepare to send on the transport.
 */
transport_xmit_t transport_send(transport_t, rank_t) ;

/* Prepare to gossip on the transport.
 */
transport_xmit_t transport_gossip(transport_t) ;

/* Prepare to merge on the transport.
 */
transport_xmit_t transport_merge(transport_t, view_contact_t) ;

void transport_xmit(transport_xmit_t, marsh_t) ;

void transport_xmit_free(transport_xmit_t) ;

void transport_deliver(transport_root_t, iovec_t) ;

void transport_usage(transport_root_t) ;

void transport_dump(transport_root_t) ;

transport_xmit_array_t transport_xmit_array_create(len_t) ;

#endif /* TRANSPORT_H */
