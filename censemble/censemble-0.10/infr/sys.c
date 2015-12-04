/**************************************************************/
/* SYS.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/config.h"
#include "infr/trans.h"
#include "infr/sys.h"
#include "infr/util.h"
#include "infr/stacktrace.h"

#include <stdlib.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <netdb.h>
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <fcntl.h>

#ifdef PURIFY
#undef h_errno
#define h_errno errno
#endif

#define LOOP_EINTR(ret, cmd) \
  do { \
    (ret) = cmd ; \
  } while ((ret) == -1 && errno == EINTR)

static bool_t dont_really_exit ;

static
void pre_exit(void) {
    if (!dont_really_exit) {
	return ;
    }
    eprintf("SYS:pre_exit:stopping rather than exiting\n") ;
    STACKTRACE() ;
#if 0
    kill(-getpgrp(), SIGSTOP) ;	/* stop all threads in my group... */
#endif
    while(1) {
	sleep(10) ;
    }
}

void sys_dont_really_exit(void) {
    dont_really_exit = TRUE ;
}

typedef union {
    struct sockaddr generic ;
    struct sockaddr_in af_inet ;
    struct sockaddr_un af_unix ;
} sockaddr_t ;

static
int protocol(const char *proto_name) {
    struct protoent *proto ;
    proto = getprotobyname(proto_name) ;
    if (!proto) {
	sys_panic_perror(("getprotobyname:no such protocol '%s'", proto_name)) ;
    }
    return proto->p_proto ;
}

static
int sock_family(sock_t s) {
    sockaddr_t sockaddr ;
    len_t len ;
    int ret ;

    len = sizeof(sockaddr.generic) ;
    ret = getsockname(s, &sockaddr.generic, &len) ;

    if (ret == -1) {
	sys_panic_perror(("SYS:sock_family:getsockname")) ;
    }

    if (len < sizeof(sockaddr.generic.sa_family)) {
	sys_panic(("SYS:sock_family:name too short")) ;
    }
    
    return sockaddr.generic.sa_family ;
}

static
len_t fill_addr(
	sock_t s,
	sys_addr_t *addr,
	sockaddr_t *sockaddr
) {
    len_t len ;
    
    switch (sock_family(s)) {

    case AF_INET: {
	sockaddr->af_inet.sin_family = AF_INET ;
	sockaddr->af_inet.sin_addr.s_addr = addr->inet.int_s ;
	sockaddr->af_inet.sin_port = addr->port.i ;
	len = sizeof(sockaddr->af_inet) ;
    } break ;
    
    case AF_UNIX: {
	memset(&sockaddr->af_unix, 0, sizeof(sockaddr->af_unix)) ;
	sockaddr->af_unix.sun_family = AF_UNIX ;
	assert(sizeof(addr) <= sizeof(sockaddr->af_unix.sun_path) - 1) ;
	memcpy(&sockaddr->af_unix.sun_path[1], addr, sizeof(*addr)) ;
	len = sizeof(sockaddr->af_unix) ;
    } break ;

    OTHERWISE_ABORT() ;
    }

    return len ;
}

static
void sock_close_exec(
        int fd
) {
    int ret ;
    long arg ;
    arg = FD_CLOEXEC ;
    ret = fcntl(fd, F_SETFD, arg) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:fcntl(F_SETFD)")) ;
    }
}

static const
uint64_t million = 1000000 ;

uint64_t sys_gettime(void) {
    int ret ;
    struct timeval tv ;
    ret = gettimeofday(&tv, NULL) ;
    if (ret == -1) {
	sys_panic_perror(("gettimeofday")) ;
    }
    assert(tv.tv_usec >= 0 && tv.tv_usec < million) ;
    return tv.tv_sec * million + tv.tv_usec ;
}

string_t sys_gethostname(void) {
    char hostname[256] ;
    int ret ;

    memset(hostname, 0, sizeof(hostname)) ; /* for Purify */
    do {
	ret = gethostname(hostname, sizeof(hostname)) ;
#ifdef __hpux
    } while (ret == -1 && errno == TRY_AGAIN) ;
#else
    } while (ret == -1 && h_errno == TRY_AGAIN) ;
#endif
    if (ret == -1) {
	sys_panic_perror(("sys_gethost:gethostname")) ;
    }
    
    return string_copy(hostname) ;
}

inet_t sys_gethost(void) {
    struct hostent *hostent ;
    char hostname[256] ;
    inet_t inet ;
    int ret ;

    record_clear(&inet) ;	/* for Purify */
    memset(hostname, 0, sizeof(hostname)) ; /* for Purify */

    do {
	ret = gethostname(hostname, sizeof(hostname)) ;

#ifdef __hpux
    } while (ret == -1 && errno == TRY_AGAIN) ;
#else
    } while (ret == -1 && h_errno == TRY_AGAIN) ;
