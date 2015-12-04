/**************************************************************/
/* TRANSPORT.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/transport.h"
#include "infr/conn.h"
#include "infr/stack.h"
#include "infr/appl.h"
#include "infr/domain.h"

static string_t name UNUSED() = "TRANSPORT" ;

typedef uint64_t transport_id_t ;

typedef struct item_t {
    transport_id_t id ;
    conn_recv_info_t info ;
    md5_t md5 ;
    transport_deliver_t deliver ;
    env_t env ;
    struct item_t *next ;
} *item_t ;

struct transport_root_t {
    sched_t sched ;
    transport_id_t transport_id ;
    uint32_t nitems ;
#define TABLE_LEN 256
    item_t table[TABLE_LEN] ;
} ;

struct transport_t {
    transport_root_t root ;
    transport_id_t id ;
    conn_t conn ;
    sched_t sched ;
    domain_t domain ;
    view_local_t ls ;
    view_state_t vs ;
} ;

struct transport_xmit_t {
    transport_t trans ;
    conn_id_t conn ;
    md5_t md5 ;
    domain_t domain ;
    domain_xmit_t xmit ;
} ;

transport_root_t transport_root(sched_t sched) {
    transport_root_t root = record_create(transport_root_t, root) ;
    record_clear(root) ;
    root->sched = sched ;
    return root ;
}

static
void add(
	transport_root_t t,
	transport_id_t id,
        conn_recv_info_t info,
	transport_deliver_t deliver,
	env_t env
) {
    item_t item = record_create(item_t, item) ;
    uint32_t hash ;
    item->id = id ;
    conn_hash_of_id(&info->id, &item->md5) ;
    item->info = info ;
    item->deliver = deliver ;
    item->env = env ;

    hash = ((uint32_t)item->md5.raw[0]) % TABLE_LEN ;
    item->next = t->table[hash] ;
    t->table[hash] = item ;
    t->nitems ++ ;

#if 0
    {
	item_t i ;
	for (i=table[hash];i;i=i->next) {
	    if (!md5_eq(&i->md5, &item->md5)) continue ;
	    if (conn_id_eq(&i->info->id, &item->info->id)) continue ;
	    eprintf("TRANSPORT:problem md5=%s\n", string_of_md5(&i->md5)) ;
	    transport_dump() ;
	    abort() ;
	}
    }
#endif

#if 0
    if ((nitems % 100) == 0) {
	eprintf("nitems=%d\n", nitems) ;
    }
#endif
}

/* Construct and enable a new transport instance.
 */
transport_t transport_create(
	transport_root_t s,
	domain_t domain,
        view_local_t ls,
        view_state_t vs,
	stack_id_t stack,
	transport_deliver_t deliver,
	env_t env
) {
    transport_t t = record_create(transport_t, t) ;
    conn_recv_info_array_t info ;
    ofs_t i ;
    len_t len ;
    assert(domain) ;
    t->root = s ;
    t->domain = domain ;
    t->id = s->transport_id ++ ;
    t->ls = ls ;
    t->vs = vs ;
    t->conn = conn_create(ls->rank,
			  vs->version,
			  vs->group,
			  ls->view_id,
			  stack,
			  vs->proto,
			  vs->view,
			  vs->nmembers) ;
    info = conn_all_recv(t->conn, FALSE, &len) ;
    assert_array_length(info, len) ;
    for (i=0;i<len;i++) {
	add(s, t->id, array_get(info, i), deliver, env) ;
    }
    array_free(info) ;
    return t ;
}
	
/* Disable the transport. 
 */
void transport_disable(transport_t t) {
    int i ;
    item_t *itemp ;
    for (i=0;i<TABLE_LEN;i++) {
	itemp=&t->root->table[i] ;
	while (*itemp) {
	    item_t item = *itemp ;
	    if (item->id != t->id) {
		itemp = &item->next ;
		continue ;
	    }
	    
	    *itemp = item->next ;
	    conn_recv_info_free(item->info) ;
	    record_free(item) ;
	    t->root->nitems -- ;
	}
    }
    conn_free(t->conn) ;
    record_free(t) ;
}

