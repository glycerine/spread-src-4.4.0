/**************************************************************/
/* IQ.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/iq.h"
#include "infr/util.h"
#include "infr/trans.h"
#include "infr/layer.h"
#include "infr/config.h"

static string_t name UNUSED() = "IQ" ;

typedef struct {
    iq_item_t msg ;
} item_t ;

struct iq_t {
    seqno_t lo ;
    seqno_t hi ;
    seqno_t read ;
    item_t *arr ;
    len_t alen ;
    unsigned int mask ;
    debug_t name ;
    iq_free_t free ;
} ;

#ifndef MINIMIZE_CODE
string_t iq_string_of_status(iq_status_t s) {
    string_t ret ;
    switch(s) {
    case IQ_DATA:  ret = "data" ;  break ;
    case IQ_UNSET: ret = "unset" ; break ;
    case IQ_RESET: ret = "reset" ; break ;
    OTHERWISE_ABORT() ;
    }
    return ret ;
}
#endif

static inline seqno_t maxi(iq_t iq) {
    return iq->lo + iq->alen ;
}

static inline seqno_t idx_of_abs(iq_t iq, seqno_t abs) {
    return abs & iq->mask ;
}

static int log2(unsigned i) {
    int pow ;
    assert(i >= 1) ;
    for(pow=0;i>1;pow++,i>>=1) ;
    return pow ;
}

static inline len_t iq_population(iq_t iq) {
    int count = 0 ;
    ofs_t ofs ;
    for (ofs=0;ofs<iq->alen;ofs++) {
	if (iq->arr[ofs].msg) {
	    count ++ ;
	}
    }
    return count ;
}

static inline void iq_check(iq_t iq) {
    assert(iq->alen == 1UL << log2(iq->alen)) ;
    assert(iq->alen == iq->mask + 1) ;
}

/* Grow to a larger size array.
 */
void iq_grow(iq_t iq, seqno_t nlen) {
    len_t olen = iq->alen ;
    unsigned omask = iq->mask ;
    unsigned nmask ;
    item_t *na ;
    seqno_t abs ;

    iq_check(iq) ;
    nlen = seqno_max(nlen, olen) ;
    nlen = 1 << log2(2*nlen-1) ;
    nlen = seqno_max(nlen, 16) ;
    nmask = nlen - 1 ;

#if 0
    eprintf("nlen=%d lo=%d maxi=%d nhi=%d pop=%d\n", nlen, iq->lo, maxi(iq), iq->lo + nlen,
	    iq_population(iq)) ;
#endif

    na = sys_alloc(sizeof(na[0]) * nlen) ;

    for (abs=iq->lo;abs<maxi(iq);abs++) {
	seqno_t oi = abs & omask ;
	seqno_t ni = abs & nmask ;
	na[ni] = iq->arr[oi] ;
    }

    for (abs=maxi(iq);abs<iq->lo+nlen;abs++) {
	seqno_t ni = abs & nmask ;
	na[ni].msg = NULL ;
    }
#if 0
    for (abs=iq->lo;abs<iq->lo+nlen;abs++) {
	seqno_t ni = abs & nmask ;
	assert(na[ni].ctl == UNSET ||
	       na[ni].ctl == DATA) ;
    }
#endif

    sys_free(iq->arr) ;
    iq->arr = na ;
    iq->alen = nlen ;
    iq->mask = nmask ;
    iq_check(iq) ;
#if 0
    eprintf("pop=%d nmask=0x%08x\n", iq_population(iq), iq->mask) ;
#endif
}

iq_t iq_create(debug_t debug, iq_free_t free) {
    iq_t iq = record_create(iq_t, iq) ;
    iq->lo = 0 ;
    iq->hi = 0 ;
    iq->alen = 1 ;
    iq->read = 0 ;
    iq->name = debug ;
    iq->mask = 0 ;
    iq->arr = sys_alloc(sizeof(iq->arr[0])) ;
    iq->free = free ;
    assert(iq->arr) ;
    memset(iq->arr, 0, sizeof(iq->arr[0])) ;
    iq->arr[0].msg = NULL ;
#if 0
#ifdef USE_GC
    gc_tag(iq, debug) ;
    gc_tag(iq->arr, debug) ;
#endif
#endif
    return iq ;
}

seqno_t iq_lo(iq_t iq) {
    return iq->lo ;
}

seqno_t iq_hi(iq_t iq) {
    return iq->hi ;
}

seqno_t iq_read(iq_t iq) {
    return iq->read ;
}

