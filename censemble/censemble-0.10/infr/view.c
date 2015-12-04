/**************************************************************/
/* VIEW.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/view.h"
#include "infr/endpt.h"
#include "infr/group.h"
#include "infr/proto.h"
#include "infr/marsh.h"

#ifndef MINIMIZE_CODE
string_t view_id_to_string(const view_id_t *id) {
    return sys_sprintf("(%llu,%s)", id->ltime,
		       endpt_id_to_string(&id->endpt)) ;
}

string_t view_state_to_string(view_state_t vs) {
    return sys_sprintf("View.state{group=%s;coord=%d;view=%s;address=%s;ltime=%llu;prev_ids=%s;proto_id=%s;xfer_view=%d;new_key=%d;key=%s;clients=%s;primary=%d;lwe=%s;uptime=%s;quorum=%d}",
			group_id_to_string(&vs->group),
			vs->coord,
			endpt_id_array_to_string(vs->view, vs->nmembers),
			addr_id_array_to_string(vs->address, vs->nmembers),
			vs->ltime,
			view_id_array_to_string(vs->prev_ids, vs->nmembers),
			proto_id_to_string(vs->proto),
			vs->xfer_view,
			vs->new_key,
			"_"/*security_string_of_key(vs->key)*/,
			bool_array_to_string(vs->clients, vs->nmembers),
			vs->primary,
			"_"/*(arrayf_to_string to_string vs->lwe)*/,
			time_to_string(vs->uptime),
			vs->quorum) ; 
}
#endif

view_id_t view_id_of_state(view_state_t vs) {
    view_id_t view_id ;
    view_id.ltime = vs->ltime ;
    assert(vs->nmembers > 0) ;
    view_id.endpt = array_get(vs->view, 0) ;
    return view_id ;
}

view_local_t view_local(const endpt_id_t *endpt, view_state_t vs) {
    struct view_local_t *ls ;
    
    ls = record_create(struct view_local_t *, ls) ;
    record_clear(ls) ;
    
    if (!endpt_array_mem(endpt, vs->view, vs->nmembers)) {
	sys_panic(("local:endpt not in view")) ;
    }

    ls->rank = endpt_array_index(endpt, vs->view, vs->nmembers) ;
    ls->endpt = *endpt ;
    ls->addr = array_get(vs->address, ls->rank) ;
#ifdef MINIMIZE_CODE
    ls->name = string_copy("<>") ;
#else
    {
	string_t s ;
	s = endpt_id_to_string(endpt) ;
	ls->name = sys_sprintf("(%llu,%s,%d)", vs->ltime, s, ls->rank) ;
	string_free(s) ;
    }
#endif
    ls->addr = array_get(vs->address, ls->rank) ;
    ls->view_id = view_id_of_state(vs) ;
    ls->am_coord = (ls->rank == vs->coord) ;
    return ls ;
}

view_state_t view_singleton(
        proto_id_t proto,
        group_id_t group,
	endpt_id_t endpt,
	addr_id_t addr,
	ltime_t ltime,
	etime_t uptime
) {
    struct view_state_t *vs = record_create(struct view_state_t *, vs) ;
    view_id_t view_id ;
    endpt_id_array_t view ;
    addr_id_array_t address ;
    ltime_array_t out_of_date ;
    bool_array_t protos ;
    bool_array_t clients ;
    view_id_array_t prev_ids ;

    assert(ltime > LTIME_INVALID) ;
    view_id.ltime = 0 ;
    view_id.endpt = endpt ;

    array_singleton(endpt_id, endpt, &view) ;
    array_singleton(addr_id, addr, &address) ;
    array_singleton(bool, FALSE, &clients) ;
    array_singleton(ltime, 1, &out_of_date) ;
    array_singleton(bool, FALSE, &protos) ;
    array_singleton(view_id, view_id, &prev_ids) ;

    vs->nmembers = 1 ;
    vs->coord = 0 ;
    vs->group = group ;
    vs->version = version_id() ;
    vs->view = view ;
    vs->address = address ;
    vs->clients = clients ;
    vs->out_of_date = out_of_date ;
    vs->protos = protos ;
    vs->ltime = ltime ;
    /*vs->params = NULL ;*/
    vs->prev_ids = prev_ids ;
    vs->proto = proto_id_copy(proto) ;
    vs->xfer_view = FALSE ;
    vs->new_key = FALSE ;
    vs->primary = FALSE ;
    vs->groupd = FALSE ;
    vs->uptime = uptime ;
    vs->quorum = 0 ;
    return vs ;
}

