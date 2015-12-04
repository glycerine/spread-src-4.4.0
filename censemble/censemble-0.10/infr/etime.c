/**************************************************************/
/* TIME.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/sys.h"
#include "infr/etime.h"

#define usecs(t) (t).usecs

static inline
etime_t create(uint64_t usecs) {
    etime_t t ;
    usecs(t) = usecs ;
    return t ;
}

etime_t time_zero(void) {
    return create(0ULL) ;
}

static const
uint64_t million = 1000000 ;

etime_t time_intern(uint64_t usecs) {
    static uint64_t base ;
    static uint64_t last ;
    if (usecs < last) {
	base += last - usecs ;
    }
    last = usecs ;
    return create(base + usecs) ;
}

etime_t time_of_secs(int sec) {
    return create(((uint64_t)sec)*million) ;
}

etime_t time_of_usecs(uint64_t usecs) {
    return create(usecs) ;
}

etime_t time_of_secs_usecs(int sec, int usec) {
    assert(usec >= 0 && usec < million) ;
    return create(((uint64_t)sec)*million + usec) ;
}

etime_t time_of_string(string_t s) {
    sys_abort() ;
}

#ifndef __KERNEL__
etime_t time_of_float(float_t f) {
    return create((uint64_t)(f * 1E6)) ;
}

float_t time_to_float(etime_t t) {
    sys_abort() ;
}
#endif

uint64_t time_to_msecs(etime_t t) {
    return usecs(t) / 1000 ;
}

uint64_t time_to_usecs(etime_t t) {
    return usecs(t) ;
}

#ifndef MINIMIZE_CODE
string_t time_to_string(etime_t t) {
    return sys_sprintf("%lu.%03lu",
		       (unsigned long)(usecs(t)/million),
		       (unsigned long)((usecs(t)%million)/1000)) ;
}

string_t time_to_string_full(etime_t t) {
    sys_abort() ;
}
#endif

etime_t time_add(etime_t t0, etime_t t1) {
    return create(usecs(t0) + usecs(t1)) ;
}

etime_t time_sub(etime_t t0, etime_t t1) {
    return create(usecs(t0) - usecs(t1)) ;
}

bool_t time_is_zero(etime_t t) {
    return (usecs(t) == 0) ;
}

bool_t time_eq(etime_t t0, etime_t t1) {
    return (usecs(t0) == usecs(t1)) ;
}

bool_t time_ge(etime_t t0, etime_t t1) {
    return (usecs(t0) >= usecs(t1)) ;
}

bool_t time_gt(etime_t t0, etime_t t1) {
    return (usecs(t0) > usecs(t1)) ;
}

etime_t time_max(etime_t t0, etime_t t1) {
    if (time_ge(t0, t1)) {
	return t0 ;
    } else {
	return t1 ;
    }
}

#ifndef __KERNEL__
etime_t time_random(etime_t t) {
    return time_of_usecs(sys_random64(usecs(t))) ;
}
#endif
