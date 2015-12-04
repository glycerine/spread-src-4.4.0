/**************************************************************/
/* SCHED.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
/* Watch out for conflict with linux sched.h.
 */
#ifndef ESCHED_H
#define ESCHED_H

#include "infr/trans.h"

typedef struct sched_t *sched_t ;

/* Create a scheduling queue.
 */
sched_t sched_create(debug_t) ;

/* Enqueue a function and its arguments.
 */
void sched_enqueue(sched_t, void (*fun)(void)) ;
void sched_enqueue_1arg(sched_t, void (*fun)(void *), void *) ;
void sched_enqueue_2arg(sched_t, void (*fun)(void *, void *), void *, void *) ;
void sched_enqueue_3arg(sched_t, void (*fun)(void *, void *, void *), void *, void *, void *) ;
void sched_enqueue_4arg(sched_t, void (*fun)(void *, void *, void *, void *), void *, void *, void *, void *) ;
#if 0
void sched_enqueue_5arg(sched_t, void (*fun)(void *, void *, void *, void *, void *), void *, void *, void *, void *, void *) ;
#endif

/* Add a callback to be made when the scheduler is empty.
 */
void sched_quiesce(sched_t, void (*fun)(void *), void*) ;

/* Is it empty?
 */
bool_t sched_empty(sched_t) ;

/* What's its size?
 */
int sched_size(sched_t) ;

/* Execute this number of steps.  Returns true if
 * some steps were made.  The #steps is assumed to
 * be > 0.
 */
bool_t sched_step(sched_t, int) ;

void sched_usage(sched_t, debug_t) ;

#endif /* ESCHED_H */
