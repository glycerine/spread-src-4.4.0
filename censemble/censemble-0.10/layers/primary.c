/**************************************************************/
/* PRIMARY.C */
/* Author: Mark Hayden, 6/2000 */
/* Based on code by: Mark Hayden, Zhen Xiao, 9/97 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "PRIMARY" ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_array_t in_view ;
    nmembers_t quorum ;
} *state_t ;

#include "infr/layer_supp.h"

static bool_t is_quorum(state_t s, view_state_t vs, bool_array_t mask_opt) {
    len_t nservers = 0 ;
    ofs_t i ;
    if (s->vs->quorum == 0) {
	/* Must all be clients...
	 */
	return FALSE ;
    }
    for (i=0;i<vs->nmembers;i++) {
	if (array_get(vs->clients, i) ||
	    (mask_opt && !array_get(mask_opt, i))) {
/*
  || (not (Arrayf.get vs.out_of_date i))
  || Arrayf.get vs.out_of_date i = vs.ltime
*/
	    continue ;
	}
	nservers ++ ;
    }
    return nservers >= s->quorum ;
}

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "in_view=%s\n" (Arrayf.bool_to_string s.in_view)
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
    s->ls = ls ; 
    s->vs = vs ;
    s->layer = layer ;
    s->in_view = bool_array_create_init(vs->nmembers, FALSE) ;
    s->quorum = vs->quorum ;

    assert(bool_array_forall(vs->clients, vs->nmembers) ||
	   vs->quorum > 0) ;

    if (s->quorum == 0 && !bool_array_forall(vs->clients, vs->nmembers)) {
	sys_panic(("primary paramter not set")) ;
    }
/*
    if 2 * quorum <= nservers vs None then 
      eprintf "Warning: primary_quorum is no more than half of the #servers\n" ;
*/
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    switch (event_type(e)) {
    default:
	up(s, e, abv) ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

	/* Update the info about which members are currently in view.
	 */
    case EVENT_PRESENT:
	bool_array_copy_into(s->in_view, event_presence(e), s->vs->nmembers) ;

	/* If we have a quorum, but we are not marked as
	 * primary, then prompt a view change.  
	 */
	/*log ((fun () -> sprintf "present am_coord=%b primary=%b quorum=%b"
	  ls.am_coord vs.primary (s.quorum vs (Some s.in_view)))) ;*/
	if (s->ls->am_coord &&
	    !s->vs->primary &&
	    is_quorum(s, s->vs, s->in_view)) {
	    dnnm(s, event_create(EVENT_PROMPT)) ;
	}
	upnm(s, e) ;
	break ;

	/* At all members, check that the ltime is one more
	 * than the current logical time.
	 */
    case EVENT_VIEW: {
	view_state_t next_vs ;
	ltime_t coord_ltime ;
	ltime_t next_ltime ;
	bool_t succ_ltime ;
	bool_t primary ;
	next_vs = event_view_state(e) ;

/* This check is invalid.
      if Arrayf.get next_vs.view 0 <> snd (List.hd next_vs.prev_ids) then (
        eprintf "PRIMARY:view_state=%s\n" (View.string_of_state next_vs) ;
	sys_panic "inconsistent view_id & view" ;
      ) ;
*/
	coord_ltime = array_get(next_vs->prev_ids, next_vs->coord).ltime ;
	assert(coord_ltime > 0) ;
	next_ltime = next_vs->ltime ;

	succ_ltime = (next_ltime == coord_ltime + 1) ;

	/* We leave the primary bit set if it was already
	 * set and the one additional criteria is met.
	 */
	primary = next_vs->primary && succ_ltime ;

	next_vs = view_state_copy(next_vs) ;

#if 0
	/* We bring all endpoints "up to date" when we find
	 * that there is a view with the primary bit set and
	 * the xfer bit is unset.  This means that all members
	 * must have completed a successful state transfer in
	 * a primary partition.  
	 */
	/* At this point, the xfer field should have been
	 * set by the XFER layer.
	 */

        if (primary && !next_vs->xfer_view) {
	    array_free(next_vs->out_of_date) ;
	    next_vs->out_of_date = ltime_array_create_init(next_vs->nmembers, next_vs->ltime) ;
	}
#endif

	/*log (fun () -> sprintf "next_vs.primary=%b succ_ltime=%b -> primary=%b"
	       next_vs.primary succ_ltime primary) ;*/
	
	next_vs->primary = primary ;
	
	upnm(s, event_set_view_state(e, next_vs)) ;
    } break ;

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
	/* EView: decide whether next view is 
	 * primary.
	 */
    case EVENT_VIEW: {
	 bool_t in_view_quorum ;
	 bool_t next_quorum ;
	 bool_t same_coord ; 
	 bool_t primary ;

	 view_state_t next_vs = event_view_state(e) ;

	 /* Check whether we have a quorum in our view.
	  */
	 in_view_quorum = is_quorum(s, s->vs, s->in_view) ;

	 /* Check whether the next view has a quorum.
	  */
	 next_quorum = is_quorum(s, next_vs, NULL) ;

	 /* Check if the next coord is the same as current one.
	  */
	 same_coord = endpt_id_eq(&array_get(s->vs->view, s->vs->coord),
				  &array_get(next_vs->view, next_vs->coord)) ;

	 /* These are the first three conditions for primariness.
	  * The next one is in upnm EView.
	  */
	 primary = in_view_quorum &&
	     next_quorum &&
	     same_coord ;

	 log(("view_state=%s", view_state_to_string(s->vs))) ;
	 log(("in_view_quorum=%d next_quorum=%d same_coord=%d -> primary=%d",
	      in_view_quorum, next_quorum, same_coord, primary)) ;

	 next_vs = view_state_copy(next_vs) ;
	 next_vs->primary = primary ;
	 dnnm(s, event_set_view_state(e, next_vs)) ;
    } break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->in_view) ;
}

LAYER_REGISTER(primary) ;
