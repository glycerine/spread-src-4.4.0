/**************************************************************/
/* UNIQUE.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/unique.h"

/* This is a generator for ids.  It keeps a skeleton id
 * that it uses to generate the next id by inserting
 * then next mux value into it.
 */
struct unique_t {
    unique_id_t id ;
    int mux ;
    bool_t warned ;
} ;

#define UNIQUE_PORT_UNSET  (0)

unique_t unique_create(inet_t inet, incarn_t incarn) {
    unique_t gen ;
    gen = record_create(unique_t, gen) ;
#ifdef PURIFY
    record_clear(gen) ;
#endif
    
    gen->id.inet = inet ;
    gen->id.incarn = incarn ;
    gen->id.port.i = UNIQUE_PORT_UNSET ;
    gen->id.mux = 0 ;
    gen->mux = 0 ;
    gen->warned = FALSE ;
    return gen ;
}

void unique_set_port(unique_t gen, net_port_t port) {
    gen->id.port = port ;
}

/* Create unique identifiers.
 */
unique_id_t unique_id(unique_t gen) {
    unique_id_t id ;
#ifdef PURIFY
    record_clear(&id) ;
#endif
    /* A warning is printed out once if a port is requested before
     * one has been installed.
     */
    if (!gen->warned &&
	gen->id.port.i == UNIQUE_PORT_UNSET) {
	gen->warned = TRUE ;
	eprintf("UNIQUE:warning:Unique.id called prior to Unique.set_port\n") ;
	eprintf("  (this may prevent some Ensemble identifiers from being unique)\n") ;
    }

    id = gen->id ;
    id.mux = gen->mux ;
    gen->mux ++ ;
    return id ;
}

/*unique_id_t unique_named(string_t name) ;*/

/* Display functions.
 */
#ifndef MINIMIZE_CODE
string_t unique_id_to_string(const unique_id_t u) {
    if (u.port.i != UNIQUE_PORT_UNSET &&
	u.port.i != 1) {
	return sys_sprintf("{%d:%u}", u.mux, trans_port_ntoh(u.port)) ;
    } else {
	return sys_sprintf("{%d}", u.mux) ;
    }
}
#endif

int unique_id_cmp(const unique_id_t *u0, const unique_id_t *u1) {
    if (u0->mux != u1->mux) {
	return (u0->mux > u1->mux) ? 1 : -1 ;
    }
    if (!inet_eq(u0->inet, u1->inet)) {
	return inet_cmp(u0->inet, u1->inet) ;
    }
    if (u0->port.i != u1->port.i) {
	return (u0->port.i > u1->port.i) ? 1 : -1 ;
    }
    if (u0->incarn != u1->incarn) {
	return (u0->incarn > u1->incarn) ? 1 : -1 ;
    }
    return 0 ;
}

bool_t unique_id_eq(const unique_id_t *u0, const unique_id_t *u1) {
    return unique_id_cmp(u0, u1) == 0 ;
}

#include "infr/marsh.h"

void hash_unique_id(hash_t *h, const unique_id_t *id) {
    hash_mux(h, id->mux) ;
    hash_inet(h, id->inet) ;
    hash_net_port(h, id->port) ;
    hash_incarn(h, id->incarn) ;
}

void marsh_unique_id(marsh_t m, const unique_id_t *id) {
    marsh_mux(m, id->mux) ;
    marsh_inet(m, id->inet) ;
    marsh_net_port(m, id->port) ;
    marsh_incarn(m, id->incarn) ;
}

void unmarsh_unique_id(unmarsh_t m, unique_id_t *id) {
#ifdef PURIFY
    record_clear(id) ;
#endif
    unmarsh_incarn(m, &id->incarn) ;
    unmarsh_net_port(m, &id->port) ;
    unmarsh_inet(m, &id->inet) ;
    unmarsh_mux(m, &id->mux) ;
}
