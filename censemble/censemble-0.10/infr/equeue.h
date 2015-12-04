/**************************************************************/
/* EQUEUE.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef EQUEUE_H
#define EQUEUE_H

#include "infr/trans.h"
#include "infr/util.h"

typedef struct equeue_t *equeue_t ;
typedef array_def(equeue) equeue_array_t ;

/* Size is size of individual elements.
 */
equeue_t equeue_create(debug_t, int size) ;

void *equeue_add(equeue_t) ;

void *equeue_take(equeue_t) ;

void equeue_clear(equeue_t) ;

int equeue_length(equeue_t) ;

bool_t equeue_empty(equeue_t) ;

void equeue_free(equeue_t) ;

int equeue_usage(equeue_t) ;

equeue_array_t equeue_array_create(len_t) ;

#endif /* EQUEUE_H */
