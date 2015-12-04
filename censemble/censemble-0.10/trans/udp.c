/**************************************************************/
/* UDP.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef __KERNEL__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#define IOVEC_FILL_WANTED
#include "infr/config.h"
#include "infr/sys.h"
#include "infr/iovec.h"
#include "trans/udp.h"
#include "infr/addr.h"
#include "infr/alarm.h"
#include "infr/domain.h"
#include "infr/transport.h"
#include "infr/util.h"
#include "infr/trans.h"

static string_t name = "UDP" ;

#define BUF_LEN (10*1024)

typedef struct {
    inet_t addr ;
    int count ;
} ipmc_t ;

typedef struct udp_domain_t {
    alarm_t alarm ;
    sock_t sock ;
    sock_t ipmc_sock ;
    net_port_t port ;
    inet_t inet ;
    buf_t buf ;
    ipmc_t *ipmc ;
    len_t nipmc ;
    domain_handler_t handler ;
    env_t handler_env ;
    bool_t enable_multicast ;
    inet_t gossip[UDP_MAX_GOSSIP] ;
    len_t ngossip ;
} *state_t ;

typedef struct {
    len_t naddr ;
    bool_t local ;
    struct {
	sock_t sock ;
	struct sockaddr_in sock_addr ;
    } dest[1] ;
    /* struct is extended by dest */
} *xmit_t ;

static
inet_t ipmc_of_hash(uint32_t hash) {
    inet_t inet ;
    string_t s ;
    s = sys_sprintf("239.255.0.%d", hash % 248) ;
    if (!sys_string_to_inet(s, &inet)) {
	sys_panic_perror(("ipmc_of_hash")) ;
    }
    sys_free(s) ;
    return inet ; 
}

static
void ipmc_add(state_t s, inet_t addr) {
    ofs_t i ;
    for (i=0;i<s->nipmc;i++) {
	if (inet_eq(s->ipmc[i].addr, addr)) {
	    break ;
	}
    }
    if (i == s->nipmc) {
	for (i=0;i<s->nipmc;i++) {
	    if (s->ipmc[i].count == 0) {
		break ;
	    }
	}
    }
    if (i == s->nipmc) {
	ipmc_t *ipmc ;
	len_t nipmc = s->nipmc ? s->nipmc * 2 : 4 ;
	ipmc = sys_alloc(sizeof(*ipmc) * nipmc) ;
	memset(ipmc, 0, sizeof(*ipmc) * nipmc) ;
	memcpy(ipmc, s->ipmc, sizeof(*ipmc) * s->nipmc) ;
	sys_free(s->ipmc) ;
	s->nipmc = nipmc ;
	s->ipmc = ipmc ;
    }
    if (s->ipmc[i].count == 0) {
	s->ipmc[i].addr = addr ;
	/*eprintf("setsockopt:join:count=%d addr=%08x\n", s->ipmc[i].count, s->ipmc[i].addr) ;*/
	sys_sockopt_join(s->ipmc_sock, sys_inet_any(), s->ipmc[i].addr) ;
    }
    s->ipmc[i].count ++ ;
}

#if 0
static
void ipmc_remove(state_t s, inet_t addr) {
    ofs_t i ;
    for (i=0;i<s->nipmc;i++) {
	if (s->ipmc[i].addr == addr) {
	    break ;
	}
    }
    assert(i < s->nipmc) ;
    s->ipmc[i].count -- ;
    if (s->ipmc[i].count == 0) {
	sys_sockopt_leave(s->ipmc_sock, s->ipmc[i].addr) ;
    }
}
#endif

static
addr_id_t addr(env_t env, addr_type_t mode) {
    state_t s = env ;
    assert(mode == ADDR_UDP) ;
    return addr_udp(s->inet, s->port) ;
}

