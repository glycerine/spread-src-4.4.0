/**************************************************************/
/* STACK.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#include "infr/stack.h"
#include "infr/marsh.h"

bool_t stack_id_eq(stack_id_t id0, stack_id_t id1) {
    return id0 == id1 ;
}

#ifndef MINIMIZE_CODE
string_t stack_id_to_string(stack_id_t id) {
    string_t s ;
    switch(id) {
    case STACK_PRIMARY: s = "primary" ; break ;
    case STACK_BYPASS: s = "bypass" ; break ;
    case STACK_GOSSIP: s = "gossip" ; break ;
    case STACK_UNRELIABLE: s = "unreliable" ; break ;
    OTHERWISE_ABORT() ;
    }
    return s ;
}
#endif

#if 0
void stack_id_check(stack_id_t id) {
    switch(id) {
    case STACK_PRIMARY:
    case STACK_BYPASS:
    case STACK_GOSSIP:
    case STACK_UNRELIABLE:
	break ;
    OTHERWISE_ABORT() ;
    }
}
#endif

void marsh_stack_id(marsh_t m, stack_id_t id) {
    marsh_enum(m, id, STACK_MAX) ;

}

void unmarsh_stack_id(unmarsh_t m, stack_id_t *ret) {
    *ret = unmarsh_enum_ret(m, STACK_MAX) ;
}