#endif


    if (ret == -1) {
	sys_panic_perror(("sys_gethost:gethostname")) ;
    }
    
    hostent = gethostbyname(hostname) ;
    if (!hostent) {
	sys_panic_perror(("sys_gethost:gethostbyname:'%s'", hostname)) ;
    }
    
    assert(hostent->h_addrtype == AF_INET) ;
    assert(hostent->h_length == sizeof(inet)) ;
    assert(hostent->h_addr_list[0]) ;
    memcpy(&inet, hostent->h_addr_list[0], sizeof(inet)) ;

#ifndef MINIMIZE_CODE
    {
	inet_t local ;
	local.int_s = inet_addr("127.0.0.1") ;
	if (inet_eq(inet, sys_inet_any()) ||
	    inet_eq(inet, local)) {
	    static bool_t warned ;
	    if (!warned) {
		warned = TRUE ;
		eprintf("SYS:gethost:warning:returning 0.0.0.0 or 127.0.0.1\n") ;
		eprintf("  (will not print this error again)\n") ;
	    }
	}
    }
#endif

    return inet ;
}

uint32_t sys_getpid(void) {
    return getpid() ;
}

void sys_pipe(
        sock_t *write,
	sock_t *read
) {
    int fds[2] ;
    int ret ;
    ret = pipe(fds) ;
    if (ret == -1) {
	sys_panic_perror(("sys_pipe")) ;
    }

    sock_close_exec(fds[0]) ;
    sock_close_exec(fds[1]) ;

    *write = fds[1] ;
    *read = fds[0] ;
}

len_t sys_pipe_recv_peek(
	sock_t sock
) {
    int ret ;
    unsigned long len ;
    ret = ioctl(sock, FIONREAD, &len, sizeof(len)) ;
    if (ret == -1) {
	sys_panic_perror(("sys_pipe_read_peek:ioctl(FIONREAD)")) ;
    }
    return (len_t) len ;
}

sock_t sys_sock_dgram(void) {
    sock_t s ;
    s = socket(PF_INET, SOCK_DGRAM, 0) ;
    if (s == -1) {
	sys_panic_perror(("sys_sock_dgram:socket")) ;
    }
    sock_close_exec(s) ;
    return s ;
}

sock_t sys_sock_stream(void) {
    sock_t s ;
    s = socket(PF_INET, SOCK_STREAM, 0) ;
    if (s == -1) {
	sys_panic_perror(("sys_sock_stream:socket")) ;
    }
    sock_close_exec(s) ;
    return s ;
}

sock_t sys_sock_unix_stream(void) {
    sock_t s ;
    s = socket(PF_UNIX, SOCK_STREAM, 0) ;
    if (s == -1) {
	sys_panic_perror(("sys_sock_unix_stream:socket")) ;
    }
    sock_close_exec(s) ;
    return s ;
}

void sys_sock_close(sock_t sock) {
    int ret ;
    LOOP_EINTR(ret, close(sock)) ;
    if (ret == -1) {
	sys_panic_perror(("close")) ;
    }
}

void sys_sockopt_reuse(sock_t s) {
    int optval = 1 ;
    int ret ;
    ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) ;
    if (ret == -1) {
	sys_panic_perror(("setsockopt(REUSEADDR)")) ;
    }
#ifndef __linux__
    ret = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) ;
    if (ret == -1) {
	sys_panic_perror(("setsockopt(REUSEPORT)")) ;
    }
#endif
}

void sys_sockopt_tcp_no_delay(sock_t s) {
    int optval = 1 ;
    int ret ;
    int family ;

    family = sock_family(s) ;

    /* Quietly ignore this on UNIX sockets.
     */
    if (family == AF_UNIX) {
	return ;
    }

    ret = setsockopt(s, protocol("TCP"), TCP_NODELAY, &optval, sizeof(optval)) ;
    if (ret == -1) {
	sys_panic_perror(("setsockopt(TCP_NODELAY)")) ;
    }
}

void sys_sock_nonblock(sock_t s) {
    long flags ;
    int ret ;
#if 0
    ret = fcntl(s, F_GETFL) ;

    if (ret == -1) {
	sys_panic_perror(("fcntl(GETFL)")) ;
    }
    
    flags = ret | O_NONBLOCK ;
#else
    flags = O_NONBLOCK ;
#endif

    ret = fcntl(s, F_SETFL, flags) ;
    if (ret == -1) {
	sys_panic_perror(("fcntl(SETFL, O_NONBLOCK)")) ;
    }

    ret = fcntl(s, F_GETFL, flags) ;
    if (ret == -1) {
	sys_panic_perror(("fcntl(GETFL, O_NONBLOCK)[2]")) ;
    }
    assert(ret | O_NONBLOCK) ;
}

void sys_sockopt_bsdcompat(sock_t s) {
#ifdef __linux__
    int optval = 1 ;
    int ret ;
    ret = setsockopt(s, SOL_SOCKET, SO_BSDCOMPAT, &optval, sizeof(optval)) ;
    if (ret == -1) {
	sys_panic_perror(("setsockopt(BSDCOMPAT)")) ;
    }
#endif
}

