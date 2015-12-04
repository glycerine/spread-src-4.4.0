/**************************************************************/
/* TRANS.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef TRANS_H
#define TRANS_H

/* Must be first so that _REENTRANT and LINUX_THREADS are defined
 * before other headers.
 */
#include "infr/config.h"

/* This disables use of stdint.h.  Yuck.
 */
//#undef HAVE_STDINT_H
//#define HAVE_STDINT_H 0

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/string.h>
#elif HAVE_STDINT_H
#include <stdint.h>
#elif HAVE_SYS_INT_TYPES_H
/* This is used on Solaris to define the uintXX_t types.
 */
#include <sys/int_types.h>
#elif HAVE_INTTYPES_H
/* This is for NetBSD.
 */
#include <inttypes.h>
#elif defined(__uint32_t_defined)
/* Nop.
 */
#else
#define __uint32_t_defined

/* Note
 */

/* Ok, we'll figure out the types to use...
 */
#if SIZEOF_CHAR == 1
typedef unsigned char uint8_t ;
#else
#error No appropriate type for uint8_t
#endif

#if SIZEOF_SHORT == 2
typedef unsigned short uint16_t ;
#elif SIZEOF_INT == 2
typedef unsigned int uint16_t ;
#else
#error No appropriate type for uint16_t
#endif

#if SIZEOF_INT == 4
typedef unsigned int uint32_t ;
#elif SIZEOF_LONG == 4
typedef unsigned long uint32_t ;
#else
#error No appropriate type for uint32_t
#endif

#if SIZEOF_LONG == 8
typedef unsigned long uint64_t ;
#elif !defined(SIZEOF_LONG_LONG)
#error No appropriate type for uint64_t
#elif SIZEOF_LONG_LONG == 8
typedef unsigned long long uint64_t ;
#else
#error No appropriate type for uint64_t
#endif

#endif

#ifndef __KERNEL__
#include <string.h>		/* for -fno-builtin */
#else
//#include <linux/types.h>
//#include <linux/string.h>
//#include <asm/string.h>
//#include <linux/net.h>
#endif

typedef void       *buf_t ;
typedef const void *cbuf_t;
typedef const char *string_t ;
typedef char       *mut_string_t ;
typedef double      float_t ;
typedef int         bool_t ;
typedef uint32_t    mux_t ;
typedef uint16_t    port_t ;	/* == 0 special */
typedef uint32_t    incarn_t ;
typedef uint32_t    salt_t ;
typedef int         vci_t ;
typedef uint32_t    rank_t ;
typedef uint32_t    nmembers_t ;
typedef uint32_t    data_int_t ;
typedef uint64_t    seqno_t ;
typedef uint64_t    ltime_t ;
typedef uint32_t    bitfield_t ;
typedef int         fanout_t ;
typedef string_t    name_t ;
typedef string_t    debug_t ;
typedef rank_t      origin_t ;
typedef uint32_t    ofs_t ;
typedef uint32_t    len_t ;
typedef void       *env_t ;
typedef uint32_t    hash_t ;

typedef struct { uint64_t i ; } lofs_t ;

static const
lofs_t lofs_zero = { 0 } ;

#ifdef __KERNEL__
#include <linux/socket.h>
typedef struct socket *sock_t ;
#else
typedef int         sock_t ;
#endif
   
typedef void (*sock_handler_t)(env_t, sock_t) ;
typedef void (*handler_t)(env_t) ;

typedef struct {
    handler_t handler ;
    env_t env ;
} closure_t ;

typedef union {
    uint8_t char_s[4] ;
    uint32_t int_s ;
} inet_t ;

typedef struct {
    uint16_t i ;
} net_port_t ;

typedef struct md5_t {
    char raw[16] ;
} md5_t ;

#undef TRUE
#undef FALSE
#define TRUE (1)
#define FALSE (0)

#define LTIME_INVALID 0
#define LTIME_FIRST 1

/* Some GCC attribute stuff.
 */
#ifdef __GNUC__
#define UNUSED()        __attribute__ ((unused))
#define FORMAT(a,b,c)   __attribute__ ((format(a,b,c)))
#define NORETURN()      __attribute__ ((noreturn))
#else
#define __FUNCTION__ "?FUNCTION?"
#define UNUSED()
#define FORMAT(a,b,c)
#define NORETURN()
#define inline
#endif

#define OTHERWISE_ABORT() default: sys_abort() ; break ;

#define NOP do {} while (0)
#define NOP_USE(x) do { if (0 && (x)) {} } while (0)
#define NOP_USE2(x, y) do { if (0 && (x) && (y)) {} } while (0)

#ifdef NDEBUG
#define assert(x) NOP_USE(x)
#define assert_eq(x, y) NOP_USE2(x, y)
#define assert_eq_bool(x, y) NOP_USE2(x, y)
#define assert_eq_uint32(x, y) NOP_USE2(x, y)
#define assert_eq_uint64(x, y) NOP_USE2(x, y)
#else
void sys_assert_fail(
        const char *assertion,
	const char *file,
	unsigned int line,
	const char *function
) NORETURN() ;

