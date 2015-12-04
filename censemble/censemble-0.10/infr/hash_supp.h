/**************************************************************/
/* HASH_SUPP.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"

#define METHOD_HELP2(class,func) class ## _hashtbl_ ## func
#define METHOD_HELP(class,func) METHOD_HELP2(class,func)
#define METHOD(func) METHOD_HELP(CLASS_NAME,func)
#define TYPE_HELP2(class) class ## _hashtbl_t
#define TYPE_HELP(class) TYPE_HELP2(class)
#define HASH_T TYPE_HELP(CLASS_NAME)
#define WRAP_HELP2(class) class ## _wrapper_t
#define WRAP_HELP(class) WRAP_HELP2(class)
#define WRAP_T WRAP_HELP(CLASS_NAME)

typedef struct WRAP_T {
    ITEM_T item ;
    struct WRAP_T *next ;
} WRAP_T ;

typedef struct {
    WRAP_T **arr ;
    len_t nitems ;
    len_t len ;
    debug_t debug ;
    struct {
	ofs_t ofs ;
	WRAP_T *w ;
    } iter ;
} HASH_T ;

uint32_t METHOD(hash)(const KEY_T *) ;
uint32_t METHOD(rehash)(const ITEM_T *) ;
bool_t METHOD(equal)(const KEY_T *, ITEM_T *) ;

#if 0
static void
METHOD(expand)(HASH_T *q) {
    WRAP_T **oarr = q->arr ;
    len_t olen = q->len ;
    ofs_t i ;

    q->len = 2 * q->len ;
    q->arr = sys_alloc(sizeof(*q->arr) * q->len) ;

    memset(q->arr, 0, sizeof(*q->arr) * q->len) ;

    for (i=0;i<olen;i++) {
	WRAP_T *w ;
	WRAP_T *next ;
	uint32_t hash ;
	for (w = oarr[i] ; w ; w = next) {
	    next = w->next ;
	    hash = METHOD(rehash)(&w->item) % q->len ;
	    w->next = q->arr[hash] ;
	    q->arr[hash] = w ;
	}
    }
    
    sys_free(oarr) ;
}
#endif

static inline
void METHOD(create)(debug_t debug, HASH_T *q, int pow2) {
    q->debug = debug ;
    q->nitems = 0 ;
    q->len = 1<<pow2 ;		/* must be a power of 2 */
    q->arr = sys_alloc(sizeof(*q->arr) * q->len) ;
    memset(q->arr, 0, sizeof(*q->arr) * q->len) ;
}

static inline
ITEM_T *METHOD(find)(HASH_T *q, const KEY_T *key) {
    WRAP_T *w ;
    for (w = q->arr[METHOD(hash)(key) % q->len] ;
	 w ;
	 w = w->next) {
	if (METHOD(equal)(key, &w->item)) {
	    break ;
	}
    }
    if (!w) {
	return NULL ;
    }
    return &w->item ;
}

static inline
void METHOD(remove_no_free)(HASH_T *q, const KEY_T *key, WRAP_T **space) {
    WRAP_T **w ;
    WRAP_T *found ;
#ifndef NDEBUG
    {
	/* Check that the key is not in the item itself.
	 */
	ITEM_T *item ;
	item = METHOD(find)(q, key) ;
	assert(item) ;
	assert((long)key + sizeof(*key) <= (long)item ||
	       (long)key >= (long)item + sizeof(*item)) ;
    }
#endif
    for (w = &q->arr[METHOD(hash)(key) % q->len] ;
	 *w ;
	 w = &(*w)->next) {
	if (METHOD(equal)(key, &(*w)->item)) {
	    break ;
	}
    }
    assert(*w) ;
    found = *w ;
    *w = found->next ;
    *space = found ;
    assert(q->nitems >= 1) ;
    q->nitems -- ;
    assert(!METHOD(find)(q, key)) ;
}

static inline
void METHOD(remove)(HASH_T *q, const KEY_T *key) {
    WRAP_T *space ;
    METHOD(remove_no_free)(q, key, &space) ;
    assert(space) ;
    sys_free(space) ;
}
    
