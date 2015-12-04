/**************************************************************/
/* ALARM.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef ALARM_H
#define ALARM_H

#include "infr/trans.h"
#include "infr/unique.h"
#include "infr/sched.h"
#include "infr/util.h"
#include "infr/transport.h"
#include "infr/etime.h"
#include <poll.h>

/* ALARM: a record for requesting timeouts on a particular
 * callback.  
 */
typedef struct alarm_handle_t *alarm_handle_t ;

/* ALARM.T: The type of alarms.
 */
typedef struct alarm_t *alarm_t ;

typedef void (*alarm_handler_t)(env_t, etime_t) ;
typedef bool_t (*alarm_poll_handler_t)(env_t) ;

typedef enum {
    ALARM_SOCKS_POLLS,
    ALARM_POLLS
} alarm_poll_type_t ;

name_t alarm_name(alarm_t) ;		/* name of alarm */
etime_t alarm_gettime(alarm_t) ;		/* get current time */
alarm_handle_t alarm_alarm(alarm_handler_t, env_t env) ; /* create alarm object */
void alarm_alarm_free(alarm_handle_t) ;

void alarm_disable(alarm_handle_t) ;
void alarm_schedule(alarm_t, alarm_handle_t, etime_t) ;
bool_t alarm_check(alarm_t) ;	/* check if alarms have timed out */
etime_t alarm_min(alarm_t) ;		/* get next alarm to go off */
void alarm_add_sock_recv(alarm_t, debug_t, sock_t, sock_handler_t, env_t) ; /* add a socket */
void alarm_rmv_sock_recv(alarm_t, sock_t) ; /* remove socket */
void alarm_add_sock_xmit(alarm_t, debug_t, sock_t, sock_handler_t, env_t) ; /* add a socket */
void alarm_rmv_sock_xmit(alarm_t, sock_t) ; /* remove socket */
void alarm_add_poll(alarm_t, name_t, alarm_poll_handler_t, env_t)  ;/* add polling function */
void alarm_rmv_poll(alarm_t, name_t) ; /* remove polling function */
void alarm_block(alarm_t) ;		/* block until next timer/socket input */
void alarm_block_extern(alarm_t, struct pollfd *socks, unsigned *nsocks, int *timeout) ;
bool_t alarm_poll(alarm_t, alarm_poll_type_t) ; /* poll for socket data */
void alarm_deliver(alarm_t, iovec_t) ;
void alarm_async(alarm_t, view_state_t, const endpt_id_t *) ;

transport_t alarm_transport(
	alarm_t,
        domain_t,
        view_local_t,
        view_state_t,
	stack_id_t,
	transport_deliver_t,
	env_t
) ;

/*alarm_handlers    : t -> Route.handlers*/
/*alarm_mbuf        : t -> Mbuf.t*/
sched_t alarm_sched(alarm_t) ; 
/*alarm_async       : t -> Async.t*/
unique_t alarm_unique(alarm_t) ;
/*alarm_local_xmits : t -> Route.xmits*/

/**************************************************************/
/* For use only by alarms.
 */

typedef etime_t (*alarm_gettime_t)(env_t) ;
typedef void (*alarm_schedule_t)(env_t, alarm_handle_t, etime_t) ;
typedef bool_t (*alarm_check_t)(env_t) ;
typedef etime_t (*alarm_min_t)(env_t) ;
typedef void (*alarm_add_sock_recv_t)(env_t, debug_t, sock_t, sock_handler_t, env_t) ;
typedef void (*alarm_rmv_sock_recv_t)(env_t, sock_t) ;
typedef void (*alarm_add_sock_xmit_t)(env_t, debug_t, sock_t, sock_handler_t, env_t) ;
typedef void (*alarm_rmv_sock_xmit_t)(env_t, sock_t) ;
typedef void (*alarm_add_poll_t)(env_t, name_t, alarm_poll_handler_t, env_t) ;
typedef void (*alarm_rmv_poll_t)(env_t, name_t) ;
typedef void (*alarm_block_t)(env_t) ;
typedef void (*alarm_block_extern_t)(env_t, struct pollfd *socks, unsigned *nsocks, int *next_alarm) ;
typedef bool_t (*alarm_poll_t)(env_t, alarm_poll_type_t) ;
/*typedef (*alarm_handlers_t)(env_t, route_handlers_t) ;*/
/*typedef (*alarm_mbuf_t)(env_t, mbuf_t) ;*/
/*typedef (*alarm_local_xmits_t)(env_t, route_xmits_t) ;*/

/*type gorp = Unique.t * Sched.t * Async.t * Route.handlers * Mbuf.t*/

alarm_t alarm_create(
	name_t,
	alarm_gettime_t,
	alarm_schedule_t,
	alarm_check_t,
	alarm_min_t,
	alarm_add_sock_recv_t,
	alarm_rmv_sock_recv_t,
	alarm_add_sock_xmit_t,
	alarm_rmv_sock_xmit_t,
	alarm_add_poll_t,
	alarm_rmv_poll_t,
	alarm_block_t,
	alarm_block_extern_t,
	alarm_poll_t,
	env_t env
) ;

void alarm_init_internal(
	alarm_t,
	/*transport_root_t,*/
	/*alarm_mbuf_t,*/
	sched_t,
	/*alarm_async_t,*/
	unique_t
	/*alarm_local_xmits_t*/
) ;


/*alarm_alm_add_poll	: string -> (bool -> bool) -> ((bool -> bool) option)*/
/*alarm_alm_rmv_poll	: string -> ((bool -> bool) option)*/

/*alarm_wrap : ((Time.t -> unit) -> Time.t -> unit) -> t -> t*/
/*alarm_c_alarm : (unit -> unit) -> (Time.t -> unit) -> alarm*/

void alarm_handle_deliver(alarm_handle_t, etime_t) ;

#endif /* ALARM_H */
