/**************************************************************/
/* IQ.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef IQ_H
#define IQ_H

#include "infr/trans.h"
#include "infr/marsh.h"

typedef void *iq_item_t ;

/**************************************************************/
/*
               	    lo       	read   	  hi
                   \/          \/        \/
     [][][][][][][][][][][][][][][][][][][][][][][]
     0<-- reset -->                      <---- unset ...

*/
/**************************************************************/

typedef struct iq_t *iq_t ;
typedef array_def(iq) iq_array_t ;

typedef enum { IQ_DATA, IQ_UNSET, IQ_RESET } iq_status_t ;

string_t iq_string_of_status(iq_status_t) ;

typedef void (*iq_free_t)(const void *item) ;

iq_t iq_create(debug_t, iq_free_t) ;

void iq_grow(iq_t, seqno_t) ;

seqno_t iq_lo(iq_t) ;
seqno_t iq_hi(iq_t) ;
void iq_set_lo(iq_t, seqno_t) ;
void iq_set_hi(iq_t, seqno_t) ;

/* ASSIGN: if this returns true then the slot was empty
 * and the item reference was appropriately taken.
 */
bool_t iq_assign(iq_t, seqno_t, iq_item_t) ;

bool_t iq_opt_insert_check_doread(iq_t, seqno_t, iq_item_t) ;

/* Split the above into two pieces.
 */
bool_t iq_opt_insert_check(iq_t, seqno_t) ;
void iq_opt_insert_doread(iq_t, seqno_t, iq_item_t) ;

void iq_add(iq_t, iq_item_t) ;

iq_status_t iq_get(iq_t, seqno_t, iq_item_t *) ;

bool_t iq_get_prefix(iq_t, seqno_t *, iq_item_t *) ;

bool_t iq_hole(iq_t, seqno_t *, seqno_t *) ;

seqno_t iq_read(iq_t) ;
bool_t iq_read_hole(iq_t, seqno_t *, seqno_t *) ;
bool_t iq_read_prefix(iq_t, seqno_t *, iq_item_t *) ;

/* The opt_update functions are used where the normal
 * case insertion does not actually insert anything
 * into the buffer.
 */
bool_t iq_opt_update_old(iq_t, seqno_t) ;

/* Split the above into two pieces.
 */
bool_t iq_opt_update_check(iq_t, seqno_t) ;
void iq_opt_update_update(iq_t, seqno_t) ;

void iq_free(iq_t) ;

/* Clear entries that haven't been read yet.
 */
void iq_clear_unread(iq_t) ;

iq_array_t iq_array_create(len_t) ;

#endif /* IQ */

/**************************************************************/
