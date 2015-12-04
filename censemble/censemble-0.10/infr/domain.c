/**************************************************************/
/* DOMAIN.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#include "infr/trans.h"
#include "infr/domain.h"

static string_t name UNUSED() = "DOMAIN" ;

domain_dest_t domain_dest_pt2pt(array_of(addr_id) addrs, len_t naddrs) {
    domain_dest_t d ;
    memset(&d, 0, sizeof(d)) ;
    d.type = DOMAIN_PT2PT ;
    d.u.pt2pt.addr = addrs ;
    d.u.pt2pt.naddr = naddrs ;
    return d ;
}

domain_dest_t domain_dest_mcast(group_id_t group, bool_t loop) {
    domain_dest_t d ;
    memset(&d, 0, sizeof(d)) ;
    d.type = DOMAIN_MCAST ;
    d.u.mcast.group = group ;
    d.u.mcast.loopback = loop ;
    return d ;
}

domain_dest_t domain_dest_gossip(group_id_t group) {
    domain_dest_t d ;
    memset(&d, 0, sizeof(d)) ;
    d.type = DOMAIN_GOSSIP ;
    d.u.gossip = group ;
    return d ;
}

void domain_dest_free(domain_dest_t d) {
    switch (d.type) {
    case DOMAIN_PT2PT:
	addr_id_array_free(d.u.pt2pt.addr) ;
	break ;
    case DOMAIN_GOSSIP:
	break ;
    case DOMAIN_MCAST:
	break ;
    OTHERWISE_ABORT() ;
    }	
}

struct domain_t {
    env_t env ;
    name_t name ;
    domain_addr_t addr ;
    domain_prepare_t prepare ;
    domain_xmit_t xmit ;
    domain_xmit_disable_t xmit_disable ;
    domain_disable_t disable ;
} ;
		       
#ifndef MINIMIZE_CODE
string_t domain_dest_to_string(domain_dest_t dest) {
    sys_abort() ; return NULL ;
}
#endif

name_t domain_name(domain_t d) {
    return d->name ;
}

addr_id_t domain_addr(domain_t d, addr_type_t mode) {
    return d->addr(d->env, mode) ;
}

domain_info_t domain_prepare(domain_t d, domain_dest_t dest) {
    return d->prepare(d->env, dest) ;
}

void domain_xmit_disable(domain_t d, domain_info_t xmit) {
    d->xmit_disable(d->env, xmit) ;
}

void domain_xmit(domain_t d, domain_info_t xmit, marsh_t marsh) {
    d->xmit(d->env, xmit, marsh) ;
}

void domain_disable(domain_t d) {
    d->disable(d->env) ;
}

domain_t domain ;

domain_t domain_create(
        env_t env,
        name_t name,
	domain_addr_t addr,
	domain_prepare_t prepare,
	domain_xmit_t xmit,
	domain_xmit_disable_t xmit_disable,
	domain_disable_t disable
) {
    domain_t d = record_create(domain_t, d) ;
    d->env = env ;
    d->name = name ;
    d->addr = addr ;
    d->prepare = prepare ;
    d->xmit = xmit ;
    d->xmit_disable = xmit_disable ;
    d->disable = disable ;
    domain = d ;
    return d ;
}
