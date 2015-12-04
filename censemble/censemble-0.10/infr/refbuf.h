/**************************************************************/
/* REFBUF.H : reference counted buffers */
/* Author: Mark Hayden, 3/95 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef REFBUF_H
#define REFBUF_H

#include "infr/trans.h"

/* Handles to refbuf objects.
 */
typedef struct refbuf_t *refbuf_t ;

#if 0
/* Create a new handle on the object.
 */
refbuf_t refbuf_copy(refbuf_t) ;

/* Release this handle.
 */
void refbuf_free(refbuf_t) ;

/* Verify status of argument.
 */
void refbuf_check(refbuf_t) ;

/* Access the wrapped object.
 */
cbuf_t refbuf_read(refbuf_t) ;
#endif

/* Wrap a buffer in a reference counter.  The buffer will not
 * be modified until it is released.
 */
refbuf_t refbuf_alloc(buf_t) ;

typedef void (*refbuf_free_t)(env_t, buf_t) ;

refbuf_t refbuf_alloc_full(refbuf_free_t, env_t, buf_t) ;

//#define REFBUF_ZERO_LEN 1024
#define REFBUF_ZERO_LEN (1UL<<16)
refbuf_t refbuf_zero(void) ;

refbuf_t refbuf_scratch(void) ;

/**************************************************************/

/* Handles to refbuf objects.
 */
struct refbuf_t {
    int count ;
    buf_t buf ;
    refbuf_free_t free ;
    env_t env ;
} ;

static inline
void refbuf_check(refbuf_t rbuf) {
    assert(rbuf) ;
    assert(rbuf->buf) ;
    assert(rbuf->count > 0) ;
}

/* Reference count updates.
 */
static inline
refbuf_t refbuf_copy(refbuf_t rbuf) {
    refbuf_check(rbuf) ;
    rbuf->count ++ ;
    return rbuf ;
}

/* Access the wrapped object.
 */
static inline
cbuf_t refbuf_read(refbuf_t rbuf) {
    refbuf_check(rbuf) ;
    return rbuf->buf ;
}

/* Access the wrapped object.
 */
static inline
buf_t refbuf_write(refbuf_t rbuf) {
    assert_eq(rbuf->count, 1) ;
    refbuf_check(rbuf) ;
    return rbuf->buf ;
}

void refbuf_free_help(refbuf_t rbuf) ;

void refbuf_free_nop(env_t, buf_t) ;

static inline
void refbuf_free(refbuf_t rbuf) {
    refbuf_check(rbuf) ;
    if (rbuf->free == refbuf_free_nop) {
	rbuf->count -- ;	/* HACK! */
    } else if (rbuf->count == 1) {
	refbuf_free_help(rbuf) ;
    } else {
	rbuf->count -- ;
    }
}

static inline
void refbuf_init(
	refbuf_t rbuf,
	buf_t buf
) {
    rbuf->count = 1 ;
    rbuf->free = refbuf_free_nop ;
    rbuf->env = NULL ;
    rbuf->buf = buf ;
}

uint64_t refbuf_active(void) ;

/**************************************************************/

#endif /*REFBUF_H*/
