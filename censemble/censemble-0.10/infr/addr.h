/**************************************************************/
/* ADDR.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef ADDR_H
#define ADDR_H

#include "infr/trans.h"
#include "infr/util.h"

typedef enum {
    ADDR_UDP,
    ADDR_NETSIM,
    ADDR_MAX
} addr_type_t ;

typedef struct {
    addr_type_t kind ;
    union {
	sys_addr_t udp ;
	mux_t netsim ;
    } u ;
} addr_id_t ;

typedef array_def(addr_id) addr_id_array_t ;

string_t addr_to_string(addr_type_t) ;
string_t addr_id_to_string(const addr_id_t *) ;
string_t addr_id_array_to_string(addr_id_array_t, len_t) ;
addr_id_array_t addr_id_array_create(len_t) ;
addr_id_array_t addr_id_array_copy(addr_id_array_t, len_t) ;
void addr_id_free(addr_id_t) ;
void addr_id_array_free(addr_id_array_t) ;

addr_id_t addr_udp(inet_t, net_port_t) ;
addr_id_t addr_netsim(mux_t) ;

/* Marshalling operations.
 */
#include "infr/marsh.h"
void marsh_addr_id(marsh_t, const addr_id_t *) ;
void unmarsh_addr_id(unmarsh_t, addr_id_t *) ;
void marsh_addr_id_array(marsh_t, addr_id_array_t, len_t) ;
void unmarsh_addr_id_array(unmarsh_t, addr_id_array_t *, len_t) ;

#endif /* ADDR_H */
