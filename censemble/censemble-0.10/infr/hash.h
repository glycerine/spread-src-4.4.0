/**************************************************************/
/* HASH.H */
/* Author: Mark Hayden, 11/2000 */
/* Copyright 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef HASH_H
#define HASH_H

static inline
void hash_init(hash_t *hash) {
    *hash = 0xFEEDFACEUL ;
}

static inline
void hash_buf(hash_t *hash_arg, const void *buf, len_t len) {
    int ofs ;
    hash_t hash = *hash_arg ;
    for (ofs=0;ofs<len;ofs++) {
	hash = (hash << 1) | (hash >> 31) ;
	hash ^= ((uint8_t*)buf)[ofs] ;
    }
    *hash_arg = hash ;
}

static inline
void hash_uint32(hash_t *hash, uint32_t v) {
    hash_buf(hash, &v, sizeof(v)) ;
}

static inline
void hash_uint64(hash_t *hash, uint64_t v) {
    hash_buf(hash, &v, sizeof(v)) ;
}

#define hash_mux(h, v) hash_uint32(h, v)
#define hash_inet(h, v) hash_uint32(h, (v).int_s)
#define hash_port(h, v) hash_uint32(h, v)
#define hash_net_port(h, v) hash_uint32(h, v.i)
#define hash_incarn(h, v) hash_uint32(h, v)

static inline
void hash_sys_addr(hash_t *hash, const sys_addr_t *v) {
    hash_inet(hash, v->inet) ;
    hash_net_port(hash, v->port) ;
}

#endif /*HASH_H*/
