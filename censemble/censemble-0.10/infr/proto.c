/**************************************************************/
/* PROTO.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/proto.h"

#if 0
struct proto_id_t { 
    string_t names ; 
} ;

struct proto_layer_t {
    string_t name ;
} ;
#endif

proto_id_t proto_id_copy(proto_id_t id) {
    return string_copy(id) ;
}

void proto_id_free(proto_id_t id) {
    sys_free(id) ;
}

string_t proto_id_to_string(proto_id_t p) {
    return p ;
}

proto_id_t proto_id_of_string(string_t p) {
    string_array_t a ;
    len_t len ;
    a = util_string_split(p, ":", &len) ;
    p = util_string_concat(":", a, len) ;
    string_array_free(a, len) ;
    return p ;
}

bool_t proto_id_eq(proto_id_t id0, proto_id_t id1) {
    assert(id0) ;
    assert(id1) ;
    return string_eq(id0, id1) ;
}

bool_t proto_id_option_eq(proto_id_t id0, proto_id_t id1) {
    if (!id0 && !id1) {
	return TRUE ;
    }
    if (!id0 || !id1) {
	return FALSE ;
    }
    return proto_id_eq(id0, id1) ;
}

proto_layer_array_t proto_layers_of_id(proto_id_t p, len_t *len) {
    return util_string_split(p, ":", len) ;
}

string_t proto_string_of_layer(proto_layer_t l) {
    return l ;
}

proto_layer_t proto_layer_of_string(string_t l) {
    return l ;
}

#include "infr/marsh.h"

void marsh_proto_id(marsh_t m, proto_id_t id) {
    marsh_string(m, id) ;
}

void unmarsh_proto_id(unmarsh_t m, proto_id_t *id) {
    unmarsh_string(m, id) ;
}