view_state_t view_state_copy(view_state_t vs0) {
    /*struct view_state_t *vs1 = record_create(vs1) ;*/
    struct view_state_t *vs1 = sys_alloc(sizeof(*vs1)) ;
    *vs1 = *vs0 ;
    vs1->view = endpt_id_array_copy(vs0->view, vs0->nmembers) ;
    vs1->address = addr_id_array_copy(vs0->address, vs0->nmembers) ;
    vs1->prev_ids = view_id_array_copy(vs0->prev_ids, vs0->nmembers) ;
    vs1->clients = bool_array_copy(vs0->clients, vs0->nmembers) ;
    vs1->out_of_date = ltime_array_copy(vs0->out_of_date, vs0->nmembers) ;
    vs1->protos = bool_array_copy(vs0->protos, vs0->nmembers) ;
    vs1->proto = proto_id_copy(vs0->proto) ;
    return vs1 ;
}

void view_check(view_state_t vs) {
    /*int i ;*/
    /*eprintf("view_check:%s\n", view_state_to_string(vs)) ;*/
    assert(vs) ;
    assert(vs->view) ;
    assert_bool(vs->groupd) ;
    assert_bool(vs->primary) ;
    assert_bool(vs->new_key) ;
    assert_bool(vs->xfer_view) ;
    assert_array_length(vs->prev_ids, vs->nmembers) ;
    assert_array_length(vs->view, vs->nmembers) ;
    assert_array_length(vs->address, vs->nmembers) ;
    assert_array_length(vs->out_of_date, vs->nmembers) ;
    assert_array_length(vs->protos, vs->nmembers) ;
    assert_array_length(vs->clients, vs->nmembers) ;
}

view_contact_t view_contact(view_state_t vs) {
    view_contact_t con ;
    view_check(vs) ;
    assert(vs->nmembers > 0) ;
    con.addr = array_get(vs->address, 0) ;
    con.endpt = array_get(vs->view, 0) ;
    con.view_id = view_id_of_state(vs) ;
    return con ;
}

view_contact_t view_contact_copy(view_contact_t con0) {
    view_contact_t con1 ;
    con1 = con0 ;
    return con1 ;
}

view_state_t view_merge(view_state_array_t a, len_t len0) {
    struct view_state_t *vs1 ;
    ofs_t i, j, k ;
    len_t len1 ;

    endpt_id_array_t view ;
    addr_id_array_t address ;
    view_id_array_t prev_ids ;
    ltime_array_t out_of_date ;
    bool_array_t protos ;
    bool_array_t clients ;
    nmembers_t quorum = 0 ;

    assert_array_length(a, len0) ;
    assert(len0 >= 1) ;

    len1 = 0 ;
    for(i=0;i<len0;i++) {
	len1 += array_get(a, i)->nmembers ;
    }
    assert(len1 > 0) ;
    
    vs1 = record_create(struct view_state_t *, vs1) ;
    *vs1 = *array_get(a, 0) ;

    prev_ids = view_id_array_create(len1) ;
    view = endpt_id_array_create(len1) ;
    address = addr_id_array_create(len1) ;
    out_of_date = ltime_array_create(len1) ;
    protos = bool_array_create(len1) ;
    clients = bool_array_create(len1) ;

    k = 0 ;
    for (i=0;i<len0;i++) {
	view_state_t vs0 = array_get(a, i) ;
	if (vs0->quorum) {
	    if (quorum && quorum != vs0->quorum) {
		sys_panic(("VIEW:two different partitions are using different quorums (%d and %d)",
			   quorum, vs0->quorum)) ;
	    }
	    quorum = vs0->quorum ;
	}
	for (j=0;j<vs0->nmembers;j++) {
	    assert(k < len1) ;
	    vs1->ltime = ltime_max(vs1->ltime, vs0->ltime) ;
	    vs1->uptime = time_max(vs1->uptime, vs0->uptime) ;
	    array_set(view, k, array_get(vs0->view, j)) ;
	    array_set(address, k, array_get(vs0->address, j)) ;

	    /* Mark node up to date if it was in a primary view.
	     */
	    array_set(out_of_date, k, vs0->primary ? vs0->ltime : array_get(vs0->out_of_date, j)) ;

	    array_set(protos, k, array_get(vs0->protos, j)) ;
	    array_set(clients, k, array_get(vs0->clients, j)) ;
	    array_set(prev_ids, k, view_id_of_state(vs0)) ;
	    k ++ ;
	}
    }
    assert(k == len1) ;

    vs1->nmembers = len1 ;
    vs1->proto = proto_id_copy(array_get(a, 0)->proto) ;
    vs1->ltime ++ ;
    vs1->prev_ids = prev_ids ;
    vs1->view = view ;
    vs1->address = address ;
    vs1->out_of_date = out_of_date ;
    vs1->protos = protos ;
    vs1->clients = clients ;
    vs1->quorum = quorum ;
    view_check(vs1) ;
    return vs1 ;
}

