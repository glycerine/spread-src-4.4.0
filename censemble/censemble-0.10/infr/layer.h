/**************************************************************/
/* LAYER.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef LAYER_H
#define LAYER_H

#include "infr/domain.h"
#include "infr/alarm.h"
#include "infr/event.h"
#include "infr/iovec.h"
#include "infr/sched.h"
#include "infr/appl_intf.h"
#include "infr/marsh.h"

typedef enum { LAYER_FULL_HDR, LAYER_LOCAL_HDR, LAYER_MAX } marsh_type_t ;

typedef struct layer_t *layer_t ;

typedef array_def(layer) layer_array_t ;

typedef struct layer_state_t {
    appl_intf_t appl_intf ;
    env_t appl_intf_env ;
    domain_t domain ;
    alarm_t alarm ;
} *layer_state_t ;

/* This takes the view_state_t reference.
 */
void layer_compose(
	layer_state_t,
	const endpt_id_t *,
	view_state_t
) ;

void
layer_up(
	layer_t,
	event_t,
	unmarsh_t
) ;

void
layer_upnm(
	layer_t,
	event_t
) ;

void
layer_dn(
	layer_t,
	event_t,
	marsh_t
) ;

void
layer_dnnm(
	layer_t,
	event_t
) ;

void
up_free(
	event_t,
	unmarsh_t
) ;

void
dn_free(
	event_t,
	marsh_t
) ;

typedef void *layer_header_t ;

typedef layer_state_t (*layer_init_t)(layer_state_t, layer_t, layer_state_t, view_local_t, view_state_t) ;
typedef void (*layer_free_t)(layer_state_t) ;
typedef void (*layer_dump_t)(layer_state_t) ;
typedef void (*layer_up_handler_t)(layer_state_t, event_t, unmarsh_t) ;
typedef void (*layer_upnm_handler_t)(layer_state_t, event_t) ;
typedef void (*layer_dn_handler_t)(layer_state_t, event_t, marsh_t) ;
typedef void (*layer_dnnm_handler_t)(layer_state_t, event_t) ;
typedef string_t (*layer_string_of_header_t)(layer_header_t) ;
typedef void (*layer_marsh_header_t)(marsh_t, layer_header_t) ;
typedef layer_header_t (*layer_unmarsh_header_t)(unmarsh_t) ;

void
layer_register(
	string_t name,
	int state_size,
	layer_init_t init,
	layer_up_handler_t up,
	layer_upnm_handler_t upnm,
	layer_dn_handler_t dn,
	layer_dnnm_handler_t dnnm,
        layer_free_t,
        layer_dump_t
) ;

#endif /* LAYER_H */
