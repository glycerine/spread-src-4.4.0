/**************************************************************/
/* REAL.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/sys.h"
#include "infr/util.h"
#include "infr/priq.h"
#include "infr/alarm.h"
#include "trans/real.h"

#include <sys/types.h>
#include <sys/time.h>
#if defined(HAVE_POLL_H) && !defined(PURIFY)
#include <sys/poll.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>

static string_t name = "REAL" ;

typedef enum { REAL_RECV, REAL_XMIT } item_type_t ;

typedef struct item_t {
    sock_t sock ;
    struct {
	unsigned count ;
	sock_handler_t upcall ;
	env_t env ;
    } recv ;
    struct {
	unsigned count ;
	sock_handler_t upcall ;
	env_t env ;
    } xmit ;
    name_t name ;
} *item_t ;

typedef struct {
    priq_t priq ;
    len_t nsocks ;		/* also applies to pollfd */
    len_t maxsocks ;		/* also applies to pollfd */
    item_t socks ;
    bool_t socks_changed ;	/* Hack! */
    struct pollfd *pollfd ;
} *state_t ;

static
void schedule(env_t env, alarm_handle_t h, etime_t time) {
    state_t s = env ;
    priq_add(s->priq, time, h) ;
}

static
etime_t gettime(env_t env) {
    return time_intern(sys_gettime()) ;
}

static
bool_t check(env_t env) {
    state_t s = env ;
    alarm_handle_t handle ;
    etime_t time = gettime(s) ;

    if (priq_empty(s->priq)) {
	return FALSE ;
    }

    if (!time_ge(time, priq_min(s->priq))) {
	return FALSE ;
    }

    priq_get(s->priq, NULL, (void**)&handle) ;
    alarm_handle_deliver(handle, time) ;
    return TRUE ;
}

static
etime_t min(env_t env) {
    sys_abort() ; return time_zero() ;
}

static
void grow(
        state_t s
) {
    if (s->nsocks == s->maxsocks) {
	item_t ns ;
	struct pollfd *np ;
	s->maxsocks = s->maxsocks * 2 + 1 ;
	assert(s->maxsocks > s->nsocks) ;
	ns = sys_alloc(sizeof(*ns) * s->maxsocks) ;
	memset(ns, 0, sizeof(*ns) * s->maxsocks) ; /* for Purify */
	memcpy(ns, s->socks, sizeof(*ns) * s->nsocks) ;
	np = sys_alloc(sizeof(*np) * s->maxsocks) ;
	memset(np, 0, sizeof(*np) * s->maxsocks) ; /* for Purify */
	memcpy(np, s->pollfd, sizeof(*np) * s->nsocks) ;
	sys_free(s->socks) ;
	sys_free(s->pollfd) ;
	s->socks = ns ;
	s->pollfd = np ;
    }
    assert(s->nsocks < s->maxsocks) ;
}

static
void add_sock(
        state_t s,
	debug_t debug,
	sock_t sock,
	sock_handler_t handle,
	env_t env,
	item_type_t type
) {
    item_t item ;
    ofs_t i ;
    s->socks_changed = TRUE ;

    for (i=0;i<s->nsocks;i++) {
	if (s->socks[i].sock == sock) {
	    break ;
	}
    }

    if (i == s->nsocks) {
	grow(s) ;
	item = &s->socks[i] ;
	item->xmit.count = 0 ;
	item->recv.count = 0 ;
	item->sock = sock ;
	item->name = debug ;
	s->pollfd[i].fd = sock ;
	s->pollfd[i].events = 0 ;
	s->nsocks ++ ;
    } else {
	item = &s->socks[i] ;
    }

    switch (type) {
    case REAL_RECV:
	item->recv.upcall = handle ;
	item->recv.env = env ;
	item->recv.count ++ ;
	break ;
    case REAL_XMIT:
	item->xmit.upcall = handle ;
	item->xmit.env = env ;
	item->xmit.count ++ ;
	break ;
    OTHERWISE_ABORT() ;
    }

    s->pollfd[i].events =
	(item->recv.count ? POLLIN : 0) |
	(item->xmit.count ? POLLOUT : 0) ;
}

static
void add_sock_recv(env_t env, debug_t debug, sock_t sock, sock_handler_t handle, env_t handle_env) {
    state_t s = env ;
    add_sock(s, debug, sock, handle, handle_env, REAL_RECV) ;
}

static
void add_sock_xmit(env_t env, debug_t debug, sock_t sock, sock_handler_t handle, env_t handle_env) {
    state_t s = env ;
    add_sock(s, debug, sock, handle, handle_env, REAL_XMIT) ;
}

