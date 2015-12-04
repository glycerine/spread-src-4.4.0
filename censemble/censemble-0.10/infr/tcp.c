#if 0
/**************************************************************/
/* TCP.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef __KERNEL__
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/iovec.h"
#include "infr/tcp.h"

static string_t name = "TCP" ;

static void do_send(sock_t s, buf_t buf, ofs_t ofs, len_t len) {
    int ret ;
    while (ofs < len) {
	ret = send(s, buf_ofs(buf, ofs), len, 0) ;
	if (ret == -1) {
	    perror("send") ;
	    exit(1) ;
	}
	assert(ret >= 0) ;
	assert((len_t)ret <= len) ;
	ofs += ret ;
	len -= ret ;
    }
}

static void do_recv(sock_t s, buf_t buf, ofs_t ofs, len_t len) {
    while (ofs < len) {
    int ret ;
	ret = recv(s, buf_ofs(buf, ofs), len, 0) ;
	if (ret == -1) {
	    perror("send") ;
	    exit(1) ;
	}
	assert(ret >= 0) ;
	assert((len_t)ret <= len) ;
	ofs += ret ;
	len -= ret ;
    }
}

void tcp_send(sock_t s, iovec_t iov) {
    len_t len = iovec_len(iov) ;
    buf_t buf ; 
    do_send(s, &len, 0, sizeof(len)) ;
    buf = sys_alloc(len) ;
    iovec_flatten_buf(name, iov, buf, 0, len) ;
    do_send(s, buf, 0, len) ;
}

iovec_t tcp_recv(sock_t s) {
    refbuf_t rbuf ;
    iovec_t iov ;
    len_t len ;
    buf_t buf ; 
    do_recv(s, &len, 0, sizeof(len)) ;
    assert(aligned4(len)) ;
    buf = sys_alloc(len) ;
    do_recv(s, buf, 0, len) ;
    rbuf = refbuf_alloc(buf) ;
    iov = iovec_alloc(rbuf, 0, len) ;
    return iov ;
}
#endif
#endif
