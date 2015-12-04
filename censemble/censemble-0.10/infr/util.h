/**************************************************************/
/* UTIL.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef UTIL_H
#define UTIL_H

#include "infr/trans.h"
#include "infr/sys.h"
#include "infr/array.h"

#ifndef NULL
#define NULL 0
#endif /* NULL */

#define eprintf sys_eprintf

#define record_create(type, var) ((type)sys_alloc(sizeof(*var)))
#define record_free(rec) sys_free(rec)
#define record_clear(rec) memset(rec, 0, sizeof(*rec))

len_t string_length(string_t) ;
string_t string_copy(string_t) ;
void string_free(string_t) ;
void string_array_free(string_array_t, len_t) ;
bool_t string_eq(string_t, string_t) ;
int string_cmp(string_t, string_t) ;

string_t util_string_concat(string_t, string_array_t, len_t) ;
string_array_t util_string_split(string_t s, string_t w, len_t *) ;
string_t hex_of_bin(cbuf_t buf, len_t len) ;

void md5(cbuf_t buf, len_t len, md5_t*) ;
bool_t md5_eq(md5_t *, md5_t *) ;
string_t string_of_md5(md5_t *) ;

string_t bool_to_string(bool_t b) ; /* doesn't allocate */
string_t seqno_to_string(seqno_t s) ; /* allocates */
string_t rank_to_string(rank_t s) ; /* allocates */

static inline
int int_min(int i0, int i1) {
    return (i0 < i1) ? i0 : i1 ;
}

static inline
int int_max(int i0, int i1) {
    return (i0 > i1) ? i0 : i1 ;
}

static inline
seqno_t seqno_min(seqno_t i0, seqno_t i1) {
    return (i0 < i1) ? i0 : i1 ;
}

static inline
seqno_t seqno_max(seqno_t i0, seqno_t i1) {
    return (i0 > i1) ? i0 : i1 ;
}

static inline
ltime_t ltime_max(ltime_t i0, ltime_t i1) {
    return (i0 > i1) ? i0 : i1 ;
}

static inline
bool_t inet_eq(inet_t i0, inet_t i1) {
    return i0.int_s == i1.int_s ;
}

static inline
int inet_cmp(inet_t i0, inet_t i1) {
    return memcmp(i0.char_s, i1.char_s, sizeof(i0.char_s)) ;
}

static inline
buf_t buf_ofs(buf_t buf, ofs_t ofs) {
    return (buf_t)((char*)buf + ofs) ;
}

static inline
cbuf_t cbuf_ofs(cbuf_t buf, ofs_t ofs) {
    return (cbuf_t)((const char*)buf + ofs) ;
}

static inline
bool_t is_aligned(uint64_t v, int pow) {
    uint64_t m = (1ULL << pow) - 1 ;
    return (v & m) == 0 ;
}

static inline
bool_t aligned4(ofs_t ofs) {
    return is_aligned(ofs, 2) ;
}

static inline
ofs_t ceil4(ofs_t ofs) {
    return (ofs + 3) & ~(ofs_t)0x3 ;
}

static inline
ofs_t floor4(ofs_t ofs) {
    return ofs & ~(ofs_t)0x3 ;
}

#endif /* UTIL_H */
