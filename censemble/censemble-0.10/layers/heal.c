/**************************************************************/
/* HEAL.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
/* Notes:

 * Only merge with higher numbered views to prevent cycles.

 * Only communicate when I think I will be able to merge
 * quickly.

 * Tries to prevent merge attempts that may fail.  Be
 * conservative.

 */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "HEAL" ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    bool_t wait_view ;
    nmembers_t merge_nmem ;
    view_contact_t merge_con ;
    bool_t casted ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "wait_view=%b\n" s.wait_view ;
    sprintf "present=%s\n" (string_of_bool_array s.present) ;
    sprintf "view_id=%s con_view_id=%s\n"
	(View.string_of_id ls.view_id)
	(View.string_of_id s.merge_vid)
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
    s->wait_view = FALSE ;
    /*s->all_present = FALSE ;*/
    /*s->present = NULL ;*/
    s->merge_nmem = vs->nmembers ;
    s->merge_con = view_contact(vs) ;
    s->casted = FALSE ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    up(s, e, abv) ;
}

static event_heal_gossip_t gossip_create(state_t s) {
    return event_heal_gossip_create(view_contact(s->vs),
				    proto_id_copy(s->vs->proto),
				    endpt_id_array_copy(s->vs->view, s->vs->nmembers),
				    s->vs->nmembers) ;
}

static void gossip_recv(state_t s, event_heal_gossip_t heal) {
    endpt_id_array_t bc_view = heal->view ;
    nmembers_t bc_nmem = heal->nmembers ;
    endpt_id_t bc_coord = heal->contact.endpt ;

    bool_t same_protocol = proto_id_eq(heal->proto, s->vs->proto) ;
    bool_t not_in_my_view = !endpt_array_mem(&bc_coord, s->vs->view, s->vs->nmembers) ;
    bool_t disjoint_views = endpt_array_disjoint(s->vs->view, s->vs->nmembers, bc_view, bc_nmem) ;

    assert_array_length(bc_view, bc_nmem) ;

    /* Check if I'm interested in the other partition at all.
     */
    if (s->ls->am_coord &&
	same_protocol &&
	not_in_my_view &&
	disjoint_views) {
	log(("received interesting bcast from %s (curr contact=%s)",
	     endpt_id_to_string(&bc_coord),
	     endpt_id_to_string(&s->merge_con.endpt))) ;

	/* First check to see if I want to merge with him.  This decision
	 * is based on several criteria.  First, if I'm a primary partition,
	 * then I never merge with other members.  Second, I only merge with
	 * partitions that have more members or who have large view ids.
	 */
	if (!s->vs->primary &&
	    (bc_nmem > s->merge_nmem ||
	     (bc_nmem == s->merge_nmem &&
	      view_id_cmp(&heal->contact.view_id, &s->merge_con.view_id) > 0))) {
	    log(("suggesting merge with %s", view_id_to_string(&s->merge_con.view_id))) ;
	    s->merge_con = heal->contact ;
	    s->merge_nmem = bc_nmem ;

	    /* If I'm not installing a view currently, start a
	     * view change.
	     */
	    if (!s->wait_view) {
		s->wait_view = TRUE ;
		dnnm(s, event_create(EVENT_PROMPT)) ;
	    }
	}

	/* Otherwise, he might think my partition is interesting.
	 */
	if (view_id_eq(&s->ls->view_id, &s->merge_con.view_id) && /* I'm not planning to merge myself */
	    !s->wait_view &&	/* I'm not waiting for a view */
	    !s->casted &&	/* don't bcast all the time */
	    (bc_nmem < s->merge_nmem ||	/* check that I am 'older' than this one */
	     (bc_nmem == s->merge_nmem &&
	      view_id_cmp(&heal->contact.view_id, &s->merge_con.view_id) < 0))) {
	    event_t e ;
	    s->casted = TRUE ;
	    log(("bcasting(recv)")) ;
	    e = event_create(EVENT_GOSSIP_EXT) ;
	    e = event_set_heal_gossip(e, gossip_create(s)) ;
	    dnnm(s, e) ;
	}
    } else {
	log(("gossip dropped from %s", endpt_id_to_string(&bc_coord))) ;
	log(("am_coord=%d same_protocol=%d not_in_my_view=%d disjoint_views=%d",
	     s->ls->am_coord, same_protocol, not_in_my_view, disjoint_views)) ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_GOSSIP_EXT: {
	event_heal_gossip_t heal ;
	heal = event_heal_gossip(e) ;
	if (heal) {
	    gossip_recv(s, heal) ;
	}
	upnm(s, e) ;
    } break ;

    case EVENT_BLOCK:
	s->wait_view = TRUE ;
	upnm(s, e) ;
	break ;

    case EVENT_BLOCK_OK:
	if (!view_id_eq(&s->merge_con.view_id, &s->ls->view_id)) {
	    log(("EBlock(suggest->%s)", endpt_id_to_string(&s->merge_con.endpt))) ;
	    event_set_contact(e, s->merge_con) ;
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
    dn(s, e, abv) ;
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_GOSSIP_EXT:
	s->casted = FALSE ;
	if (s->ls->am_coord &&	/* I'm elected */
	    !s->wait_view &&	/* I'm not waiting for view */
	    view_id_eq(&s->ls->view_id, &s->merge_con.view_id) && /* I'm not planning to merge */
	    TRUE/*all_present(s)*/) {               /* everyone is present */
	    e = event_set_heal_gossip(e, gossip_create(s)) ;
	}
	dnnm(s, e) ;
	break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    /* BUG: merge_co */
}

LAYER_REGISTER(heal) ;
