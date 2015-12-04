/**************************************************************/
/* STACK.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef STACK_H
#define STACK_H

#include "infr/trans.h"

typedef enum { 
    STACK_PRIMARY,
    STACK_BYPASS,
    STACK_GOSSIP,
    STACK_UNRELIABLE,
    STACK_MAX
} stack_id_t ;

bool_t stack_id_eq(stack_id_t id0, stack_id_t id1) ;

string_t stack_id_to_string(stack_id_t) ;

/* Marshalling operations.
 */
#include "infr/marsh.h"
void marsh_stack_id(marsh_t, stack_id_t) ;
void unmarsh_stack_id(unmarsh_t, stack_id_t *) ;

#endif /* STACK_H */
