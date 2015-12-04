/**************************************************************/
/* CONN.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/conn.h"
#include "infr/view.h"

static string_t name UNUSED() = "CONN" ;

/* This should uniquely specify a connection set.
 */
struct conn_key_t {
    version_id_t version ;
    endpt_id_t endpt ;
    group_id_t group ;
    stack_id_t stack ;
    proto_id_t proto ;
    view_id_t view_id ;
} ;

/* Set of connection identifiers being used to communicate
 * with peers.
 */
struct conn_t {
    conn_key_t key ;
    rank_t rank ;
    nmembers_t nmembers ;
} ;

void conn_hash_of_id(const conn_id_t *id, md5_t *md5) {
    marsh_t m = marsh_create(NULL) ;
    marsh_version_id(m, id->version) ;
    marsh_group_id(m, &id->group) ;
    marsh_stack_id(m, id->stack) ;
    if (id->proto_option) {
	marsh_bool(m, TRUE) ;
	marsh_proto_id(m, id->proto_option) ;
    } else {
	marsh_bool(m, FALSE) ;
    }
    if (id->view_id_option) {
	marsh_bool(m, TRUE) ;
	marsh_view_id(m, id->view_id_option) ;
    } else {
	marsh_bool(m, FALSE) ;
    }
    marsh_rank(m, id->sndr_mbr) ;
    marsh_rank(m, id->dest_mbr) ;
    if (id->dest_endpt_option) {
	marsh_bool(m, TRUE) ;
	marsh_endpt_id(m, id->dest_endpt_option) ;
    } else {
	marsh_bool(m, FALSE) ;
    }
    marsh_md5_calc(m, md5) ;
    marsh_free(m) ;
}

#ifndef MINIMIZE_CODE
string_t conn_kind_to_string(conn_kind_t kind) {
    switch(kind) {
    case CONN_CAST: return "Cast" ;
    case CONN_SEND: return "Send" ;
    case CONN_ASYNC: return "Async" ;
    case CONN_OTHER: return "Other" ;
    OTHERWISE_ABORT() ;
    }
}
#endif

static inline
conn_mbr_t mbr_none(void) {
    return (conn_mbr_t)(-1) ; 
}

#if 0
static inline
bool_t mbr_is_none(conn_mbr_t mbr) {
    return (mbr == (conn_mbr_t)(-1)) ;
}
#endif

/*
let squash_sender id = 
  let sender = mbr_of_int id.sndr_mbr in
  let id = { id with sndr_mbr = -2
  (sender,id)
*/

/* When digesting the marshalled object, we can skip
 * the marshalling header.
 */
/*
let hash_of_id id =
  let id = Marshal.to_string id no_sharing in
  let len = String.length id in
  Buf.digest_substring id Marshal.header_size (len - Marshal.header_size)
*/

#ifndef MINIMIZE_CODE
string_t conn_id_to_string(conn_id_t *id) {
    md5_t md5 ;
    string_t proto ;
/*
 = Trace.debug "" (fun id ->
  let (view_id,sndr_mbr,dest_mbr,dest_endpt) =
    id.view_id,id.sndr_mbr,id.dest_mbr,id.dest_endpt
  in
  let kind = match id.view_id,mbr_of_int id.sndr_mbr,mbr_of_int id.dest_mbr,id.dest_endpt with
    | Some _, Some _, Some _, None   -> "Pt2pt"
    | Some _, Some _, None  , None   -> "Cast"
    | Some _, None  , None  , Some _ -> "Merge"
    | None  , None  , None  , None   -> "Gossip"
    | _ -> "?" 
  in
*/
    conn_hash_of_id(id, &md5) ;
    if (id->proto_option) {
	proto = sys_sprintf("Some(%s)", proto_id_to_string(id->proto_option)) ;
    } else {
	proto = string_copy("None") ;
    }

    return 
	sys_sprintf("{Conn:%s:%s:%s:%s:from=%d:to=%d/%s:hash=%s}",
		    group_id_to_string(&id->group),
		    stack_id_to_string(id->stack),
		    proto,
		    id->view_id_option ? view_id_to_string(id->view_id_option) : "None",
		    id->sndr_mbr, id->dest_mbr,
		    id->dest_endpt_option ? endpt_id_to_string(id->dest_endpt_option) : "None", 
		    hex_of_bin(&md5, sizeof(md5))) ;
}

