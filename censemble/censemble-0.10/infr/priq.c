/**************************************************************/
/* PRIQ.C: heap-based priority queues */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/etime.h"
#include "infr/util.h"
#include "infr/priq.h"

typedef struct {
    priq_key_t key ;
    priq_data_t data ;
} item_t ;

struct priq_t {
    item_t *table ;
    int nitems ;
    int table_len ;
} ;

static inline
bool_t priq_ge(etime_t a, etime_t b) {
    return time_ge(a,b) ;
}

#ifndef MINIMIZE_CODE
void priq_dump(priq_t pq) {
    int i ;
    eprintf("PRIQ") ;
    for (i=1;i<pq->nitems+1;i++) {
	eprintf(":%s", time_to_string(pq->table[i].key)) ;
    }
    eprintf("\n") ;
}
#endif

priq_t priq_create(void) {
    priq_t pq = record_create(priq_t, pq) ;
    pq->nitems = 0 ;
    pq->table_len = 16 ;
    pq->table = sys_alloc(sizeof(pq->table[0]) * pq->table_len) ;
    return pq ;
}
    
inline int priq_length(priq_t pq) {
    return pq->nitems ;
}

inline bool_t priq_empty(priq_t pq) {
    return priq_length(pq) == 0 ; 
}

inline priq_key_t priq_min(priq_t pq) {
    assert(!priq_empty(pq)) ;
    return pq->table[1].key ;
}

static inline item_t *get(priq_t pq, int i) {
    assert(i != 0) ;
    assert(i <= pq->nitems) ;
    assert(i < pq->table_len) ;
    return &pq->table[i] ;
}

static inline void unset(priq_t pq, int i) {
#ifdef USE_GC
    item_t *item = get(pq, i) ;
    //item->key = NULL ;
    item->data = NULL ;
#endif
}
    
static inline void assign(priq_t pq, int i, priq_key_t k, priq_data_t d) {
    item_t *item = get(pq, i) ;
    item->key = k ;
    item->data = d ;
}
    
static inline void set(priq_t pq, int i0, int i1) {
    item_t *item0 = get(pq, i0) ;
    item_t *item1 = get(pq, i1) ;
    *item0 = *item1 ; 
}

static inline priq_key_t get_key(priq_t pq, int i) {
    return get(pq, i)->key ;
}

/* Grow priority queue array by at least one element.
 */
static void grow(priq_t pq) {
    item_t *old_table = pq->table ; 
    int i ;
    /*eprintf("PRIQ:length=%d\n", pq->table_len) ;*/
    /*priq_dump(pq) ;*/
    assert(pq->table_len > 0) ;
    pq->table_len <<= 1 ;
    pq->table = sys_alloc(sizeof(pq->table[0]) * pq->table_len) ;
    assert(pq->table) ;
    memset(pq->table, 0, sizeof(pq->table[0]) * pq->table_len) ;
    for (i=1;i<=pq->nitems;i++) {
	pq->table[i] = old_table[i] ;
    }
    sys_free(old_table) ;
}

void priq_free(priq_t pq) {
    assert(pq->nitems == 0) ;
    sys_free(pq->table) ;
    record_free(pq) ;
}

void priq_add(priq_t pq, priq_key_t key, priq_data_t data) {
    int i ;
    int j ;

    if (pq->nitems + 1 >= pq->table_len) {
	grow(pq) ;
    }
    pq->nitems ++ ;

    for (i=pq->nitems;i>0;i=j) {
	j = i >> 1 ;
	if (j <= 0 || 
	    priq_ge(key, get_key(pq, j))) {
	    break ;
	}
	set(pq, i, j) ;
	i = j ;
    }

    assign(pq, i, key, data) ;

    /*eprintf("ADD:%s\n", time_to_string(key)) ;*/
    /*priq_dump(pq) ;*/
}

