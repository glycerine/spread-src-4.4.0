/**************************************************************/
/* NETSIM.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef NETSIM_H
#define NETSIM_H

#include "infr/alarm.h"
#include "infr/domain.h"

alarm_t netsim_alarm(sched_t, unique_t) ;

domain_t netsim_domain(alarm_t) ;

void netsim_usage(void) ;

#endif /* NETSIM_H */