static
env_t prepare(env_t env, domain_dest_t dest) {
    state_t s = env ;
    xmit_t xmit ;
    switch(dest.type) {

    case DOMAIN_PT2PT: {
	addr_id_t addr ;
	ofs_t ofs ;
	len_t len = dest.u.pt2pt.naddr ;
	assert_array_length(dest.u.pt2pt.addr, dest.u.pt2pt.naddr) ;
	xmit = sys_alloc(sizeof(*xmit) - sizeof(xmit->dest) + len * sizeof(xmit->dest[0])) ;
	xmit->naddr = len ;
	xmit->local = FALSE ;
	for (ofs=0;ofs<len;ofs++) {
	    addr = array_get(dest.u.pt2pt.addr, ofs) ;
	    assert(addr.kind == ADDR_UDP) ;
	    xmit->dest[ofs].sock = s->sock ;
	    xmit->dest[ofs].sock_addr.sin_family = AF_INET ;
	    xmit->dest[ofs].sock_addr.sin_addr.s_addr = addr.u.udp.inet.int_s ;
	    xmit->dest[ofs].sock_addr.sin_port = addr.u.udp.port.i ;
	}
    } break ;

    case DOMAIN_MCAST: {
	sys_abort() ;
#if 0
	inet_t addr = ipmc_of_hash(group_id_to_hash(&dest.u.mcast.group)) ;
	/*eprintf("addr=%08x hash=%08x\n", addr.int_s, group_id_to_hash(&dest.u.mcast.group)) ;*/
	ipmc_add(s, addr) ;
	/*xmit->local = TRUE ;*/
#endif
    } break ;

    case DOMAIN_GOSSIP: {
	ofs_t ofs ;
	len_t len ;

	if (s->enable_multicast) {
	    len = s->ngossip + 1 ;
	} else {
	    len = s->ngossip ;
	}

	if (!len) {
	    static bool_t warned ;
	    if (!warned) {
		warned = TRUE ;
		eprintf("UDP:warning:no addresses available for gossipping\n") ;
		eprintf("  (will not print this warning again)\n") ;
	    }
	}
	
	xmit = sys_alloc(sizeof(*xmit) - sizeof(xmit->dest) + len * sizeof(xmit->dest[0])) ;
	xmit->local = TRUE ;
	xmit->naddr = len ;

	for (ofs=0;ofs<s->ngossip;ofs++) {
	    net_uint16_t net_port = trans_hton16(UDP_GOSSIP_PORT) ;
	    xmit->dest[ofs].sock = s->sock ;
	    xmit->dest[ofs].sock_addr.sin_family = AF_INET ;
	    xmit->dest[ofs].sock_addr.sin_addr.s_addr = s->gossip[ofs].int_s ;
	    xmit->dest[ofs].sock_addr.sin_port = net_port.net ;
	}

	if (s->enable_multicast) {
	    inet_t addr = ipmc_of_hash(group_id_to_hash(&dest.u.gossip)) ;
	    net_uint16_t net_port = trans_hton16(UDP_MULTICAST_PORT) ;
	    /*eprintf("addr=%08x hash=%08x\n", addr, group_id_to_hash(&dest.u.mcast.group)) ;*/
	    ipmc_add(s, addr) ;
	    assert(len == s->ngossip + 1) ;
	    xmit->dest[len-1].sock = s->ipmc_sock ;
	    xmit->dest[len-1].sock_addr.sin_family = AF_INET ;
	    xmit->dest[len-1].sock_addr.sin_addr.s_addr = addr.int_s ;
	    xmit->dest[len-1].sock_addr.sin_port = net_port.net ;
	}
    } break ;

    OTHERWISE_ABORT() ;
    }
    return xmit ;
}

static
void do_xmit(env_t env, env_t xmit_env, marsh_t marsh) {
    state_t s = env ;
    xmit_t xmit = xmit_env ;
    ofs_t ofs ;
    struct msghdr mh ;
    int ret ;
    len_t len ;
    iovec_t iov ;
#if 0
#ifdef __GNUC__
    struct iovec mhiov[iov->len + 1] ;
#endif
#endif
#define MAX_IOV 48		/* Yuck */
    struct iovec mhiov[MAX_IOV] ;

    memset(&mh, 0, sizeof(mh)) ; /* Needed for DUNIX, Linux */
    mh.msg_namelen = sizeof(xmit->dest[0].sock_addr) ;
    mh.msg_iov = mhiov ;

    marsh_get(marsh, (void**)&mhiov[0].iov_base, &len, &iov) ;
    iovec_check(iov) ;

    mhiov[0].iov_len = len ;
    assert(mhiov[0].iov_len >= 0) ;

    {
	int niov = MAX_IOV - 1 ;
	iovec_fill(iov, &mhiov[1], &niov) ;
	mh.msg_iovlen = niov + 1 ;
    }

    for (ofs=0;ofs<xmit->naddr;ofs++) {
#if 0
	{
	    net_uint16_t net_port ;
	    net_port.net = xmit->sock_addr[ofs].sin_port ;
	    eprintf("UDP:send sock=%d inet=%s port=%d len=%d\n",
		    xmit->sock,
		    sys_inet_to_string((inet_t)xmit->sock_addr[ofs].sin_addr.s_addr),
		    ens_ntoh16(net_port), len + iovec_len(iov)) ;
	}
#endif
	mh.msg_name = (void*)&xmit->dest[ofs].sock_addr ;
	ret = sendmsg(xmit->dest[ofs].sock, &mh, 0) ;
	if (ret == -1) {
	    static bool_t printed ;
	    if (!printed) {
		printed = TRUE ;
		eprintf("UDP:sendmsg:error:%s (will not print this error again)\n", sys_strerror()) ;
	    }
	}
    }

    if (!xmit->local) {
	marsh_free(marsh) ;
	return ;
    }
    
    {
	iovec_t iov = marsh_to_iovec(marsh) ;
	if (!s->handler) {
	    alarm_deliver(s->alarm, iov) ;
	} else {
	    s->handler(s->handler_env, iov) ;
	}
    }
}