static
void rmv_sock(
        state_t s,
	sock_t sock,
	item_type_t type
) {
    item_t item ;
    int i ;
    s->socks_changed = TRUE ;

    for (i=0;i<s->nsocks;i++) {
	if (s->socks[i].sock == sock) {
	    break ;
	}
    }
    assert(i < s->nsocks) ;

    item = &s->socks[i] ;

    switch (type) {
    case REAL_RECV:
	assert(item->recv.count) ;
	item->recv.count -- ;
	break ;
    case REAL_XMIT:
	assert(item->xmit.count) ;
	item->xmit.count -- ;
	break ;
    OTHERWISE_ABORT() ;
    }
    
    s->pollfd[i].events =
	(item->recv.count ? POLLIN : 0) |
	(item->xmit.count ? POLLOUT : 0) ;

    if (item->recv.count ||
	item->xmit.count) {
	return ;
    }

    for (i++;i<s->nsocks;i++) {
	s->socks[i-1] = s->socks[i] ;
	s->pollfd[i-1] = s->pollfd[i] ;
    }

    s->nsocks -- ;
}

static
void rmv_sock_recv(env_t env, sock_t sock) {
    state_t s = env ;
    rmv_sock(s, sock, REAL_RECV) ;
}

static
void rmv_sock_xmit(env_t env, sock_t sock) {
    state_t s = env ;
    rmv_sock(s, sock, REAL_XMIT) ;
}

static
void add_poll(env_t env, name_t name, alarm_poll_handler_t handler, env_t poll_env) {
    sys_abort() ;
}

static
void rmv_poll(env_t env, name_t name) {
    sys_abort() ;
}

#if defined(HAVE_POLL) && !defined(PURIFY)
#define VIA_POLL (1)
#else
#define VIA_POLL (0)
#endif

static
bool_t poll_via_poll(env_t env, alarm_poll_type_t type) {
    state_t s = env ;
    int ret ;
    ofs_t i ;

#if (VIA_POLL) 
    ret = poll(s->pollfd, s->nsocks, 0) ;
#else
    sys_abort() ;
#endif
    
    if (ret == -1) {
	if (errno == EINTR) {
	    return TRUE ;	/* right? */
	}
	sys_panic_perror(("poll")) ;
    }
    
    if (ret == 0) {
	return FALSE ;
    }

    s->socks_changed = FALSE ;
    for (i=0;i<s->nsocks;i++) {
	short revents = s->pollfd[i].revents ;
	item_t item = &s->socks[i] ;
	assert(item->recv.count || item->xmit.count) ;
	if (revents & POLLNVAL) {
	    sys_panic(("REAL:do_poll:poll:invalid socket fd=%u events=%u counts=(%u,%u)",
		       s->pollfd[i].fd, s->pollfd[i].events,
		       item->recv.count, item->xmit.count)) ;
	}
	if (item->recv.count &&
	    s->pollfd[i].revents & (POLLIN | POLLERR | POLLHUP)) {	    
	    item->recv.upcall(item->recv.env, item->sock) ;
	    if (s->socks_changed){
		break ;
	    }
	}
	if (item->xmit.count &&
	    s->pollfd[i].revents & (POLLOUT | POLLERR | POLLHUP)) {	    
	    item->xmit.upcall(item->xmit.env, item->sock) ;
	    if (s->socks_changed){
		break ;
	    }
	}
    }
    return TRUE ;
}

static
void block_via_poll(env_t env) {
    state_t s = env ;
    int timeout ;
    int ret ;

    if (priq_empty(s->priq)) {
	timeout = -1 ;
    } else {
	etime_t time = priq_min(s->priq) ;
	etime_t now = time_intern(sys_gettime()) ;
	if (time_ge(now, time)) {
	    return ;
	}
	time = time_sub(time, now) ;
	timeout = time_to_msecs(time) ;
	if (timeout < 0) {	/* handle overflow */
	    timeout = INT_MAX ;
	}
    }
    
#if (VIA_POLL) 
    ret = poll(s->pollfd, s->nsocks, timeout) ;
#else
    sys_abort() ;
#endif

    if (ret == -1 &&
	errno != EINTR) {
	sys_panic_perror(("poll")) ;
    }
}

static
int fill_fdsets(state_t s, fd_set *rd, fd_set *wr) {
    int max_fd = 0 ;
    int i ;
    FD_ZERO(rd) ;
    FD_ZERO(wr) ;
    for (i=0;i<s->nsocks;i++) {
	item_t item = &s->socks[i] ;
	if (item->sock >= max_fd) {
	    max_fd = item->sock + 1 ;
	}
	assert(item->recv.count ||
	       item->xmit.count) ;
	if (item->recv.count) {
	    FD_SET(item->sock, rd) ;
	}
	if (item->xmit.count) {
	    FD_SET(item->sock, wr) ;
	}
    }
    return max_fd ;
}