static 
transport_xmit_t create_xmit(transport_t t, const conn_id_t *id, domain_dest_t dest) {
    transport_xmit_t xmit = record_create(transport_xmit_t, xmit) ;
    xmit->trans = t ;
    xmit->conn = *id ;
    conn_hash_of_id(&xmit->conn, &xmit->md5) ;
#if 0
    eprintf("hash:%s\n", hex_of_bin(&xmit->md5, sizeof(xmit->md5))) ;
#endif
    xmit->xmit = NULL ;		/* ??? */
    xmit->xmit = domain_prepare(t->domain, dest) ;
    xmit->domain = t->domain ;
    domain_dest_free(dest) ;
    return xmit ;
}

void transport_async(
	transport_root_t root,
	view_state_t vs,
	const endpt_id_t *endpt
) {
    conn_id_t id ;
    md5_t md5 ;
    uint32_t hash ;
    item_t item ;

    id.version = vs->version ;
    id.group = vs->group ;
    id.stack = STACK_PRIMARY ;
    id.proto_option = vs->proto ;
#if 0				/* Watch out: this is impacted by conn_merge() */
    {
	view_id_t view_id ;
	view_id = view_id_of_state(vs) ;
	id.view_id_option = &view_id ;
    }
#else
    id.view_id_option = NULL ;
#endif
    id.sndr_mbr = (rank_t)-1 ;
    id.dest_mbr = (rank_t)-1 ;
    id.dest_endpt_option = endpt ;
    conn_hash_of_id(&id, &md5) ;
    hash = ((uint32_t) md5.raw[0]) % TABLE_LEN ;
    for (item=root->table[hash];item;item=item->next) {
	if (!md5_eq(&item->md5, &md5)) {
	    continue ;
	}
	sched_enqueue_4arg(root->sched,
			   (void*)item->deliver,
			   (void*)item->env,
			   (void*)CONN_ASYNC, /* HACK! */
			   /*(void*)item->info->kind,*/
			   (void*)item->info->rank,
			   /*(void*)TRUE,*/	/* BUG */
			   NULL) ;
    }
}


/* Prepare to cast on the transport. 
 */
transport_xmit_t transport_cast(transport_t t) {
    conn_id_t id = conn_multi_send(t->conn) ;
    ofs_t ofs ;
    len_t len = t->vs->nmembers ;
    addr_id_array_t addrs = addr_id_array_create(len - 1) ;
    for (ofs=0;ofs<len-1;ofs++) {
	array_set(addrs, ofs, array_get(t->vs->address, ofs < t->ls->rank ? ofs : ofs + 1)) ;
    }
    
    return create_xmit(t, &id, domain_dest_pt2pt(addrs, len - 1)) ;
}

/* Prepare to send on the transport.
 */
transport_xmit_t transport_send(transport_t t, rank_t rank) {
    conn_id_t id = conn_pt2pt_send(t->conn, rank) ;
    addr_id_array_t addrs ;
    array_singleton(addr_id, array_get(t->vs->address, rank), &addrs) ;
    return create_xmit(t, &id, domain_dest_pt2pt(addrs, 1)) ;
}

/* Prepare to gossip on the transport.
 */
transport_xmit_t transport_gossip(transport_t t) {
    conn_id_t id = conn_gossip(t->conn) ;
    return create_xmit(t, &id, domain_dest_gossip(t->vs->group)) ;
}

/* Prepare to merge on the transport.
 */
transport_xmit_t transport_merge(
        transport_t t,
	view_contact_t contact
) {
    endpt_id_t *endpt_opt = record_create(endpt_id_t *, endpt_opt) ;
    conn_id_t id ;
    addr_id_array_t addrs ;
    *endpt_opt = contact.endpt ;
    id = conn_merge_send(t->conn, contact.view_id, endpt_opt) ;
    array_singleton(addr_id, contact.addr, &addrs) ;
    return create_xmit(t, &id, domain_dest_pt2pt(addrs, 1)) ;
}

