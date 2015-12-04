/**************************************************************/
/* MARSH.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/hash.h"
#include "infr/marsh.h"

static string_t name UNUSED() = "MARSH" ;

/* Some things to think about before trying to switch to an
 * iovec-only setup for marsh's: the buf will need to be
 * malloc'd, so will the rbuf, and to avoid an iovec_append,
 * we'll need a new iovec operation.  It's not worth it.
 */

/* Logically, the (buf,len) portion precedes the iov portion.
 */
struct marsh_buf_t {
    iovec_t iov ;
    len_t len ;			/* length of the buffer */
    char buf[0] ;
} ;

/* Logically, the (buf,tot_len) portion precedes the iov portion.
 */
struct marsh_t {
    char *buf ;
    ofs_t ofs ;
    len_t tot_len ;
    iovec_t iov ;
} ;

#define MARSH_BUF_MIN_LEN 128

static inline
len_t min_len(len_t len) {
    return len < MARSH_BUF_MIN_LEN ? MARSH_BUF_MIN_LEN : len ;
}

inline
void marsh_buf_check(marsh_buf_t buf) {
    iovec_check(buf->iov) ;
}

marsh_t marsh_create(iovec_t iov) {
    marsh_t m = sys_alloc(sizeof(*m)) ;
    m->tot_len = MARSH_BUF_MIN_LEN ;
    m->ofs = m->tot_len ;
    m->buf = sys_alloc_atomic(m->tot_len) ;
    if (!iov) {
	iov = iovec_empty() ;
    } else {
	iov = iovec_take(iov) ;
    }
    m->iov = iov ;
    return m ;
}

void marsh_buf_free(marsh_buf_t buf) {
    marsh_buf_check(buf) ;
    iovec_free(buf->iov) ;
    sys_free(buf) ;
}

void marsh_free(marsh_t m) {
    sys_free(m->buf) ;
    if (m->iov) {
	iovec_free(m->iov) ;
    }
    sys_free(m) ;
}

void marsh_get(marsh_t m, void **buf, len_t *len, iovec_t *iov) {
    *buf = m->buf + m->ofs ;
    *len = m->tot_len - m->ofs ;
    *iov = m->iov ;
}

iovec_t marsh_to_iovec(marsh_t m) {
    iovec_t iov ;
    iov = iovec_of_buf(m->buf, m->ofs, m->tot_len - m->ofs) ;
    iov = iovec_append(iov, m->iov) ;
    sys_free(m) ;
    return iov ;
}

#if 0
iovec_t marsh_payload(marsh_t m) {
    if (m->iov) {
	return iovec_copy(m->iov) ;
    } else {
	return iovec_empty(debug) ;
    }
}

len_t marsh_payload_len(marsh_t m) {
    if (m->iov) {
	return iovec_len(m->iov) ;
    } else {
	return 0 ;
    }
}
#endif

marsh_buf_t marsh_to_buf(marsh_t m) {
    len_t len ;
    marsh_buf_t buf ;
    len = m->tot_len - m->ofs ;
    buf = sys_alloc_atomic(sizeof(*buf) + len) ;
    buf->len = len ;
    buf->iov = iovec_copy(m->iov) ;
    memcpy(buf->buf, m->buf + m->ofs, m->tot_len - m->ofs) ;
    return buf ;
}

#if 0
marsh_t marsh_of_iovec(iovec_t iov) {
    marsh_t m = do_alloc(sizeof(*m)) ;
    m->tot_len = iovec_len(iov) ;
    m->ofs = 0 ;
    m->buf = sys_alloc(m->tot_len) ;
    iovec_flatten_buf(iov, m->buf, 0, m->tot_len) ;
    return m ;
}
#endif

marsh_t marsh_of_buf(marsh_buf_t buf) {
    marsh_t m ;
    marsh_buf_check(buf) ;
    m = sys_alloc(sizeof(*m)) ;
    m->tot_len = min_len(buf->len) ;
    m->ofs = m->tot_len - buf->len ;
    m->buf = sys_alloc(m->tot_len) ;
    memcpy(buf_ofs(m->buf, m->ofs), buf->buf, buf->len) ;
    m->iov = iovec_copy(buf->iov) ;
    return m ;
}

bool_t marsh_eq(marsh_t m0, marsh_t m1) {
    if (m0->tot_len - m0->ofs != m1->tot_len - m1->ofs) {
	return FALSE ;
    }
    return !memcmp(m0->buf + m0->ofs, m1->buf + m1->ofs, m0->tot_len - m0->ofs) ;
}

