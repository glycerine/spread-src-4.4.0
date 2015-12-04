/**************************************************************/
/* TOP.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
/* State transitions for this layer:

   OLD_STATE:
     NEW_STATE: event

   Normal:
     Merging: EBlockOk
     NextView: EView, EBlockOk

   Merging:
     NextView: EView, EMergeFailed, EMergeDenied

   NextView:
     *sink*
*/
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "TOP" ;

typedef enum {
    TOP_NORMAL,
    TOP_MERGING,
    TOP_NEXT_VIEW,
} status_t ;

typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;

    /*policy		     : (Endpt.id * Addr.set -> bool) option ; *//* Authorization policy */
    etime_t sweep ;		/* gossip sweep interval (if any) */
    bool_t gossip ;		/* initiate EGossipExt events? */
    bool_t account ;		/* initiate EAccount events? */
    bool_t dump_linger ;	/* dump when this stack has been lingering */
    bool_t dump_fail ;		/* fail on dump events */
    status_t status ;		/* state of the protocol */
    bool_t elected ;		/* have I been elected */
    etime_t next_sweep ;		/* time of next sweep */
    bool_array_t suspects ;	/* suspected members */
    bool_array_t failed ;	/* failed members */
    view_state_array_t mergers ; /* Mergers for next view */
    len_t nmergers ;
    bool_t dn_block ;		/* I've done a EBlock */
    bool_t up_prompt ;		/* I've been prompted */
    bool_t leaving ;		/* Are we leaving? */
    bool_t dn_block_abv ;	/* EBlock was from above! */
    bool_t up_block_ok ;	/* I've recd an EBlockOk */
    etime_t dbg_block_time ;	/* time when I blocked */
    etime_t dbg_block_time_first ; /* time when I blocked */
} *state_t ;

#include "infr/layer_supp.h"

#if 0
static string_t string_of_status(status_t status) {
    string_t ret ;
    switch(status) {
    case TOP_NORMAL:
	ret = "normal" ;
	break ;
    case TOP_MERGING:
	ret = "merging" ;
	break ;
    case TOP_NEXT_VIEW:
	ret = "next_view" ;
	break ;
    OTHERWISE_ABORT() ;
    }
    return ret ;
}
#endif

#ifndef MINIMIZE_CODE
static void dump(state_t s) {
    sys_abort() ;
#if 0
    sprintf "state=%s leaving=%b\n" (string_of_state s.state) s.leaving ;
    sprintf "dn_block=%b elected=%b\n" s.dn_block s.elected ;
    sprintf "failed=%s\n" (Arrayf.bool_to_string s.failed) ;
    sprintf "dbg_block_time_first=%s\n" (Time.to_string s.dbg_block_time_first)
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
    /*s->policy	= s.exchange;*/
    s->sweep	     = time_of_secs(1) ;
    s->gossip	     = TRUE ;
    s->account	     = TRUE ;
    s->dump_linger   = FALSE ;
    s->dump_fail     = FALSE ;
    s->status	     = TOP_NORMAL ;
    s->failed	     = bool_array_create_init(vs->nmembers, FALSE) ;
    s->suspects      = bool_array_create_init(vs->nmembers, FALSE) ;
    s->leaving	     = FALSE ;
    s->dn_block	     = FALSE ;
    s->dn_block_abv  = FALSE ;
    s->up_prompt     = FALSE ;
    s->up_block_ok   = FALSE ;
    s->mergers	     = array_empty(view_state) ;
    s->nmergers      = 0 ;
    s->next_sweep    = time_zero() ;
    s->elected	     = ls->am_coord ;
    s->dbg_block_time = time_zero() ;
    s->dbg_block_time_first = time_zero() ;
}


