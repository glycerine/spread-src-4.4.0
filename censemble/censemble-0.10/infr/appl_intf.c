/**************************************************************/
/* APPL_INTF.C: application interface */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/appl_intf.h"

static string_t name UNUSED() = "APPL_INTF" ;

appl_action_t appl_cast(env_t msg) {
    appl_action_t a = record_create(appl_action_t, a) ;
    a->type = APPL_CAST ;
    a->u.cast = msg ;
    return a ;
}

appl_action_t appl_suspect(bool_array_t suspect) {
    appl_action_t a = record_create(appl_action_t, a) ;
    a->type = APPL_SUSPECT ;
    a->u.suspect = suspect ;
    return a ;
}

appl_action_t appl_send1(rank_t dest, void *msg) {
    appl_action_t a = record_create(appl_action_t, a) ;
    a->type = APPL_SEND1 ;
    a->u.send1.dest = dest ;
    a->u.send1.msg = msg ;
    return a ;
}

appl_action_t appl_leave(void) {
    appl_action_t a = record_create(appl_action_t, a) ;
    a->type = APPL_LEAVE ;
    return a ;
}

appl_action_t appl_xfer_done(void) {
    appl_action_t a = record_create(appl_action_t, a) ;
    a->type = APPL_XFER_DONE ;
    return a ;
}

void appl_action_free(appl_action_t a) {
    switch(a->type) {
	
    case APPL_CAST:
	iovec_free(a->u.cast) ;
	break ;

    case APPL_SEND:
	array_free(a->u.send.dests) ;
	iovec_free(a->u.send.msg) ;
	break ;

    case APPL_SEND1:
	iovec_free(a->u.send1.msg) ;
	break ;

    case APPL_MIGRATE:
    case APPL_LEAVE:
    case APPL_XFER_DONE:
    case APPL_REKEY:
    case APPL_DUMP:
    case APPL_PROMPT:
    case APPL_TIMEOUT:
    case APPL_BLOCK:
    case APPL_NO_OP:
	/* nothing */
	break ;
    case APPL_PROTO:
	proto_id_free(a->u.proto) ;
	break ;
    case APPL_SUSPECT:
	array_free(a->u.suspect) ;
	break ;
    OTHERWISE_ABORT() ;
    }

    record_free(a) ;
}