static
bool_t poll_via_select(env_t env, alarm_poll_type_t type) {
    state_t s = env ;
    struct timeval tv ;
    fd_set rd ;
    fd_set wr ;
    int max_fd ;
    int ret ;
    int i ;
    max_fd = fill_fdsets(s, &rd, &wr) ;
    
    tv.tv_sec = 0 ;
    tv.tv_usec = 0 ;
    ret = select(max_fd, &rd, &wr, NULL, &tv) ;
    
    if (ret == -1) {
	sys_panic_perror(("REAL:select")) ;
    }
    
    if (ret == 0) {
	return FALSE ;
    }

    s->socks_changed = FALSE ;
    for (i=0;i<s->nsocks;i++) {
	item_t item = &s->socks[i] ;
	assert(item->recv.count ||
	       item->xmit.count) ;
	if (item->recv.count &&
	    FD_ISSET(item->sock, &rd)) {
	    item->recv.upcall(item->recv.env, item->sock) ;
	    if (s->socks_changed){
		break ;
	    }
	}
	if (item->xmit.count &&
	    FD_ISSET(item->sock, &wr)) {
	    item->xmit.upcall(item->xmit.env, item->sock) ;
	    if (s->socks_changed){
		break ;
	    }
	}
    }
    return TRUE ;
}

static
void block_via_select(env_t env) {
    state_t s = env ;
    struct timeval tv, *tvp ;
    int max_fd ;
    fd_set rd ;
    fd_set wr ;
    int ret ;

    if (priq_empty(s->priq)) {
	tvp = NULL ;
    } else {
	etime_t time = priq_min(s->priq) ;
	etime_t now = gettime(s) ;
	uint64_t usecs ;
	if (time_ge(now, time)) {
	    return ;
	}
	time = time_sub(time, now) ;
	usecs = time_to_usecs(time) ;
	tv.tv_sec = usecs / 1000000 ;
	tv.tv_usec = usecs % 1000000 ;
	tvp = &tv ;
    }
    
    max_fd = fill_fdsets(s, &rd, &wr) ;
    ret = select(max_fd, &rd, &wr, NULL, tvp) ;
    if (ret == -1) {
	sys_panic(("REAL:select:%s", sys_strerror())) ;
    }
}

static
bool_t do_poll(env_t env, alarm_poll_type_t type) {
    bool_t ret ;
    if (VIA_POLL) {
	ret = poll_via_poll(env, type) ;
    } else {
	ret = poll_via_select(env, type) ;
    }
    return ret ; 
}

static
void block(env_t env) {
    if (VIA_POLL) {
	block_via_poll(env) ;
    } else {
	block_via_select(env) ;
    }
}

static
void block_extern(
	env_t env,
	struct pollfd *socks,
	unsigned *nsocks,
	int *timeout_arg
) {
    state_t s = env ;

    if (nsocks) {
	if (socks) {
	    int copy_socks ;
	    copy_socks = s->nsocks < *nsocks ? s->nsocks : *nsocks ;
	    memcpy(socks, s->pollfd, copy_socks * sizeof(socks[0])) ;
	}
	*nsocks = s->nsocks ;
    }

    if (timeout_arg) {
	int timeout ;
	if (priq_empty(s->priq)) {
	    timeout = -1 ;
	} else {
	    etime_t time = priq_min(s->priq) ;
	    etime_t now = time_intern(sys_gettime()) ;
	    if (time_ge(now, time)) {
		return ;
	    }
	    time = time_sub(time, now) ;
	    timeout = time_to_msecs(time) ;
	    if (timeout < 0) {	/* handle overflow */
		timeout = INT_MAX ;
	    }
	}
	*timeout_arg = timeout ;
    }
}

alarm_t real_alarm(
	sched_t sched,
	unique_t unique
) {
    state_t s = record_create(state_t, s) ;
    alarm_t a ;
    s->priq = priq_create() ;
    s->maxsocks = 0 ;
    s->nsocks = 0 ;
    s->socks = sys_alloc(sizeof(*s->socks) * s->maxsocks) ;
    s->pollfd = sys_alloc(sizeof(*s->pollfd) * s->maxsocks) ;
    a = alarm_create(name,
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
		 block,
		 block_extern,
		 do_poll,
		 s) ;
    alarm_init_internal(a, sched, unique) ;
    return a ;
}
