/**************************************************************/
/* CONN.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef CONN_H
#define CONN_H

#include "infr/trans.h"
#include "infr/version.h"
#include "infr/view.h"
#include "infr/endpt.h"
#include "infr/group.h"
#include "infr/stack.h"

typedef rank_t conn_mbr_t ;
typedef const endpt_id_t *conn_dest_endpt_t ;

/* BUG: should be opaque.
 */
struct conn_id_t {
    version_id_t version ;
    group_id_t group ;
    stack_id_t stack ;
    proto_id_t proto_option ;
    view_id_t *view_id_option ;
    conn_mbr_t sndr_mbr ;
    conn_mbr_t dest_mbr ;
    conn_dest_endpt_t dest_endpt_option ;
} ;

/* Identifier used for communication routing.
 */
typedef struct conn_id_t conn_id_t ;

typedef enum { CONN_CAST, CONN_SEND, CONN_ASYNC, CONN_OTHER } conn_kind_t ;

/* Set of connection identifiers used to communicate
 * with peers.
 */
typedef struct conn_t *conn_t ;

typedef struct conn_key_t *conn_key_t ;

typedef struct {
    conn_id_t id ;
    conn_kind_t kind ;
    rank_t rank ;
} *conn_recv_info_t ;

typedef array_def(conn_recv_info) conn_recv_info_array_t ;

/* Constructor.
 */
conn_t conn_create(
        rank_t,
	version_id_t,
	group_id_t,
	view_id_t,
	stack_id_t,
	proto_id_t,
	endpt_id_array_t,
	nmembers_t
) ;

void conn_free(conn_t) ;

conn_key_t conn_key(conn_t) ;

bool_t conn_id_eq(conn_id_t *, conn_id_t *) ;

conn_id_t conn_pt2pt_send(conn_t, rank_t) ;
conn_id_t conn_multi_send(conn_t) ;
conn_id_t conn_merge_send(conn_t, view_id_t, conn_dest_endpt_t) ;
conn_id_t conn_gossip(conn_t) ;

conn_recv_info_array_t conn_all_recv(conn_t, bool_t scaled, len_t *) ;
void conn_recv_info_free(conn_recv_info_t) ;

/* Display functions.
 */
string_t conn_id_to_string(conn_id_t *) ;
string_t conn_recv_info_to_string(conn_recv_info_t) ;

string_t conn_string_of_kind(conn_kind_t) ;

/* Create a 16-byte MD5 hash of an id.
 */
void conn_hash_of_id(const conn_id_t *, md5_t *) ;

#endif /* CONN_H */
