/**************************************************************/
/* AF_ENSEMBLE.C */
/* Author: Mark Hayden, 6/2000 */
/* Copyright 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include <trans.h>
#include <util.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/inet.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <asm/uaccess.h>
#include "sys.h"
#include "iovec.h"
#include "view.h"

static string_t name = "AF_ENSEMBLE" ;

#define DBGTRACE() printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__)

static
int get_iovec(struct iovec *user_iov, iovec_t *iovec_ret) {
    struct iovec iov ;
    buf_t buf ;
    refbuf_t rbuf ;
    iovec_t iovec ;
    DBGTRACE() ;
    iovec_ret = NULL ;
    if (copy_from_user(&iov, user_iov, sizeof(iov))) {
	return -EFAULT ;
    }
    DBGTRACE() ;
    if (iov.iov_len > (1<<16)) {
	return -EINVAL ;
    }
    DBGTRACE() ;
    buf = util_alloc(iov.iov_len) ;
    if (!buf) {
	return -ENOMEM ;
    }
    DBGTRACE() ;
    if (copy_from_user(buf, iov.iov_base, iov.iov_len)) {
	util_free(buf) ;
	return -EFAULT ;
    }
    DBGTRACE() ;
    rbuf = refbuf_alloc(name, buf) ;
    if (!rbuf) {
	util_free(buf) ;
	return -ENOMEM ;
    }
    DBGTRACE() ;
    iovec = iovec_alloc(name, rbuf, 0, iov.iov_len) ;
    if (!iovec) {
	refbuf_free(name, rbuf) ;
	return -ENOMEM ;
    }
    DBGTRACE() ;
    *iovec_ret = iovec ;
    return 0 ;
}

static
int do_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg_int) {
    void *arg = (void*) arg_int ;
    iovec_t iovec ;
    int err ;
    unmarsh_t unmarsh ;
    view_state_t vs ;
    
    DBGTRACE() ;
    err = get_iovec(arg, &iovec) ;
    if (err) {
	return err ;
    }

    DBGTRACE() ;
    unmarsh = unmarsh_of_iovec(name, iovec) ;
    if (!unmarsh) {
	iovec_free(name, iovec) ;
	return -ENOMEM ;
    }

    DBGTRACE() ;
    unmarsh_view_state(unmarsh, &vs) ;
    if (!vs) {
	return -ENOMEM ;
    }
    unmarsh_free(unmarsh) ;
    
    DBGTRACE() ;
    eprintf("view:%s\n", view_state_to_string(vs)) ;
    view_state_free(vs) ;
    return 0 ;
}

#define PF_ENSEMBLE 31

static
struct proto_ops dgram_ops = {
    PF_ENSEMBLE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    do_ioctl,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
} ;

static
int do_create(struct socket *sock, int protocol) {
    //struct sock *sk;
    //struct proto *prot;

    if (sock->type != SOCK_DGRAM) {
	return -EINVAL ;
    }

    sock->state = SS_UNCONNECTED ;
    sock->sk = NULL ;		/* BUG? */
    sock->ops = &dgram_ops ;

    DBGTRACE() ;
    sock->sk = sk_alloc(PF_INET, GFP_KERNEL, 1);
    DBGTRACE() ;
    if (!sock->sk) {
	return -ENOMEM ;
    }
    DBGTRACE() ;
    sock_init_data(sock, sock->sk) ;
    DBGTRACE() ;
    sock->sk->destruct = NULL ;
    DBGTRACE() ;
    sock->sk->family = PF_ENSEMBLE ;
    DBGTRACE() ;
#if 0
    {
	extern int main(int argc, const char *argv[]) ;
	const char *argv[] = { "./rand", NULL } ;
	main(1, argv) ;
    }
#endif
    DBGTRACE() ;
    MOD_INC_USE_COUNT ;
    return 0 ;
}

static
struct net_proto_family family_ops = {
    PF_ENSEMBLE,
    do_create
} ;

#ifdef MODULE
int init_module(void) {
    sock_register(&family_ops);
    return 0 ;
}

void cleanup_module(void) {
}
#endif
