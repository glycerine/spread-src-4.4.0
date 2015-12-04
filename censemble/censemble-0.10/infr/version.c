/**************************************************************/
/* VERSION.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/version.h"

static struct version_id_t version = {
    "0.10"
} ;

version_id_t version_id(void) {
    return version ;
}

bool_t version_id_eq(const version_id_t *id0, const version_id_t *id1) {
    return string_eq(id0->name, id1->name) ;
}

#ifndef MINIMIZE_CODE
string_t version_id_to_string(const version_id_t *version) {
    return string_copy(version->name) ;
}
#endif

#include "infr/marsh.h"

void marsh_version_id(marsh_t m, version_id_t id) {
    marsh_string(m, id.name) ;
}

void unmarsh_version_id(unmarsh_t m, version_id_t *ret) {
    version_id_t id ;
    string_t s ;
    unmarsh_string(m, &s) ;
    assert(string_length(s) < VERSION_MAX - 1) ;
    memset(&id.name, 0, sizeof(id.name)) ;
    strcpy(id.name, s) ;
    string_free(s) ;
    *ret = id ;
}
