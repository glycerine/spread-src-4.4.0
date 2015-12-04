/**************************************************************/
/* NETSIM.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/addr.h"
#include "infr/etime.h"
#include "infr/priq.h"
#include "infr/domain.h"
#include "infr/trace.h"
#include "infr/alarm.h"
#include "trans/netsim.h"
#include "infr/transport.h"

static string_t name = "NETSIM" ;

#define log(args) LOG_GEN(LOG_DATA, name, "", name, "", args)

#define NPOLLS 4

typedef struct {
    priq_t priq ;
    etime_t time ;
    etime_t next_log ;
    struct {
	name_t name ;
	alarm_poll_handler_t upcall ;
	env_t env ;
	int count ;
    } polls[NPOLLS] ;
} *state_t ;

static
void schedule(env_t env, alarm_handle_t h, etime_t time) {
    state_t s = env ;
    priq_add(s->priq, time, h) ;
}

static
etime_t gettime(env_t env) {
    state_t s = env ;
    return s->time ;
}

static
bool_t check(env_t env) {
    state_t s = env ;
    alarm_handle_t handle ;
    etime_t time ;
    log(("check")) ;

    if (priq_empty(s->priq)) {
	return FALSE ;
    }

    if (!time_ge(s->time, priq_min(s->priq))) {
	return FALSE ;
    }

    priq_get(s->priq, &time, (void**)&handle) ;
    assert(time_ge(s->time, time)) ;
    log(("check:delivering")) ;
    alarm_handle_deliver(handle, s->time) ;
    return TRUE ;
}

static
etime_t min(env_t env) {
    sys_abort() ; return time_zero() ;
}

static
void add_sock_recv(env_t env, debug_t debug, sock_t sock, sock_handler_t handle, env_t handle_env) {
    sys_abort() ; return ;
}

static
void rmv_sock_recv(env_t env, sock_t sock) {
    sys_abort() ;
}

static
void add_sock_xmit(env_t env, debug_t debug, sock_t sock, sock_handler_t handle, env_t handle_env) {
    sys_abort() ;
}

static
void rmv_sock_xmit(env_t env, sock_t sock) {
    sys_abort() ;
}

static
void add_poll(env_t env, name_t name, alarm_poll_handler_t handler, env_t poll_env) {
    state_t s = env ;
    int i ;
    for (i=0;i<NPOLLS;i++) {
	if (s->polls[i].count &&
	    string_eq(s->polls[i].name, name)) {
	    break ;
	}
    }
    if (i == NPOLLS) {
	for (i=0;i<NPOLLS;i++) {
	    if (!s->polls[i].count) {
		break ;
	    }
	}
	assert(i < NPOLLS) ;
	s->polls[i].name = name ;
	s->polls[i].upcall = handler ;
	s->polls[i].env = poll_env ;
	s->polls[i].count = 0 ;
    }
    s->polls[i].count ++ ;
}

static
void rmv_poll(env_t env, name_t name) {
    /*state_t s = env ;*/
    sys_abort() ;
}

static
void do_block(env_t env) {
    state_t s = env ;
    if (priq_empty(s->priq)) {
	sys_abort() ;
    }
#if 0
    eprintf("NETSIM:advancing %s -> %s\n", 
	    time_to_string(s->time), time_to_string(priq_min(s->priq))) ;
    priq_dump(s->priq) ;
#endif
    s->time = priq_min(s->priq) ;

    log(("alarm:advancing to %s", time_to_string(s->time))) ;
#if 0
    if (time_ge(s->time, s->next_log)) {
	eprintf("NETSIM:advancing to %s\n", time_to_string(s->time)) ;
	s->next_log = time_add(s->time, time_of_secs(100)) ;
    }
#endif
}