void sys_assert_eq_uint64_fail(
	const char *exp0,
	const char *exp1,
	uint64_t v0,
	uint64_t v1,
	const char *file,
	unsigned int line,
	const char *function
) NORETURN() ;

#define assert(x) \
  do { \
    if (!(x)) { \
       sys_assert_fail(#x, __FILE__, __LINE__, __FUNCTION__) ; \
    } \
  } while (0)

#define assert_eq_uint64(x, y) \
  do { \
    uint64_t tmp_x = (x) ; \
    uint64_t tmp_y = (y) ; \
    if (tmp_x != tmp_y) { \
       sys_assert_eq_uint64_fail(#x, #y, tmp_x, tmp_y, \
                                 __FILE__, __LINE__, __FUNCTION__) ; \
    } \
  } while (0)

#define assert_eq(x, y) assert_eq_uint32(x, y) ;

/* Use uint64_eq failure function.
 */
#define assert_eq_uint32(x, y) \
  do { \
    uint64_t tmp_x = (x) ; \
    uint64_t tmp_y = (y) ; \
    if (tmp_x != tmp_y) { \
       sys_assert_eq_uint64_fail(#x, #y, tmp_x, tmp_y, \
                                 __FILE__, __LINE__, __FUNCTION__) ; \
    } \
  } while (0)

#define assert_eq_bool(x, y) \
  do { \
    uint64_t tmp_x = (x) ; \
    uint64_t tmp_y = (y) ; \
    assert_bool(tmp_x) ; \
    assert_bool(tmp_y) ; \
    if (tmp_x != tmp_y) { \
       sys_assert_eq_uint64_fail(#x, #y, tmp_x, tmp_y, \
                                 __FILE__, __LINE__, __FUNCTION__) ; \
    } \
  } while (0)
#endif

static inline 
void assert_bool(bool_t b) {
    assert(b == TRUE || b == FALSE) ;
}

#if 0
#ifdef MINIMIZE_CODE
#define to_string() do { return "<>" ; } while (0)
#else
#define to_string() NOP
#endif
#endif

/* For byteswap operations.
 */
#ifndef __KERNEL__
#include <sys/types.h>
#include <netinet/in.h>
#else
#include <asm/byteorder.h>
#endif

typedef struct {
    uint16_t net ;
} net_uint16_t ;

typedef struct {
    uint32_t net ;
} net_uint32_t ;

typedef struct {
    uint64_t net ;
} net_uint64_t ;

typedef struct {
    uint16_t net ;
} rnet_uint16_t ;

typedef struct {
    uint32_t net ;
} rnet_uint32_t ;

typedef struct {
    uint64_t net ;
} rnet_uint64_t ;

static inline
net_uint16_t trans_hton16(uint16_t h) {
    net_uint16_t n ;
    n.net = htons(h) ;
    return n ;
}

static inline
uint16_t trans_ntoh16(net_uint16_t n) {
    uint16_t h ;
    h = ntohs(n.net) ;
    return h ;
}

static inline
net_uint32_t trans_hton32(uint32_t h) {
    net_uint32_t n ;
    n.net = htonl(h) ;
    return n ;
}

static inline
uint32_t trans_ntoh32(net_uint32_t n) {
    uint32_t h ;
    h = ntohl(n.net) ;
    return h ;
}

static inline
uint64_t swap_uint64(uint64_t v) {
#if WORDS_BIGENDIAN
    return v ;
#else
    return htonl((uint32_t)(v >> 32)) | ((uint64_t)(htonl((uint32_t)(v & 0xffffffffUL))) << 32) ;
#endif
}

static inline
net_uint64_t trans_hton64(uint64_t h) {
    net_uint64_t n ;
    n.net = swap_uint64(h) ;
    return n ;
}

static inline
uint64_t trans_ntoh64(net_uint64_t n) {
    uint64_t h ;
    h = swap_uint64(n.net) ;
    return h ;
}


static inline
rnet_uint16_t trans_htor16(uint16_t h) {
    rnet_uint16_t n ;
    n.net = h ;
    return n ;
}

static inline
uint16_t trans_rtoh16(rnet_uint16_t n) {
    uint16_t h ;
    h = n.net ;
    return h ;
}

static inline
rnet_uint32_t trans_htor32(uint32_t h) {
    rnet_uint32_t n ;
    n.net = h ;
    return n ;
}

static inline
uint32_t trans_rtoh32(rnet_uint32_t n) {
    uint32_t h ;
    h = n.net ;
    return h ;
}

static inline
rnet_uint64_t trans_htor64(uint64_t h) {
    rnet_uint64_t n ;
    n.net = h ;
    return n ;
}

static inline
uint64_t trans_rtoh64(rnet_uint64_t n) {
    uint64_t h ;
    h = n.net ;
    return h ;
}

static inline
port_t trans_port_ntoh(net_port_t np) {
    net_uint16_t n ;
    n.net = np.i ;
    return trans_ntoh16(n) ;
}

static inline
net_port_t trans_port_hton(port_t p) {
    net_uint16_t n ;
    net_port_t np ;
    n = trans_hton16(p) ;
    np.i = n.net ;
    return np ;
}

#endif /* TRANS_H */
