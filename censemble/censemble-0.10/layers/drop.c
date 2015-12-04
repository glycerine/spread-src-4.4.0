/**************************************************************/
/* DROP.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"
#include "infr/etime.h"
#include "infr/priq.h"
#include "infr/alarm.h"
#include "infr/marsh.h"

static string_t name = "DROP" ;

typedef struct {
    enum { DROP_UP, DROP_UPNM } type ;
    union {
	struct {
	    event_t event ;
	    unmarsh_t abv ;
	} up ;
	struct {
	    event_t event ;
	} upnm ;
    } u ;	
} *item_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    alarm_t alarm ;
    priq_t priq ;
    bool_t partition ;
    bool_array_t failed ;
    int acct_delivered ;
    int acct_dropped ;
    etime_t next_sweep ;
} *state_t ;
	
#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
}
#endif

static void init(
        state_t s,
        layer_t layer,
	layer_state_t state,
	view_local_t ls,
	view_state_t vs
) {
    s->ls = ls ; 
    s->vs = vs ;
    s->layer = layer ;
    s->alarm = state->alarm ;
    s->priq = priq_create() ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->partition = FALSE ;
    s->acct_dropped = 0 ;
    s->acct_delivered = 0 ;
    s->next_sweep = time_zero() ;
#if 0
    //s->alarm = NULL ;//Elink.alarm_get_hack ()
#endif
}

static bool_t distrib(state_t s, etime_t *delay) {
    float_t drop_rate = 0.01 ;
    float_t drop_delay = 0.002 ;
    float_t p = sys_random(1000000) / 1000000.0 ;
    if (p < drop_rate) {
	s->acct_dropped ++ ;
	return FALSE ;
    } else {
	*delay = time_of_float(p * drop_delay) ;
	return TRUE ;
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    endpt_id_t origin ;
    unmarsh_endpt_id(abv, &origin) ;
    if (s->partition) {
	sys_abort() ;
    } else {
	etime_t delay ;
	if (!distrib(s, &delay)) {
	    up_free(e, abv) ;
	} else {
	    etime_t when = time_add(alarm_gettime(s->alarm), delay) ;
	    item_t item = record_create(item_t, item) ;
	    /*eprintf("type=%s\n", event_type_to_string(event_type(e))) ;*/
	    item->type = DROP_UP ;
	    item->u.up.event = e ;
	    item->u.up.abv = abv ;
	    priq_add(s->priq, when, item) ;
	    dnnm(s, event_timer_time(when)) ;
	}
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_FAIL:
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	upnm(s, e) ;
	break ;
	
    case EVENT_INIT:
	dnnm(s, event_timer_time(time_zero())) ;
	upnm(s, e) ;
	break ;

    case EVENT_TIMER: {
	etime_t time = event_time(e) ;
	item_t item ;
	while (priq_get_upto(s->priq, time, NULL, (void**)&item)) {
	    s->acct_delivered ++ ;
	    switch(item->type) {
	    case DROP_UP: {
		rank_t origin = event_peer(item->u.up.event) ;
		if (origin >= 0 &&
		    array_get(s->failed, origin)) {
		    up_free(item->u.up.event, item->u.up.abv) ;
		} else {
		    up(s, item->u.up.event, item->u.up.abv) ;
		}
	    } break ;

	    case DROP_UPNM:
		upnm(s, item->u.upnm.event) ;
		break ;

	    OTHERWISE_ABORT() ;
	    }
	    record_free(item) ;
	}

	if (time_ge(time, s->next_sweep)) {
	    if (!time_is_zero(s->next_sweep) &&
		sys_random(5) == 1) {
		rank_t i ;
		bool_array_t suspects = bool_array_create_init(s->vs->nmembers, FALSE) ;

		for(i=0;i<s->vs->nmembers;i++) {
		    if (i == s->ls->rank) {
			continue ;
		    }
		    if (sys_random(4) == 0) {
			array_set(suspects, i, TRUE) ;
		    }
		}
		if (bool_array_exists(suspects, s->vs->nmembers)) {
		    dnnm(s, event_suspect_reason_create(suspects, name)) ;
		} else {
		    array_free(suspects) ;
		}
	    }

#if 0
	    /* Suspicions are randomly generated every 0-8 seconds.
	     */
	    s->next_sweep = time_add(time, time_of_usecs(sys_random(1<<23/*8M*/))) ;
#else
	    s->next_sweep = time_add(time, time_of_usecs(sys_random(1<<20/*1M*/))) ;
#endif
	    dnnm(s, event_timer_time(s->next_sweep)) ; /* request next sweep */
	}
	upnm(s, e) ;
    } break ; 
	
    case EVENT_GOSSIP_EXT: {
	/*endpt_id_t origin = NULL ;*/
	etime_t delay ;
/*
	let origin =
      	getExtender (function
	  | HealGos(_,(_,endpt),_,_) -> Some (Some endpt)
	  | SwitchGos(_,(_,endpt),_) -> Some (Some endpt)
	  | _ -> None
        ) None ev 
      in
*/
	if (1 /*!origin*/) {
	    upnm(s, e) ;
	} else if (s->partition) {
	    sys_abort() ;
	} else if (!distrib(s, &delay)) {
	    event_free(e) ;
	} else {
	    /* Deliver after a certain delay....
	     */
	    etime_t when = time_add(alarm_gettime(s->alarm), delay) ;
	    item_t item = record_create(item_t, item) ;
	    item->type = DROP_UPNM ;
	    item->u.upnm.event = e ;
	    priq_add(s->priq, when, item) ;
	    dnnm(s, event_timer_time(when)) ;
	}
    } break ;

    case EVENT_ACCOUNT:
	log(("delivered=%d dropped=%d", s->acct_delivered, s->acct_dropped)) ;
	upnm(s, e) ;
	break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    marsh_endpt_id(abv, &s->ls->endpt) ;
    dn(s, e, abv) ;
}

static void dnnm_handler(state_t s, event_t e) {
    dnnm(s, e) ;
}

static void free_handler(state_t s) {
    array_free(s->failed) ;
    while (!priq_empty(s->priq)) {
	item_t item ;
	priq_get(s->priq, NULL, (void**)&item) ;
	switch(item->type) {
	case DROP_UP:
	    event_free(item->u.up.event) ;
	    unmarsh_free(item->u.up.abv) ;
	    break ;
	    
	case DROP_UPNM:
	    event_free(item->u.upnm.event) ;
	    break ;
	    
	OTHERWISE_ABORT() ;
	} 
	record_free(item) ;
    }
    priq_free(s->priq) ;
}

LAYER_REGISTER(drop)
