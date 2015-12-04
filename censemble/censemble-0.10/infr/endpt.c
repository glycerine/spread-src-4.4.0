/**************************************************************/
/* ENDPT.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/sys.h"
#include "infr/util.h"
#include "infr/unique.h"
#include "infr/endpt.h"

/* Create unique identifiers.
 */
endpt_id_t endpt_id(unique_t unique) {
    struct endpt_id_t id ;
    id.unique = unique_id(unique) ;
    return id ;
}

/*endpt_id_t endpt_named(string_t name) ;*/

/* Display functions.
 */
#ifndef MINIMIZE_CODE
string_t endpt_id_to_string(const endpt_id_t *e) {
    string_t s ; 
    string_t t ;
    s = unique_id_to_string(e->unique) ;
    t = sys_sprintf("Endpt%s", s) ; 
    string_free(s) ;
    return t ;
}
#endif

/*
string_t endpt_array_to_string(endpt_id_array_t a) {
    return util_array_to_string((void*)endpt_id_to_string, (void*)a) ;
}
*/

int endpt_id_cmp(const endpt_id_t *e0, const endpt_id_t *e1) {
    assert(e0) ;
    assert(e1) ;
    return unique_id_cmp(&e0->unique, &e1->unique) ;
}

bool_t endpt_id_eq(const endpt_id_t *e0, const endpt_id_t *e1) {
    assert(e0) ;
    assert(e1) ;
    return unique_id_eq(&e0->unique, &e1->unique) ;
}

bool_t endpt_id_option_eq(const endpt_id_t *e0, const endpt_id_t *e1) {
    if (!e0 && !e1) {
	return TRUE ;
    }
    if (!e0 || !e1) {
	return FALSE ;
    }
    return endpt_id_eq(e0, e1) ;
}

/* BUG: should replace this with O(nlogn) implementation.
 */
bool_t endpt_array_disjoint(
        endpt_id_array_t a0,
	len_t len0,
	endpt_id_array_t a1,
	len_t len1
) {
    ofs_t i, j ;
    assert_array_length(a0, len0) ;
    assert_array_length(a1, len1) ;
    for (i=0;i<len0;i++) {
	for (j=0;j<len1;j++) {
	    if (endpt_id_eq(&array_get(a0, i), &array_get(a1, j))) {
		return FALSE ;
	    }
	}
    }
    return TRUE ;
}
    

bool_t endpt_array_mem(const endpt_id_t *e, endpt_id_array_t a, len_t len) {
    ofs_t i ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (endpt_id_eq(e, &array_get(a, i))) {
	    return TRUE ;
	}
    }
    return FALSE ;
}

ofs_t endpt_array_index(const endpt_id_t *e, endpt_id_array_t a, len_t len) {
    ofs_t i ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (endpt_id_eq(e, &array_get(a, i))) {
	    return i ;
	}
    }
    sys_abort() ;
}

#if 0
endpt_id_array_t endpt_array_append(
        endpt_id_array_t a0,
	len_t len0,
	endpt_id_array_t a1,
	len_t len1
) {
    endpt_id_array_t a2 ;
    ofs_t i ;
    assert_array_length(a0, len0) ;
    assert_array_length(a1, len1) ;
    a2 = array_create(endpt_id, len0 + len1) ;
    for (i=0;i<len0;i++) {
	array_set(a2, i, array_get(a0, i)) ;
    }
    for (i=0;i<len1;i++) {
	array_set(a2, len0 + i, array_get(a1, i)) ;
    }
    return a2 ;
}
#endif

void hash_endpt_id(hash_t *h, const endpt_id_t *id) {
    hash_unique_id(h, &id->unique) ;
}

#include "infr/marsh.h"

void marsh_endpt_id(marsh_t m, const endpt_id_t *id) {
    marsh_unique_id(m, &id->unique) ;
}

void unmarsh_endpt_id(unmarsh_t m, endpt_id_t *id) {
    unmarsh_unique_id(m, &id->unique) ;
}


#include "infr/array_supp.h"
ARRAY_CREATE(endpt_id)
ARRAY_PTR_TO_STRING(endpt_id)
ARRAY_PTR_MARSH(endpt_id, endpt_id)
ARRAY_COPY(endpt_id)
