/**************************************************************/
/* LAYER_SOCK.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/event.h"
#include "infr/sched.h"
#include "infr/layer.h"
#include "infr/util.h"
#include "infr/view.h"
#include "infr/trans.h"

static string_t name = "TEMPLATE" ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
} *state_t ;

#include "infr/layer_supp.h"

static void dump(state_t s) {
    abort() ;
}

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
    return s ;
}


static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    marsh_t marsh = marsh_create(NULL) ;
    marsh_enum(marsh, UP, MAX) ;
    marsh_iovec(marsh, 

    switch (event_type(e)) {
    default:
	up(s, e, abv) ;
	break ;
    }
}

static void uplm_handler(state_t s, event_t e, unmarsh_t msg, iovec_t iov) {
    switch (event_type(e)) {
    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    EVENT_DUMP_HANDLE() ;
    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    default:
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    default:
	dnnm(s, e) ;
	break ;
    }
}

void template_register(void) {
    layer_register(name, init, up_handler, uplm_handler, upnm_handler,
		   dn_handler, dnnm_handler, dump) ;
}

