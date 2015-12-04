/**************************************************************/
/* ASYNC.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef ASYNC_H
#define ASYNC_H

typedef struct async_id_t *async_id_t ;

async_id_t async_id(endpt_id_t, group_id_t) ;

string_t async_string_of_id(async_id_t) ;

#endif /* ASYNC_H */