void transport_xmit_free(transport_xmit_t xmit) {
    domain_xmit_disable(xmit->domain, xmit->xmit) ;
    if (xmit->conn.view_id_option) {
	record_free(xmit->conn.view_id_option) ;
    }
    if (xmit->conn.dest_endpt_option) {
	record_free(xmit->conn.dest_endpt_option) ;
    }
    record_free(xmit) ;
}

void transport_deliver(
        transport_root_t root,
	iovec_t iov
) {
    iovec_t md5_iov ;
    item_t item ;
    md5_t md5 ;
    uint32_t hash ;
    bool_t found_one ;
    
    iovec_split(iov, sizeof(md5), &md5_iov, &iov) ;
    iovec_flatten_buf(md5_iov, &md5, 0, sizeof(md5)) ;
    iovec_free(md5_iov) ;

    hash = ((uint32_t) md5.raw[0]) % TABLE_LEN ;

    found_one = FALSE ;
    for (item=root->table[hash];item;item=item->next) {
	/*if (!conn_id_eq(table[i].info->id, xmit->conn)) continue ;*/
	if (!md5_eq(&item->md5, &md5)) {
	    continue ;
	}

	found_one = TRUE ;
#if 0
	eprintf("TRANSPORT:hit! len=%d kind=%s\n",
		iovec_len(iov),
		conn_string_of_kind(item->info->kind)) ;
#endif
	/* There is an issue in using the scheduler here.  The
	 * problem is with DBD transports where a message is
	 * scheduled for delivery before it is closed and then is
	 * delivered after the close.  Memory corruption results.
	 *
	 * On the other hand, a view change could cause the
	 * transports to be changed out from underneath us.
	 * The right solution is to have an array of handlers
	 * with a refcount for releasing them.  Or to modify
	 * the DBD code so that it is not as tempermental about
	 * late delivery.  This also allows us to avoid unnecessary
	 * iovec copies.
	 */
#if 0
	sched_enqueue_4arg(root->sched,
			   (void*)item->deliver,
			   (void*)item->env,
			   (void*)item->info->kind,
			   (void*)item->info->rank,
			   /*(void*)TRUE,*/	/* BUG */
			   iovec_copy(iov)) ;
#else
	item->deliver(item->env, item->info->kind, item->info->rank, iovec_copy(iov)) ;
#endif
    }
#if 0
    eprintf("TRANSPORT:deliver %d %s\n",
	    found_one, string_of_md5(&md5)) ;
#endif
    iovec_free(iov) ;
}

void transport_xmit(transport_xmit_t xmit, marsh_t msg) {
    /*eprintf("xmit:md5=%s\n", string_of_md5(&xmit->md5)) ;*/
    marsh_md5(msg, &xmit->md5) ;
    domain_xmit(xmit->domain, xmit->xmit, msg) ;
}

#ifndef MINIMIZE_CODE
void transport_usage(transport_root_t root) {
    int count ;
    int i ;
    count = 0 ;
    for (i=0;i<TABLE_LEN;i++) {
	item_t item ;
	for (item=root->table[i];item;item=item->next) {
	    count ++ ;
	}
    }
    eprintf("transport_usage=%d\n", count) ;    
}

void transport_dump(transport_root_t root) {
    int i ;
    eprintf("TRANSPORT:dump\n") ;
    for (i=0;i<TABLE_LEN;i++) {
	item_t item ;
	for (item=root->table[i];item;item=item->next) {
	    string_t s ;
	    s = conn_recv_info_to_string(item->info) ;
	    eprintf("  %s %s\n", string_of_md5(&item->md5), s) ;
	}
    }
}
#endif

#include "infr/array_supp.h"
ARRAY_CREATE(transport_xmit)
