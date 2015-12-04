/**************************************************************/
/* PROTO.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef PROTO_H
#define PROTO_H

#include "infr/trans.h"
#include "infr/util.h"


/*typedef struct proto_id_t *proto_id_t ;	*//* stack */
/*typedef struct proto_layer_t *proto_layer_t ; *//* layer */

/* BUG: these should be opaque.
 */
typedef string_t proto_layer_t ;          /* layer */
typedef string_t proto_id_t ;	        /* stack */
/*typedef array_def(proto_layer_t) proto_layer_array_t ;*/
typedef string_array_t proto_layer_array_t ;

bool_t proto_id_eq(proto_id_t, proto_id_t) ;
bool_t proto_id_option_eq(proto_id_t, proto_id_t) ;

proto_id_t proto_id_of_string(string_t) ;

proto_id_t proto_id_copy(proto_id_t) ;
void proto_id_free(proto_id_t) ;

string_t proto_id_to_string(proto_id_t) ;

proto_layer_array_t proto_layers_of_id(proto_id_t, len_t *) ;

proto_id_t proto_id_of_layers(proto_layer_array_t) ;

string_t proto_string_of_layer(proto_layer_t) ;

proto_layer_t proto_layer_of_string(string_t) ;

/* Marshalling operations.
 */
#include "infr/marsh.h"
void marsh_proto_id(marsh_t, proto_id_t) ;
void unmarsh_proto_id(unmarsh_t, proto_id_t *) ;

/**************************************************************/
#endif /* PROTO_H */