static void do_block(state_t s) {
    /* Checking am_coord is a hack for singleton's leaving
     * a group.
      */
    if ((s->ls->am_coord || s->elected) &&
	!s->dn_block) {
	if (s->status == TOP_MERGING) { 
	    sys_panic(("merging w/o EBlock")) ;
	}
	s->dn_block = TRUE ;
	dnnm(s, event_create(EVENT_BLOCK)) ;
    }
    log(("blocking")) ;
}

static void do_fail(state_t s) {
    event_t e ;
    const_bool_array_t tmp ;
    if (!s->elected ||
	bool_array_super(s->failed, s->suspects, s->vs->nmembers)) {
	return ;
    }
    do_block(s) ;
    tmp = s->failed ;
    s->failed = bool_array_map_or(s->failed, s->suspects, s->vs->nmembers) ;
    bool_array_copy_into(s->suspects, s->failed, s->vs->nmembers) ;
    array_free(tmp) ;
    e = event_create(EVENT_FAIL) ;
    e = event_set_failures(e, bool_array_copy(s->failed, s->vs->nmembers)) ;
    dnnm(s, e) ;
}

static void do_view(state_t s) {
    view_state_t vs ;
    view_state_t tmp ;

    log_sync(("delivering EView")) ;
    if (!s->elected) {
	sys_panic(("do_view when not coord")) ;
    }

    /* Remove failed members from the view.
     */
    vs = view_fail(s->vs, s->failed) ;
    tmp = vs ;
    /* Merge new members into the view.
     */ 

    {
	view_state_array_t views = view_state_array_prependi(vs, s->mergers, s->nmergers) ;
	vs = view_merge(views, s->nmergers + 1) ;
	array_free(views) ;
    }

    view_state_free(tmp) ;

    {
	event_t e ;
	e = event_create(EVENT_VIEW) ;
	e = event_set_view_state(e, vs) ;
	dnnm(s, e) ;
    }
    s->status = TOP_NEXT_VIEW ;
}

