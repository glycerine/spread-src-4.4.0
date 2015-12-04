/**************************************************************/
/* GROUP.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef GROUP_H
#define GROUP_H

/* BUG: should be opaque.
 */
#define GROUP_SIZE 28
struct group_id_t {
    enum { GROUP_NAMED, GROUP_ANONYMOUS, GROUP_MAX } type ;
    union {
	unique_id_t anonymous ;
	char named[GROUP_SIZE] ;
    } u ;
} ;

/* Group identifiers.
 */
typedef struct group_id_t group_id_t ;

/* Create unique identifiers.
 */
group_id_t group_id(unique_id_t) ;
group_id_t group_named(string_t) ;

/* Comparison operations.
 */
bool_t group_id_eq(const group_id_t *, const group_id_t *) ;

/* Display functions.
 */
string_t group_id_to_string(const group_id_t *) ;

uint32_t group_id_to_hash(const group_id_t *) ;

/* Marshalling operations.
 */
#include "infr/marsh.h"
void marsh_group_id(marsh_t, const group_id_t *) ;
void unmarsh_group_id(unmarsh_t, group_id_t *) ;

#endif /* GROUP_H */
