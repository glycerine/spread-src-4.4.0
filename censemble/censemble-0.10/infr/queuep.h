/**************************************************************/
/* QUEUEP.H: pointer-based queues */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef QUEUEP_H
#define QUEUEP_H

#include "infr/trans.h"
#include "infr/util.h"

typedef struct queuep {
    struct queuep *prev ;
    struct queuep *next ;
} queuep_t ;

static inline
void queuep_check(queuep_t *q) {
#if 0
#ifndef NDEBUG
    queuep_t *e ;
    queuep_t *witness ;
    int len ;
    int power2 ;
    assert(q) ;
    assert(q->prev->next == q) ;
    witness = q ;
    power2 = 2 ;
    for (e = q->prev, len = 0 ;
	 e != q ;
	 e = e->prev, len++ ) {
	assert(e) ;
	assert(e->prev->next == e) ;
	assert(e != witness) ;
	if (len == power2) {
	    power2 *= 2 ;
	    witness = e ;
	}
    }
#endif
#endif
}

static inline
void queuep_create(queuep_t *q) {
    q->prev = q ;
    q->next = q ;
}

static inline
len_t queuep_length(queuep_t *q) {
    queuep_t *e ;
    ofs_t ofs ;
    queuep_check(q) ;
    for (e = q->next, ofs = 0 ;
	 e != q ;
	 e = e->next, ofs ++) ;
   return ofs ;
}

static inline
void queuep_entry_init(queuep_t *q) {
    q->prev = NULL ;
    q->next = NULL ;
}

static inline
bool_t queuep_entry_active(queuep_t *q) {
    assert(!q->prev == !q->next) ;
    return (q->next != NULL) ;
}

static inline
void queuep_add_help(queuep_t *q, queuep_t *prev, queuep_t *e) {
    queuep_t *next ;
    queuep_check(q) ;
    next = prev->next ;
    next->prev = e ;
    prev->next = e ;
    e->next = next ;
    e->prev = prev ;
    queuep_check(q) ;
}

static inline
void queuep_add(queuep_t *q, queuep_t *m) {
    queuep_add_help(q, q, m) ;
}

/* Remove a particular item.
 */
static inline
void queuep_remove_hack(queuep_t *e) {
    queuep_t *prev ;
    queuep_t *next ;
    assert(e->prev) ;
    assert(e->next) ;
    prev = e->prev ;
    next = e->next ;
    next->prev = prev ;
    prev->next = next ;
    e->next = NULL ;
    e->prev = NULL ;
}

static inline
void queuep_remove(queuep_t *q, queuep_t *e) {
    queuep_check(q) ;
    assert(e->prev) ;
    assert(e->prev != e) ;
    queuep_remove_hack(e) ;
    queuep_check(q) ;
}

static inline
queuep_t *queuep_take(queuep_t *q) {
    queuep_t *m = q->prev ;
    queuep_remove(q, m) ;
    return m ;
}

static inline
queuep_t *queuep_peek(queuep_t *q) {
    return q->prev ;
}

static inline
bool_t queuep_empty(queuep_t *q) {
    queuep_check(q) ;
    return q->prev == q ;
}

/* This should be constant time.
 */
static inline
void queuep_transfer(queuep_t *q0, queuep_t *q1) {
    queuep_t *e ;
    queuep_check(q0) ;
    queuep_check(q1) ;
    while(!queuep_empty(q1)) {
	e = queuep_take(q1) ;
	queuep_add(q0, e) ;
    }
    queuep_check(q0) ;
    queuep_check(q1) ;
}

/* Same as above, but also add in the q1 "entry".
 */
static inline
void queuep_transfer_all(queuep_t *q0, queuep_t *q1) {
    queuep_transfer(q0, q1) ;
    queuep_entry_init(q1) ;
    queuep_add(q0, q1) ;
}

#if 0
typedef struct {
    queuep_t queue ;
    unsigned count ;
} queuep_test_t ;

static inline
void queuep_test(void) {
    queuep_t q[2] ;
    queuep_test_t *x ;
    unsigned count[3] ;
    count[0] = 0 ;
    count[1] = 0 ;
    count[2] = 0 ;
    queuep_create(&q[0]) ;
    queuep_create(&q[1]) ;
    while(1) {
	switch (sys_random(3)) {
	case 0:
	    x = sys_alloc(sizeof(*x)) ;
	    queuep_entry_init(&x->queue) ;
	    x->count = count[0]++ ;
	    /*eprintf("queuep_test:0:%d\n", x->count) ;*/
	    queuep_add(&q[0], &x->queue) ;
	    break ;
	case 1:
	    if (queuep_empty(&q[0])) {
		break ;
	    }
	    x = (queuep_test_t*) queuep_take(&q[0]) ;
	    /*eprintf("queuep_test:1:%d\n", x->count) ;*/
	    assert(x->count == count[1]++) ;
	    queuep_add(&q[1], &x->queue) ;
	    break ;
	case 2:
	    if (queuep_empty(&q[1])) {
		break ;
	    }
	    x = (queuep_test_t*) queuep_take(&q[1]) ;
	    /*eprintf("queuep_test:2:%d\n", x->count) ;*/
	    assert(x->count == count[2]++) ;
	    sys_free(x) ;
	    break ;
	default:
	    sys_abort() ;
	    break ;
	}
    }

}
#endif

#endif QUEUEP_H