/* Set multicast to be looped back.
 */
void sys_sockopt_multicast_loop(sock_t s) {
    uint8_t val = 1 ;
    int ret ;
    ret = setsockopt(s, protocol("IP"), IP_MULTICAST_LOOP, &val, sizeof(val)) ;
    if (ret == -1) {
	sys_panic_perror(("setsockopt(IP_MULTICAST_LOOP)")) ;
    }
}

net_port_t sys_bind_any(sock_t s, inet_t inet) {
    struct sockaddr_in sock_addr ;
#ifdef __linux__
    socklen_t namelen ;
#else
    int namelen ;
#endif
    int ret ;

    sock_addr.sin_family = AF_INET ;
    sock_addr.sin_addr.s_addr = inet.int_s ;
    sock_addr.sin_port = 0 ;
    ret = bind(s, (void*)&sock_addr, sizeof(sock_addr)) ;
    if (ret == -1) {
	sys_panic_perror(("sys_bind_any:bind")) ;
    }

    namelen = sizeof(sock_addr) ;
    ret = getsockname(s, (void*)&sock_addr, &namelen) ;
    if (ret == -1) {
	sys_panic_perror(("sys_bind_any:getsockname")) ;
    }
    
    assert(namelen == sizeof(sock_addr)) ;

    {
	net_port_t ret ;
	ret.i = sock_addr.sin_port ;
	return ret ;
    }
}

void sys_bind(sock_t s, sys_addr_t *addr) {
    sockaddr_t sockaddr ;
    int ret ;
    int len ;

    len = fill_addr(s, addr, &sockaddr) ;

    ret = bind(s, &sockaddr.generic, len) ;
    if (ret == -1) {
	sys_panic_perror(("sys_bind:bind")) ;
    }
}

bool_t sys_addr_eq(
	const sys_addr_t *a0,
	const sys_addr_t *a1
) {
    return a0->inet.int_s == a1->inet.int_s &&
	a0->port.i == a1->port.i ;
}

string_t sys_addr_to_string(const sys_addr_t *a) {
    string_t tmp ;
    string_t ret ;
    net_uint16_t net ;
    net.net = a->port.i ;
    tmp = sys_inet_to_string(a->inet) ;
    ret = sys_sprintf("{inet=%s;port=%u}",
		      tmp, trans_ntoh16(net)) ;
    string_free(tmp) ;
    return ret ;
}

len_t sys_recvfrom(
        sock_t sock,
	buf_t buf,
	len_t len,
	sys_addr_t *sys_addr
) {
    sockaddr_t addr ;
    int addr_len ;
    int ret ;
    addr_len = sizeof(addr.af_inet) ;
    LOOP_EINTR(ret, recvfrom(sock, buf, len, 0, &addr.generic, &addr_len)) ;
    if (ret == -1) {
	switch (errno) {
	case EAGAIN:
	    return 0 ;
	    break ;
	default:
	    sys_panic_perror(("SYS:recvfrom")) ;
	    break ; 
	}
    }
    assert_eq(addr.generic.sa_family, AF_INET) ;
    sys_addr->inet.int_s = addr.af_inet.sin_addr.s_addr ;
    sys_addr->port.i = addr.af_inet.sin_port ;
    return (len_t)ret ;
}

len_t sys_recv(sock_t sock, buf_t buf, len_t len) {
    int ret ;
    LOOP_EINTR(ret, recv(sock, buf, len, 0)) ;
    if (ret == -1) {
	switch (errno) {
	case EAGAIN:
	case EPIPE:
	case ECONNRESET:
	    eprintf("SYS:sys_recv:recv:error=%s\n", sys_strerror()) ;
	    return 0 ;
	}
	sys_panic_perror(("SYS:recv")) ;
    }
    return (len_t)ret ;
}

len_t sys_recv_stream(sock_t sock, buf_t buf, len_t len, bool_t *eof) {
    int ret ;
    *eof = FALSE ;
    LOOP_EINTR(ret, recv(sock, buf, len, 0)) ;
    if (ret == -1) {
	switch (errno) {
	case EAGAIN:
	    return 0 ;
	    break ;
	case EPIPE:
	case ECONNRESET:
	    *eof = TRUE ;
	    if (0) {
		eprintf("SYS:sys_recv:recv:error=%s\n", sys_strerror()) ;
	    }
	    return 0 ;
	    break ;
	default:
	    sys_panic_perror(("SYS:recv")) ;
	    break ;
	}
    }
    if (!ret) {
	*eof = TRUE ;
    }
    return (len_t)ret ;
}

void sys_pipe_recv(sock_t sock, buf_t buf, len_t len) {
    int ret ;
    LOOP_EINTR(ret, read(sock, buf, len)) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:pipe_recv:recv")) ;
    }
    assert_eq(ret, len) ;
}

