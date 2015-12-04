/**************************************************************/
/* DISPLAY.C */
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

static string_t name = "DISPLAY" ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
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
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    eprintf("%s:up:%s\n", name, event_type_to_string(event_type(e))) ;
    up(s, e, abv) ;
}

static void upnm_handler(state_t s, event_t e) {
    switch (event_type(e)) {
    case EVENT_TIMER:
	break ;
    default:
	eprintf("%s:upnm:%s\n", name, event_type_to_string(event_type(e))) ;
	break ;
    }
    upnm(s, e) ;
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    eprintf("%s:dn:%s\n", name, event_type_to_string(event_type(e))) ;
    dn(s, e, abv) ;
}

static void dnnm_handler(state_t s, event_t e) {
    switch (event_type(e)) {
    case EVENT_TIMER:
	break ;
    default:
	eprintf("%s:dnnm:%s\n", name, event_type_to_string(event_type(e))) ;
	break ;
    }
    dnnm(s, e) ;
}

static void free_handler(state_t s) {
}

LAYER_REGISTER(display) ;