len_t marsh_buf_length(marsh_buf_t m) {
    return m->len + iovec_len(m->iov) ;
}

len_t marsh_length(marsh_t m) {
    return m->tot_len - m->ofs + iovec_len(m->iov) ;
}

static
void grow_buf(marsh_t m, len_t len) {
    len_t used = m->tot_len - m->ofs ;
    len_t tot_len ;
    buf_t buf ;
    for (tot_len = m->tot_len * 2 ;
	 used + len > tot_len ;
	 tot_len *= 2) ;
    buf = sys_alloc(tot_len) ;
    memcpy(buf_ofs(buf, tot_len - used),
	   buf_ofs(m->buf, m->ofs), used) ;
    sys_free(m->buf) ;
    m->buf = buf ;
    m->ofs = tot_len - used ;
    m->tot_len = tot_len ;
    assert(m->ofs >= len) ;
}

void marsh_buf(marsh_t m, cbuf_t buf, len_t len) {
    if (m->ofs < len) {
	grow_buf(m, len) ;
	assert(m->ofs >= len) ;
    }
    m->ofs -= len ;
    memcpy(m->buf + m->ofs, buf, len) ;
}

void unmarsh_free(unmarsh_t m) {
    assert(m) ;
    iovec_free(m->iov) ;
    if (m->free_me) {
	sys_free(m) ;
    }
}

len_t unmarsh_length(unmarsh_t m) {
    assert(m->ofs <= iovec_len(m->iov)) ;
    return iovec_len(m->iov) - m->ofs ;
}

void unmarsh_done(unmarsh_t m) {
    assert(!unmarsh_length(m)) ;
    unmarsh_free(m) ;
}

inline
void unmarsh_of_iovec_flat(unmarsh_t m, iovec_t iov) {
    iovec_check(iov) ;
    m->ofs = 0 ;
    iovec_loc_init(&m->loc) ;
    m->iov = iov ;
    m->free_me = FALSE ;
}

unmarsh_t unmarsh_of_iovec(iovec_t iov) {
    unmarsh_t m = sys_alloc(sizeof(*m)) ;
    unmarsh_of_iovec_flat(m, iov) ;
    m->free_me = TRUE ;
    return m ;
}

iovec_t unmarsh_to_iovec(unmarsh_t m) {
    iovec_t iov ;
    iov = iovec_sub_take(m->iov, m->ofs, iovec_len(m->iov) - m->ofs) ;
    if (m->free_me) {
	sys_free(m) ;
    }
    return iovec_take(iov) ;	/* PERF */
}

unmarsh_t marsh_to_unmarsh_copy(marsh_t m) {
    len_t len ;
    iovec_t iov ;
    len = m->tot_len - m->ofs ;
    iov = iovec_of_buf_copy(m->buf, m->ofs, len) ;
    iov = iovec_append(iov, iovec_copy(m->iov)) ;
    return unmarsh_of_iovec(iov) ;
} 

iovec_t marsh_to_iovec_copy(marsh_t m) {
    len_t len ;
    iovec_t iov ;
    len = m->tot_len - m->ofs ;
    iov = iovec_of_buf_copy(m->buf, m->ofs, len) ;
    iov = iovec_append(iov, iovec_copy(m->iov)) ;
    return iov ;
} 

unmarsh_t marsh_to_unmarsh(marsh_t m) {
    iovec_t iov ;
    iov = marsh_to_iovec(m) ;
    return unmarsh_of_iovec(iov) ;
} 

marsh_buf_t unmarsh_to_buf(unmarsh_t m) {
    marsh_buf_t buf = sys_alloc_atomic(sizeof(*buf)) ;
    buf->len = 0 ;
    buf->iov = iovec_sub(m->iov, m->ofs, iovec_len(m->iov) - m->ofs) ;
    marsh_buf_check(buf) ;
    return buf ;
}

unmarsh_t unmarsh_of_buf(marsh_buf_t buf) {
    unmarsh_t m = sys_alloc(sizeof(*m)) ;
    assert(buf->len == 0) ;
    m->iov = iovec_copy(buf->iov) ;
    m->ofs = 0 ;
    iovec_loc_init(&m->loc) ;
    m->free_me = TRUE ;
    marsh_buf_check(buf) ;
    return m ;
}

void unmarsh_buf(unmarsh_t m, void *buf, len_t len) {
    iovec_read_scan(&m->loc, m->iov, m->ofs, len, buf) ;
    m->ofs += len ;
}