void sys_pipe_send(sock_t sock, buf_t buf, len_t len) {
    int ret ;
    LOOP_EINTR(ret, write(sock, buf, len)) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:pipe_send:send")) ;
    }
    assert_eq(ret, len) ;
}

void sys_listen(sock_t sock) {
    int ret ;
    LOOP_EINTR(ret, listen(sock, 128)) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:listen")) ;
    }
}

void sys_accept(sock_t sock, sock_t *client) {
    int ret ;
    LOOP_EINTR(ret, accept(sock, NULL, 0)) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:accept fd=%u", sock)) ;
    }
    sock_close_exec(ret) ;
    *client = ret ;
}

bool_t sys_connect_nonblock(
        sock_t s,
	sys_addr_t *addr,
	bool_t *complete
) {
    sockaddr_t sockaddr ;
    bool_t error ;
    len_t len ;
    int ret ;

    len = fill_addr(s, addr, &sockaddr) ;

    error = FALSE ;
    *complete = TRUE ;
    LOOP_EINTR(ret, connect(s, &sockaddr.generic, len)) ;
    if (ret == -1) {
	switch (errno) {
	case EINPROGRESS:
	    *complete = FALSE ;
	    break ;

	case ECONNREFUSED:
	case ETIMEDOUT:
	case ENETUNREACH:
	case EAGAIN:
	    error = TRUE ;
#if 1
	    eprintf("SYS:connect_nonblock:%s\n",
		    sys_strerror()); 
#endif
	    break ;

	default:
	    sys_panic_perror(("SYS:connect_nonblock")) ;
	    break ;
	}
    }

    return error ;
}

bool_t sys_connect_nonblock_complete(sock_t sock) {
    int err ;
    int ret ;
    int len ;

    err = 0 ;
    len = sizeof(err) ;
    ret = getsockopt(sock, SOL_SOCKET, SO_ERROR,
		     &err, &len) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:connect_nonblock_complete")) ;
    }
    if (len != sizeof(err)) {
	sys_panic(("SYS:connect_nonblock_complete:bad optlen")) ;
    }
    if (err == 0) {
	return FALSE ;
    }
    return TRUE ;
}

#ifdef __linux__
len_t sys_recv_peek(sock_t sock) {
    unsigned long len ;
    int ret ;
#ifdef PURIFY
    return -1 ;			/* seems to always just return 0 */
#endif
    ret = ioctl(sock, TIOCINQ, &len) ;
    if (ret == -1) {
	sys_panic_perror(("ioctl(TIOCINQ)")) ;
    }
    return len ;
}

len_t sys_send_peek(sock_t sock) {
    unsigned long len ;
    int ret ;
#ifdef PURIFY
    return -1 ;
#endif
    ret = ioctl(sock, TIOCOUTQ, &len) ;
    if (ret == -1) {
	sys_panic_perror(("ioctl(TIOCOUTQ)")) ;
    }
    return len ;
}
#else
len_t sys_recv_peek(sock_t sock) {
    return -1 ;
}

len_t sys_send_peek(sock_t sock) {
    return -1 ;
}
#endif

void sys_sendto(
        sock_t sock,
	buf_t buf,
	len_t len,
	sys_addr_t *sys_addr
) {
    sockaddr_t addr ;
    int ret ;
    addr.af_inet.sin_family = AF_INET ;
    addr.af_inet.sin_addr.s_addr = sys_addr->inet.int_s  ;
    addr.af_inet.sin_port = sys_addr->port.i ;
    LOOP_EINTR(ret, sendto(sock, buf, len, 0, &addr.generic, sizeof(addr.af_inet))) ;
    if (ret == -1) {
	static bool_t printed ;
	if (!printed) {
	    printed = TRUE ;
	    eprintf("SYS:sendto:error:%s (will not print this error again)\n", sys_strerror()) ;
	}
    } else {
	assert(ret == len) ;
    }
}

len_t sys_send(
        sock_t sock,
	buf_t buf,
	len_t len
) {
    int ret ;
    LOOP_EINTR(ret, send(sock, buf, len, 0)) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:send")) ;
    }
    return ret ;
}

#if 1
    /* Old version: lets kernel pick first interface to join
     */
