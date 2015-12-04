/**************************************************************/
/* REFBUF.C : manage pools of buffers */
/* Author: Mark Hayden, 3/95 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/trans.h"
#include "infr/refbuf.h"

uint64_t nactive ;

static char zero_buf[REFBUF_ZERO_LEN] ;
static struct refbuf_t zero_rbuf ;

refbuf_t refbuf_zero(void) {
    if (zero_rbuf.count == 0) {
	refbuf_init(&zero_rbuf, zero_buf) ;
    }
    assert(zero_rbuf.buf == zero_buf) ;
    zero_rbuf.count ++ ;
    return &zero_rbuf ;
}

static char scratch_buf[REFBUF_ZERO_LEN] ;
static struct refbuf_t scratch_rbuf ;

refbuf_t refbuf_scratch(void) {
    if (scratch_rbuf.count == 0) {
	refbuf_init(&scratch_rbuf, scratch_buf) ;
    }
    assert(scratch_rbuf.buf == scratch_buf) ;
    scratch_rbuf.count ++ ;
    return &scratch_rbuf ;
}

void refbuf_free_help(refbuf_t rbuf) {
    refbuf_free_t rfree = rbuf->free ;
    env_t env = rbuf->env ;
    buf_t buf = rbuf->buf ;
    assert(rbuf->count == 1) ;
    assert(rbuf != &zero_rbuf) ;
    record_free(rbuf) ;
    nactive -- ;
    if (rfree) {
	rfree(env, buf) ;
    } else {
	sys_free(buf) ;
    }
}

inline
refbuf_t refbuf_alloc_full(refbuf_free_t rfree, env_t env, buf_t buf) {
    refbuf_t rbuf = record_create(refbuf_t, rbuf) ;
    nactive ++ ;
    rbuf->buf = buf ;
    rbuf->count = 1 ;
    rbuf->free = rfree ;
    rbuf->env = env ;
    return rbuf ;
}

refbuf_t refbuf_alloc(buf_t buf) {
    return refbuf_alloc_full(NULL, NULL, buf) ;
}

uint64_t refbuf_active(void) {
    return nactive ;
}

void
refbuf_free_nop(env_t env, buf_t buf) {
    /* Nop.
     */
}

uint64_t refbuf_active(void) ;

#if 0
/* Base handle to refbuf object.
 */
struct refbuf_root_t {
    struct refbuf_t rbuf ;
} ;
int refbuf_count(debug_t, refbuf_root_t) ;
void refbuf_create(debug_t, buf_t, refbuf_root_t *, refbuf_t *) ;
refbuf_t refbuf_take(debug_t, refbuf_t) ;
/* Info is called to combine information
 * about operations on refcount objects.
 */
string_t refbuf_info(string_t, string_t) ;
string_t refbuf_info2(string_t, string_t, string_t) ;
#endif