static void up_handler(state_t s, event_t e, unmarsh_t msg) {
    switch (event_type(e)) {

    case EVENT_MERGE_REQUEST:
	if (s->status != TOP_NEXT_VIEW) {
	    view_state_t mergers = event_mergers(e) ;
	    ofs_t i ;
	    for (i=0;i<s->nmergers;i++) {
		if (!endpt_array_disjoint(mergers->view, mergers->nmembers, 
					  array_get(s->mergers, i)->view,
					  array_get(s->mergers, i)->nmembers)) {
		    break ;
		}
	    }
	    if (i == s->nmergers &&
		endpt_array_disjoint(s->vs->view, s->vs->nmembers, 
				     mergers->view, mergers->nmembers)) {
		view_state_array_t tmp = s->mergers ;
		s->mergers = view_state_array_appendi(s->mergers, s->nmergers++,
						      view_state_copy(mergers)) ;
		array_free(tmp) ;
		log(("%s", event_to_string(e))) ;
		do_block(s) ;
	    }
	}
	up_free(e, msg) ;
	break ;

    OTHERWISE_ABORT() ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_VIEW:
	log_sync(("got EView")) ;
	log(("%s", event_to_string(e))) ;
	s->status = TOP_NEXT_VIEW ;
	upnm(s, e) ;
	break ;

    case EVENT_LEAVE:
	/* Don't pass the leave event up.  The do_block
	 * is necessary in case we are a singleton group.
	 * The suspicions will not necessarily cause a
	 * view change.  The Exit is needed in case we
	 * are running with the groupd.
	 */
	do_block(s) ;
	s->leaving = TRUE ;
	dnnm(s, event_create(EVENT_EXIT)) ;
	event_free(e) ;
	break ;

    case EVENT_FAIL: {
	const_bool_array_t tmp ;
	log(("Fail:%s", bool_array_to_string(event_failures(e), s->vs->nmembers))) ;
	tmp = s->failed ;
	s->failed = bool_array_map_or(s->failed, event_failures(e), s->vs->nmembers) ;
	assert(!array_get(s->failed, s->ls->rank)) ;
	array_free(tmp) ;
	tmp = s->suspects ;
	s->suspects = bool_array_map_or(s->suspects, s->failed, s->vs->nmembers) ;
	array_free(tmp) ;
	event_free(e) ;
    } break ;

    case EVENT_MERGE_FAILED:
    case EVENT_MERGE_DENIED: {
	/* Should do this only if it was the coordinator,
	 * otherwise, should just fail the member. [MH: ???]
	 */
	if (s->status == TOP_MERGING) {
	    do_view(s) ;
	}
	event_free(e) ;
    } break ;

    case EVENT_SUSPECT: {
	const_bool_array_t tmp ;
	log(("%s", event_to_string(e))) ;
	tmp = s->suspects ;
	assert(!array_get(s->suspects, s->ls->rank)) ;
	assert(!array_get(event_suspects(e), s->ls->rank)) ;
	s->suspects = bool_array_map_or(s->suspects, event_suspects(e), s->vs->nmembers) ;
	array_free(tmp) ;
	assert(!array_get(s->suspects, s->ls->rank)) ;
	if (s->elected) {
	    do_fail(s) ;
	}
	event_free(e) ;
    } break ;

    case EVENT_BLOCK:
	dnnm(s, event_create(EVENT_BLOCK_OK)) ;
	event_free(e) ;
	break ;

    case EVENT_BLOCK_OK:
	log(("%s", event_to_string(e))) ;
	s->up_block_ok = TRUE ;

	/* If we got a EBlock from above (from the groupd),
	 * then we pass the EBlockOk up.  Otherwise we
	 * generate an EView or an EMerge event.  
	 */
	if (s->dn_block_abv) {
	    upnm(s, e) ;
	} else {
	    if (!s->dn_block) {
		sys_panic(("EBlockOk w/o EBlock")) ;
	    }

	    if (s->status == TOP_NORMAL) {
		/* Check if merging.	
		 */
		if (event_has_contact(e) &&
		    !bool_array_exists(s->failed, s->vs->nmembers) &&
		    s->nmergers == 0) {
		    event_t e2 ;
		    log(("sending MergeRequest")) ;
		    e2 = event_create(EVENT_MERGE_REQUEST) ;
		    e2 = event_set_contact(e2, event_contact(e)) ;
		    dn(s, e2, marsh_create(NULL)) ;
		    s->status = TOP_MERGING ;
		} else {
		    do_view(s) ;
		}
	    }
	    event_free(e) ;
	}
	break ;

    case EVENT_TIMER: {
	etime_t time = event_time(e) ;
	if (time_ge(time, s->next_sweep)) {
	    s->next_sweep = time_add(time, s->sweep) ;
	    dnnm(s, event_timer_time(s->next_sweep)) ;
	    if (s->ls->am_coord && s->gossip) {
		dnnm(s, event_create(EVENT_GOSSIP_EXT)) ;
	    }
	    if (s->account) {
		dnnm(s, event_create(EVENT_ACCOUNT)) ;
	    }
	}

	if (s->status == TOP_NORMAL) {
	    s->dbg_block_time = time ;
	} else {
	    /*
	      if s.dump_linger && Time.gt time (Time.add s.dbg_block_time (Time.of_int 200)) then (
	      if s.dbg_block_time_first = Time.invalid then
	      s.dbg_block_time_first <- s.dbg_block_time ;
	      eprintf "TOP:lingering stack, time=%s\n" (Time.to_string time) ;
	      s.dbg_block_time <- time ;
	      dump vf s ;
	      dnnm (create name EDump[]) ;
	      )
	    */
	}
	event_free(e) ;
    } break ;

    case EVENT_PROTO:
	/* Just here for view change perf tests.
	 */
	upnm(s, event_create(EVENT_PROMPT)) ;
	event_free(e) ;
	break ;

    case EVENT_PROMPT:
	do_block (s) ;
	s->up_prompt = TRUE ;
	/*upnm(s, e) ;*/
	event_free(e) ;
	break ;

    case EVENT_ELECT:
	s->elected = TRUE ;
	dnnm(s, event_timer_time(time_zero())) ;

	/* If prompted, then start blocking.
	 */
	if (s->up_prompt) {
	    do_block(s) ;
	}

	do_fail(s) ;
	event_free(e) ;
	break ;

    case EVENT_ACCOUNT:
	/*
	  logb (fun () -> [
	  sprintf "view=%s" (View.to_string vs.view) ;
	  sprintf "state=%s dn_block=%b elected=%b leaving=%b" 
	  (string_of_state s.state) s.dn_block s.elected s.leaving ;
	  sprintf "failed=%s" (Arrayf.bool_to_string s.failed)
	  ]) ;
	*/
	event_free(e) ;
	break ;

#ifndef MINIMIZE_CODE
    case EVENT_DUMP:
	dump(s) ;
	if (s->dump_fail) {
	    sys_panic(("got EDump:exiting")) ;
	}
	event_free(e) ;
	break ;
#endif

    case EVENT_INIT:
	/* If there are any unauthorized members then remove them.
	 * Return true if this is the case --- we remove them.
	 */
	/*
	  let suspicions =
	  Arrayf.map2 (fun e addr -> not (check_policy s (e,addr))) 
	  vs.view vs.address
	  in
	  if Arrayf.exists (fun i x -> x) suspicions then (
	  log (fun () -> sprintf "removing untrusted members:%s" 
	  (string_of_bool_array (Arrayf.to_array suspicions))
	  ) ;
	  dnnm (suspectReason name suspicions name)
	*/
	event_free(e) ;
	break; 

    case EVENT_EXIT:
	upnm(s, e) ;
	break ;
	
    case EVENT_ASYNC:
    case EVENT_STABLE:
    case EVENT_GOSSIP_EXT:
    case EVENT_XFER_DONE:
    case EVENT_LOST_MESSAGE:
    case EVENT_STABLE_REQ:
    case EVENT_SYNC_INFO:
    case EVENT_PRESENT:
    case EVENT_REKEY: 
	event_free(e) ;
	break ;

    default:
#ifndef MINIMIZE_CODE
	eprintf("%s:dropping:upnm:%s\n", name, event_type_to_string(event_type(e))) ;
#endif
	event_free(e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    sys_abort() ;
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    case EVENT_EXIT:
	dnnm(s, e) ;
	break ;

    case EVENT_FAIL:
	/* The group daemon is not so good about sending
	 * monotonically increasing failures.
	 */
	assert(bool_array_super(event_failures(e), s->failed, s->vs->nmembers)) ;
	bool_array_copy_into(s->failed, event_failures(e), s->vs->nmembers) ;
	assert(!array_get(s->failed, s->ls->rank)) ;
	dnnm(s, event_set_failures(e, s->failed)) ;
	break ;
	
    case EVENT_BLOCK:
	log(("EBlock")) ;
	/* This was causing problems with big pbcast tests...
	 */
	if (s->dn_block_abv) {
	    sys_panic(("warning:got 2nd EBlock from above")) ;
	    /*sys_panic(("warning:got 2nd EBlock from above, sending EBlockOk (hack!)")) ;*/
	    upnm(s, event_create(EVENT_BLOCK_OK)) ; /* hack! */
	} else {
	    s->dn_block_abv = TRUE ;
	    if (s->dn_block) {
		sys_panic(("EBlock from above when blocked")) ;
	    }
	    s->dn_block = TRUE ;
	    dnnm(s, event_create(EVENT_BLOCK)) ;
	}
	
    OTHERWISE_ABORT() ;
    }
}

static void free_handler(state_t s) {
    array_free(s->failed) ;
    array_free(s->suspects) ;
    view_state_array_free(s->mergers, s->nmergers) ;
}

LAYER_REGISTER(top) ;