string_t conn_recv_info_to_string(conn_recv_info_t info) {
    return sys_sprintf("{Conn_recv_info:rank=%d:id=%s}",
		       info->rank,
		       conn_id_to_string(&info->id)) ;
}
#endif

/*
let string_of_key key =
  sprintf "{Conn.t.id:ver=%s;endpt=%s;group=%s;view_id=%s;stack=%s;proto=%s}" 
    (Version.string_of_id key.key_version)
    (Endpt.string_of_id key.key_endpt)
    (Group.string_of_id key.key_group)
    (View.string_of_id key.key_view_id)
    (Stack_id.string_of_id key.key_stack)
    (string_of_option Proto.string_of_id key.key_proto)

let string_of_t t = string_of_key t.key
*/

/* Create a record of connections for an id & view.
 */

conn_t conn_create(
        rank_t rank,
	version_id_t version,
	group_id_t group,
	view_id_t view_id,
	stack_id_t stack_id,
	proto_id_t proto_id,
	endpt_id_array_t view,
	len_t nmembers
) {
    endpt_id_t endpt = array_get(view, rank) ;
    conn_key_t key = record_create(conn_key_t, key) ;
    conn_t conn = record_create(conn_t, conn) ;
    key->endpt = endpt ;
    key->stack = stack_id ;
    key->group = group ;
    key->version = version ;
    key->view_id = view_id ;
    key->proto = proto_id ;
    conn->key = key ;
    conn->rank = rank ;
    conn->nmembers = nmembers ;
    return conn ;
}

void conn_free(conn_t conn) {
    record_free(conn->key) ;
    record_free(conn) ;
}

conn_id_t conn_id(
        version_id_t version,
	group_id_t group,
	stack_id_t stack,
	proto_id_t proto,
	view_id_t *view_id,
	conn_mbr_t sndr_mbr,
	conn_mbr_t dest_mbr,
	conn_dest_endpt_t dest_endpt
) {
    conn_id_t id ;
    id.version = version ;
    id.group = group ;
    id.stack = stack ;
    id.proto_option = proto ;
    id.view_id_option = view_id ;
    id.sndr_mbr = sndr_mbr ;
    id.dest_mbr = dest_mbr ;
    id.dest_endpt_option = dest_endpt ;
    return id ;
}

static conn_id_t idgen(
        conn_t conn,
	proto_id_t proto,
	view_id_t *view_id,
	conn_mbr_t sndr_mbr,
	conn_mbr_t dest_mbr,
	conn_dest_endpt_t dest_endpt
) {
    return conn_id(conn->key->version,
		   conn->key->group,
		   conn->key->stack,
		   proto,
		   view_id,
		   sndr_mbr,
		   dest_mbr,
		   dest_endpt) ;
}

bool_t conn_id_eq(conn_id_t *id0, conn_id_t *id1) {
#if 0
    eprintf("CONN_ID_EQ:%d:%d:%d:%d:%d:%d:%d:%d\n",
	    version_id_eq(&id0->version, &id1->version),
	    group_id_eq(&id0->group, &id1->group),
	    stack_id_eq(id0->stack, id1->stack),
	    proto_id_option_eq(id0->proto_option, id1->proto_option),
	    view_id_option_eq(id0->view_id_option, id1->view_id_option),
	    (id0->sndr_mbr == id1->sndr_mbr),
	    (id0->dest_mbr == id1->dest_mbr),
	    endpt_id_option_eq(id0->dest_endpt_option,
			       id1->dest_endpt_option)) ;
#endif
    return (version_id_eq(&id0->version, &id1->version) &&
	    group_id_eq(&id0->group, &id1->group) &&
	    stack_id_eq(id0->stack, id1->stack) &&
	    proto_id_option_eq(id0->proto_option, id1->proto_option) &&
	    view_id_option_eq(id0->view_id_option, id1->view_id_option) &&
	    (id0->sndr_mbr == id1->sndr_mbr) &&
	    (id0->dest_mbr == id1->dest_mbr) &&
	    endpt_id_option_eq(id0->dest_endpt_option,
			       id1->dest_endpt_option)) ;
}

conn_id_t conn_merge_recv(conn_t c) {
    view_id_t *vid ;
#if 0 /* Watch out!  This impacts transport_async() */
    vid = record_create(view_id_t *, vid) ;
    *vid = c->key->view_id ;
#else
    vid = NULL ;
#endif
    return idgen(c, c->key->proto, vid, mbr_none(), mbr_none(), &c->key->endpt) ;
}

