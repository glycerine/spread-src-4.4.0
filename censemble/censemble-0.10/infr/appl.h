/**************************************************************/
/* APPL.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef APPL_H
#define APPL_H

#include "infr/sched.h"
#include "infr/alarm.h"

void appl_init(void) ;

void appl_mainloop(alarm_t) ;

bool_t appl_poll(alarm_t) ;

void appl_block(alarm_t) ;

void appl_usage(void) ;

void appl_process_args(int argc, char *argv[]) ;

#endif /* APPL_H */