#ifndef __KERNEL__
void marsh_md5_calc(marsh_t m, md5_t *calc) {
    MD5Context ctx ;
    MD5Init(&ctx) ;
    MD5Update(&ctx, m->buf + m->ofs, m->tot_len - m->ofs) ;
    assert(m->iov) ;
    iovec_md5_update(m->iov, &ctx) ;
    MD5Final((void*)calc, &ctx) ;
}

void unmarsh_md5_calc(unmarsh_t m, md5_t *calc) {
    iovec_t iov ;
    MD5Context ctx ;
    MD5Init(&ctx) ;
    iov = iovec_sub(m->iov, m->ofs, iovec_len(m->iov) - m->ofs) ;
    iovec_md5_update(iov, &ctx) ;
    iovec_free(iov) ;
    MD5Final((void*)calc, &ctx) ;
}
#endif

void marsh_hash_calc(marsh_t m, hash_t *h) {
    assert(m->iov) ;
    hash_buf(h, m->buf + m->ofs, m->tot_len - m->ofs) ;
    iovec_hash(m->iov, h) ;
}

void marsh_uint8(marsh_t m, uint8_t v) {
    marsh_buf(m, &v, sizeof(v)) ;
}

void unmarsh_uint8(unmarsh_t m, uint8_t *v) {
    unmarsh_buf(m, v, sizeof(*v)) ;
}

#if 1
void marsh_uint16(marsh_t m, uint16_t v) {
    marsh_buf(m, &v, sizeof(v)) ;
}

void unmarsh_uint16(unmarsh_t m, uint16_t *v) {
    uint16_t tmp ;
    unmarsh_buf(m, &tmp, sizeof(tmp)) ;
    *v = tmp ;
}
#endif

void marsh_uint32(marsh_t m, uint32_t v) {
    rnet_uint32_t net ;
    net = trans_htor32(v) ;
    marsh_buf(m, &net, sizeof(net)) ;
}

void unmarsh_uint32(unmarsh_t m, uint32_t *v) {
    rnet_uint32_t net ;
    unmarsh_buf(m, &net, sizeof(net)) ;
    *v = trans_rtoh32(net) ;
}

void marsh_uint64(marsh_t m, uint64_t v) {
    rnet_uint64_t net ;
    net = trans_htor64(v) ;
    marsh_buf(m, &net, sizeof(net)) ;
}

void unmarsh_uint64(unmarsh_t m, uint64_t *v) {
    rnet_uint64_t net ;
    unmarsh_buf(m, &net, sizeof(net)) ;
    *v = trans_rtoh64(net) ;
}

/* We force booleans to be represented as chars.
 */
void marsh_bool(marsh_t m, bool_t v) {
    uint8_t v8 = v ;
    assert_bool(v) ;
    assert_bool(v8) ;
    marsh_buf(m, &v8, sizeof(v8)) ;
}

void unmarsh_bool(unmarsh_t m, bool_t *v) {
    uint8_t v8 ;
    bool_t b ;
    unmarsh_buf(m, &v8, sizeof(v8)) ;
    assert_bool(v8) ;
    b = v8 ;
    assert_bool(b) ;
    *v = b ;
}

void marsh_hdr(marsh_t m, bool_t v) {
    marsh_bool(m, v) ;
}

void unmarsh_hdr(unmarsh_t m, bool_t *v) {
    unmarsh_bool(m, v) ;
}

void marsh_rank(marsh_t m, rank_t v) {
    assert(sizeof(v) == sizeof(uint32_t)) ;
    marsh_uint32(m, v) ;
}

void unmarsh_rank(unmarsh_t m, rank_t *v) {
    assert(sizeof(*v) == sizeof(uint32_t)) ;
    unmarsh_uint32(m, v) ;
}

void marsh_seqno(marsh_t m, seqno_t v) {
    assert(sizeof(v) == sizeof(uint64_t)) ;
    marsh_uint64(m, v) ;
}

void unmarsh_seqno(unmarsh_t m, seqno_t *v) {
    assert(sizeof(*v) == sizeof(uint64_t)) ;
    unmarsh_uint64(m, v) ;
}

void marsh_ltime(marsh_t m, ltime_t v) {
    assert(sizeof(v) == sizeof(uint64_t)) ;
    marsh_uint64(m, v) ;
}

void unmarsh_ltime(unmarsh_t m, ltime_t *v) {
    assert(sizeof(*v) == sizeof(uint64_t)) ;
    unmarsh_uint64(m, v) ;
}