void sys_sockopt_join(sock_t s, inet_t interface, inet_t inet) {
    struct ip_mreq mreq ;
    int ret ;

    memset(&mreq, 0, sizeof(mreq)) ;
    mreq.imr_multiaddr.s_addr = inet.int_s ;
    mreq.imr_interface.s_addr = interface.int_s ;
    ret = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		     &mreq, sizeof(mreq)) ;
    if (ret == -1) {
	static bool_t printed ;
	if (!printed) {
	    printed = TRUE ;
	    eprintf("SYS:sys_sockopt_join:error:%s (will continue)\n", sys_strerror()) ;
	    eprintf("  (this means there is a problem with IP multicast)\n") ;
	    eprintf("  (will not print this error again)\n") ;
	}
    }
}
#else
void sys_sockopt_join(sock_t s, inet_t inet) {
    struct ifconf ifc;
    struct ifreq *ifr;
    int i ;
    int n ;
    int ret ;

    /* First find all the interfaces.
     */
    for (n=10;;n += 10) {
	int len = sizeof(*ifr) * n ;
	ifc.ifc_len = len ;
	ifc.ifc_req = sys_alloc(ifc.ifc_len) ;
	
	ret = ioctl(s, SIOCGIFCONF, &ifc) ;
	if (ret == -1) {
	    sys_panic_perror(("ioctl(SIOCGIFCONF)")) ;
	}
	if (ifc.ifc_len < len) {
	    break ;
	}
	sys_free(ifc.ifc_req) ;
    }
    n = ifc.ifc_len / sizeof(*ifr) ;
    
    /* Then join on all them.
     */
    for (i=0;i<n;i++) {
	struct ip_mreq mreq ;
	struct sockaddr *if_addr ;
	if_addr = &ifc.ifc_req[i].ifr_addr ;
	assert(if_addr->sa_family == AF_INET) ;

	memset(&mreq, 0, sizeof(mreq)) ;
	mreq.imr_multiaddr.s_addr = inet.int_s ;
	mreq.imr_interface = ((struct sockaddr_in*)if_addr)->sin_addr ;

	{
	    inet_t tmp ;
	    string_t s ;
	    tmp.int_s = mreq.imr_interface.s_addr ;
	    s = sys_inet_to_string(tmp) ;
	    eprintf("adding interface '%s'\n", s) ;
	}
	
	ret = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			 &mreq, sizeof(mreq)) ;
	if (ret == -1) {
	    static bool_t printed ;
	    if (!printed) {
		printed = TRUE ;
		eprintf("SYS:sys_sockopt_join:error:%s (will continue)\n", sys_strerror()) ;
		eprintf("  (this means there is a problem with IP multicast)\n") ;
		eprintf("  (will not print this error again)\n") ;
	    }
	}
    }
    
    sys_free(ifc.ifc_req) ;
}
#endif

#if 0
static int if_readconf(void)
{
    int numreqs = 30;
    struct ifconf ifc;
    struct ifreq *ifr;
    int n, err = -1;
    int skfd;

    /* SIOCGIFCONF currently seems to only work properly on AF_INET sockets
       (as of 2.1.128) */ 
    skfd = get_socket_for_af(AF_INET);
    if (skfd < 0) {
	fprintf(stderr, _("warning: no inet socket available: %s\n"),
		strerror(errno));
	/* Try to soldier on with whatever socket we can get hold of.  */
	skfd = sockets_open(0);
	if (skfd < 0)
	    return -1;
    }

    ifc.ifc_buf = NULL;
    for (;;) {
	ifc.ifc_len = sizeof(struct ifreq) * numreqs;
	ifc.ifc_buf = xrealloc(ifc.ifc_buf, ifc.ifc_len);

	if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
	    perror("SIOCGIFCONF");
	    goto out;
	}
	if (ifc.ifc_len == sizeof(struct ifreq) * numreqs) {
	    /* assume it overflowed and try again */
	    numreqs += 10;
	    continue;
	}
	break;
    }

    ifr = ifc.ifc_req;
    for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) {
	lookup_interface(ifr->ifr_name,0);
	ifr++;
    }
    err = 0;

out:
    free(ifc.ifc_buf);
    return err;
}
#endif

void sys_sockopt_leave(sock_t s, inet_t interface, inet_t inet) {
#if 1
    /* Changes above need to be reflected here, too.
     */
    sys_abort() ;
#else
    struct ip_mreq mreq ;
    int ret ;

    memset(&mreq, 0, sizeof(mreq)) ;
    mreq.imr_multiaddr.s_addr = inet.int_s ;
    mreq.imr_interface.s_addr = interface.int_s ;
    ret = setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		     &mreq, sizeof(mreq)) ;
    if (ret == -1) {
	eprintf("SYS:sys_sockopt_leave:error:%s (will continue)\n", sys_strerror()) ;
	eprintf("  (this means there is a problem with IP multicast)\n") ;
    }
#endif
}

bool_t sys_inet_is_multicast(inet_t inet) {
    return (inet.int_s & htonl(0xf0000000)) == htonl(0xe0000000) ;
}

void sys_setsock_sendbuf(sock_t sock, len_t len) {
    int ret ;
    int optval = len ;
    
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&optval, sizeof(optval)) ;
    if (ret == -1 && errno != ENOBUFS) {
	sys_panic_perror(("setsockopt:sendbuf")) ;
    }
}

void sys_setsock_recvbuf(sock_t sock, len_t len) {
    int ret ;
    int optval = len ;
    
    ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&optval, sizeof(optval)) ;
    if (ret == -1 && errno != ENOBUFS) {
	sys_panic_perror(("setsockopt:recvbuf")) ;
    }
}

