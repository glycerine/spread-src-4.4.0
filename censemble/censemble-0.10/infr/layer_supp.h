/**************************************************************/
/* LAYER_SUPP.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#include "infr/trace.h"
#undef LOG_GEN

#ifdef NDEBUG
#undef sys_panic
#define sys_panic(args) sys_abort()
#define LOG_GEN(a,b,c,d,e) NOP
#else
#undef sys_panic
#define sys_panic(args) \
   do { \
     sys_eprintf("%s:", name) ; \
     sys_do_panic args ; \
   } while (0)
#define LOG_GEN(_trig_name, _trig_suff, _disp_name, _disp_suff, _args) \
    do { \
        static struct log_line_t __line__; \
        if (!__line__.__logme__) { \
		  log_register (_trig_name,_trig_suff, __FILE__, __LINE__,&__line__); \
		} \
        if (__line__.__logme__ > 1) { \
        string_t out = sys_sprintf _args ; \
        sys_eprintf("%s%s:%s:%s\n", _disp_name, _disp_suff, s->ls->name, out) ; \
	string_free(out) ; \
      } \
    } while(0)
#endif
#define log(args)  LOG_GEN(name, "", name, "", args)
#define log_sync(args) LOG_GEN("SYNCING", "", "SYNCING:", name, args)
#define log_flow(args) LOG_GEN("FLOW", "", "FLOW:", name, args)
#define logn(args) LOG_GEN(name, "N", name, "N", args)
#define logp(args) LOG_GEN(name, "P", name, "P", args)
#define logb(args) LOG_GEN(name, "B", name, "B", args)

static inline void up(state_t s, event_t e, unmarsh_t m) {
    layer_up(s->layer, e, m) ;
}

static inline void upnm(state_t s, event_t e) {
    layer_upnm(s->layer, e) ;
}

static inline void dn(state_t s, event_t e, marsh_t m) {
    layer_dn(s->layer, e, m) ;
}

static inline void dnnm(state_t s, event_t e) {
    layer_dnnm(s->layer, e) ;
}

/* Definitions for handlers for this layer.  These
 * use the locally defined state_t type.
 */
typedef void (*this_layer_init_t)(state_t, layer_t, layer_state_t, const view_local_t, const view_state_t) ;
typedef void (*this_layer_dump_t)(state_t) ;
typedef void (*this_layer_up_handler_t)(state_t, event_t, unmarsh_t) ;
typedef void (*this_layer_upnm_handler_t)(state_t, event_t) ;
typedef void (*this_layer_dn_handler_t)(state_t, event_t, marsh_t) ;
typedef void (*this_layer_dnnm_handler_t)(state_t, event_t) ;
typedef void (*this_layer_free_t)(state_t) ;

static void init(state_t, layer_t, layer_state_t, view_local_t, view_state_t) ;
#ifndef MINIMIZE_CODE
static void dump(state_t) ;
#endif
static void up_handler(state_t, event_t, unmarsh_t) ;
static void upnm_handler(state_t, event_t) ;
static void dn_handler(state_t, event_t, marsh_t) ;
static void dnnm_handler(state_t, event_t) ;
static void free_handler(state_t) ;

#ifdef MINIMIZE_CODE
#define EVENT_DUMP_HANDLE()
#else
#define EVENT_DUMP_HANDLE() \
    case EVENT_DUMP: \
	dump(s) ; \
	upnm(s, e) ; \
	break; 
#endif

/* This secondary function is needed to get typechecking to occur on
 * the handler functions.
 */
static inline void
do_layer_register(
	string_t name,
	this_layer_init_t init,
	this_layer_up_handler_t up,
	this_layer_upnm_handler_t upnm,
	this_layer_dn_handler_t dn,
	this_layer_dnnm_handler_t dnnm,
	this_layer_free_t free,
        this_layer_dump_t dump
) {
    layer_register(name, sizeof(struct state_t),
		   (layer_init_t)init,
		   (layer_up_handler_t)up,
		   (layer_upnm_handler_t)upnm,
		   (layer_dn_handler_t)dn,
		   (layer_dnnm_handler_t)dnnm,
		   (layer_free_t)free,
		   (layer_dump_t)dump) ;
}

#ifdef MINIMIZE_CODE
#define LAYER_REGISTER(_name) \
  void _name ## _register(void) { \
    do_layer_register(name, init, up_handler, upnm_handler, \
		      dn_handler, dnnm_handler, free_handler, NULL) ; \
  }
#else
#define LAYER_REGISTER(_name) \
  void _name ## _register(void) { \
    do_layer_register(name, init, up_handler, upnm_handler, \
		      dn_handler, dnnm_handler, free_handler, dump) ; \
  }
#endif
