/**************************************************************/
/* INTER.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
/* BUG: we do not stay around very long to ensure
 * that other endpoints got the new view.
 */
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "INTER" ;

typedef enum { NOHDR, DATA, MAX } header_type_t ;
	
typedef enum {
    INTER_NORMAL,
    INTER_MERGING,
    INTER_FAILED_MERGE,
    INTER_INSTALLED_VIEW
} status_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    status_t state ;		/* automata state */
    bool_t blocked ;		/* have I seen an EBlockOk? */
    view_state_array_t mergers ; /* views merging with me */
    len_t nmergers ;
    view_state_t next_vs ;

    etime_t next_sweep ;
    etime_t sweep ;
    etime_t timeout ;

    struct {
	event_t event ;
	marsh_buf_t abv ;
    } merge ;
} *state_t ;

#include "infr/layer_supp.h"

#ifndef MINIMIZE_CODE
static string_t string_of_status(status_t status) UNUSED() ;
static string_t string_of_status(status_t status) {
    string_t ret ;
    switch(status) {
    case INTER_NORMAL: ret = "Normal" ; break ;
    case INTER_MERGING: ret = "Merging" ; break ;
    case INTER_FAILED_MERGE: ret = "Failed_Merge" ; break ;
    case INTER_INSTALLED_VIEW: ret = "Installed_View" ; break ;
    OTHERWISE_ABORT() ;
    }
    return ret ;
}	
#endif

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "am_coord=%b, blocked=%b\n" ls.am_coord s.blocked ;
    sprintf "view_id=%s\n" (View.string_of_id ls.view_id) ;
    sprintf "view=%s\n" (View.to_string vs.view) ;
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
    s->state = INTER_NORMAL ;
    s->blocked = FALSE ;
    s->mergers = array_empty(view_state) ;
    s->nmergers = 0 ;
    s->next_sweep = time_zero() ;
    s->sweep = time_of_secs(1) ;
    s->timeout = time_zero() ;
    s->merge.event = NULL ;
    s->merge.abv = NULL ;
    s->next_vs = NULL ;
}