void sys_setsock_no_udp_checksum(sock_t sock) {
#ifdef __linux__
    int ret ;
    int optval = 1 ;
    
    ret = setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, (void*)&optval, sizeof(optval)) ;
    if (ret == -1) {
	sys_panic_perror(("setsockopt:no_udp_checksum")) ;
    }
#endif
}

string_t sys_inet_to_string(inet_t inet) {
    struct in_addr in_addr ;
    in_addr.s_addr = inet.int_s ;
    return string_copy(inet_ntoa(in_addr)) ;
}

inet_t sys_inet_any(void) {
    inet_t inet ;
    inet.int_s = 0 ;
    return inet ;
}

bool_t sys_string_to_inet(string_t s, inet_t *inet) {
    struct hostent *hostent ;
    record_clear(inet) ;
    do {
	hostent = gethostbyname(s) ;

#ifdef __hpux
    } while (!hostent && errno == TRY_AGAIN) ;
#else
    } while (!hostent && h_errno == TRY_AGAIN) ;
#endif


    if (hostent) {
	assert(hostent->h_addrtype == AF_INET) ;
	assert(hostent->h_length == sizeof(*inet)) ;
	assert(hostent->h_addr_list[0]) ;
	memcpy(inet, hostent->h_addr_list[0], sizeof(*inet)) ;
	return TRUE ;
    }

    inet->int_s = inet_addr(s) ;
    if (inet->int_s != INADDR_NONE) {
	return TRUE ;
    }

    return FALSE ;
}

#if 0
static bool_t got_signal = FALSE ;

static void sighandler(int ignore) {
    //eprintf("SYS:caught sigint\n") ;
    //exit(0) ;
    got_signal = 1 ;
}

bool_t sys_signal_received(void) {
    return got_signal ;
}
#else
static
bool_t got_signal ;

static
void sigseg_handler(int ignore) {
    eprintf("SYS:caught sigseg\n") ;
    if (!got_signal) {
	got_signal = TRUE ;
	pre_exit() ;
    }
    abort() ;
}

static
void sigint_handler(int ignore) {
    eprintf("SYS:caught sigint\n") ;
    if (!got_signal) {
	got_signal = TRUE ;
	pre_exit() ;
    }
    exit(1) ;
}
#endif

void sys_catch_sigint(void) {
    void *ret ;
    ret = signal(SIGINT, sigint_handler) ;
    if (ret == (void*)SIG_ERR) {
	sys_panic_perror(("signal(SIGINT)")) ;
    }
    ret = signal(SIGPIPE, SIG_IGN) ;
    if (ret == (void*)SIG_ERR) {
	sys_panic_perror(("signal(SIGPIPE)")) ;
    }
    if (0) {
	ret = signal(SIGSEGV, sigseg_handler) ;
	if (ret == (void*)SIG_ERR) {
	    sys_panic_perror(("signal(SIGSEGV)")) ;
	}
	ret = signal(SIGBUS, sigseg_handler) ;
	if (ret == (void*)SIG_ERR) {
	    sys_panic_perror(("signal(SIGBUS)")) ;
	}
    }
}

#ifndef NDEBUG
void sys_assert_fail(
        const char *assertion,
	const char *file,
	unsigned int line,
	const char *function
) {
    eprintf("%s:%d: Assertion failed '%s' file=%s line=%d function=%s\n",
	    file, line, assertion, file, line, function) ;
    pre_exit() ;
    STACKTRACE() ;
    abort() ;
}

void sys_assert_eq_uint64_fail(
	const char *exp0,
	const char *exp1,
	uint64_t v0,
	uint64_t v1,
	const char *file,
	unsigned int line,
	const char *function
) {
    eprintf("%s:%d: Assertion failed '%s'(0x%llx)==(0x%llx)'%s' (file=%s line=%d function=%s\n",
	    file, line, exp0, v0, v1, exp1, file, line, function) ;
    pre_exit() ;
    STACKTRACE() ;
    abort() ;

}
#endif

void sys_do_abort(void) {
    pre_exit() ;
    //STACKTRACE() ;
    abort() ;
}

void sys_exit(int i) {
    pre_exit() ;
    exit(i) ;
}

void sys_eprintf(const char *fmt, ...) {
    va_list ap ;
    va_start(ap, fmt) ;
    vfprintf(stderr, fmt, ap) ;
    va_end(ap) ;
    fflush(stderr) ;
}

void sys_do_panic(const char *fmt, ...) {
    va_list ap ;
    va_start(ap, fmt) ;
    vfprintf(stderr, fmt, ap) ;
    va_end(ap) ;
    fprintf(stderr, "\n") ;
    fflush(stderr) ;
    sys_do_abort() ;
}

