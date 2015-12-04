/**************************************************************/
/* APPL.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#include "infr/appl.h"
#include "infr/trace.h"
#include "infr/sched.h"
#include "infr/layer.h"
#include "infr/alarm.h"
#include "infr/sys.h"

sched_t appl_sched ;

bool_t appl_poll(alarm_t alarm) {
    sched_t sched = alarm_sched(alarm) ;
    bool_t got_some = FALSE ;
#if 0
    if (sys_signal_received()) {
	sys_exit(0) ;
    }
#endif
#if 0
    {
	extern void __chkr_garbage_detector(void) ;
	//extern void __chkr_garbage_siginfo(void) ;
	//__chkr_garbage_siginfo() ;
	__chkr_garbage_detector() ;
    }
#endif

    if (sched_step(sched, 100)) {
	got_some = TRUE ;
    }
    if (alarm_check(alarm)) {
	got_some = TRUE ;
    }
    if (alarm_poll(alarm, ALARM_SOCKS_POLLS)) {
	got_some = TRUE ;
    }
    appl_usage() ;
    return got_some ;
}

void appl_block(alarm_t alarm) {
    alarm_block(alarm) ;
}

void appl_mainloop(alarm_t alarm) {
    while(1) {
	if (appl_poll(alarm)) {
	    continue ;
	}
	alarm_block(alarm) ;
    }
}

#define REGISTER(a) \
  { extern void a ## _register(void) ; \
    a ## _register() ; \
  }

void appl_init(void) {
    extern void (*signal(int signum, void (*handler)(int)))(int) ;
    static bool_t inited ;

    if (inited) {
	return ;
    }
    inited = TRUE ;

/*#define FENCE_BELOW*/
#ifdef FENCE_BELOW
    { extern int EF_PROTECT_BELOW ;
      EF_PROTECT_BELOW = 1 ; }
#endif
/*#define FENCE_FREE*/
#ifdef FENCE_FREE
    { extern int EF_PROTECT_FREE ;
      EF_PROTECT_FREE = 1 ; }
#endif

    sys_catch_sigint() ;

#if 1
    REGISTER(bottom) ;
#if 1
#ifndef MINIMIZE_CODE
    REGISTER(chk_fifo) ;
    REGISTER(chk_sync) ;
    REGISTER(chk_trans) ;
    REGISTER(display) ;
    REGISTER(drop) ;
#endif
#endif
    REGISTER(elect) ;
    REGISTER(heal) ;
    REGISTER(inter) ;
    REGISTER(intra) ;
    REGISTER(leave) ;
    REGISTER(local) ;
    REGISTER(mnak) ;
    REGISTER(present) ;
    REGISTER(primary) ;
    REGISTER(pt2pt) ;
    REGISTER(pt2ptw) ;
    REGISTER(suspect) ;
    REGISTER(stable) ;
    REGISTER(sync) ;
    REGISTER(top) ;
    REGISTER(top_appl) ;
    REGISTER(vsync) ;
    REGISTER(xfer) ;
    REGISTER(wrapper) ;
#endif
}

#include "trans/netsim.h"
#include "infr/transport.h"

void appl_usage(void) {
    static int count ;
    if ((count ++) % 100000) {
	return ;
    }
#if 0
    sched_usage(alarm_sched(ealarm), "sched") ;
    netsim_usage() ;
    transport_usage() ;
#endif
}

void appl_process_args(int argc, char *argv[]) {
    int i ;
    for (i=1;i<argc;i++) {
	if (string_eq(argv[i], "-trace")) {
	    if (++i >= argc) {
		continue ;
	    }
	    log_add(argv[i]) ;
	}
    }
}
