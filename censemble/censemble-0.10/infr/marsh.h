/**************************************************************/
/* MARSH.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef MARSH_H
#define MARSH_H

#include "infr/trans.h"
#include "infr/util.h"
#include "infr/etime.h"
#include "infr/iovec.h"

typedef struct marsh_t *marsh_t ;
typedef struct unmarsh_t *unmarsh_t ;
typedef struct marsh_buf_t *marsh_buf_t ;

marsh_t marsh_create(iovec_t iov) ;
void marsh_free(marsh_t) ;

unmarsh_t unmarsh_create(void) ;
void unmarsh_free(unmarsh_t) ;
void unmarsh_done(unmarsh_t) ;

iovec_t marsh_to_iovec(marsh_t) ;
unmarsh_t unmarsh_of_iovec(iovec_t) ;

void marsh_get(marsh_t, void **buf, len_t *, iovec_t *iov) ;

iovec_t unmarsh_to_iovec(unmarsh_t) ;
#if 0
marsh_t marsh_of_iovec(iovec_t) ;
string_t marsh_to_string(marsh_t) ;
#endif

marsh_buf_t marsh_to_buf(marsh_t) ;
marsh_buf_t unmarsh_to_buf(unmarsh_t) ;
marsh_t marsh_of_buf(marsh_buf_t) ;
unmarsh_t unmarsh_of_buf(marsh_buf_t) ;
len_t marsh_buf_length(marsh_buf_t) ;
void marsh_buf_free(marsh_buf_t) ;

unmarsh_t marsh_to_unmarsh_copy(marsh_t) ;
unmarsh_t marsh_to_unmarsh(marsh_t) ;

iovec_t marsh_to_iovec_copy(marsh_t) ;

bool_t marsh_eq(marsh_t, marsh_t) ;
len_t marsh_length(marsh_t) ;
len_t unmarsh_length(unmarsh_t) ;

void marsh_buf(marsh_t m, cbuf_t buf, len_t len) ;
void unmarsh_buf(unmarsh_t m, buf_t buf, len_t len) ;

#if 1
void marsh_uint8(marsh_t, uint8_t) ;
void unmarsh_uint8(unmarsh_t, uint8_t*) ;

void marsh_uint16(marsh_t, uint16_t) ;
void unmarsh_uint16(unmarsh_t, uint16_t*) ;

void marsh_uint32(marsh_t, uint32_t) ;
void unmarsh_uint32(unmarsh_t, uint32_t*) ;

void marsh_uint64(marsh_t, uint64_t) ;
void unmarsh_uint64(unmarsh_t, uint64_t*) ;
#endif

void marsh_bool(marsh_t, bool_t) ;
void unmarsh_bool(unmarsh_t, bool_t*) ;

void marsh_rank(marsh_t, rank_t) ;
void unmarsh_rank(unmarsh_t, rank_t*) ;

void marsh_seqno(marsh_t, seqno_t) ;
void unmarsh_seqno(unmarsh_t, seqno_t*) ;

void marsh_ltime(marsh_t, ltime_t) ;
void unmarsh_ltime(unmarsh_t, ltime_t*) ;

void marsh_len(marsh_t, len_t) ;
void unmarsh_len(unmarsh_t, len_t*) ;

void marsh_mux(marsh_t, mux_t) ;
void unmarsh_mux(unmarsh_t, mux_t*) ;

void marsh_incarn(marsh_t, incarn_t) ;
void unmarsh_incarn(unmarsh_t, incarn_t*) ;

void marsh_inet(marsh_t, inet_t) ;
void unmarsh_inet(unmarsh_t, inet_t*) ;

void marsh_port(marsh_t, port_t) ;
void unmarsh_port(unmarsh_t, port_t*) ;

void marsh_net_port(marsh_t, net_port_t) ;
void unmarsh_net_port(unmarsh_t, net_port_t*) ;

void marsh_sys_addr(marsh_t, const sys_addr_t*) ;
void unmarsh_sys_addr(unmarsh_t, sys_addr_t*) ;

void marsh_bool_array(marsh_t, const_bool_array_t, len_t) ;
void unmarsh_bool_array(unmarsh_t, const_bool_array_t *, len_t) ;

void marsh_ltime_array(marsh_t, ltime_array_t, len_t) ;
void unmarsh_ltime_array(unmarsh_t, ltime_array_t *, len_t) ;

void marsh_seqno_array(marsh_t, seqno_array_t, len_t) ;
void unmarsh_seqno_array(unmarsh_t, seqno_array_t *, len_t) ;

void marsh_string(marsh_t, string_t) ;
void unmarsh_string(unmarsh_t, string_t*) ;

void marsh_md5(marsh_t, md5_t *) ;
void unmarsh_md5(unmarsh_t, md5_t *) ;

void marsh_md5_calc(marsh_t, md5_t *) ;
void unmarsh_md5_calc(unmarsh_t, md5_t *) ;

void marsh_hash_calc(marsh_t, hash_t *) ;

void marsh_hdr(marsh_t, bool_t) ;
void unmarsh_hdr(unmarsh_t, bool_t*) ;

/* C enums are unsigned ints.
 */
void marsh_enum(marsh_t, unsigned, unsigned) ;
void unmarsh_enum(unmarsh_t, unsigned *, unsigned) ;
unsigned unmarsh_enum_ret(unmarsh_t, unsigned) ;

void marsh_time(marsh_t, etime_t) ;
void unmarsh_time(unmarsh_t, etime_t *) ;

void marsh_buf_check(marsh_buf_t) ;

/**************************************************************/

void unmarsh_of_iovec_flat(unmarsh_t, iovec_t) ;

struct unmarsh_t {
    ofs_t ofs ;
    iovec_loc_t loc ;
    iovec_t iov ;
    bool_t free_me ;
} ;

/**************************************************************/

#endif /* MARSH_H */