conn_id_t conn_multi_recv(conn_t c, rank_t rank) {
    view_id_t *vid = record_create(view_id_t *, vid) ;
    *vid = c->key->view_id ;
    return idgen(c, c->key->proto, vid, rank, mbr_none(), NULL) ;
}

conn_id_t conn_pt2pt_recv(conn_t c, rank_t rank) {
    view_id_t *vid = record_create(view_id_t *, vid) ;
    *vid = c->key->view_id ;
    return idgen(c, c->key->proto, vid, rank, c->rank, NULL) ;
}

conn_id_t conn_gossip(conn_t c) {
    return idgen(c, NULL, NULL, mbr_none(), mbr_none(), NULL) ;
}

conn_id_t conn_multi_send(conn_t c) {
    view_id_t *vid = record_create(view_id_t *, vid) ;
    *vid = c->key->view_id ;
    return idgen(c, c->key->proto, vid, c->rank, mbr_none(), NULL) ;
}

conn_id_t conn_pt2pt_send(conn_t c, rank_t rank) {
    view_id_t *vid = record_create(view_id_t *, vid) ;
    *vid = c->key->view_id ;
    return idgen(c, c->key->proto, vid, c->rank, rank, NULL) ;
}

conn_id_t conn_merge_send(conn_t c, view_id_t view_id, conn_dest_endpt_t dest) {
    view_id_t *vid ;
#if 0
    vid = record_create(view_id_t *, vid) ;
    *vid = view_id ;
#else
    vid = NULL ;
#endif
#if 0
    {
	conn_id_t tmp ;
	tmp = idgen(c, c->key->proto, vid, mbr_none(), mbr_none(), dest) ;
	eprintf("conn:%s\n", conn_id_to_string(&tmp)) ;
	return tmp ;
    }
#else
    return idgen(c, c->key->proto, vid, mbr_none(), mbr_none(), dest) ;
#endif
}

conn_key_t conn_key(conn_t c) {
    return c->key ;
}

static conn_recv_info_t recv_info(
        conn_id_t id,
	conn_kind_t kind,
	rank_t rank
) {
    conn_recv_info_t info = record_create(conn_recv_info_t, info) ;
    /*eprintf("recv:%s\n", conn_id_to_string(&id)) ;*/
    info->id = id ;
    info->kind = kind ;
    info->rank = rank ;
    return info ;
}

conn_recv_info_array_t conn_recv_info_array_create(len_t) ;

conn_recv_info_array_t conn_all_recv(conn_t c, bool_t scaled, len_t *len_ret) {
    conn_recv_info_array_t info ;
    nmembers_t nmembers ;
    ofs_t i ;
    ofs_t j ;

    assert(len_ret) ;
    assert(!scaled) ;
    if (c->key->stack == STACK_GOSSIP) {
	array_singleton(conn_recv_info, recv_info(conn_gossip(c), CONN_OTHER, mbr_none()), &info) ;
	*len_ret = 1 ;
	return info ;
    }
    
    nmembers = c->nmembers ;
    info = conn_recv_info_array_create((len_t)(nmembers * 2)) ;

    j = 0 ;
    array_set(info, j++, recv_info(conn_gossip(c), CONN_OTHER, mbr_none())) ;
    array_set(info, j++, recv_info(conn_merge_recv(c), CONN_OTHER, mbr_none())) ;

    for (i=0;i<nmembers;i++) {
	if (i == c->rank) {
	    continue ;
	}
	array_set(info, j++, recv_info(conn_pt2pt_recv(c, i), CONN_SEND, i)) ;
    }

    for (i=0;i<nmembers;i++) {
	if (i == c->rank) {
	    continue ;
	}
	array_set(info, j++, recv_info(conn_multi_recv(c, i), CONN_CAST, i)) ;
    }
    assert(j == nmembers * 2) ;
    *len_ret = nmembers * 2 ;
    return info ;
}

void conn_recv_info_free(conn_recv_info_t info) {
    if (info->id.view_id_option) {
	record_free(info->id.view_id_option) ;
    }
    record_free(info) ;
}

/**************************************************************/
/*let ltime id = match id.view_id with None -> -1 | Some (ltime,_) -> ltime*/
/**************************************************************/

#include "infr/array_supp.h"

ARRAY_CREATE(conn_recv_info)
