/**************************************************************/
/* XFER.C */
/* Author: Mark Hayden, 6/2000 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
/* Rules for determining whether a view is a state transfer
 * view.
 *
 * Singleton views are *never* XFER views.
 *
 * Non-singleton views are XFER if (A or (B and not C)) holds.
 * Where A, B, and C are:
 *
 *   A. This view was merged from several previous views.
 *      This property is checked in EView when the new
 *      view is ready.
 *
 *   B. Previous view was a XFER view.  This property is
 *      also checked un EView when the new view is ready.
 *
 *   C. All members in previous view took XferDone action.
 *      This property is checked at the coordinator in EView
 *      when sending new view out.  The coordinator examines
 *      the xfer_done array, which marks which members did
 *      XferDone in the current view (and sent message to
 *      the coordinator.
 */

/* NOTE: the Primary protocol requires that the xfer field
 * in the view state be set by the time the EView event is
 * going up the protocol stack.  
 */

/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "XFER" ;
	
typedef enum { NOHDR, XFER_DONE, MAX } header_type_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_array_t xfer_done ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "xfer_done=%s\n" (string_of_bool_array s.xfer_done) ;
#endif
}
#endif

/* CHECK_XFER_DONE: mark the member as being done.  If I'm
 * coordinator and everyone is now done, then prompt a
 * view change.
 */
static void check_xfer_done(state_t s, rank_t rank) {
    bool_t prev = array_get(s->xfer_done, rank) ; /* save previous val */
    array_set(s->xfer_done, rank, TRUE) ; /* mark entry as true */

    if (s->vs->xfer_view &&	/* this is an XFER view */
	s->ls->am_coord &&	/* I am coordinator */
	!prev &&		/* this entry used to be false */
	bool_array_forall(s->xfer_done, s->vs->nmembers)) { /* all entries are now true */
	dnnm(s, event_create(EVENT_PROMPT)) ; /* start view change */
    }
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
    s->xfer_done = bool_array_create_init(vs->nmembers, FALSE) ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    switch (unmarsh_enum_ret(abv, MAX)) {
    case NOHDR:
	up(s, e, abv) ;
	break ;

	/* ECast|ESend(XferDone): other member is telling me
	 * that its state transfer is complete.
	 */
    case XFER_DONE:
	check_xfer_done(s, event_peer(e)) ;
	up_free(e, abv) ;
	break ;

    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

	/* EXferDone: Application has completed state transfer.
	 * If haven't done so already, tell rest of the group
	 * (or only myself if I'm the coordinator).
	 */
    case EVENT_XFER_DONE:
	log(("xfer_done")) ;
	if (s->ls->am_coord) {
	    check_xfer_done(s, s->ls->rank) ;
	} else {
	    /* PERF: check if necessary to send this info out */
	    marsh_t msg = marsh_create(NULL) ;
	    marsh_enum(msg, XFER_DONE, MAX) ;
	    dn(s, event_send_peer(s->vs->coord), msg) ;
	}
	upnm(s, e) ;
	break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    default:
	marsh_enum(abv, NOHDR, MAX) ;
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

	/* EView: coordinator tells others if everyone did
	 * XferDone.
	 */
    case EVENT_VIEW: {
	bool_t xfer_all_done = bool_array_forall(s->xfer_done, s->vs->nmembers) ;
	view_state_t vs_new = event_view_state(e) ;
	ofs_t i ;
	bool_t xfer_view ;
	for (i=0;i<vs_new->nmembers;i++) {
	    if (!view_id_eq(&s->ls->view_id, &array_get(vs_new->prev_ids, i))) {
		break ;
	    }
	}

	xfer_view = (vs_new->nmembers > 1) &&
	    ((i < vs_new->nmembers) || (s->vs->xfer_view && !xfer_all_done)) ;

	vs_new = view_state_copy(vs_new) ;
	vs_new->xfer_view = xfer_view ;
	/*log(("view:view_id=%s", view_id_to_string(&s->ls->view_id))) ;*/
	log(("view:view_state=%s", view_state_to_string(vs_new))) ;
	log(("view:xfer_view=%d all_done=%d", xfer_view, xfer_all_done)) ;
	e = event_set_view_state(e, vs_new) ;
	dnnm(s, e) ;
    } break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->xfer_done) ;
}

LAYER_REGISTER(xfer) ;
