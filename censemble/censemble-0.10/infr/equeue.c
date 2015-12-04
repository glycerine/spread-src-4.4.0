/**************************************************************/
/* EQUEUE.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/equeue.h"
#include "infr/trans.h"
#include "infr/util.h"

struct equeue_t {
    void *arr ;
    ofs_t head ;
    len_t len ;
    len_t alen ;		/* power of 2 */
    len_t size ;
    debug_t debug ;
} ;

static void
grow(equeue_t q) {
    void * oarr = q->arr ;
    int oalen = q->alen ;
    int ohead = q->head ;
    ofs_t i ;

    q->head = 0 ;
    q->alen = 2 * q->alen ;
    q->arr = sys_alloc(q->size * q->alen) ;
    memset(q->arr, 0, q->size * q->alen) ;

#ifndef MINIMIZE_CODE
    if (q->alen * q->size > 1<<12) {
	eprintf("QUEUE:expand:getting large:len=%d:%s\n", q->alen * q->size, q->debug) ;
    }
#endif

    /* This could be done with 2 block copies.
     */
    for (i=0;i<q->len;i++) {
	memcpy(buf_ofs(q->arr, i * q->size), 
	       buf_ofs(oarr, ((ohead + i) & (oalen - 1)) * q->size),
	       q->size) ;
    }
    sys_free(oarr) ;
}

equeue_t equeue_create(debug_t debug, int size) {
    equeue_t q ;
    q = record_create(equeue_t, q) ;
    q->debug = debug ;
    q->size = size ;
    q->alen = 1<<5 ;		/* must be a power of 2 */
    q->head = 0 ;
    q->len = 0 ;
    q->arr = sys_alloc(size * q->alen) ;
    memset(q->arr, 0, size * q->alen) ;
    return q ;
}

bool_t equeue_empty(equeue_t q) {
    return !q->len ;
}

int equeue_length(equeue_t q) {
    return q->len ;
}

void *equeue_add(equeue_t q) {
    void *ret ;
    if (q->len >= q->alen) {
	grow(q) ;
    }
    ret = buf_ofs(q->arr, ((q->head + q->len) & (q->alen - 1)) * q->size) ;
    q->len ++ ;
    return ret ;
}

void *equeue_take(equeue_t q) {
    void *ret ;
    assert(q->len > 0) ;
    ret = buf_ofs(q->arr, (q->head * q->size)) ;
    q->head = (q->head + 1) & (q->alen - 1) ;
    q->len -- ;
    return ret ;
}

void equeue_clear(equeue_t q) {
    ofs_t i ;
    for (i=0;i<q->len;i++) {
	memset(buf_ofs(q->arr, ((q->head + i) & (q->alen - 1))*q->size), 0, q->size) ;
    }
    q->head = 0 ;
    q->len = 0 ;
}

void equeue_free(equeue_t q) {
    sys_free(q->arr) ;
    record_free(q) ;
}

int equeue_usage(equeue_t q) {
    return q->alen ;
}

#ifndef DBD_KERNEL
#include "infr/array_supp.h"
ARRAY_CREATE(equeue)
#endif