void marsh_len(marsh_t m, len_t v) {
    assert(sizeof(v) == sizeof(uint32_t)) ;
    marsh_uint32(m, v) ;
}

void unmarsh_len(unmarsh_t m, len_t *v) {
    assert(sizeof(*v) == sizeof(uint32_t)) ;
    unmarsh_uint32(m, v) ;
}

#if 0
void marsh_port(marsh_t m, port_t v) {
    assert(sizeof(v) == sizeof(uint16_t)) ;
    marsh_uint16(m, v) ;
}

void unmarsh_port(unmarsh_t m, port_t *v) {
    assert(sizeof(*v) == sizeof(uint16_t)) ;
    unmarsh_uint16(m, v) ;
}
#endif

void marsh_mux(marsh_t m, mux_t v) {
    assert(sizeof(v) == sizeof(uint32_t)) ;
    marsh_uint32(m, v) ;
}

void unmarsh_mux(unmarsh_t m, mux_t *v) {
    assert(sizeof(*v) == sizeof(uint32_t)) ;
    unmarsh_uint32(m, v) ;
}

void marsh_incarn(marsh_t m, incarn_t v) {
    assert(sizeof(v) == sizeof(uint32_t)) ;
    marsh_uint32(m, v) ;
}

void unmarsh_incarn(unmarsh_t m, incarn_t *v) {
    assert(sizeof(*v) == sizeof(uint32_t)) ;
    unmarsh_uint32(m, v) ;
}

void marsh_inet(marsh_t m, inet_t v) {
    assert(sizeof(v) == sizeof(uint32_t)) ;
    marsh_buf(m, &v, sizeof(v)) ;
}

void unmarsh_inet(unmarsh_t m, inet_t *v) {
    assert(sizeof(*v) == sizeof(uint32_t)) ;
    unmarsh_buf(m, v, sizeof(*v)) ;
}

void marsh_net_port(marsh_t m, net_port_t v) {
    marsh_buf(m, &v, sizeof(v)) ;
}

void unmarsh_net_port(unmarsh_t m, net_port_t *v) {
    unmarsh_buf(m, v, sizeof(*v)) ;
}

void marsh_sys_addr(marsh_t m, const sys_addr_t *v) {
    marsh_net_port(m, v->port) ;
    marsh_inet(m, v->inet) ;
}

void unmarsh_sys_addr(unmarsh_t m, sys_addr_t *v) {
#ifdef PURIFY
    record_clear(v) ;
#endif
    unmarsh_inet(m, &v->inet) ;
    unmarsh_net_port(m, &v->port) ;
}

void marsh_md5(marsh_t m, md5_t *v) {
    marsh_buf(m, v, sizeof(*v)) ;
}

void unmarsh_md5(unmarsh_t m, md5_t *v) {
    unmarsh_buf(m, v, sizeof(*v)) ;
}

void marsh_enum(marsh_t m, unsigned v, unsigned max) {
    assert(max <= 255) ;
    assert(v < max) ;
    marsh_uint8(m, (uint8_t)v) ;
}

unsigned unmarsh_enum_ret(unmarsh_t m, unsigned max) {
    uint8_t v ;
    assert(max <= 255) ;
    unmarsh_uint8(m, &v) ;
    assert(v < max) ;
    return (unsigned)v ;
}

void unmarsh_enum(unmarsh_t m, unsigned *ret, unsigned max) {
    *ret = unmarsh_enum_ret(m, max) ;
}

void marsh_string(marsh_t m, string_t v) {
    uint32_t len = string_length(v) ;
    marsh_buf(m, v, len) ;
    marsh_uint32(m, len) ;
}

void unmarsh_string(unmarsh_t m, string_t *ret) {
    mut_string_t v ;
    uint32_t len ;
    unmarsh_uint32(m, &len) ;
    v = sys_alloc(len + 1) ;
    unmarsh_buf(m, v, len) ;
    v[len] = '\0' ;
    *ret = v ;
}

void marsh_time(marsh_t m, etime_t id) {
    marsh_uint64(m, id.usecs) ;
}

void unmarsh_time(unmarsh_t m, etime_t *ret) {
    etime_t id ;
    unmarsh_uint64(m, &id.usecs) ;
    *ret = id ;
}

#ifndef DBD_KERNEL
#include "infr/array_supp.h"
ARRAY_MARSH(seqno, seqno)
ARRAY_MARSH(bool, bool)
ARRAY_MARSH(ltime, ltime)
#endif
