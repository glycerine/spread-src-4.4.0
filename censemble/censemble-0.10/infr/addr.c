/**************************************************************/
/* ADDR.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#include "infr/trans.h"
#include "infr/addr.h"
#include "infr/sys.h"

addr_id_t addr_udp(inet_t inet, net_port_t port) {
    addr_id_t id ;
    record_clear(&id) ;
    id.kind = ADDR_UDP ;
    id.u.udp.inet = inet ;
    id.u.udp.port = port ;
    return id ;
}

addr_id_t addr_netsim(mux_t mux) {
    addr_id_t id ;
    memset(&id, 0, sizeof(id)) ;
    id.kind = ADDR_NETSIM ;
    id.u.netsim = mux ;
    return id ;
}

#ifndef MINIMIZE_CODE
string_t addr_id_to_string(const addr_id_t *id) {
    string_t ret ;
    switch(id->kind) {
    case ADDR_UDP: {
	string_t s = sys_addr_to_string(&id->u.udp) ; 
	ret = sys_sprintf("Addr{Udp;%s}", s) ;
	string_free(s) ;
    } break ;
    case ADDR_NETSIM:
	ret = sys_sprintf("Addr{Netsim;mux=%u}", id->u.netsim) ;
	break ;

    OTHERWISE_ABORT() ;
    }
    return ret ;
}
#endif

string_t addr_string_of_addr(addr_type_t) ;

void addr_id_array_free(addr_id_array_t a) {
    array_free(a) ;
}

/* Marshalling operations.
 */
#include "infr/marsh.h"

void marsh_addr_id(marsh_t m, const addr_id_t *id) {
    switch(id->kind) {
    case ADDR_UDP:
	marsh_inet(m, id->u.udp.inet) ;
	marsh_net_port(m, id->u.udp.port) ;
	break ;

    case ADDR_NETSIM:
	marsh_mux(m, id->u.netsim) ;
	break ;

    OTHERWISE_ABORT() ;
    }
    marsh_enum(m, id->kind, ADDR_MAX) ;
}

void unmarsh_addr_id(unmarsh_t m, addr_id_t *ret) {
    addr_id_t id ;
    record_clear(&id) ;
    id.kind = unmarsh_enum_ret(m, ADDR_MAX) ;
    switch(id.kind) {
    case ADDR_UDP:
	unmarsh_net_port(m, &id.u.udp.port) ;
	unmarsh_inet(m, &id.u.udp.inet) ;
	break ;
    case ADDR_NETSIM:
	unmarsh_mux(m, &id.u.netsim) ;
	break ;

    OTHERWISE_ABORT() ;
    }
    *ret = id ;
}

#include "infr/array_supp.h"

ARRAY_CREATE(addr_id)
ARRAY_PTR_TO_STRING(addr_id)
ARRAY_COPY(addr_id)
ARRAY_PTR_MARSH(addr_id, addr_id)

