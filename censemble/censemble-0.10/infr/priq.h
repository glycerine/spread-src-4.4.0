/**************************************************************/
/* PRIQ.H: heap-based priority queues */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef PRIQ_H
#define PRIQ_H

#include "infr/trans.h"
#include "infr/etime.h"

typedef etime_t priq_key_t ;
typedef void *priq_data_t ;
typedef struct priq_t *priq_t ;

priq_t priq_create(void) ;

/* Add an item.
 */
void priq_add(priq_t, priq_key_t, priq_data_t) ;

/* Take the smallest item.  The queue must
 * not be empty.
 */
void priq_get(priq_t, priq_key_t *, priq_data_t *) ;

/* Get an item at most as large as the given key.
 * Returns true iff an item was taken.
 */
bool_t priq_get_upto(priq_t, priq_key_t, priq_key_t *, priq_data_t *) ;

/* The number of items in the queue.
 */
int priq_length(priq_t) ;

/* The minimum item in the queue.  The queue must
 * not be empty.
 */
priq_key_t priq_min(priq_t) ;

/* Is the priq empty?
 */
bool_t priq_empty(priq_t) ;

/* Release the queue.  The queue must already be empty.
 */
void priq_free(priq_t) ;

int priq_usage(priq_t) ;

#ifndef MINIMIZE_CODE
void priq_dump(priq_t pq) ;
#endif
#endif /* PRIQ_H */