static void take(priq_t pq) {
    priq_key_t last_key ;
    int next ;
    int i ;
    assert(!priq_empty(pq)) ;
    last_key = get_key(pq, pq->nitems) ;
    for (i=1;;i=next) {
	int left = i << 1 ;
	int right = left + 1 ;
	priq_key_t next_key ;
	priq_key_t left_key ;
	priq_key_t right_key ;
	assert(i <= pq->nitems) ;

	if (left >= pq->nitems) {
	    break ;
	}

	left_key = get_key(pq, left) ;

	if (right >= pq->nitems) {
	    next = left ;
	    next_key = left_key ;
	} else {
	    right_key = get_key(pq, right) ;
	    if (priq_ge(right_key, left_key)) {
		next = left ;
		next_key = left_key ;
	    } else {
		next = right ;
		next_key = right_key ;
	    }
	}

	if (priq_ge(next_key, last_key)) {
	    break ;
	}
	set(pq, i, next) ;
    }
    set(pq, i, pq->nitems) ;
    unset(pq, pq->nitems) ;
    pq->nitems -- ;
}

void priq_get(priq_t pq, priq_key_t *key, priq_data_t *data) {
    item_t *item ;
    assert(!priq_empty(pq)) ;
    item = get(pq, 1) ;
    if (key) {
	*key = item->key ;
    }
    assert(data) ;
    *data = item->data ;
    take(pq) ;
    /*eprintf("GET:%s\n", time_to_string(*key)) ;*/
    /*priq_dump(pq) ;*/
}

bool_t priq_get_upto(priq_t pq, priq_key_t upto, priq_key_t *key, priq_data_t *data) {
    item_t *item ;
    if (priq_empty(pq) ||
	!priq_ge(upto, priq_min(pq))) {
	return FALSE ;
    }
    item = get(pq, 1) ;
    assert(item) ;
    if (key) {
	*key = item->key ;
    }
    assert(data) ;
    *data = item->data ;
    take(pq) ;
    return TRUE ;
}

/**************************************************************/
/* Stuff for debugging. 
 */
#ifndef NDEBUG
static void check (priq_t pq) UNUSED() ;
static void check (priq_t pq) {
    int i ;

#if 0
    for (i=pq->nitems+1;i<pq->table_len;i++) {
	if (pq->table[i].key) {
	    //eprintf "size=%d, i=%d\n" pq.nitems i ;
	    sys_panic("unused got modified") ;
	}
    }
#endif

    for (i=2;i<=pq->nitems;i++) {
	int j = i >> 1 ;
	if (!priq_ge(get_key(pq, i), get_key(pq, j))) {
	    /*
	    eprintf("predicate broke: %d,%d > %d,%d\n"
		    j (get_key pq j)
		    i (get_key pq i)) ;
	    print pq ;
	    */
	    sys_panic(("predicate broke")) ;
	}
    }
}

#define ALEN 1023

void priq_test(void) {
    priq_t pq = priq_create() ;
    etime_t a[ALEN] ;
    priq_data_t data ;
    int n ;

    for (n=0;n<1023;n++) {
	int i ;
	int j ;
	int count ;

	count = sys_random((n + 10) * 11) ;

	for(i=0;i<n;i++) {
	    a[i] = time_of_secs(i + 1) ;
	}

	/* Scramble things.
	 */
	for(i=1;i<count;i++) {
	    if (n == 0) {
		continue ;
	    }
	    j = sys_random(n) ;
	    if (time_eq(a[j], time_zero())) {
		priq_get(pq, &a[j], &data) ;
		assert(data == NULL) ;
	    } else {
		priq_add(pq, a[j], NULL) ;
		a[j] = time_of_secs(0) ;
	    }
	}

	/* Insert them all.
	 */
	while (priq_length(pq) < n) {
	    j = sys_random(n) ;
	    if (time_eq(a[j], time_zero())) {
		/* Nothing.
		 */
	    } else {
		priq_add(pq, a[j], NULL) ;
		a[j] = time_of_secs(0) ;
	    }
	}

	/* Remove them and check they are in order.
	 */
	for (i=0;i<n;i++) {
	    priq_get(pq, &a[i], &data) ;
	    assert(data == NULL) ;
	    /*eprintf("%03d time=%s\n", i, time_to_string(a[i])) ;*/
	    assert(time_eq(a[i], time_of_secs(i + 1))); 
	}

        /*eprintf("OK len=%d\n", n) ;*/
    }
    /*eprintf("OK\n") ;*/
    sys_exit(0) ; 
}
#endif

int priq_usage(priq_t pq) {
    return pq->table_len ;
}
