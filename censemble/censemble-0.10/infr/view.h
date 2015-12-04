/**************************************************************/
/* VIEW.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef VIEW_H
#define VIEW_H

#include "infr/endpt.h"
#include "infr/group.h"
#include "infr/async.h"
#include "infr/etime.h"
#include "infr/addr.h"
#include "infr/proto.h"
#include "infr/version.h"

typedef struct view_id_t {
    ltime_t ltime ;
    endpt_id_t endpt ;
} view_id_t ;

typedef array_def(view_id) view_id_array_t ;

typedef struct view_contact_t {
    endpt_id_t endpt ;
    addr_id_t addr ;
    view_id_t view_id ;
} view_contact_t ;

string_t view_id_to_string(const view_id_t *) ;
string_t view_id_array_to_string(view_id_array_t, len_t) ;

typedef struct view_state_t {
    rank_t nmembers ;
    version_id_t version ;
    group_id_t group ;
    rank_t coord ;
    ltime_t ltime ;
    bool_t primary ;
    proto_id_t proto ;
    bool_t groupd ;
    bool_t xfer_view ;
    bool_t new_key ;
    etime_t uptime ;
    endpt_id_array_t view ;
    addr_id_array_t address ;
    ltime_array_t out_of_date ;
    const_bool_array_t protos ;
    const_bool_array_t clients ;
    view_id_array_t prev_ids ;
    nmembers_t quorum ;		/* YUCK */
} *view_state_t ;

typedef array_def(view_state) view_state_array_t ;

typedef struct view_local_t {
    endpt_id_t endpt ;
    addr_id_t addr ;
    rank_t rank ;
    name_t name ;
    view_id_t view_id ;
    bool_t am_coord ;
    async_id_t async ;
} const * view_local_t ;

view_state_t view_state_copy(view_state_t) ;

void view_state_free(view_state_t) ;

void view_local_free(view_local_t) ;

string_t view_state_to_string(view_state_t) ;

bool_t view_state_eq(view_state_t, view_state_t) ;

view_id_t view_id_of_state(view_state_t) ;

view_local_t view_local(const endpt_id_t *, view_state_t) ;

void view_local_free(view_local_t) ;

view_state_t view_singleton(
	proto_id_t,
        group_id_t,
	endpt_id_t,
	addr_id_t,
	ltime_t,
	etime_t
) ;
	
void view_check(view_state_t) ;

view_contact_t view_contact(view_state_t) ;
view_contact_t view_contact_copy(view_contact_t) ;

view_state_t view_merge(view_state_array_t, len_t) ;

view_state_t view_fail(view_state_t, const_bool_array_t) ;

view_id_array_t view_id_array_create(len_t) ;
view_id_array_t view_id_array_copy(view_id_array_t, len_t) ;
bool_t view_id_array_mem(const view_id_t *, view_id_array_t, len_t) ;

view_state_array_t view_state_array_create(len_t) ;
view_state_array_t view_state_array_appendi(view_state_array_t, len_t, view_state_t) ;
view_state_array_t view_state_array_prependi(view_state_t, view_state_array_t, len_t) ;
void view_state_array_free(view_state_array_t, len_t) ;

int view_id_cmp(const view_id_t *, const view_id_t *) ;
bool_t view_id_eq(const view_id_t *, const view_id_t *) ;
bool_t view_id_option_eq(const view_id_t *, const view_id_t *) ;

string_t view_contact_to_string(const view_contact_t *) ;

/* Marshalling operations.
 */
#include "infr/marsh.h"
void marsh_view_state(marsh_t, view_state_t) ;
void unmarsh_view_state(unmarsh_t, view_state_t *) ;
void marsh_view_id(marsh_t, const view_id_t *) ;
void unmarsh_view_id(unmarsh_t, view_id_t *) ;
void marsh_view_contact(marsh_t, const view_contact_t *) ;
void unmarsh_view_contact(unmarsh_t, view_contact_t *) ;

#endif /* VIEW_H */