static inline item_t *get_unsafe(iq_t iq, seqno_t i) {
    seqno_t idx = idx_of_abs(iq, i) ;
    assert(idx >= 0) ;
    assert(idx < iq->alen) ;
    return &iq->arr[idx] ;
}

static inline void clear_item(iq_t iq, seqno_t i) {
    item_t *item = get_unsafe(iq, i) ;
    if (!item->msg) {
	return ;
    }
    iq->free(item->msg) ;
    item->msg = NULL ;
}

static void overflow(iq_t iq, seqno_t new_hi) {
    assert(new_hi >= maxi(iq)) ;
    iq_grow(iq, (new_hi - iq->lo + 1)) ;
    assert(new_hi < maxi(iq)) ;
}

void iq_set_hi(iq_t iq, seqno_t i) {
    if (i > iq->hi) {
	iq->hi = i ;
    }
}

void iq_set_lo(iq_t iq, seqno_t n) {
    seqno_t lo = iq->lo ;
    ofs_t i ;
    if (n <= lo) {
	return ;
    }
    if (n >= maxi(iq)) {
	overflow(iq, n) ;
    }
    for (i=lo;i<n;i++) {
	clear_item(iq, i) ;
    }
    iq->lo = n ;
    iq_set_hi(iq, n) ;
}

static inline void do_set(item_t *item, iq_item_t msg) {
    assert(!item->msg) ;
    assert(msg) ;
    item->msg = msg ;
}

static inline bool_t set(iq_t iq, seqno_t i, iq_item_t msg) {
    item_t *item ;
    assert(msg) ;
    if (i < iq->lo) {
	return FALSE ;
    }
    iq_set_hi(iq, i + 1) ;
    if (i >= maxi(iq)) {
	overflow(iq, i) ;
    }
    item = get_unsafe(iq, i) ;

    if (item->msg) {
	return FALSE ;
    }

    do_set(item, msg) ;
    return TRUE ;
}

/* ASSIGN: if this returns true then the slot was empty
 * and the Iovecl ref was appropriately incremented.
 */
bool_t iq_assign(iq_t iq, seqno_t seqno, iq_item_t msg) {
    return set(iq, seqno, msg) ;
}

bool_t iq_opt_insert_check(iq_t iq, seqno_t i) {
    return i == iq->read && i == iq->hi ;
}

void iq_opt_insert_doread(iq_t iq, seqno_t i, iq_item_t msg) {
    item_t *item ;
    seqno_t next ;
    assert(iq_opt_insert_check(iq, i)) ;
    assert (i >= iq->lo) ;
    next = i + 1 ;
    iq->read = next ;
    iq->hi = next ;
    if (i >= maxi(iq)) {
	overflow(iq, i) ;
    }
    item = get_unsafe(iq, i) ;
    do_set(item, msg) ;
}

bool_t iq_opt_insert_check_doread(iq_t iq, seqno_t i, iq_item_t msg) {
    if (iq_opt_insert_check(iq, i)) {
	iq_opt_insert_doread(iq, i, msg) ;
	return TRUE ;
    } else {
	return FALSE ;
    }
}

void iq_add(iq_t iq, iq_item_t msg) {
    if (!set(iq, iq->hi, msg)) {
	sys_panic(("add:sanity")) ;
    }
}

iq_item_t get_msg(iq_t iq, seqno_t i) {
    item_t *item ;
    assert(i >= iq->lo) ;
    if (i >= maxi(iq)) {
	return NULL ;
    }
    item = get_unsafe(iq, i) ;
    return item->msg ;
}

iq_status_t iq_get(iq_t iq, seqno_t i, iq_item_t *msg) {
    assert(msg) ;
    if (i < iq->lo) {
	return IQ_RESET ;
    } else if (i >= maxi(iq)) {
	return IQ_UNSET ;
    } else {
	item_t *item = get_unsafe(iq, i) ;
	if (!item->msg) {
	    return IQ_UNSET ;
	} 
	*msg = item->msg ;
	return IQ_DATA ;
    }
}

bool_t iq_get_prefix(iq_t iq, seqno_t *seqno, iq_item_t *msg) {
    seqno_t lo = iq->lo ;
    item_t *item ;

    if (!get_msg(iq, lo)) {
	return FALSE ;
    }
    item = get_unsafe(iq, lo) ;

    *msg = item->msg ;
    *seqno = lo ;

    /* Watch out!  The iq_set_lo will try to zap this
     * item.
     */
    item->msg = NULL ;

    iq_set_lo(iq, (lo + 1)) ;
    return TRUE ;
}

