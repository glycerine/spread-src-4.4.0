/**************************************************************/
/* TRACE.H */
/* Authors: Mark Hayden, Chet Murthy 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef TRACE_H
#define TRACE_H

#include "infr/trans.h"

typedef struct log_line_t {
    string_t s0 ;
    string_t s1 ;
    string_t function ;
    int line ;
    enum { 
	LOG_LINE_UNINIT = 0,
	LOG_LINE_UNSET,
	LOG_LINE_SET,
    } __logme__ ;
    struct log_line_t *next ;
} *log_line_t ;

#define LOG_INFO 1 
#define LOG_DATA 2

//#define LOG_LEVEL LOG_INFO
#define LOG_LEVEL LOG_DATA

#if defined(MINIMIZE_CODE)
#define log_add(a) NOP
#define log_register (a,b,c,d,e,f) NOP
#define log_enable (a,b) NOP
#define log_check (a,b) NOP
#define LOG_GEN(a,b,c,d,e,f) NOP
#define LOG_GEN_STR(a,b,c,d,e,f,g) NOP
#else
void log_add(string_t) ;
void log_register(string_t s0, string_t s1, string_t function, int line, log_line_t linep) ;
void log_enable(string_t function, int line) ;
bool_t log_check(string_t s0, string_t s1) ;

#if 0
void log_fire(
        string_t trig_name,
	string_t trig_suffix,
	string_t function,
	int line,
	log_line_t linep,
        string_t disp_name,
	string_t disp_suffix,
	...
) ;
#endif

#define LOG_GEN(_level, _trig_name, _trig_suff, _disp_name, _disp_suff, _args) \
    do { \
        static struct log_line_t __line__ ; \
        if (__line__.__logme__ == LOG_LINE_UNSET) { \
	    break ; \
	} \
        if (__line__.__logme__ == LOG_LINE_UNINIT) { \
	    log_register(_trig_name, _trig_suff, __FILE__, __LINE__, &__line__) ; \
        } \
        if (__line__.__logme__ == LOG_LINE_SET) { \
	    eprintf("%s%s:", _disp_name, _disp_suff) ; \
	    eprintf _args ; \
	    eprintf("\n") ; \
        } \
    } while(0)

#define LOG_GEN_STR(_level, _trig_name, _trig_suff, _disp_name, _disp_suff, _string, _args) \
    do { \
        static struct log_line_t __line__ ; \
        if (__line__.__logme__ == LOG_LINE_UNSET) { \
	    break ; \
	} \
        if (__line__.__logme__ == LOG_LINE_UNINIT) { \
	    log_register(_trig_name, _trig_suff, __FILE__, __LINE__, &__line__); \
        } \
        if (__line__.__logme__ == LOG_LINE_SET) { \
	    eprintf("%s%s:%s:", _disp_name, _disp_suff, (_string)) ; \
	    eprintf _args ; \
	    eprintf("\n") ; \
        } \
    } while(0)

#endif

#endif /* TRACE_H */
