/**************************************************************/
/* TRACE.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/trace.h"
#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#if !defined(MINIMIZE_CODE)
#define MAX_LOGS 128

static
struct log {
    enum { LOG_NAME, LOG_LINE } discr ;
    union {
	string_t name ;
	struct {
	    string_t function ;
	    int line ;
	} line ;
    } u;
} logs[MAX_LOGS] ;

static int nlogs = 0 ;
static log_line_t lines = NULL ;

void log_add(string_t s) {
    int i ;
    log_line_t cur ;
    for (i=0;i<nlogs;i++) {
	if (logs[i].discr != LOG_NAME) continue;
	if (!logs[i].u.name) continue ;
	if (string_eq(logs[i].u.name,  s)) break ;
    }
    if (i < nlogs) {
	return ;
    }
    if (nlogs >= MAX_LOGS) {
	eprintf("TRACE:log_add:table overflow\n") ;
	return ;
    }
    logs[nlogs].discr = LOG_NAME;
    logs[nlogs].u.name = string_copy(s) ;
    nlogs ++ ;
    
    for (cur=lines; cur; cur = cur->next) {
	if (!strncmp(s, cur->s0, strlen (cur->s0)) &&
	    !strcmp(s+strlen (cur->s0), cur->s1)) {
	    cur->__logme__ = LOG_LINE_SET ;
	}
    }
}

void log_enable(string_t function, int line) {
    log_line_t cur ;
    int i ;
    for (cur=lines;cur; cur = cur->next) {
	if (0==strcmp (function, cur->function) && cur->line == line) {
	    cur->__logme__ = LOG_LINE_SET ;
	    return;
	}
    }
    
    for (i=0;i<nlogs;i++) {
	if (logs[i].discr != LOG_LINE) {
	    continue ;
	}
	if (!strcmp (logs[i].u.line.function, function) &&
	    logs[i].u.line.line == line) {
	    return ;
	}
    }
    if (nlogs >= MAX_LOGS) {
	eprintf("TRACE:log_add:table overflow\n") ;
	return ;
    }
    logs[nlogs].discr = LOG_LINE ;
    logs[nlogs].u.line.function = string_copy(function) ;
    logs[nlogs].u.line.line = line ;
    nlogs ++ ;
}

static
bool_t log_check_line(string_t function, int line) {
    int i ;

    for (i=0;i<nlogs;i++) {
	if (logs[i].discr != LOG_LINE ||
	    !logs[i].u.line.function ||
	    strcmp(logs[i].u.line.function, function) ||
	    logs[i].u.line.line != line) {
	    continue ;
	}
	break ;
    }

    return (i < nlogs) ;
}

bool_t log_check(string_t s0, string_t s1) {
    int i ;
    
    for (i=0;i<nlogs;i++) {
	if (logs[i].discr != LOG_NAME ||
	    !logs[i].u.name ||
	    strncmp(logs[i].u.name, s0, strlen(s0)) ||
	    strcmp(logs[i].u.name + strlen(s0), s1)) {
	    continue ;
	}
	break ;
    }
    
    return (i < nlogs) ;
}

void log_register(
        string_t s0,
	string_t s1,
	string_t function,
	int line_no,
	log_line_t line
) {
    if (0) {
	eprintf("TRACE:register (%s,%s,%s:%d)\n", s0, s1, function, line_no) ;
    }
    line->s0 = s0 ;
    line->s1 = s1 ;
    line->function = function ;
    line->line = line_no ;
    line->__logme__ = 1 ;
    line->next = lines ;
    lines = line ;

    if (log_check(s0, s1) ||
	log_check_line(function, line_no)) {
	if (0) {
	    eprintf("TRACE:register (%s,%s,%s:%d) ALREADY ENABLED\n",
		    s0, s1, function, line_no) ;
	}
	lines->__logme__ = LOG_LINE_SET ;
    }
    
}

#endif
