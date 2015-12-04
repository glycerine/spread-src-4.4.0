/**************************************************************/
/* GOSSIP.C */
/* Author: Mark Hayden, 7/00 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
/*

 * This implements a simple "reflector" service that can be
 * used to locate other processes in the absense of IP multicast.

 * To use this: set UDP_USE_GOSSIP to 1 in config.h and run ./gossip
 * on the same machine that you are running other programs.  Note
 * that this currently only works for processes all on a single
 * machine.

 */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/etime.h"
#include "infr/sys.h"

#define GOSSIP_HOLD_SECS (30)

typedef struct {
    bool_t active ;
    etime_t time ;
    sys_addr_t addr ;
} client_t ;

int main(int argc, char *argv[]) {
    client_t *clients ;
    int nclients ;
    sys_addr_t addr ;
    int sock ;

    addr.port.i = trans_hton16(UDP_GOSSIP_PORT).net ;
    addr.inet = sys_inet_any() ;
    sock = sys_sock_dgram() ;
    sys_bind(sock, &addr) ;
    sys_sockopt_bsdcompat(sock) ;
    sys_setsock_recvbuf(sock, SOCK_BUF_SIZE) ;
    sys_setsock_sendbuf(sock, SOCK_BUF_SIZE) ;

    nclients = 0 ;
    clients = sys_alloc(0) ;
    
    while(1) {
	sys_addr_t addr ;
#define MAX_BUF (1<<16)
	char buf[MAX_BUF] ;
	//unsigned addr_len ;
	int ret ;
	int ofs ;
	etime_t now ;
	
	ret = sys_recvfrom(sock, buf, sizeof(buf), &addr) ;
	if (ret == -1) {
	    sys_panic_perror(("GOSSIP:recvfrom")) ;
	}
#if 0
	eprintf("GOSSIP:got message from %s len=%d\n",
		sys_addr_to_string(&addr), ret) ;
#endif

	for (ofs=0;ofs<nclients;ofs++) {
	    if (!clients[ofs].active) {
		continue ;
	    }
	    if (sys_addr_eq(&addr, &clients[ofs].addr)) {
		break ;
	    }
	}

	if (ofs == nclients) {
	    for (ofs=0;ofs<nclients;ofs++) {
		if (!clients[ofs].active) {
		    break ;
		}
	    }
	    
	    if (ofs == nclients) {
		client_t *old = clients ;
		len_t nold = nclients ;
		nclients = nclients * 2 + 1 ;
		clients = sys_alloc(nclients * sizeof(clients[0])) ;
		memset(clients, 0, nclients * sizeof(clients[0])) ;
		memcpy(clients, old, nold * sizeof(clients[0])) ;
		ofs = nold ;
		sys_free(old) ;
		assert(!clients[ofs].active) ;
	    }
	    
	    assert(ofs < nclients) ;
	    clients[ofs].active = TRUE ;
	    clients[ofs].addr = addr ;
	}

	now = time_intern(sys_gettime()) ;
	clients[ofs].time = now ;
	
	for (ofs=0;ofs<nclients;ofs++) {
	    if (time_ge(time_sub(now, clients[ofs].time), time_of_secs(GOSSIP_HOLD_SECS))) {
		clients[ofs].active = FALSE ;
	    }
	    if (!clients[ofs].active) {
		continue ;
	    }
	    sys_sendto(sock, buf, ret, &clients[ofs].addr) ;
	}
    }
}
