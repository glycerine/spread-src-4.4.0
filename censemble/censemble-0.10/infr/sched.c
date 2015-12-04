/**************************************************************/
/* SCHED.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/sched.h"
#include "infr/equeue.h"
#include "infr/util.h"

typedef struct sched_entry_t {
    int count ;
    void *fun ;
    void *arg0 ;
    void *arg1 ;
    void *arg2 ;
    void *arg3 ;
    void *arg4 ;
} *sched_entry_t, sched_entry_t_np ;

#define ITEM_T sched_entry_t_np
#define CLASS_NAME entry
#include "infr/equeue_supp.h"

struct sched_t {
    entry_equeue_t queue ;
    entry_equeue_t quiesce ;
} ;

#if 0
typedef struct sched_quiesce_t {
    void (*fun)() ;
    void *arg ;
} *sched_quiesce_t ;
#endif

/* Create a scheduling queue.
 */
sched_t sched_create(debug_t d) {
    sched_t s = record_create(sched_t, s) ;
    entry_equeue_create(d, &s->queue) ;
    entry_equeue_create(d, &s->quiesce) ;
    return s ;
}

static inline sched_entry_t add(sched_t s) {
    return entry_equeue_add(&s->queue) ;
}

/* Enqueue a function and its arguments.
 */
void sched_enqueue(sched_t s, void (*fun)(void)) {
    sched_entry_t e = add(s) ;
    assert(fun) ;
    e->count = 0 ;
    e->fun = fun ;
}

void sched_enqueue_1arg(sched_t s, void (*fun)(void *),
			void *arg0) {
    sched_entry_t e = add(s) ;
    assert(fun) ;
    e->count = 1 ;
    e->fun = fun ;
    e->arg0 = arg0 ;
}

void sched_enqueue_2arg(sched_t s, void (*fun)(void *, void *),
			void *arg0, void *arg1) {
    sched_entry_t e = add(s) ;
    assert(fun) ;
    e->count = 2 ;
    e->fun = fun ;
    e->arg0 = arg0 ;
    e->arg1 = arg1 ;
}

void sched_enqueue_3arg(sched_t s, void (*fun)(void *, void *, void *),
			void *arg0, void *arg1, void *arg2) {
    sched_entry_t e = add(s) ;
    assert(fun) ;
    e->count = 3 ;
    e->fun = fun ;
    e->arg0 = arg0 ;
    e->arg1 = arg1 ;
    e->arg2 = arg2 ;
}

void sched_enqueue_4arg(sched_t s, void (*fun)(void *, void *, void *, void *),
			void *arg0, void *arg1, void *arg2, void *arg3) {
    sched_entry_t e = add(s) ;
    assert(fun) ;
    e->count = 4 ;
    e->fun = fun ;
    e->arg0 = arg0 ;
    e->arg1 = arg1 ;
    e->arg2 = arg2 ;
    e->arg3 = arg3 ;
}

#if 0
void sched_enequeue_5arg(sched_t s, void (*fun)(void *, void *, void *, void *, void *),
			void *arg0, void *arg1, void *arg2, void *arg3, void *arg4) {
    sched_entry_t e = add(s) ;
    assert(fun) ;
    e->count = 5 ;
    e->fun = fun ;
    e->arg0 = arg0 ;
    e->arg1 = arg1 ;
    e->arg2 = arg2 ;
    e->arg3 = arg3 ;
    e->arg4 = arg4 ;
}
#endif

void sched_quiesce(sched_t s, void (*fun)(void *), void *arg) {
    sched_entry_t e = entry_equeue_add(&s->quiesce) ;
    assert(fun) ;
    e->count = 1 ;
    e->fun = fun ;
    e->arg0 = arg ;
}

/* Is it empty?
 */
bool_t sched_empty(sched_t s) {
    return entry_equeue_empty(&s->queue) ;
}

/* What's its size?
 */
int sched_size(sched_t s) {
    return entry_equeue_length(&s->queue) ;
}

typedef void (*fun0_t)(void) ;
typedef void (*fun1_t)(void *) ;
typedef void (*fun2_t)(void *, void *) ;
typedef void (*fun3_t)(void *, void *, void *) ;
typedef void (*fun4_t)(void *, void *, void *, void *) ;
typedef void (*fun5_t)(void *, void *, void *, void *, void *) ;


/* Execute this number of steps.  Returns true if
 * some steps were made.  The #steps is assumed to
 * be > 0.
 */
bool_t sched_step(sched_t s, int count) {
    int i ;
    for (i=0;i<count;i++) {
	sched_entry_t e ;
	if (entry_equeue_empty(&s->queue)) {
	    break ;
	}
	e = entry_equeue_take(&s->queue) ;
	assert(e->fun) ;
	switch(e->count) {
	case 0:
	    ((fun0_t)e->fun)() ;
	    break ;
	case 1:
	    ((fun1_t)e->fun)(e->arg0) ;
	    break ;
	case 2:
	    ((fun2_t)e->fun)(e->arg0, e->arg1) ;
	    break ;
	case 3:
	    ((fun3_t)e->fun)(e->arg0, e->arg1, e->arg2) ;
	    break ;
	case 4:
	    ((fun4_t)e->fun)(e->arg0, e->arg1, e->arg2, e->arg3) ;
	    break ;
	case 5:
	    ((fun5_t)e->fun)(e->arg0, e->arg1, e->arg2, e->arg3, e->arg4) ;
	    break ;
	OTHERWISE_ABORT() ;
	}
#if 0				/* DON'T DO THIS!!!! */
#ifdef USE_GC
	e->fun = NULL ;
	e->arg0 = NULL ;
	e->arg1 = NULL ;
	e->arg2 = NULL ;
	e->arg3 = NULL ;
	e->arg4 = NULL ;
#endif
#endif

    }

    /* If the queue is empty, then execute any quiesce
     * actions.
     */
    while (entry_equeue_empty(&s->queue) &&
	   !entry_equeue_empty(&s->quiesce)) {
	sched_entry_t e = entry_equeue_take(&s->quiesce) ;
	assert(e->count == 1) ;
	((fun1_t)e->fun)(e->arg0) ;
#if 0				/* DON'T DO THIS!!!! */
#ifdef USE_GC
	e->fun = NULL ;
	e->arg0 = NULL ;
#endif
#endif
    }
    
    return (i > 0) ;
}

#ifndef MINIMIZE_CODE
void sched_usage(sched_t s, debug_t debug) {
    eprintf("sched:%s:queue=%d\n", debug, entry_equeue_usage(&s->queue)) ;
    eprintf("sched:%s:quiesce=%d\n", debug, entry_equeue_usage(&s->quiesce)) ;
}
#endif
