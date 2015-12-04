/**************************************************************/
/* GROUP.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/util.h"
#include "infr/unique.h"
#include "infr/group.h"

static string_t name UNUSED() = "GROUP" ;

group_id_t group_named(string_t s) {
    group_id_t id ;
    id.type = GROUP_NAMED ;
    assert(string_length(s) < GROUP_SIZE - 1) ;
    memset(&id.u.named, 0, sizeof(id.u.named)) ;
    strcpy(id.u.named, s) ;
    return id ;
}

group_id_t group_id(unique_id_t unique) {
    group_id_t group ;
    group.type = GROUP_ANONYMOUS ;
    group.u.anonymous = unique ;
    return group ;
}

bool_t group_id_eq(const group_id_t *id0, const group_id_t *id1) {
    if (id0->type != id1->type) {
	return FALSE ;
    }
    assert(id0->type == id1->type) ;
    switch (id0->type) {
    case GROUP_NAMED:
	return string_eq(id0->u.named, id1->u.named) ;
	break ;
    case GROUP_ANONYMOUS:
	return unique_id_eq(&id0->u.anonymous, &id1->u.anonymous) ;
	break ;
    OTHERWISE_ABORT() ;
    }
    sys_abort() ;
}

#ifndef MINIMIZE_CODE
string_t group_id_to_string(const group_id_t *id) {
    string_t ret ;
    switch (id->type) {
    case GROUP_NAMED:
	ret = sys_sprintf("Group{%s}", id->u.named) ;
	break ;
    case GROUP_ANONYMOUS:
	ret = sys_sprintf("Group{%s}", unique_id_to_string(id->u.anonymous)) ;
	break ;
    OTHERWISE_ABORT() ;
    }
    return ret ;
}
#endif

uint32_t group_id_to_hash(const group_id_t *id) {
    marsh_t marsh = marsh_create(NULL) ;
    md5_t md5 ;
    uint32_t ret ;
    marsh_group_id(marsh, id) ;
    marsh_md5_calc(marsh, &md5) ;
    memcpy(&ret, &md5, sizeof(ret)) ;
    marsh_free(marsh) ;
    /*eprintf("GROUP:group_id_to_hash %s -> %08x\n", group_id_to_string(id), ret) ;*/
    return ret ;
}

#include "infr/marsh.h"

void marsh_group_id(marsh_t m, const group_id_t *id) {
    switch(id->type) {
    case GROUP_NAMED:
	marsh_string(m, id->u.named) ;
	break ;
    case GROUP_ANONYMOUS:
	marsh_unique_id(m, &id->u.anonymous) ;
	break ;
    OTHERWISE_ABORT() ;
    }
    marsh_enum(m, id->type, GROUP_MAX) ;
}

void unmarsh_group_id(unmarsh_t m, group_id_t *ret) {
    group_id_t id ;
    uint8_t type ;
    type = unmarsh_enum_ret(m, GROUP_MAX) ;
    id.type = type ;
    switch(type) {
    case GROUP_NAMED: {
	string_t s ;
	unmarsh_string(m, &s) ;
	assert(string_length(s) < GROUP_SIZE - 1) ;
	memset(&id.u.named, 0, sizeof(id.u.named)) ;
	strcpy(id.u.named, s) ;
	string_free(s) ;
    } break ;
    case GROUP_ANONYMOUS:
	unmarsh_unique_id(m, &id.u.anonymous) ;
	break ;
    OTHERWISE_ABORT() ;
    }
    *ret = id ;
}