view_state_t view_fail(view_state_t vs0, const_bool_array_t fail) {
    len_t len0 = vs0->nmembers ;
    len_t len1 ;
    ofs_t i, j ;
    struct view_state_t *vs1 ;

    endpt_id_array_t view ;
    addr_id_array_t address ;
    ltime_array_t out_of_date ;
    bool_array_t protos ;
    bool_array_t clients ;
    view_id_array_t prev_ids ;

    len1 = 0 ;
    for (i=0;i<len0;i++) {
	if (!array_get(fail, i)) {
	    len1 ++ ;
	}
    }
    assert(len1 > 0) ;
    
    vs1 = record_create(struct view_state_t *, vs1) ;
    *vs1 = *vs0 ;

    view = endpt_id_array_create(len1) ;
    address = addr_id_array_create(len1) ;
    out_of_date = ltime_array_create(len1) ;
    protos = bool_array_create(len1) ;
    clients = bool_array_create(len1) ;
    prev_ids = view_id_array_create(len1) ;

    for (i=0,j=0;i<len0;i++) {
	if (array_get(fail, i)) {
	    continue ;
	}
	assert(j < len1) ;
	array_set(view, j, array_get(vs0->view, i)) ;
	array_set(address, j, array_get(vs0->address, i)) ;
	array_set(out_of_date, j, array_get(vs0->out_of_date, i)) ;
	array_set(protos, j, array_get(vs0->protos, i)) ;
	array_set(clients, j, array_get(vs0->clients, i)) ;
	array_set(prev_ids, j, array_get(vs0->prev_ids, i)) ;
	j++ ;
    }
    assert(j == len1) ;
   
    vs1->nmembers = len1 ;
    vs1->view = view ;
    vs1->address = address ;
    vs1->out_of_date = out_of_date ;
    vs1->protos = protos ;
    vs1->clients = clients ;
    vs1->proto = proto_id_copy(vs0->proto) ;
    vs1->prev_ids = prev_ids ;
    view_check(vs1) ;
    return vs1 ;
}

void view_state_free(view_state_t vs) {
    array_free(vs->view) ;
    array_free(vs->address) ;
    array_free(vs->out_of_date) ;
    array_free(vs->protos) ;
    array_free(vs->clients) ;
    array_free(vs->prev_ids) ;
    proto_id_free(vs->proto) ;
    record_free(vs) ;
}

void view_local_free(view_local_t ls) {
    string_free(ls->name) ;
    record_free(ls) ;
}

int view_id_cmp(const view_id_t *id0, const view_id_t *id1) {
    if (id0->ltime != id1->ltime) {
	return (id0->ltime > id1->ltime) ? 1 : -1 ;
    }
    return endpt_id_cmp(&id0->endpt, &id1->endpt) ;
}

inline bool_t view_id_eq(const view_id_t *id0, const view_id_t *id1) {
    return !view_id_cmp(id0, id1) ;
}

bool_t view_id_option_eq(const view_id_t *id0, const view_id_t *id1) {
    if (!id0 && !id1) {
	return TRUE ;
    }
    if (!id0 || !id1) {
	return FALSE ;
    }
    return view_id_eq(id0, id1) ;
}

bool_t view_id_array_mem(const view_id_t *id, view_id_array_t a, len_t len) {
    ofs_t i ;
    assert_array_length(a, len) ;
    for (i=0;i<len;i++) {
	if (view_id_eq(id, &array_get(a, i))) {
	    return TRUE ;
	}
    }
    return FALSE ;
}

view_state_array_t view_state_array_appendi(view_state_array_t a0, len_t len, view_state_t vs) {
    view_state_array_t a1 ;
    ofs_t i ;
    assert_array_length(a0, len) ;
    a1 = view_state_array_create(len + 1) ;
    for (i=0;i<len;i++) {
	array_set(a1, i, array_get(a0, i)) ;
    }
    array_set(a1, len, vs) ;
    return a1 ;
}

view_state_array_t view_state_array_prependi(view_state_t vs, view_state_array_t a0, len_t len) {
    view_state_array_t a1 ;
    ofs_t i ;
    assert_array_length(a0, len) ;
    a1 = view_state_array_create(len + 1) ;
    array_set(a1, 0, vs) ;
    for (i=0;i<len;i++) {
	array_set(a1, i + 1, array_get(a0, i)) ;
    }
    return a1 ;
}