static void xmit_merges(state_t s, etime_t time) {
    switch (s->state) {
    case INTER_MERGING:
	if (!time_is_zero(time) &&
	    time_is_zero(s->timeout)) {
	    s->timeout = time_add(time, time_of_secs(10)) ;
	}
	if (!time_is_zero(time) &&
	    time_ge(time, s->timeout)) {
	    s->state = INTER_FAILED_MERGE ;
	    upnm(s, event_create(EVENT_MERGE_FAILED)) ;
	} else {
	    marsh_t marsh = marsh_of_buf(s->merge.abv) ;
	    marsh_view_state(marsh, s->vs) ;
	    marsh_enum(marsh, DATA, MAX) ;
	    assert(s->merge.event) ;
	    dn(s, event_copy(s->merge.event, s->vs->nmembers),
	       marsh) ;
	}
	break ;
    case INTER_INSTALLED_VIEW: {
	ofs_t i ;
	log(("ETimer:xmitting view prev_ids=%s",
	     view_id_array_to_string(s->next_vs->prev_ids, s->next_vs->nmembers))) ;
	for (i=0;i<s->nmergers;i++) {
	    view_state_t merge_vs = array_get(s->mergers, i) ;
	    view_contact_t contact = view_contact(merge_vs) ;
	    marsh_t msg = marsh_create(NULL) ;
	    marsh_view_state(msg, s->next_vs) ;
	    marsh_enum(msg, DATA, MAX) ;
	    log(("EView:with:%s", view_contact_to_string(&contact))) ;
	    dn(s, event_set_contact(event_create(EVENT_MERGE_GRANTED), contact), msg) ;
	}
    } break ;
    OTHERWISE_ABORT() ;
    }
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    header_type_t type UNUSED() = unmarsh_enum_ret(abv, MAX) ;
    switch (event_type(e)) {
    case EVENT_MERGE_REQUEST: {
	view_state_t merge_vs ;
	endpt_id_array_t merge_view ;
	bool_t seen_before ;
	ofs_t ofs ;
	assert(type == DATA) ;

	/* EMergeRequest (Coordinator): merge request from remote
	 * coordinator.
	 */
	unmarsh_view_state(abv, &merge_vs) ;
	merge_view = merge_vs->view ;
	
	seen_before = FALSE ;
	for (ofs=0;ofs<s->nmergers;ofs++) {
	    if (!endpt_array_disjoint(merge_view, merge_vs->nmembers,
				      array_get(s->mergers, ofs)->view,
				      array_get(s->mergers, ofs)->nmembers)) {
		seen_before = TRUE ;
	    }
	}

	if (s->ls->am_coord &&	/* I'm coord */
	    !s->blocked &&	/* haven't blocked yet */
	    !seen_before) {	/* no endpt that I've seen before */
	    /* Accept request.  Update logical time, add to
	     * lists of possibly previous view_ids and the endpts
	     * we've seen so far.  Then send on up.  
	     */
	    view_state_array_t tmp = s->mergers ;
	    log(("EMergeRequest:accepted:%s", endpt_id_array_to_string(merge_view, merge_vs->nmembers))) ;
	    s->mergers = view_state_array_appendi(s->mergers, s->nmergers++, merge_vs) ;
	    array_free(tmp) ;
	    e = event_set_mergers(e, view_state_copy(merge_vs)) ;
	    up(s, e, abv) ;
	} else {
	    ofs_t i ;
	    view_id_t merge_view_id = view_id_of_state(merge_vs) ;
	    for (i=0;i<s->nmergers;i++) {
		view_id_t merger_view_id = view_id_of_state(array_get(s->mergers, i)) ;
		if (view_id_eq(&merge_view_id, &merger_view_id)) {
		    break ;
		}
	    }
	    if (i == s->nmergers) {
		marsh_t msg = marsh_create(NULL) ;
		marsh_view_state(msg, merge_vs) ;
		marsh_enum(msg, DATA, MAX) ;

		/* Deny request.
		 */
		log(("EMergeRequest:rejected:%s%s%s%s",
		     endpt_id_array_to_string(merge_view, merge_vs->nmembers),
		     (!s->ls->am_coord ? ":not_coord" : ""),
		     (s->blocked ? ":blocked" : ""),
		     !seen_before ?  "" : ":seen_before")) ;
		dn(s, event_set_contact(event_create(EVENT_MERGE_DENIED), view_contact(merge_vs)), msg) ;
	    }
	    view_state_free(merge_vs) ;
	    up_free(e, abv) ;
	}
    } break ;

    case EVENT_MERGE_GRANTED: {
	view_state_t vs ;
	assert(type == DATA) ;
	unmarsh_view_state(abv, &vs) ;

	log(("EMergeGranted:prev_ids=%s",
	     view_id_array_to_string(vs->prev_ids, vs->nmembers))) ;

	/* EMergeGranted(Coordinator): Remote merging
	 * coordinator: new view arrived from coordinator.
	 * bounce off bottom.  
	 */
	/* BUG: also check that correct members are in the view. */
	if (s->ls->am_coord &&
	    s->state == INTER_MERGING &&
	    view_id_array_mem(&s->ls->view_id, vs->prev_ids, vs->nmembers)) {
	    log(("EMergeGranted:accepted")) ;
	    assert (vs->ltime > s->vs->ltime) ;

	    /* Install the view locally.
	     */
	    s->state = INTER_INSTALLED_VIEW ;
	    dnnm(s, event_set_view_state(event_create(EVENT_VIEW), vs)) ;
	} else {
	    log(("EMergeGranted:rejected%s%s%s",
		 s->ls->am_coord ? "" : ":not_coord",
		 s->state == INTER_MERGING ? "" : ":not_merging",
		 view_id_array_mem(&s->ls->view_id, vs->prev_ids, vs->nmembers) ? "" : ":not_in_prev_ids")) ;
	    if (!view_id_array_mem(&s->ls->view_id, vs->prev_ids, vs->nmembers)) {
		log(("EMergeGranted:view_id=%s prev_ids=%s",
		     view_id_to_string(&s->ls->view_id),
		     view_id_array_to_string(vs->prev_ids, vs->nmembers))) ;
	    }
	    view_state_free(vs) ;
	}
	up_free(e, abv) ;
    } break ;

    case EVENT_MERGE_DENIED: {
	view_state_t vs ;
	view_id_t view_id ;
	assert(type == DATA) ;
	unmarsh_view_state(abv, &vs) ;
	view_id = view_id_of_state(vs) ;
	view_state_free(vs) ; vs = NULL ;

	/* EMergeDenied: if the view state matches mine,
	 * then pass up an EMergeFailed event.
	 */
	if (s->ls->am_coord &&
	    s->state == INTER_MERGING &&
	    view_id_eq(&s->ls->view_id, &view_id)) {
	    log(("EMergeDenied(this view)")) ;
	    s->state = INTER_FAILED_MERGE ;
	    upnm(s, event_create(EVENT_MERGE_FAILED)) ;
	}
	up_free(e, abv) ;
    } break ;

    default:
	assert(type == NOHDR) ;
	up(s, e, abv) ;
	break; 
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_VIEW:
	s->state = INTER_INSTALLED_VIEW ;
	upnm(s, e) ;
	break ;

    case EVENT_BLOCK_OK:
	/* EBlockOk: mark state and pass on.
	 */
	s->blocked = TRUE ; 
	upnm(s, e) ;
	break ;
	
    case EVENT_TIMER: {
	etime_t time = event_time(e) ;

	if ((s->state == INTER_MERGING ||
	     (s->state == INTER_INSTALLED_VIEW && s->next_vs)) &&
	    time_ge(time, s->next_sweep)) {

	    /*log(("ETimer:state=%s", string_of_status(s->state)) ;*/

	    s->next_sweep = time_add(time, s->sweep) ;
	    dnnm(s, event_timer_time(s->next_sweep)) ;

	    xmit_merges(s, time) ;
	}
	upnm(s, e) ;
    } break ;

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    case EVENT_MERGE_REQUEST: {
	view_contact_t con ;
	/* EMergeRequest. Sends merge request.  Note that EMerge
	 * should only be done after zero failures and by the
	 * original coordinator of the group.  
	 */
	/* Check a bunch of assertions. */
	con = event_contact(e) ;
	assert(s->ls->am_coord) ;
	assert(!endpt_array_mem(&con.endpt, s->vs->view, s->vs->nmembers)) ;
	assert(s->state == INTER_NORMAL) ;

	log(("EMergeRequest:with:%s", endpt_id_to_string(&con.endpt))) ;
	s->state = INTER_MERGING ;
	s->merge.event = e ;
	s->merge.abv = marsh_to_buf(abv) ;
	marsh_free(abv) ;

	xmit_merges(s, time_zero()) ;

	dnnm(s, event_timer_time(time_zero())) ;
    } break ;

    default:
	marsh_enum(abv, NOHDR, MAX) ;
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    case EVENT_VIEW: {
	/* EView: Forward view to merging groups.
	 */
	assert(!s->next_vs) ;
	s->next_vs = view_state_copy(event_view_state(e)) ;
	assert (s->state != INTER_INSTALLED_VIEW) ;

	/* Calculate the new view id and add to event.
	 */
	s->state = INTER_INSTALLED_VIEW ;

	/* Send view state to merging coordinators (who will forward
	 * it for me).  
	 */
	log(("EView:view=%s", endpt_id_array_to_string(s->next_vs->view, s->next_vs->nmembers))) ;
	
	xmit_merges(s, time_zero()) ;

	/*log(("do_view:prev_ids=%s", view_id_array_to_string(new_vs->prev_ids)) ;*/

	dnnm(s, event_timer_time(time_zero())) ;

	/* Pass view to my partition.
	 */
	dnnm(s, e) ;
    } break ;

    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    view_state_array_free(s->mergers, s->nmergers) ;
    if (s->next_vs) {
	view_state_free(s->next_vs) ;
    }
    if (s->merge.event) {
	event_free(s->merge.event) ;
    }
    if (s->merge.abv) {
	marsh_buf_free(s->merge.abv) ;
    }
}

LAYER_REGISTER(inter) ;
