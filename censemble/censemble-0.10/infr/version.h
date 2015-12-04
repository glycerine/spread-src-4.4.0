/**************************************************************/
/* VERSION.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef VERSION_H
#define VERSION_H

/* BUG: should be opaque.
 */
#define VERSION_MAX 16
struct version_id_t {
    char name[VERSION_MAX] ;
} ;

typedef struct version_id_t version_id_t ;

version_id_t version_id(void) ;

bool_t version_id_eq(const version_id_t *, const version_id_t *) ;

string_t version_id_to_string(const version_id_t *) ;

#include "infr/marsh.h"
void marsh_version_id(marsh_t, version_id_t) ;
void unmarsh_version_id(unmarsh_t, version_id_t *) ;

#endif /* VERSION_H */
