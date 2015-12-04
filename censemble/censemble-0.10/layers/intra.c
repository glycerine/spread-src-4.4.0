/**************************************************************/
/* INTRA.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/view.h"
#include "infr/event.h"
#include "infr/layer.h"
#include "infr/iovec.h"

static string_t name = "INTRA" ;

typedef enum { NOHDR, INTRA_VIEW, INTRA_FAIL, MAX } header_type_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_t elected ;		/* am I the coordinator? */
    bool_t new_view ;		/* have I done a EView? */
    bool_array_t failed ;	/* failed members */
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
  sprintf "new_view=%b elected=%b failed=%s\n" 
    (Once.isset s.new_view) (Once.isset s.elected)
      (Arrayf.bool_to_string s.failed) ;
#endif
}
#endif

static void init(
        state_t s,
        layer_t layer,
	layer_state_t state,
	view_local_t ls,
	view_state_t vs
) {
    s->elected = FALSE ;
    s->new_view = FALSE ;
    s->layer = layer ;
    s->failed = bool_array_create_init(vs->nmembers, FALSE) ;
    s->ls = ls ;
    s->vs = vs ;
}

static void up_handler(state_t s, event_t e, unmarsh_t msg) {
    header_type_t type = unmarsh_enum_ret(msg, MAX) ;
    rank_t origin = event_peer(e) ;
    switch(event_type(e)) {
    case EVENT_CAST:
	switch(type) {

	case INTRA_VIEW: {
	    /* New view arrived.  If not from me, check it out.  If
	     * accepted, bounce off bottom.  
	     */
	    view_state_t new_vs ;
	    unmarsh_view_state(msg, &new_vs) ;

	    if (origin != s->ls->rank) {
		if (s->elected ||	/* I'm coordinator */
		    s->new_view ||	/* I've already accepted a view */
		    s->ls->rank < origin || /* my rank is lower */
		    array_get(s->failed, origin) || /* coord is failed */
		    !endpt_array_mem(&s->ls->endpt, new_vs->view, new_vs->nmembers)) { /* I am not in the new view */
		    bool_array_t suspects ;
		    suspects = bool_array_setone(s->vs->nmembers, origin) ;
		    log(("View:rejected:%s", endpt_id_array_to_string(new_vs->view, s->vs->nmembers))) ;
		    dnnm(s, event_suspect_reason_create(suspects, name)) ;
		} else {
		    log(("View:accepted:%s", endpt_id_array_to_string(new_vs->view, new_vs->nmembers))) ;
		    s->new_view = TRUE ;
		    dnnm(s, event_set_view_state(event_create(EVENT_VIEW), view_state_copy(new_vs))) ;
		}
	    }
	    view_state_free(new_vs) ;
	    up_free(e, msg) ;
	} break ;

	case INTRA_FAIL: {
	    /* New failure announced.  If not from me, check it out.
	     * If accepted, bounce off bottom. 
	     */
	    const_bool_array_t failures ;
	    unmarsh_bool_array(msg, &failures, s->vs->nmembers) ;
	    if (origin != s->ls->rank) {
		if (s->elected || /* I've been elected */
		    s->ls->rank < origin || /* my rank is lower */
		    bool_array_min_false(failures, s->vs->nmembers) < origin || /* failures don't include all lower ranked members */
		    !bool_array_super(failures, s->failed, s->vs->nmembers) || /* he doesn't include failures I've seen */
		    array_get(s->failed, origin) || /* coord is failed */
		    array_get(failures, origin) || /* he is failing himself */
		    array_get(failures, s->ls->rank)) { /* I am being failed */
		    bool_array_t suspects ;
		    suspects = bool_array_setone(s->vs->nmembers, origin) ;
		    log(("Fail:rejected:%s", bool_array_to_string(failures, s->vs->nmembers))) ;
		    dnnm(s, event_suspect_reason_create(suspects, name)) ;
		} else {
		    log(("Fail:accepted:%s", bool_array_to_string(failures, s->vs->nmembers))) ;
		    bool_array_copy_into(s->failed, failures, s->vs->nmembers) ;
		    dnnm(s, event_set_failures(event_create(EVENT_FAIL),
					       bool_array_copy(failures, s->vs->nmembers))) ;
		}
	    }
	    array_free(failures) ;
	    up_free(e, msg) ;
	} break ;

	case NOHDR:
	    up(s, e, msg) ;
	    break ;
	    
	OTHERWISE_ABORT() ;
	}
	break ;

    default:
	assert(type == NOHDR) ;
	up(s, e, msg) ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_ELECT:
	/* I've been elected.
	 */
	s->elected = TRUE ;
	upnm(s, e) ;
	break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t msg) {
    marsh_enum(msg, NOHDR, MAX) ;
    dn(s, e, msg) ;
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_VIEW: {
	view_state_t vs = event_view_state(e) ;

	/* Send out view and bounce locally.
	 */
	if (s->new_view) {
	    log(("dropping EView because view is already accepted")) ;
	    event_free(e) ;
	} else {
	    marsh_t msg = marsh_create(NULL) ;
	    s->new_view = TRUE ;
	    log(("EView:%s", endpt_id_array_to_string(s->vs->view, s->vs->nmembers))) ;
	    assert(s->elected) ;
	    marsh_view_state(msg, vs) ;
	    marsh_enum(msg, INTRA_VIEW, MAX) ;
	    dn(s, event_cast(), msg) ;
	    dnnm(s, e) ;
	}
    } break ;
	
    case EVENT_FAIL: {
	/* Send out failures and bounce locally.
	 */
	marsh_t msg = marsh_create(NULL) ;
	assert(s->elected) ;
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	log(("EFail:%s", bool_array_to_string(s->failed, s->vs->nmembers))) ;
	marsh_bool_array(msg, s->failed, s->vs->nmembers) ;
	marsh_enum(msg, INTRA_FAIL, MAX) ;
	dn(s, event_cast(), msg) ;
	dnnm(s, e) ;
    } break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->failed) ;
}

LAYER_REGISTER(intra) ;
