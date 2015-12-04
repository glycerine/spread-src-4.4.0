/**************************************************************/
/* ARRAY.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/sys.h"
#include "infr/trans.h"
#include "infr/array.h"

void bool_array_copy_into(bool_array_t dst, const_bool_array_t src, len_t len) {
    ofs_t ofs ;
    assert(dst) ;
    assert_array_length(dst, len) ;
    for (ofs=0;ofs<len;ofs++) {
	array_set(dst, ofs, array_get(src, ofs)) ;
    }
}

void seqno_array_copy_into(seqno_array_t dst, const_seqno_array_t src, len_t len) {
    ofs_t ofs ;
    assert(dst) ;
    assert_array_length(dst, len) ;
    for (ofs=0;ofs<len;ofs++) {
	array_set(dst, ofs, array_get(src, ofs)) ;
    }
}

void ltime_array_copy_into(ltime_array_t dst, const_ltime_array_t src, len_t len) {
    ofs_t ofs ;
    assert(dst) ;
    assert_array_length(dst, len) ;
    for (ofs=0;ofs<len;ofs++) {
	array_set(dst, ofs, array_get(src, ofs)) ;
    }
}

bool_array_t bool_array_copy(bool_array_t a0, len_t len) {
    ofs_t ofs ;
    bool_array_t a1 = bool_array_create(len) ;
    for (ofs=0;ofs<len;ofs++) {
	array_set(a1, ofs, array_get(a0, ofs)) ;
    }
    return a1 ;
}

void array_free(const void *a) {
    sys_free(a) ;
}

bool_array_t bool_array_create_init(len_t len, bool_t b) {
    bool_array_t ret ;
    ofs_t i ;
    
    ret = bool_array_create(len) ;
    for (i=0;i<len;i++) {
	array_set(ret, i, b) ;
    }
    return ret ;
}

ltime_array_t ltime_array_create_init(len_t len, ltime_t b) {
    ltime_array_t ret ;
    ofs_t i ;
    
    ret = ltime_array_create(len) ;
    for (i=0;i<len;i++) {
	array_set(ret, i, b) ;
    }
    return ret ;
}

bool_array_t bool_array_setone(len_t len, ofs_t ofs) {
    bool_array_t ret ;
    assert(ofs < len) ;
    ret = bool_array_create_init(len, FALSE) ;
    array_set(ret, ofs, TRUE) ;
    return ret ;
}

int_array_t int_array_create_init(len_t len, int v) {
    int_array_t ret ;
    ofs_t i ;

    ret = int_array_create(len) ;
    for (i=0;i<len;i++) {
	array_set(ret, i, v) ;
    }
    return ret ;
}

seqno_array_t seqno_array_create_init(len_t len, seqno_t v) {
    seqno_array_t ret ;
    ofs_t i ;

    ret = seqno_array_create(len) ;
    for (i=0;i<len;i++) {
	array_set(ret, i, v) ;
    }
    return ret ;
}

void int_array_incr(int_array_t a, ofs_t i) {
    assert_array_bounds(a, i) ;
    array_set(a, i, array_get(a, i) + 1) ;
}

void seqno_array_incr(seqno_array_t a, ofs_t i) {
    assert_array_bounds(a, i) ;
    array_set(a, i, array_get(a, i) + 1) ;
}

void int_array_add(int_array_t a, ofs_t i, int v) {
    assert_array_bounds(a, i) ;
    array_set(a, i, array_get(a, i) + v) ;
}

void int_array_sub(int_array_t a, ofs_t i, int v) {
    assert_array_bounds(a, i) ;
    array_set(a, i, array_get(a, i) - v) ;
}

bool_t bool_array_super(
        const_bool_array_t a0,
	const_bool_array_t a1,
	len_t len
) {
    ofs_t i ;
    assert_array_length(a0, len) ;
    assert_array_length(a1, len) ;
    for (i=0;i<len;i++) {
	if (!array_get(a0, i) && array_get(a1, i)) return FALSE ;
    }
    return TRUE ;
}

ofs_t bool_array_min_false(const_bool_array_t a, len_t len) {
    ofs_t i ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (!array_get(a, i)) break ;
    }
    return i ;
}

bool_t bool_array_exists(const_bool_array_t a, len_t len) {
    ofs_t i ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (array_get(a, i)) break ;
    }
    return (i < len) ;
}

bool_t bool_array_forall(const_bool_array_t a, len_t len) {
    ofs_t i ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (!array_get(a, i)) break ;
    }
    return (i == len) ;
}

ltime_t ltime_array_max(const_ltime_array_t a, len_t len) {
    ofs_t i ;
    ltime_t ltime = LTIME_INVALID ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (array_get(a, i) <= ltime) continue ;
	ltime = array_get(a, i) ;
    }
    return ltime ;
}

seqno_t seqno_array_min(const_seqno_array_t a, len_t len) {
    ofs_t i ;
    seqno_t seqno = -1 ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (array_get(a, i) >= seqno) continue ;
	seqno = array_get(a, i) ;
    }
    return seqno ;
}

bool_array_t bool_array_map_or(
        const_bool_array_t a0,
	const_bool_array_t a1,
	len_t len
) {
    ofs_t i ;
    bool_array_t a2 = bool_array_create(len) ;
    assert_array_length(a0, len) ;
    assert_array_length(a1, len) ;
    for (i=0;i<len;i++) {
	array_set(a2, i, array_get(a0, i) || array_get(a1, i)) ;
    }
    return a2 ;
}

#include "infr/array_supp.h"
ARRAY_CREATE(int)
ARRAY_CREATE(bool)
ARRAY_CREATE(inet)
ARRAY_CREATE(seqno)
ARRAY_CREATE(ltime)
ARRAY_CREATE(string)
ARRAY_TO_STRING(bool)
ARRAY_TO_STRING(seqno)
ARRAY_COPY(int)
ARRAY_COPY(seqno)
ARRAY_COPY(ltime)
ARRAY_FREE(string)
MATRIX_CREATE(bool)
MATRIX_CREATE(seqno)