void sys_do_panic_perror(const char *fmt, ...) {
    int saved_errno = errno ;
    va_list ap ;
    va_start(ap, fmt) ;
    vfprintf(stderr, fmt, ap) ;
    va_end(ap) ;
    fprintf(stderr, ":%s(%u)\n", strerror(saved_errno), errno) ;
    fflush(stderr) ;
    sys_do_abort() ;
}

//#define USE_GC
#ifdef USE_GC
//#define GC_DEBUG
#include <gc.h>

static int in_final ;

typedef struct {
    void *back ;
    stacktrace_t stack ;
    uint32_t magic ;
} header_t ;

#define MAGIC (0xFACEFEEDUL)

static
void finalize(void *obj, void *ignore) {
    header_t *h = obj ;
    if (in_final) {
	abort() ;
    }
    in_final = 1 ;
    fprintf(stderr, "SYS:leak 0x%08x\n", (uint32_t)obj) ;
    if (h->back != h) {
	abort() ;
    }
    if (h->magic != MAGIC) {
	abort() ;
    }
    stacktrace_dump(&h->stack) ;
    in_final = 0 ;
}

static inline
void *gc_malloc(size_t size) {
    header_t *h ;
    size_t total = size + sizeof(header_t) ;
    if (in_final) {
	abort() ;
    }
    h = GC_MALLOC(total) ;
    assert(h) ;
    h->magic = MAGIC ;
    h->back = h ;
    //fprintf(stderr, "SYS:alloc 0x%08x\n", (uint32_t)h) ;
    //stacktrace_get(obj) ;
#if 1
    GC_REGISTER_FINALIZER(h, finalize, NULL, NULL, NULL) ;
#endif
    return h + 1 ;
}

#define stash_it(obj) stacktrace_get(&(((header_t*)obj) - 1)->stack)

static inline
void gc_free(void *obj) {
    header_t *h ;
    h = ((header_t*)obj) - 1 ;
    if (h->magic != MAGIC) {
	abort() ;
    }
    if (h->back != h) {
	abort() ;
    }
    GC_FREE(h) ;
}

void *gc_malloc(size_t) ;
void gc_free(void *) ;
#define malloc(a) gc_malloc(a)
#define free(a) gc_free(a)
#else
#define stash_it(obj) NOP
#endif

inline
void *sys_alloc_try(len_t len) {
    void *obj ;
    if (len == 0) {
	len = 1 ;
    }
    obj = malloc(len) ;
    if (!obj) {
	eprintf("SYS:failed malloc len=%d\n", len) ;
    }

/*#define ALLOC_SCRAMBLE*/
    if (obj) {
#ifdef ALLOC_SCRAMBLE
	ofs_t i ;
	unsigned char c = random() % 254 ;
	for (i=0;i<len;i++) {
	    ((unsigned char*)obj)[i] = (c + i) % 254 + 1 ;
	}
#endif
	stash_it(obj) ;
    }
    return obj ;
}

#if 0
void *sys_alloc(len_t len) {
    void *obj ;
    obj = sys_alloc_try(len) ;
    assert(obj) ;
    return obj ;
}
#else
inline
void *sys_alloc(len_t len) {
    void *obj ;
    if (len == 0) {
	len = 1 ;
    }
    obj = malloc(len) ;
    if (!obj) {
	eprintf("SYS:failed malloc len=%d\n", len) ;
    }

/*#define ALLOC_SCRAMBLE*/
    if (obj) {
#ifdef ALLOC_SCRAMBLE
	ofs_t i ;
	unsigned char c = random() % 254 ;
	for (i=0;i<len;i++) {
	    ((unsigned char*)obj)[i] = (c + i) % 254 + 1 ;
	}
#endif
	stash_it(obj) ;
    }
    assert(obj) ;
    return obj ;
}
#endif

#if 0
void *sys_alloc_atomic(len_t len) {
    return sys_alloc(len) ;
}
#endif

void sys_free(const void *obj) {
    assert(obj) ;
    free((void*)obj) ;		/* strip the "const" */
}

#define MAX_FMT 1024
#define MAX_STRING 1024
string_t sys_sprintf(const char *fmt_arg, ...) {
    va_list va ;
    mut_string_t ret ;
    len_t len ;
    char buf[MAX_STRING] ;
    char fmt[MAX_FMT] ;
    strcpy(fmt, fmt_arg) ;
    assert(strlen(fmt) < MAX_FMT) ;

#if 0
    /* Yuck.
     */
    while(p = strstr(fmt, "%t")) {
	p[1] = 's' ;
    }
#endif

    va_start(va, fmt_arg) ;
#if HAVE_VSNPRINTF
    /* MAX_STRING is the right length to use here.
     */
    len = vsnprintf(buf, MAX_STRING, fmt, va) ;
#elif HAVE_VSPRINTF
    len = vsprintf(buf, fmt, va) ;
#else
#error No vsprintf function available.
#endif
    if (len < 0) {
	abort() ;
    }
    va_end(va) ;
    assert(len < MAX_STRING) ;
    assert(buf[len] == '\0') ;
    assert(strlen(buf) == len) ;
    ret = sys_alloc(len + 1) ;
    strcpy(ret, buf) ;

#if 0
    va_start(va, fmt_arg) ;
    va_end(va) ;
#endif

    return ret ;
}