bool_t view_state_eq(view_state_t vs0, view_state_t vs1) {
    return vs0->ltime == vs1->ltime ; /* BUG */
}

#ifndef MINIMIZE_CODE
string_t view_contact_to_string(const view_contact_t *c) {
    return sys_sprintf("Contact{endpt=%s;view_id=%s;addr=%s}",
			endpt_id_to_string(&c->endpt),
			view_id_to_string(&c->view_id),
			/*addr_id_to_string(c->addr)*/"_") ;
}
#endif

void marsh_view_id(marsh_t m, const view_id_t *id) {
    marsh_ltime(m, id->ltime) ;
    marsh_endpt_id(m, &id->endpt) ;
}

void unmarsh_view_id(unmarsh_t m, view_id_t *ret) {
    view_id_t id ;
    unmarsh_endpt_id(m, &id.endpt) ;
    unmarsh_ltime(m, &id.ltime) ;
    *ret = id ;
}

#include "infr/array_supp.h"
ARRAY_CREATE(view_id)
ARRAY_PTR_MARSH(view_id, view_id)

void marsh_view_state(marsh_t m, view_state_t vs) {
    view_check(vs) ;
    marsh_version_id(m, vs->version) ;
    marsh_group_id(m, &vs->group) ;
    marsh_rank(m, vs->coord) ;
    marsh_ltime(m, vs->ltime) ;
    marsh_rank(m, vs->quorum) ;
    marsh_bool(m, vs->primary) ;
    marsh_proto_id(m, vs->proto) ;
    marsh_bool(m, vs->groupd) ;
    marsh_bool(m, vs->xfer_view) ;
    marsh_bool(m, vs->new_key) ;
    marsh_time(m, vs->uptime) ;
    marsh_endpt_id_array(m, vs->view, vs->nmembers) ;
    marsh_addr_id_array(m, vs->address, vs->nmembers) ;
    marsh_ltime_array(m, vs->out_of_date, vs->nmembers) ;
    marsh_bool_array(m, vs->protos, vs->nmembers) ;
    marsh_bool_array(m, vs->clients, vs->nmembers) ;
    marsh_view_id_array(m, vs->prev_ids, vs->nmembers) ;
    marsh_rank(m, vs->nmembers) ;
}

void unmarsh_view_state(unmarsh_t m, view_state_t *ret) {
    struct view_state_t *vs = record_create(struct view_state_t *, vs) ;
    unmarsh_rank(m, &vs->nmembers) ;
    unmarsh_view_id_array(m, &vs->prev_ids, vs->nmembers) ;
    unmarsh_bool_array(m, &vs->clients, vs->nmembers) ;
    unmarsh_bool_array(m, &vs->protos, vs->nmembers) ;
    unmarsh_ltime_array(m, &vs->out_of_date, vs->nmembers) ;
    unmarsh_addr_id_array(m, &vs->address, vs->nmembers) ;
    unmarsh_endpt_id_array(m, &vs->view, vs->nmembers) ;
    unmarsh_time(m, &vs->uptime) ;
    unmarsh_bool(m, &vs->new_key) ;
    unmarsh_bool(m, &vs->xfer_view) ;
    unmarsh_bool(m, &vs->groupd) ;
    unmarsh_proto_id(m, &vs->proto) ;
    unmarsh_bool(m, &vs->primary) ;
    unmarsh_rank(m, &vs->quorum) ;
    unmarsh_ltime(m, &vs->ltime) ;
    unmarsh_rank(m, &vs->coord) ;
    unmarsh_group_id(m, &vs->group) ;
    unmarsh_version_id(m, &vs->version) ;
    view_check(vs) ;
    *ret = vs ;
}

void marsh_view_contact(marsh_t m, const view_contact_t *c) {
    marsh_endpt_id(m, &c->endpt) ;
    marsh_addr_id(m, &c->addr) ;
    marsh_view_id(m, &c->view_id) ;
}

void unmarsh_view_contact(unmarsh_t m, view_contact_t *ret) {
    view_contact_t c ;
    record_clear(&c) ;
    unmarsh_view_id(m, &c.view_id) ;
    unmarsh_addr_id(m, &c.addr) ;
    unmarsh_endpt_id(m, &c.endpt) ;
    *ret = c ;
}

void view_state_unmarsh(marsh_t m, view_state_t *vs) {
    sys_abort() ;
}

ARRAY_COPY(view_id)
ARRAY_PTR_TO_STRING(view_id)
ARRAY_FREE(view_state)
ARRAY_CREATE(view_state)