static inline
ITEM_T *METHOD(add_no_alloc)(HASH_T *q, const KEY_T *key, WRAP_T *w) {
    uint32_t hash ;
    assert(!METHOD(find)(q, key)) ;
    hash = METHOD(hash)(key) % q->len ;
    w->next = q->arr[hash] ;
    q->arr[hash] = w ;
    q->nitems ++ ;
    return &w->item ;
}

static inline
ITEM_T *METHOD(add)(HASH_T *q, const KEY_T *key) {
    WRAP_T *w ;
    w = sys_alloc(sizeof(*w)) ;
    return METHOD(add_no_alloc)(q, key, w) ;
}

static inline
void METHOD(clear)(HASH_T *q) {
    ofs_t i ;
    for (i=0;i<q->len;i++) {
	WRAP_T *w ;
	WRAP_T *next ;
	for (w = q->arr[i] ; w ; w = next) {
	    next = w->next ;
	    sys_free(w) ;
	}
	q->arr[i] = NULL ;
    }
    q->nitems = 0 ;
}

static inline
void METHOD(free_no_free)(HASH_T *q) {
    sys_free(q->arr) ;
    memset(q, 0, sizeof(*q)) ;
}

static inline
void METHOD(free)(HASH_T *q) {
    METHOD(clear)(q) ;
    METHOD(free_no_free)(q) ;
}

static inline
int METHOD(size)(HASH_T *q) {
#if 0
    ofs_t i ;
    len_t count = 0 ;
    for (i=0;i<q->len;i++) {
	WRAP_T *w ;
	for (w = q->arr[i] ; w ; w = w->next) {
	    count ++ ;
	}
    }
    assert(count == q->nitems) ;
#endif
    return q->nitems ;
}

static inline
ITEM_T *METHOD(iter_next)(HASH_T *q) {
    assert(q->iter.w) ;
    q->iter.w = q->iter.w->next ;
    if (!q->iter.w) {
	for ( q->iter.ofs++ ; q->iter.ofs < q->len ;  q->iter.ofs++ ) {
	    if (q->arr[q->iter.ofs]) {
		q->iter.w = q->arr[q->iter.ofs] ;
		break ;
	    }
	}
    }
    if (!q->iter.w) {
	return NULL ;
    }
    return &q->iter.w->item ;
}

static inline
ITEM_T *METHOD(iter_init)(HASH_T *q) { 
    q->iter.w = NULL ;
    for ( q->iter.ofs = 0 ; q->iter.ofs < q->len ;  q->iter.ofs++ ) {
	if (q->arr[q->iter.ofs]) {
	    q->iter.w = q->arr[q->iter.ofs] ;
	    break ;
	}
    }
    if (!q->iter.w) {
	return NULL ;
    }
    return &q->iter.w->item ;
}

#if 0
static inline
array_of(ITEM) METHOD(to_array)(HASH_T *q) {
    ofs_t i ;
    ofs_t ofs ;
    len_t len ;
    array_of(ITEM) a ;
    len = METHOD(size)(q) ;
    a = ITEM # _array_create(len) ;
    ofs = 0 ;
    for (i=0;i<q->len;i++) {
	WRAP_T *w ;
	for (w = q->arr[i] ; w ; w = w->next) {
	    array_set(a, ofs, w->item) ;
	    ofs ++ ;
	}
    }
    return a ;
}
#endif

#undef METHOD_HELP2
#undef METHOD_HELP
#undef METHOD
#undef TYPE_HELP2
#undef TYPE_HELP
#undef WRAP_HELP2
#undef WRAP_HELP
#undef HASH_T

#if 0
static inline
array_of(client_state) client_hashtbl_to_array(client_hashtbl_t *q) {
    ofs_t i ;
    ofs_t ofs ;
    len_t len ;
    array_of(client_state) a ;
    len = client_hashtbl_size(q) ;
    a = client_state_array_create(len) ;
    ofs = 0 ;
    for (i=0;i<q->len;i++) {
	client_wrapper_t *w ;
	for (w = q->arr[i] ; w ; w = w->next) {
	    array_set(a, ofs, w->item) ;
	    ofs ++ ;
	}
    }
    return a ;
}
#endif
