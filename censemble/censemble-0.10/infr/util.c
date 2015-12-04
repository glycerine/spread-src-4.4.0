/**************************************************************/
/* UTIL.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#ifndef __KERNEL__
#include <string.h>
#endif
#include "infr/util.h"
#include "infr/sys.h"
#include "infr/config.h"

inline
int string_cmp(string_t s0, string_t s1) {
    if (s0 == s1) {
	return 0 ;
    }
    return strcmp(s0, s1) ;
}

bool_t string_eq(string_t s0, string_t s1) {
    return !string_cmp(s0, s1) ;
}

len_t string_length(string_t s) {
    return strlen(s) ;
}

string_t string_copy(string_t s) {
    mut_string_t t ;
    len_t len = string_length(s) + 1 ;
    t = sys_alloc(len) ;
    memcpy(t, s, len) ;
    return t ;
}

void string_free(string_t s) {
    sys_free(s) ;
}

#ifndef DBD_KERNEL
string_t util_string_concat(string_t sep, string_array_t a, len_t alen) {
    int sep_len = strlen(sep) ;
    len_t len ;
    mut_string_t s ;
    ofs_t i ;
    int o ;
    assert_array_length(a, alen) ;
    len = sep_len * (alen + 1) ;
    for (i=0;i<alen;i++) {
	len += strlen(array_get(a, i)) ;
    }
    s = sys_alloc(len + 1) ;
    o = 0 ;
    strcpy(s + o, sep) ;
    o += sep_len ;
    for (i=0;i<alen;i++) {
	string_t as = array_get(a, i) ;
	strcpy(s + o, as) ;
	o += strlen(as) ;
	strcpy(s + o, sep) ;
	o += sep_len ;
    }
    return s ;
}

#ifndef MINIMIZE_CODE
string_t bool_to_string(bool_t b) {
    assert(b == FALSE || b == TRUE) ;
    if (b) {
	return "T" ;
    } else {
	return "f" ;
    }
}

string_t seqno_to_string(seqno_t s) {
    return sys_sprintf("%llu", s) ;
}

string_t rank_to_string(rank_t s) {
    return sys_sprintf("%u", s) ;
}
#endif

static bool_t util_strtok(string_t s, string_t w, ofs_t start, ofs_t *ofs, len_t *len) {
    ofs_t i ;
    assert(start <= strlen(s)) ;
    for (i=start;s[i] && strchr(w, s[i]);i++) ;
    if (!s[i]) {
	return FALSE ;
    }

    *ofs = i ;
    for (;s[i] && !strchr(w, s[i]);i++) ;
    *len = i - *ofs ;
    assert(*len > 0) ;
    return TRUE ;
}

string_array_t util_string_split(string_t s, string_t w, len_t *len_ret) {
    ofs_t i ;
    len_t n ;
    ofs_t ofs ;
    len_t len ;
    string_array_t a ;
    
    ofs = 0 ;
    for (i=0;;i++) {
	if (!util_strtok(s, w, ofs, &ofs, &len)) {
	    break ;
	}
	ofs += len ;
    }
    n = i ;

    a = string_array_create(n) ;
    
    ofs = 0 ;
    for (i=0;;i++) {
	mut_string_t s_i ;
	if (!util_strtok(s, w, ofs, &ofs, &len)) {
	    break ;
	}
	assert(i < n) ;
	s_i = sys_alloc(len + 1) ;
	strncpy(s_i, s + ofs, len) ;
	s_i[len] = '\0' ;
	array_set(a, i, s_i) ;
	ofs += len ;
    }
    assert(i == n) ;
    assert(len_ret) ;
    *len_ret = n ;
    return a ;
}

bool_t seqno_array_eq(const_seqno_array_t a0, const_seqno_array_t a1, len_t len) {
    ofs_t ofs ;
    assert_array_length(a0, len) ;
    assert_array_length(a1, len) ;
    for (ofs=0;ofs<len;ofs++) {
	if (array_get(a0, ofs) != array_get(a1, ofs)) {
	    break ;
	}
    }
    return (ofs == len) ;
}

static char char_of_hex(int i) {
    assert(i >= 0 && i < 16) ;
    if (i >= 0 && i < 10) {
	return '0' + i ;
    } else {
	return 'a' + (i - 10) ;
    }
}

string_t hex_of_bin(cbuf_t buf, len_t len) {
    mut_string_t s = sys_alloc(2 * len + 1) ;
    ofs_t i ;
    for (i=0;i<len;i++) {
	uint8_t b = ((uint8_t*)buf)[i] ;
	s[2*i + 0] = char_of_hex(b / 0x10) ;
	s[2*i + 1] = char_of_hex(b & 0x0f) ;
    }
    s[2*i] = '\0' ;
    return s ;
}

#ifndef __KERNEL__
#include "infr/md5.h"

#if 0
void string_to_md5(string_t s, md5_t *m) {
    MD5Context ctx ;
    MD5Init(&ctx) ;
    MD5Update(&ctx, s, string_length(s)) ;
    MD5Final(m->raw, &ctx) ;
}
#endif

void md5(cbuf_t buf, len_t len, md5_t *m) {
    MD5Context ctx ;
    MD5Init(&ctx) ;
    MD5Update(&ctx, buf, len) ;
    MD5Final(m->raw, &ctx) ;
}

bool_t md5_eq(md5_t *m0, md5_t *m1) {
    return !memcmp(m0, m1, sizeof(*m0)) ;
}

string_t string_of_md5(md5_t *m) {
    return hex_of_bin(m, sizeof(*m)) ;
}
#endif

#if 0
static
void qsort_swap(void *base, size_t nmemb, size_t size, size_t idx0, size_t idx1) {
    int i ;
    char tmp ;
    for (i=0;i<size;i++) {
	tmp = *(char*)(base + size * idx0 + i) ;
	*(char*)(base + size * idx0 + i) =
	    *(char*)(base + size * idx1 + i) ;
	*(char*)(base + size * idx1 + i) = tmp ;
    }
}

static
void qsort_help(void *base, size_t nmemb, size_t size,
		int (*compar)(const void *, const void *),
		size_t ofs, size_t len) {
    int pivot ;
    size_t hi ;
    size_t lo ;
    int i ;
    assert(ofs >= 0) ;
    assert(len >= 0) ;
    assert(ofs + len <= nmemb) ;
    if (len <= 1) {
	return ;
    }
    pivot = ofs ;
    hi = ofs + len - 1 ;
    lo = ofs + 1 ;
    eprintf("qsort: ofs=%03d len=%03d\n", ofs, len) ;
    
    while (hi > lo) {
	while (hi > lo && compar(base + size * pivot, base + size * hi) <= 0) {
	    hi -- ;
	}
	while (hi > lo && compar(base + size * pivot, base + size * lo) >= 0) {
	    lo ++ ;
	}
	if (hi > lo) {
	    qsort_swap(base, nmemb, size, lo, hi) ;
	}
    }
    assert(hi == lo) ;
    eprintf("qsort:%03d %03d %03d lo=%d hi=%d\n", ofs, pivot, ofs + len, lo, hi) ;

    if (compar(base + size * hi, base + size * 0) <= 0) {
	pivot = hi ;
	qsort_swap(base, nmemb, size, 0, pivot) ;
    } else {
	pivot = hi - 1 ;
	qsort_swap(base, nmemb, size, 0, pivot) ;
    }

    for (i=ofs;i<ofs+len;i++) {
	eprintf("%03d:%03d\n", i, ((unsigned*)base)[i]) ;
    }

    for (i=ofs;i<pivot;i++) {
	assert(compar(base + size * i, base + size * pivot) <= 0) ;
    }
    for (i=pivot + 1;i<ofs + len;i++) {
	assert(compar(base + size * pivot, base + size * i) <= 0) ;
    }

    qsort_help(base, nmemb, size, compar, ofs, pivot - ofs) ;
    qsort_help(base, nmemb, size, compar, pivot + 1, ofs + len - (pivot + 1)) ;

    for (i=ofs;i<ofs+len;i++) {
	eprintf("DONE:%03d:%03d\n", i, ((unsigned*)base)[i]) ;
    }

    for (i=ofs + 1;i<ofs+len;i++) {
	assert(compar(base + size * (i - 1), base + size * i) <= 0) ;
    }
}

static
void qsort(void *base, size_t nmemb, size_t size,
	   int (*compar)(const void *, const void *)) {
    qsort_help(base, nmemb, size, compar, 0, nmemb) ;
}

static
int qsort_debug_cmp(const void *v0, const void *v1) {
    unsigned i0 = * (unsigned*) v0 ;
    unsigned i1 = * (unsigned*) v1 ;
    if (i0 < i1) {
	return -1 ;
    } else if (i0 > i1) {
	return 1 ;
    } else {
	return 0 ;
    }
}

void qsort_test(void) {
    unsigned count ;
    for (count=0;;count++) {
	unsigned *a ;
	unsigned ofs ;
	unsigned len ;
	len = sys_random(100) ;
	len = 3 ;
	eprintf("qsort_test:count=%u len=%u\n", count, len) ;
	a = sys_alloc(sizeof(a[0] * len)) ;
	for (ofs=0;ofs<len;ofs++) {
	    a[ofs] = sys_random(1000) ;
	}
	qsort(a, len, sizeof(a[0]), qsort_debug_cmp) ;

	for (ofs=1;ofs<len;ofs++) {
	    assert(a[ofs - 1] <= a[ofs]) ;
	}
    }
}
#endif
#endif
