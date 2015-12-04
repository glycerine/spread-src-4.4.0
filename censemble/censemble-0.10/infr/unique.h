/**************************************************************/
/* UNIQUE.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef UNIQUE_H
#define UNIQUE_H

#include "infr/trans.h"

/* Id generator.
 */
typedef struct unique_t *unique_t ;

/* An id consists of an IP address, UDP port number, randomized
 * incarnation number, and a multiplexing number (mux).
 */
/* BUG: Should be opaque.
 */
struct unique_id_t {
    mux_t mux ;
    inet_t inet ;
    net_port_t port ;
    incarn_t incarn ;
} ;

/* Unique identifiers.
 */
typedef struct unique_id_t unique_id_t ;

/* Constructors.
 */
unique_t unique_create(inet_t, incarn_t) ;
unique_id_t unique_id(unique_t) ;

/* Display functions
 */
string_t unique_id_to_string(unique_id_t) ;

/* Hash an id
 */
int unique_hash_of_id(const unique_id_t *) ;

void unique_set_port(unique_t, net_port_t) ;

bool_t unique_id_eq(const unique_id_t *, const unique_id_t *) ;

int unique_id_cmp(const unique_id_t *, const unique_id_t *) ;

void hash_unique_id(hash_t *, const unique_id_t *) ;

/* Marshalling operations.
 */
#include "infr/marsh.h"
void marsh_unique_id(marsh_t, const unique_id_t *) ;
void unmarsh_unique_id(unmarsh_t, unique_id_t *) ;

#endif /* UNIQUE_H */
