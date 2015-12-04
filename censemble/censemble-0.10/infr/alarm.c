/**************************************************************/
/* ALARM.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/sched.h"
#include "infr/priq.h"
#include "infr/alarm.h"
#include "infr/transport.h"

/*static string_t name = "ALARM" ;*/

struct alarm_handle_t {
    alarm_handler_t upcall ;
    env_t env ;
    int count ;
    bool_t live ;
} ;

struct alarm_t {
    env_t env ;
    name_t name ;
    etime_t last_time ;
    alarm_gettime_t gettime ;
    alarm_schedule_t schedule ;
    alarm_check_t check ;
    alarm_min_t min ;
    alarm_add_sock_recv_t add_sock_recv ;
    alarm_rmv_sock_recv_t rmv_sock_recv ;
    alarm_add_sock_xmit_t add_sock_xmit ;
    alarm_rmv_sock_xmit_t rmv_sock_xmit ;
    alarm_add_poll_t add_poll ;
    alarm_rmv_poll_t rmv_poll ;
    alarm_block_t block ;
    alarm_block_extern_t block_extern ;
    alarm_poll_t poll ;
    /*alarm_handlers_t*/
    /*alarm_mbuf_t*/
    sched_t sched ;
    transport_root_t transport ;
    /*alarm_async_t*/
    unique_t unique ;
    /*alarm_local_xmits_t*/
} ;

void alarm_disable(alarm_handle_t handle) {
    sys_abort() ;
}

void alarm_schedule(alarm_t a, alarm_handle_t handle, etime_t time) {
    assert(handle->live == TRUE) ;
    assert(handle->count >= 0) ;
    handle->count ++ ;
    a->schedule(a->env, handle, time) ;
}

name_t alarm_name(alarm_t a) {
    return a->name ;
}

etime_t alarm_gettime(alarm_t a) {
#if 0
    now = time_of_secs_usecs(tv.tv_sec, tv.tv_usec) ;

    /* Yuck!!  BUG!!
     */
    if (time_gt(last, now)) {
#if 0
	eprintf("SYS:gettime:time going backwards %d usecs\n",
		(int)(last.usecs - now.usecs)) ;
#endif
	return last ;
    }

    last = now ;
    return now ;
#endif
    return a->gettime(a->env) ;
}

alarm_handle_t alarm_alarm(alarm_handler_t handler, env_t env) {
    alarm_handle_t h = record_create(alarm_handle_t, h) ;
    h->upcall = handler ;
    h->env = env ;
    h->count = 0 ;
    h->live = TRUE ;
    return h ;
}


static inline
void check_free(alarm_handle_t h) {
    if (h->live == FALSE &&
	h->count == 0) {    
	record_free(h) ;
    }
}

void alarm_alarm_free(alarm_handle_t h) {
    assert(h->count >= 0) ;
    assert(h->live == TRUE) ;
    h->live = FALSE ;
    check_free(h) ;
}

void alarm_handle_deliver(alarm_handle_t h, etime_t time) {
    alarm_handler_t handler ;
    env_t env ;
    assert_bool(h->live) ;
    assert(h->count > 0) ;
    if (h->live) {
	handler = h->upcall ;
	env = h->env ;
	assert(handler) ;
    } else {
	env = NULL ;
	handler = NULL ;
    }
    h->count -- ;
    check_free(h) ;
    if (handler) {
	handler(env, time) ;
    }
}

bool_t alarm_check(alarm_t a) {
    return a->check(a->env) ;
}

etime_t alarm_min(alarm_t a) {
    return a->min(a->env) ;
}

void alarm_add_sock_recv(alarm_t a, debug_t debug, sock_t sock, sock_handler_t handler, env_t handler_env) {
    a->add_sock_recv(a->env, debug, sock, handler, handler_env) ;
}

void alarm_rmv_sock_recv(alarm_t a, sock_t sock) {
    a->rmv_sock_recv(a->env, sock) ;
}

void alarm_add_sock_xmit(alarm_t a, debug_t debug, sock_t sock, sock_handler_t handler, env_t handler_env) {
    a->add_sock_xmit(a->env, debug, sock, handler, handler_env) ;
}

void alarm_rmv_sock_xmit(alarm_t a, sock_t sock) {
    a->rmv_sock_xmit(a->env, sock) ;
}

void alarm_add_poll(alarm_t a, name_t name, alarm_poll_handler_t upcall, env_t env) {
    a->add_poll(a->env, name, upcall, env) ;
}

void alarm_rmv_poll(alarm_t a, name_t name) {
    a->rmv_poll(a->env, name) ;
}

void alarm_block(alarm_t a) {
    a->block(a->env) ;
}

void alarm_block_extern(alarm_t a, struct pollfd *socks, unsigned *nsocks, int *timeout) {
    assert(a->block_extern) ;
    a->block_extern(a->env, socks, nsocks, timeout) ;
}

bool_t alarm_poll(alarm_t a, alarm_poll_type_t type) {
    return a->poll(a->env, type) ;
}

sched_t alarm_sched(alarm_t a) {
    return a->sched ;
}

transport_t alarm_transport(
	alarm_t a,
	domain_t domain,
        view_local_t ls,
        view_state_t vs,
	stack_id_t stack,
	transport_deliver_t deliver,
	env_t env
) {
    return transport_create(a->transport, domain, ls, vs, stack, deliver, env) ;
}

void alarm_deliver(alarm_t a, iovec_t iov) {
    transport_deliver(a->transport, iov) ;
}

void alarm_async(alarm_t a, view_state_t vs, const endpt_id_t *endpt) {
    transport_async(a->transport, vs, endpt) ;
}

/*alarm_async       : t -> Async.t*/

unique_t alarm_unique(alarm_t a) {
    if (!a->unique) {
	sys_panic(("REAL:unique not installed")) ;
    }
    return a->unique ;
}
/*alarm_local_xmits : t -> Route.xmits*/

alarm_t alarm_create(
	name_t name,
	alarm_gettime_t gettime,
	alarm_schedule_t schedule,
	alarm_check_t check,
	alarm_min_t min,
	alarm_add_sock_recv_t add_sock_recv,
	alarm_rmv_sock_recv_t rmv_sock_recv,
	alarm_add_sock_xmit_t add_sock_xmit,
	alarm_rmv_sock_xmit_t rmv_sock_xmit, 
	alarm_add_poll_t add_poll,
	alarm_rmv_poll_t rmv_poll,
	alarm_block_t block,
	alarm_block_extern_t block_extern,
	alarm_poll_t poll,
	env_t env
) {
    alarm_t a = record_create(alarm_t, a) ;
    record_clear(a) ;
    a->last_time = time_zero() ;
    a->name = name ;
    a->env = env ;
    a->gettime = gettime ;
    a->schedule = schedule ;
    a->check = check ;
    a->min = min ;
    a->add_sock_recv = add_sock_recv ;
    a->rmv_sock_recv = rmv_sock_recv ;
    a->add_sock_xmit = add_sock_xmit ;
    a->rmv_sock_xmit = rmv_sock_xmit ;
    a->add_poll = add_poll ;
    a->rmv_poll = rmv_poll ;
    a->block = block ;
    a->block_extern = block_extern ;
    a->poll = poll ;
    return a ;
}

void alarm_init_internal(
	alarm_t a,
	/*transport_root_t transport,*/
	/*alarm_mbuf_t,*/
	sched_t sched,
	/*alarm_async_t,*/
	unique_t unique
	/*alarm_local_xmits_t*/
) {
    a->sched = sched ;
    a->unique = unique ;
    a->transport = transport_root(sched) ;
}