static
bool_t do_poll(env_t env, alarm_poll_type_t type) {
    state_t s = env ;
    int i ;
    bool_t got_some = FALSE ;
    s->time = time_add(s->time, time_of_usecs(1000ULL)) ;
    for (i=0;i<NPOLLS;i++) {
	if (!s->polls[i].count) {
	    continue ;
	}
	if (s->polls[i].upcall(s->polls[i].env)) {
	    got_some = TRUE ;
	}
    }
    return got_some ;
}

static
alarm_t do_alarm_init(void) {
    ofs_t ofs ;
    state_t s = record_create(state_t, s) ;
    s->priq = priq_create() ;
    s->time = time_zero() ;
    s->next_log = time_zero() ;
    for (ofs=0;ofs<NPOLLS;ofs++) {
	s->polls[ofs].count = 0 ;
    }
    return alarm_create(name,
		 gettime,
		 schedule,
		 check,
		 min,
		 add_sock_recv,
		 rmv_sock_recv,
		 add_sock_xmit,
		 rmv_sock_xmit,
		 add_poll,
		 rmv_poll,
		 do_block,
		 NULL, 
		 do_poll,
		 s) ;
}

/**************************************************************/

typedef struct {
    alarm_t alarm ;
    alarm_handle_t alarm_handle ;
    priq_t priq ;
    etime_t milli ;
    etime_t micro ;
    mux_t mux ;
} *dstate_t ;

static
addr_id_t addr(env_t env, addr_type_t mode) {
    dstate_t s = env ;
    assert(mode == ADDR_NETSIM) ;
    return addr_netsim(s->mux) ;
}

static
void *prepare(env_t env, domain_dest_t dest) {
    /*state_t s = env ;*/
    return NULL ;
}

static
void do_xmit(env_t env, env_t xmit_env, marsh_t marsh) {
    dstate_t s = env ;
    etime_t now = alarm_gettime(s->alarm) ;
    iovec_t iov = iovec_take(marsh_to_iovec(marsh)) ;
    etime_t deliv ;
    assert(xmit_env == NULL) ;
    deliv = time_add(now, s->milli) ;
    log(("adding timeout %s", time_to_string(deliv))) ;
    alarm_schedule(s->alarm, s->alarm_handle, deliv) ;
    priq_add(s->priq, deliv, iov) ;
}

static
void disable(env_t env) {
    sys_abort() ;
}

static
void xmit_disable(env_t env, env_t xmit_env) {
    /* Nothing to release.
     */
}

static
void alarm_handler(env_t env, etime_t now) {
    dstate_t s = env ;
    log(("alarm time=%s", time_to_string(now))) ;
    while (!priq_empty(s->priq) &&
	   time_ge(now, priq_min(s->priq))) {
	iovec_t iov ;
	log(("domain:delivering")) ;
	priq_get(s->priq, NULL, (void**)&iov) ;
	alarm_deliver(s->alarm, iov) ;
    }
}

domain_t netsim_domain(alarm_t alarm) {
    dstate_t s = record_create(dstate_t, s) ;
    net_port_t port ;
    s->mux = 0 ;
    s->alarm = alarm ;
    s->priq = priq_create() ;
    s->milli = time_of_usecs(1000ULL) ;
    s->micro = time_of_usecs(1ULL) ;
    s->alarm_handle = alarm_alarm(alarm_handler, s) ;
    port.i = 1 ;
    unique_set_port(alarm_unique(alarm), port) ; /* this disables Unique warnings */
    return domain_create(s, name, addr, prepare, do_xmit, xmit_disable, disable) ;
}

alarm_t netsim_alarm(
	sched_t sched,
	unique_t unique
) {
    alarm_t a = do_alarm_init() ;
    alarm_init_internal(a, sched, unique) ;
    return a ;
}

#ifndef MINIMIZE_CODE
void netsim_usage(void) {
#if 0
    eprintf("netsim:alarm(%d)\n", priq_usage(a->priq)) ;
    eprintf("netsim:domain(%d)\n", priq_usage(d->priq)) ;
#endif
}
#endif