static inline seqno_t next_set(iq_t iq, seqno_t i) {
    for (;;i++) {
	if (i >= iq->hi ||
	    get_msg(iq, i)) {
	    return i ;
	}
    }
}
	
static bool_t hole_help(iq_t iq, seqno_t i, seqno_t *lo, seqno_t *hi) {
    seqno_t first_set = next_set(iq, i) ;
    if (first_set <= iq->lo ||
	i >= iq->hi) {
	return FALSE ;
    } 
    *lo = i ;
    *hi = first_set ;
    return TRUE ;
}

bool_t iq_hole(iq_t iq, seqno_t *lo, seqno_t *hi) {
    return hole_help(iq, iq->lo, lo, hi) ;
}

bool_t iq_read_hole(iq_t iq, seqno_t *lo, seqno_t *hi) {
    assert(iq->read >= iq->lo) ;
    return hole_help(iq, iq->read, lo, hi) ;
}

bool_t iq_read_prefix(iq_t iq, seqno_t *seqno, iq_item_t *msg) {
    seqno_t read = iq->read ;
    item_t *item ;

    if (!get_msg(iq, read)) {
	return FALSE ;
    }
    item = get_unsafe(iq, read) ;
    *msg = item->msg ;
    *seqno = read ;
    iq->read = read + 1 ;
    return TRUE ;
}

/* The opt_update functions are used where the normal
 * case insertion does not actually insert anything
 * into the buffer.
 */
bool_t iq_opt_update_old(iq_t, seqno_t) ;

/* Split the above into two pieces.
 */
bool_t iq_opt_update_check(iq_t iq, seqno_t i) {
    return (i == iq->lo && 
	    i == iq->hi) ;
}

void iq_opt_update_update(iq_t iq, seqno_t i) {
    assert(iq_opt_update_check(iq, i)) ;
    i++ ;
    iq_set_lo(iq, i) ;
    iq->hi = i ;
}

void iq_free(iq_t iq) {
    iq_set_lo(iq, iq->hi) ;
    sys_free(iq->arr) ;
    record_free(iq) ;
}

void iq_clear_unread(iq_t iq) {
    seqno_t i ;
    if (iq->hi >= maxi(iq)) {
	overflow(iq, iq->hi) ;
    }
    for (i=iq->read;i<=iq->hi;i++) {
	clear_item(iq, i) ;
    }
    iq->hi = iq->read ;
}

#if 0
#define IQ_CHECK_SIZE 1000
void iq_test(void) {
    int i ;
    
    for(i=1;i<10000000;i++) {
	iq_t iq = iq_create(name) ;
	if ((i % 1) == 0) {
	    eprintf("IQ:test(%d)\n", i) ;
	}

	while (1) {
	    switch(util_random(2)) {
	    case 0: {
		marsh_t marsh = marsh_create(NULL) ;
		marsh_buf_t buf = marsh_to_buf(name, marsh) ;
		marsh_free(marsh) ;
		if (!iq_assign(iq, util_random(IQ_CHECK_SIZE), buf, iov)) {
		    iovec_free(iov) ;
		    marsh_buf_free(buf) ;
		}
	    } break ;
	    case 1: {
		marsh_buf_t buf ;
		seqno_t seqno ;
		iovec_t iov ;
		if (!iq_get_prefix(iq, &seqno, &buf, &iov)) {
		    break ;
		}
		//eprintf("seqno=%d\n", seqno) ;
		iovec_free(iov) ;
		marsh_buf_free(buf) ;
		if (seqno >= IQ_CHECK_SIZE - 1) {
		    goto out ;
		}
	    } break ;
	    case 2: {
		marsh_t marsh = marsh_create(NULL) ;
		marsh_buf_t buf = marsh_to_buf(name, marsh) ;
		iovec_t iov = iovec_empty(name) ;
		marsh_free(marsh) ;
		iq_add(iq, buf, iov) ;
	    } break ;
	    case 3: {
		marsh_t marsh = marsh_create(NULL) ;
		marsh_buf_t buf = marsh_to_buf(name, marsh) ;
		iovec_t iov = iovec_empty(name) ;
		marsh_free(marsh) ;
		if (!iq_opt_insert_check_doread(iq, util_random(IQ_CHECK_SIZE), buf, iov)) {
		    iovec_free(iov) ;
		    marsh_buf_free(buf) ;
		}
	    } break ;
	    }
	}
    out:
	iq_free(iq) ;
    }
}
#endif

#include "infr/array_supp.h"
ARRAY_CREATE(iq)