#if 0
void sys_scanf(const char *fmt, ...) {
}
#endif

bool_t sys_string_to_uint64(string_t s, uint64_t *i) {
    int ret ;
    /* L is used rather than q because of checkergcc.
     */
    *i = 0 ;
    ret = sscanf(s, "%Li", i) ;
    return ret == 1 ;
}

bool_t sys_string_to_uint32(string_t s, uint32_t *i) {
    int ret ;
    *i = 0 ;
    ret = sscanf(s, "%i", i) ;
    return ret == 1 ;
}

uint32_t sys_random(uint32_t max) {
    uint32_t ret ;
    assert(max > 0) ;
    ret = ((uint32_t)random()) % max ;
    assert(ret >= 0) ;
    assert(ret < max) ;
    return ret ;
}

uint64_t sys_random64(uint64_t max) {
    uint64_t ret ;
    assert(max > 0) ;
    ret =
	((uint64_t)random() & 0x0000ffffULL << 48) |
	((uint64_t)random() & 0x00ffffffULL << 24) |
	((uint64_t)random() & 0x00ffffffULL <<  0) ;
    ret %= max ;
    assert(ret >= 0) ;
    assert(ret < max) ;
    return ret ;
}

void sys_random_seed(void) {
    long int seed = sys_getpid() ^ sys_gettime() ;
    //eprintf("SYS:seed=%lu\n", seed) ;
    srandom(seed) ;
}

string_t sys_strerror(void) {
    return strerror(errno) ;
}

void sys_suicide(int argc, char *argv[]/*, char *envp[]*/) {
    //extern int main(int argc, char *argv[]) ;
    int ret ;
    eprintf("SYS:suicide\n") ;
    
    if (0) {
	int fd ;
	for (fd = 3 ; fd < 4096 ; fd ++ ) {
	    ret = close(fd) ;
	    if (ret == -1 &&
		errno != EBADF) {
		sys_panic_perror(("SYS:suicide:close")) ;
	    }
	}
    }
    
    ret = execvp(argv[0], argv /*, envp*/) ;
    if (ret == -1) {
	sys_panic_perror(("SYS:suicide:execve")) ;
    }
#if 0    
    ret = main(argc, argv) ;
    exit(ret) ;
#endif
}

#ifdef PURIFY
int sendmsg(int sock, const struct msghdr *msg, int flags) {
    char *buf ;
    int ofs ;
    int len ;
    int ret ;
    int i ;
    for (len=0,i=0;i<msg->msg_iovlen;i++) {
	len += msg->msg_iov[i].iov_len ;
    }
    buf = sys_alloc(len) ;
    for (ofs=0,i=0;
	 i<msg->msg_iovlen;
	 ofs+=msg->msg_iov[i].iov_len,i++) {
	memcpy(buf + ofs, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len) ;
    }
    assert(ofs == len) ;
    ret = sendto(sock, buf, len, flags, msg->msg_name, msg->msg_namelen) ;
    sys_free(buf) ;
    return ret ;
}

int recvmsg(int sock, struct msghdr *msg, int flags) {
    char *buf ;
    int ofs ;
    int len ;
    int ret ;
    int end ;
    int i ;
    for (len=0,i=0;i<msg->msg_iovlen;i++) {
	len += msg->msg_iov[i].iov_len ;
    }
    buf = sys_alloc(len) ;
    memset(buf, 0, len) ;
    ret = recvfrom(sock, buf, len, flags, msg->msg_name, &msg->msg_namelen) ;
    end = ret ;
    if (end < 0) {
	end = 0 ;
    }
    //eprintf("ret=%d %s\n", ret, hex_of_bin(buf, end)) ;
    for (ofs=0,i=0;
	 i<msg->msg_iovlen && ofs < end;
	 ofs+=msg->msg_iov[i].iov_len,i++) {
	int amt ;
	amt = msg->msg_iov[i].iov_len ;
	if (ofs + amt > end) {
	    amt = end - ofs ;
	}
	
	//eprintf("i=%u ofs=%u amt=%u end=%u buf=%08lx<-%08lx\n",
	//i, ofs, amt, end, (unsigned long)msg->msg_iov[i].iov_base,
	//(unsigned long)buf) ;
	memcpy(msg->msg_iov[i].iov_base, buf + ofs, amt) ;
	//eprintf("gorp %s\n", hex_of_bin(msg->msg_iov[i].iov_base, amt)) ;
    }
    assert(ofs == len) ;
    sys_free(buf) ;
    return ret ;
}

int poll(void) {
    sys_abort() ;
}
#endif
