/**************************************************************/
/* ARRAY.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef ARRAY_H
#define ARRAY_H

#ifdef ARRAY_LENGTH_DEBUG
#define array_def(type) \
  struct { \
    len_t length ; \
    type ## _t body[1] ; \
  } *
#define matrix_def(type) \
  struct { \
    len_t length ; \
    type ## _array_t body[1] ; \
  } *
#else
#define array_def(type) \
  struct { \
    type ## _t body[1] ; \
  } *
#define matrix_def(type) \
  struct { \
    type ## _array_t body[1] ; \
  } *
#endif

#define array_of(type) type ## _array_t
#define matrix_of(type) type ## _matrix_t

typedef int int_t ; /* HACK */

typedef array_def(int) int_array_t ;
typedef array_def(bool) bool_array_t ;
typedef array_def(inet) inet_array_t ;
typedef array_def(seqno) seqno_array_t ;
typedef array_def(rank) rank_array_t ;
typedef array_def(ltime) ltime_array_t ;
typedef array_def(string) string_array_t ;

typedef matrix_def(bool) bool_matrix_t ;
typedef matrix_def(seqno) seqno_matrix_t ;

#define const_int_array_t int_array_t
#define const_bool_array_t bool_array_t
#define const_seqno_array_t seqno_array_t
#define const_ltime_array_t ltime_array_t
#define const_rank_array_t rank_array_t

void array_free(const void *) ;

#define array_set(a,b,c) (a)->body[b] = (c)
#define array_get(a,b) ((a)->body[b])

#define matrix_get(a,b,c) ((a)->body[b]->body[c])
#define matrix_set(a,b,c,d) (a)->body[b]->body[c] = (d)

#define array_singleton(type, item, ret) \
  do { \
    *(ret) = type ## _array_create(1) ; \
    array_set(*(ret), 0, (item)) ; \
  } while(0) 

#define array_empty(type) type ## _array_create(0)

#ifdef ARRAY_LENGTH_DEBUG
#define array_length(a) ((a)->length)
#define assert_array_length(a, len) assert(array_length(a) == (len))
#define assert_array_bounds(a,i) assert((len_t)(i) < array_length(a))
#else
#define array_length(a) sys_abort()
#define assert_array_length(a, len) NOP
#define assert_array_bounds(a,i) NOP
#endif


bool_array_t bool_array_create(len_t) ;
int_array_t int_array_create(len_t) ;
seqno_array_t seqno_array_create(len_t) ;
ltime_array_t ltime_array_create(len_t) ;
string_array_t string_array_create(len_t) ;

bool_array_t bool_array_create_init(len_t, bool_t) ;
int_array_t int_array_create_init(len_t, int) ;
seqno_array_t seqno_array_create_init(len_t, seqno_t) ;
ltime_array_t ltime_array_create_init(len_t, ltime_t) ;

bool_matrix_t bool_matrix_create(len_t) ;
seqno_matrix_t seqno_matrix_create(len_t) ;

int_array_t int_array_copy(const_int_array_t, len_t) ;
ltime_array_t ltime_array_copy(const_ltime_array_t, len_t) ;
bool_array_t bool_array_copy( const_bool_array_t, len_t) ;
seqno_array_t seqno_array_copy(const_seqno_array_t, len_t) ;

void bool_array_copy_into(bool_array_t, const_bool_array_t, len_t) ;
void seqno_array_copy_into(seqno_array_t, const_seqno_array_t, len_t) ;
void ltime_array_copy_into(ltime_array_t, const_ltime_array_t, len_t) ;

string_t bool_array_to_string(const_bool_array_t, len_t) ;
string_t seqno_array_to_string(const_seqno_array_t, len_t) ;
string_t int_array_to_string(const_int_array_t, len_t) ;

bool_t bool_array_super(const_bool_array_t, const_bool_array_t, len_t) ;
bool_t bool_array_exists(const_bool_array_t, len_t) ;
bool_t bool_array_forall(const_bool_array_t, len_t) ;
ofs_t bool_array_min_false(const_bool_array_t, len_t) ;
bool_array_t bool_array_map_or(const_bool_array_t, const_bool_array_t, len_t) ;
bool_array_t bool_array_setone(len_t, ofs_t) ;

void seqno_array_incr(seqno_array_t, ofs_t) ;
void int_array_incr(int_array_t, ofs_t) ;
void int_array_add(int_array_t, ofs_t, int) ;
void int_array_sub(int_array_t, ofs_t, int) ;
bool_t seqno_array_eq(const_seqno_array_t, const_seqno_array_t, len_t) ;
ltime_t ltime_array_max(const_ltime_array_t, len_t) ;
seqno_t seqno_array_min(const_seqno_array_t, len_t) ;

#endif /*ARRAY_H*/