static
void disable(env_t env) {
    /*state_t s = env ;*/
    sys_abort() ;
}

static
void xmit_disable(env_t env, env_t xmit_env) {
    /*state_t s = env ;*/
    xmit_t xmit = xmit_env ;
    record_free(xmit) ;
}

static
void recv_handler(env_t env, sock_t sock) {
    state_t s = env ;
    iovec_t iov ;
    buf_t buf ;
    int ret ;

    if (!s->buf) {
	s->buf = sys_alloc_atomic(BUF_LEN) ;
    }

    ret = sys_recv(sock, s->buf, BUF_LEN) ;
#if 0
    eprintf("UDP:recv len=%d sock=%d\n", ret, sock) ;
#endif

    assert(ret >= 16) ;

    /* If the message is much smaller than the buffer, copy it to a
     * buffer of the right size.
     */
    if (ret < BUF_LEN / 8) {
	buf = sys_alloc_atomic((len_t)ret) ;
	memcpy(buf, s->buf, (len_t)ret) ;
    } else {
	buf = s->buf ;
	s->buf = NULL ;
    }

    iov = iovec_of_buf(buf, 0, (len_t)ret) ;
    if (!s->handler) {
	alarm_deliver(s->alarm, iov) ;
    } else {
	s->handler(s->handler_env, iov) ;
    }
}

state_t udp_domain_full(
        alarm_t alarm,
	env_t handler_env,
	domain_handler_t handler,
	inet_t inet,
	bool_t bind_all,
	bool_t enable_multicast,
	inet_t gossip[],
	len_t ngossip
) {
    state_t s = record_create(state_t, s) ;

    s->alarm = alarm ;
    s->buf = NULL ;
    s->ipmc = sys_alloc(0) ;
    s->nipmc = 0 ;
    s->handler = handler ;
    s->handler_env = handler_env ;
    s->enable_multicast = enable_multicast ;
    assert(ngossip <= UDP_MAX_GOSSIP) ;
    memcpy(s->gossip, gossip, ngossip * sizeof(s->gossip[0])) ;
    s->ngossip = ngossip ;

    s->sock = sys_sock_dgram() ;
    sys_sockopt_bsdcompat(s->sock) ;

    s->inet = inet ;
    if (bind_all) {
	s->port = sys_bind_any(s->sock, sys_inet_any()) ;
    } else {
	s->port = sys_bind_any(s->sock, s->inet) ;
    }
	
    sys_setsock_recvbuf(s->sock, SOCK_BUF_SIZE) ;
    sys_setsock_sendbuf(s->sock, SOCK_BUF_SIZE) ;
    alarm_add_sock_recv(alarm, name, s->sock, recv_handler, s) ;

    if (s->enable_multicast) {
	sys_addr_t addr ;
	s->ipmc_sock = sys_sock_dgram() ;
	sys_sockopt_multicast_loop(s->ipmc_sock) ;
	sys_sockopt_reuse(s->ipmc_sock) ;
	addr.port.i = trans_hton16(UDP_MULTICAST_PORT).net ;
	addr.inet = sys_inet_any() ;
	sys_bind(s->ipmc_sock, &addr) ;
	alarm_add_sock_recv(alarm, name, s->ipmc_sock, recv_handler, s) ;
    } else {
	s->ipmc_sock = -1 ;
    }
    

#if 0
    eprintf("UDP:port=%d udp_sock=%d ipmc_sock=%d\n",
	    s->port, s->sock, s->ipmc_sock) ;
#endif
    unique_set_port(alarm_unique(alarm), s->port) ;

    return s ;
}

domain_t udp_domain(
        alarm_t alarm
) {
    inet_t gossip ;
    udp_domain_t s ;
    gossip = sys_gethost() ;
    s = udp_domain_full(alarm,
			alarm, (domain_handler_t)alarm_deliver,
			sys_gethost(),
			TRUE, TRUE,
			&gossip, 1) ;
    return udp_domain_project(s) ;
}

domain_t udp_domain_project(state_t s) {
    return domain_create(s, name,
			 addr,
			 prepare,
			 do_xmit,
			 xmit_disable,
			 disable) ;
}
#endif
