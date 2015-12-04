/**************************************************************/
/* ETIME.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef ETIME_H
#define ETIME_H

#include "infr/trans.h"

struct etime_t {
    uint64_t usecs ;
} ;

typedef struct etime_t etime_t ;

etime_t time_zero(void) ;

/* Similar to time_of_usecs, but handles issues when time
 * goes backwards.  This function has static state in it.
 */
etime_t time_intern(uint64_t) ;

etime_t time_of_secs(int) ;
etime_t time_of_secs_usecs(int, int) ;
etime_t time_of_usecs(uint64_t) ;
etime_t time_of_string(string_t) ;
etime_t time_of_float(float_t) ;

uint64_t time_to_msecs(etime_t) ; /* milliseconds */
uint64_t time_to_usecs(etime_t) ; /* microseconds */
float_t time_to_float(etime_t) ;
string_t time_to_string(etime_t) ;
string_t time_to_string_full(etime_t) ;

etime_t time_add(etime_t, etime_t) ;
etime_t time_sub(etime_t, etime_t) ;

bool_t time_is_zero(etime_t) ;
bool_t time_is_invalid(etime_t) ;
bool_t time_ge(etime_t, etime_t) ;
bool_t time_gt(etime_t, etime_t) ;
bool_t time_eq(etime_t, etime_t) ;
etime_t time_max(etime_t, etime_t) ;
etime_t time_random(etime_t) ;

#endif /* ETIME_H */
