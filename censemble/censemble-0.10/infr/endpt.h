/**************************************************************/
/* ENDPT.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef ENDPT_H
#define ENDPT_H

#include "infr/unique.h"
#include "infr/util.h"

/* BUG: should be opaque.
 */
struct endpt_id_t {
    unique_id_t unique ;
} ;

/* Endpoint identifiers.
 */
typedef struct endpt_id_t endpt_id_t ;
typedef array_def(endpt_id) endpt_id_array_t ;

/* Create unique identifiers.
 */
endpt_id_t endpt_id(unique_t) ;

endpt_id_t endpt_named(string_t) ;

/* Comparison functions.
 */
bool_t endpt_id_eq(const endpt_id_t *, const endpt_id_t *) ;
bool_t endpt_id_option_eq(const endpt_id_t *, const endpt_id_t *) ;
int endpt_id_cmp(const endpt_id_t *, const endpt_id_t *) ;

/* Display operations.
 */
string_t endpt_id_to_string(const endpt_id_t *) ;
string_t endpt_id_array_to_string(endpt_id_array_t, len_t) ;

/* Helper functions.
 */
endpt_id_array_t endpt_id_array_create(len_t) ;
endpt_id_array_t endpt_id_array_copy(endpt_id_array_t, len_t) ;
bool_t endpt_array_mem(const endpt_id_t *, endpt_id_array_t, len_t) ;
ofs_t endpt_array_index(const endpt_id_t *, endpt_id_array_t, len_t) ;
/*endpt_id_array_t endpt_array_append(endpt_id_array_t, len_t, endpt_id_array_t, len_t) ;*/
bool_t endpt_array_disjoint(endpt_id_array_t, len_t, endpt_id_array_t, len_t) ;
/*endpt_id_array_t endpt_array_union(endpt_id_array_t, endpt_id_array_t) ;*/

void hash_endpt_id(hash_t *, const endpt_id_t *) ;

/* Marshalling operations.
 */
#include "infr/marsh.h"
void marsh_endpt_id(marsh_t, const endpt_id_t *) ;
void unmarsh_endpt_id(unmarsh_t, endpt_id_t *) ;
void marsh_endpt_id_array(marsh_t, endpt_id_array_t, len_t) ;
void unmarsh_endpt_id_array(unmarsh_t, endpt_id_array_t *, len_t) ;

#endif /* ENDPT_H */
