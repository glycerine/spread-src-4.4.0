/**************************************************************/
/* EQUEUE_SUPP.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"

#define METHOD_HELP2(class,func) class ## _equeue_ ## func
#define METHOD_HELP(class,func) METHOD_HELP2(class,func)
#define METHOD(func) METHOD_HELP(CLASS_NAME,func)
#define TYPE_HELP2(class) class ## _equeue_t
#define TYPE_HELP(class) TYPE_HELP2(class)
#define EQUEUE_T TYPE_HELP(CLASS_NAME)

typedef struct {
    ITEM_T *arr ;
    ofs_t head ;
    len_t len ;
    len_t alen ;		/* power of 2 */
    debug_t debug ;
} EQUEUE_T ;

static void
METHOD(expand)(EQUEUE_T *q) {
    ITEM_T *oarr = q->arr ;
    int oalen = q->alen ;
    int ohead = q->head ;
    ofs_t i ;

    q->head = 0 ;
    q->alen = 2 * q->alen ;
    q->arr = sys_alloc(sizeof(*q->arr) * q->alen) ;

    memset(q->arr, 0, sizeof(*q->arr) * q->alen) ;
    
    /* This could be done with 2 block copies.
     */
    for (i=0;i<q->len;i++) {
	q->arr[i] = oarr[(ohead + i) & (oalen - 1)] ;
    }
    sys_free(oarr) ;
}

static inline
void METHOD(create)(debug_t debug, EQUEUE_T *q) {
    q->debug = debug ;
    q->alen = 1<<3 ;		/* must be a power of 2 */
    q->head = 0 ;
    q->len = 0 ;
    q->arr = sys_alloc(sizeof(*q->arr) * q->alen) ;
    memset(q->arr, 0, sizeof(*q->arr) * q->alen) ;
}

static inline
bool_t METHOD(empty)(EQUEUE_T *q) {
    return !q->len ;
}

static inline
len_t METHOD(length)(EQUEUE_T *q) {
    return q->len ;
}

static inline
ITEM_T *METHOD(add)(EQUEUE_T *q) {
    ITEM_T *ret ;
    if (q->len >= q->alen) {
	METHOD(expand)(q) ;
    }
    ret = &q->arr[(q->head + q->len) & (q->alen - 1)] ;
    q->len ++ ;
    return ret ;
}

static inline
ITEM_T *METHOD(take)(EQUEUE_T *q) {
    ITEM_T *ret ;
    assert(q->len > 0) ;
    ret = &q->arr[q->head] ;
    q->head = (q->head + 1) & (q->alen - 1) ;
    q->len -- ;
    return ret ;
}

static inline
void METHOD(clear)(EQUEUE_T *q) {
    ofs_t i ;
    for (i=0;i<q->len;i++) {
	memset(&q->arr[(q->head + i) & (q->alen - 1)], 0, sizeof(*q->arr)) ;
    }
    q->head = 0 ;
    q->len = 0 ;
}

static inline
void METHOD(free)(EQUEUE_T *q) {
    sys_free(q->arr) ;
}

static inline
int METHOD(usage)(EQUEUE_T *q) {
    return q->alen ;
}

static inline
void METHOD(transfer)(EQUEUE_T *dst, EQUEUE_T *src) {
    ITEM_T *dst_item ;
    ITEM_T *src_item ;
    while (!METHOD(empty)(src)) {
	src_item = METHOD(take)(src) ;
	dst_item = METHOD(add)(dst) ;
	*dst_item = *src_item ;
    }
}

#undef METHOD_HELP2
#undef METHOD_HELP
#undef METHOD
#undef TYPE_HELP2
#undef TYPE_HELP
#undef HASH_T
