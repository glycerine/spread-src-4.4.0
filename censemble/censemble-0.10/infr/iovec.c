/**************************************************************/
/* IOVEC.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#ifndef __KERNEL__
#include <sys/uio.h>
#endif
#define IOVEC_FILL_WANTED
#include "infr/iovec.h"

static string_t name UNUSED() = "IOVEC" ;

#if 0
typedef struct {
    refbuf_t rbuf ;
    ofs_t ofs ;
    len_t len ;
} iovec_body_t ;

struct iovec_t {
    len_t len ;
    iovec_body_t body[0] ;
} ;
#endif

iovec_t iovec_empty(void) {
    return iovec_fresh(0) ;
}

inline
void iovec_bounds(iovec_t iov, ofs_t ofs, len_t len) {
#ifndef NDEBUG
    if (ofs < 0 || len < 0 || len + ofs > iovec_len(iov)) {
	sys_panic(("IOVEC:bounds:ofs=%d len=%d total_len=%d",
		   ofs, len, iovec_len(iov))) ;
    }
#endif
}

#ifndef NDEBUG
void iovec_check(iovec_t iov) {
    ofs_t i ;
    assert(iov) ;
    for (i=0;i<iov->len;i++) {
	//assert(iov->body[i].len) ;
	refbuf_check(iov->body[i].rbuf) ;
    }
}
#endif

inline
iovec_t iovec_alloc(refbuf_t rbuf, ofs_t ofs, len_t len) {
    iovec_t iov ;
    iov = iovec_fresh(1) ;
    iov->len = 1 ;
    iov->body[0].rbuf = rbuf ;
    iov->body[0].ofs = ofs ;
    iov->body[0].len = len ;
    return iov ;
}

iovec_t iovec_append(iovec_t iov0, iovec_t iov1) {
    len_t len0 = iov0->len ;
    len_t len1 = iov1->len ;
    iovec_t iov ;
    ofs_t ofs ;
    iovec_check(iov0) ;
    iovec_check(iov1) ;
    
    if (len0 == 0) {
	iovec_free(iov0) ;
	return iov1 ;
    }

    if (len1 == 0) {
	iovec_free(iov1) ;
	return iov0 ;
    }

#if 0
    /* Untested code to do a merge when the iovecs share the
     * middle segment.  Actually, it has a bug, because the
     * refcounts are messed up.
     */
    {
	iovec_body_t *body ;
	body = &iov0->body[len0 - 1] ;
	assert(len0) ;
	assert(len1) ;
	if (body->rbuf == iov1->body[0].rbuf &&
	    body->ofs + body->len == iov1->body[0].ofs) {
	    iov1->body[0].ofs = body->ofs ;
	    iov1->body[0].len += body->len ;
	    len0 -- ;
	}
    }
#endif

    iov = iovec_fresh(len0 + len1) ;
    for(ofs=0;ofs<len0;ofs++) {
	iov->body[ofs] = iov0->body[ofs] ;
    }
    for(ofs=0;ofs<len1;ofs++) {
	iov->body[ofs + len0] = iov1->body[ofs] ;
    }
    if (iov0->free_me) {
	sys_free(iov0) ;
    }
    if (iov1->free_me) {
	sys_free(iov1) ;
    }
    return iov ;
}

bool_t iovec_is_empty(iovec_t iov) {
    return iov->len == 0 || iovec_len(iov) == 0 ;
}

/*#ifndef MINIMIZE_CODE*/
bool_t iovec_eq(iovec_t iov0, iovec_t iov1) {
    ofs_t ofs0 = 0 ;
    ofs_t ofs1 = 0 ;
    len_t len0 ;
    len_t len1 ;
    len_t len ;
    ofs_t iofs0 = 0 ;
    ofs_t iofs1 = 0 ;
    len_t ilen0 = iov0->len ;
    len_t ilen1 = iov1->len ;
    iovec_body_t *body0 ;
    iovec_body_t *body1 ;
    
    while (iofs0 < ilen0 || iofs1 < ilen1) {
	/* Skip over empty segments.
	 */
	if (iofs0 < ilen0 &&
	    ofs0 == iov0->body[iofs0].len) {
	    ofs0 = 0 ;
	    iofs0 ++ ;
	    continue ;
	}
	/* Skip over empty segments.
	 */
	if (iofs1 < ilen1 &&
	    ofs1 == iov1->body[iofs1].len) {
	    ofs1 = 0 ;
	    iofs1 ++ ;
	    continue ;
	}

	/* If we are at the end of one then
	 * they are not equal.
	 */
	if (iofs0 >= ilen0 ||
	    iofs1 >= ilen1) {
	    break ;
	}

	body0 = &iov0->body[iofs0] ;
	body1 = &iov1->body[iofs1] ;

	/* Take the maximum left in either segment.
	 */
	len0 = body0->len - ofs0 ;
	len1 = body1->len - ofs1 ;
	len = len0 < len1 ? len0 : len1 ;
	assert(len > 0) ;

	/* Compare them.
	 */
	if (memcmp(refbuf_read(body0->rbuf) + body0->ofs + ofs0,
		   refbuf_read(body1->rbuf) + body1->ofs + ofs1,
		   len)) {
	    break ;
	}
	
	/* Advance.
	 */
	ofs0 += len ;
	ofs1 += len ;
    }
    return (iofs0 == ilen0 && iofs1 == ilen1) ;
}
/*#endif*/

len_t iovec_flatten_buf(iovec_t iov, buf_t buf, ofs_t ofs, len_t len) {
    ofs_t i ;
    iovec_bounds(iov, ofs, len) ;
    for (i=0;i<iov->len;i++) {
	iovec_body_t *body = &iov->body[i] ;
	memcpy(buf_ofs(buf, ofs),
	       cbuf_ofs(refbuf_read(body->rbuf), body->ofs), body->len) ;
	ofs += body->len ;
    }
    return iovec_len(iov) ;
}

static inline
ofs_t sub_help(iovec_t iov, iovec_loc_t *loc, ofs_t ofs, bool_t doing_hi) {
    len_t leni ;
    len_t endi ;
    iovec_bounds(iov, ofs, 0) ;
    while (1) {
	assert (loc->ofs <= ofs) ;
	assert (loc->iov < iov->len) ;

	leni = iov->body[loc->iov].len ;
	endi = loc->ofs + leni ;
	if (ofs < endi ||
	    (doing_hi && ofs == endi)) {
	    break ;
	}

	loc->iov ++ ;
	loc->ofs += leni ;
    }

    return ofs - loc->ofs ;
}

iovec_t iovec_sub_scan(iovec_loc_t *loc, iovec_t iov, ofs_t ofs, len_t len) {
    ofs_t ofs_lo ;
    ofs_t ofs_hi ;
    ofs_t iov_lo ;
    ofs_t iov_hi ;
    ofs_t i ;
    iovec_t ret ;
    iovec_body_t *body ;
    
    iovec_bounds(iov, ofs, len) ;

    if (len == 0) {
	return iovec_empty() ;
    }

    /* Get the iovec number and offset for beginning and
     * end.
     */
    ofs_lo = sub_help(iov, loc, ofs, FALSE) ;
    iov_lo = loc->iov ;
    ofs_hi = sub_help(iov, loc, ofs + len, TRUE) ;
    iov_hi = loc->iov ;
#if 0
    eprintf("IOVEC:sub:ofs=%d len=%d lo_iov=%d lo_ofs=%d hi_iov=%d hi_ofs=%d\n",
	    ofs, len, iov_lo, ofs_lo, iov_hi, ofs_hi) ;
#endif
    if (iov_lo == iov_hi) {
	body = &iov->body[iov_lo] ;
	ret = iovec_alloc(refbuf_copy(body->rbuf),
			  body->ofs + ofs_lo, len) ;
    } else {
	int niov = iov_hi - iov_lo + 1 ;
	assert (iov_lo < iov_hi) ;
	ret = iovec_fresh(niov) ;
	ret->len = niov ;

	/* Handle beginning.
	 */
	body = &iov->body[iov_lo] ;
	ret->body[0].rbuf = refbuf_copy(body->rbuf) ;
	ret->body[0].ofs = body->ofs + ofs_lo ;
	ret->body[0].len = body->len - ofs_lo ;

	/* Middle part is just copied.
	 */
	for (i=1;i<ret->len - 1;i++) {
	    ret->body[i] = iov->body[iov_lo + i] ;
	    ret->body[i].rbuf = refbuf_copy(ret->body[i].rbuf) ;
	}

	/* Handle end.
	 */
	body = &iov->body[iov_hi] ;
	ret->body[ret->len - 1].rbuf = refbuf_copy(body->rbuf) ;
	ret->body[ret->len - 1].ofs = body->ofs ;
	ret->body[ret->len - 1].len = ofs_hi ;
    }

    assert(iovec_len(ret) == len) ;
    return ret ;
}

#if 1
len_t iovec_read_net_len(iovec_t iov, ofs_t ofs) {
    iovec_loc_t loc ;
    iovec_body_t *body ;
    len_t ret ;
    iovec_loc_init(&loc) ;
    iovec_bounds(iov, ofs, sizeof(ret)) ;
    sub_help(iov, &loc, ofs, FALSE) ;
    body = &iov->body[loc.iov] ;
    assert(ofs - loc.ofs + sizeof(ret) <= body->len) ;
    memcpy(&ret, 
	   cbuf_ofs(refbuf_read(body->rbuf), body->ofs + (ofs - loc.ofs)),
	   sizeof(ret)) ;
    ret = ntohl(ret) ;
    return ret ;
}
#endif

iovec_t iovec_sub(iovec_t iov, ofs_t ofs, len_t len) {
    iovec_loc_t loc ;
    iovec_loc_init(&loc) ;
    return iovec_sub_scan(&loc, iov, ofs, len) ;
}

iovec_t iovec_sub_take(iovec_t iov, ofs_t ofs, len_t len) {
    iovec_loc_t loc ;
    iovec_t ret ;
    iovec_loc_init(&loc) ;
    ret = iovec_sub_scan(&loc, iov, ofs, len) ;
    iovec_free(iov) ;
    return ret ;
}

void iovec_split(iovec_t iov, ofs_t ofs, iovec_t *iov0, iovec_t *iov1) {
    iovec_loc_t loc ;
    len_t len = iovec_len(iov) ;
    iovec_loc_init(&loc) ;
    *iov0 = iovec_sub_scan(&loc, iov, 0, ofs) ;
    *iov1 = iovec_sub_scan(&loc, iov, ofs, len - ofs) ;
    iovec_free(iov) ;
}

#ifndef __KERNEL__
void iovec_md5_update(iovec_t iov, MD5Context *ctx) {
    ofs_t ofs ;
    for (ofs=0;ofs<iov->len;ofs++) {
	iovec_body_t *b = &iov->body[ofs] ;
	MD5Update(ctx, cbuf_ofs(refbuf_read(b->rbuf), b->ofs), b->len) ;
    }
}
#endif

void iovec_hash(iovec_t iov, hash_t *h) {
    ofs_t ofs ;
    for (ofs=0;ofs<iov->len;ofs++) {
	iovec_body_t *b = &iov->body[ofs] ;
	hash_buf(h, cbuf_ofs(refbuf_read(b->rbuf), b->ofs), b->len) ;
    }
}

void iovec_xfer(
        iovec_loc_t *loc_dst,
	iovec_t iovec_dst,
	ofs_t ofs_dst,
        iovec_loc_t *loc_src,
	iovec_t iovec_src,
	ofs_t ofs_src,
	len_t len
) {
    iovec_body_t *body_src ;
    iovec_body_t *body_dst ;

    iovec_bounds(iovec_src, ofs_src, len) ;
    iovec_bounds(iovec_dst, ofs_dst, len) ;
    
    if (!len) {
	return ;
    }

    ofs_src = sub_help(iovec_src, loc_src, ofs_src, FALSE) ;
    body_src = &iovec_src->body[loc_src->iov] ;

    ofs_dst = sub_help(iovec_dst, loc_dst, ofs_dst, FALSE) ;
    body_dst = &iovec_dst->body[loc_dst->iov] ;

    while (len) {
	len_t len_src ;
	len_t len_dst ;
	len_t amt ;

	assert(ofs_src <= body_src->len) ;
	if (ofs_src == body_src->len) {
	    ofs_src = 0 ;
	    body_src ++ ;
	}
	assert(body_src - iovec_src->body < iovec_src->len) ;

	assert(ofs_dst <= body_dst->len) ;
	if (ofs_dst == body_dst->len) {
	    ofs_dst = 0 ;
	    body_dst ++ ;
	}
	assert(body_dst - iovec_dst->body < iovec_dst->len) ;

	len_src = body_src->len - ofs_src ;
	len_dst = body_dst->len - ofs_dst ;
	
	if (len_src > len_dst) {
	    amt = len_dst ;
	} else {
	    amt = len_src ;
	}
	
	if (amt > len) {
	    amt = len ;
	}

	memcpy(buf_ofs(refbuf_write(body_dst->rbuf), body_dst->ofs + ofs_dst),
	       cbuf_ofs(refbuf_read(body_src->rbuf), body_src->ofs + ofs_src),
	       amt) ;
	
	ofs_src += amt ;
	ofs_dst += amt ;
	len -= amt ;
    }
}

typedef void (*copy_t)(buf_t, refbuf_t, ofs_t, len_t) ;

static inline
void iovec_scan(
        iovec_loc_t *loc,
	iovec_t iov,
	ofs_t ofs,
	len_t len,
	buf_t buf,
	copy_t copy
) {
    ofs_t i ;
    ofs_t ofs_lo ;
    ofs_t ofs_hi ;
    ofs_t iov_lo ;
    ofs_t iov_hi ;
    ofs_t ofs_dst ;
    iovec_body_t *body ;

    iovec_bounds(iov, ofs, len) ;
    
    if (!len) {
	return ;
    }

    ofs_lo = sub_help(iov, loc, ofs, FALSE) ;
    iov_lo = loc->iov ;
    ofs_hi = sub_help(iov, loc, ofs + len, TRUE) ;
    iov_hi = loc->iov ;
    
    if (iov_lo == iov_hi) {
	body = &iov->body[iov_lo] ;
	copy(buf, body->rbuf, body->ofs + ofs_lo, ofs_hi - ofs_lo) ;
	return ;
    }

    ofs_dst = 0 ;
    
    /* Beginning.
     */
    body = &iov->body[iov_lo] ;
    assert(body->len > ofs_lo) ;
    copy(buf_ofs(buf, ofs_dst), body->rbuf, body->ofs + ofs_lo, body->len - ofs_lo) ;
    ofs_dst += body->len - ofs_lo ;
    
    /* Middle.
     */
    for (i=iov_lo + 1;i<iov_hi;i++) {
	body = &iov->body[i] ;
	copy(buf_ofs(buf, ofs_dst), body->rbuf, body->ofs, body->len) ;
	ofs_dst += body->len ;
    }
    
    /* End.
     */
    body = &iov->body[iov_hi] ;
    copy(buf_ofs(buf, ofs_dst), body->rbuf, body->ofs, ofs_hi) ;
    ofs_dst += ofs_hi ;

    assert(iov_lo != iov_hi) ;
    assert(ofs_dst == len) ;
}

static inline
void scan_write(buf_t buf, refbuf_t rbuf, ofs_t ofs, len_t len) {
    memcpy(buf_ofs(refbuf_write(rbuf), ofs), buf, len) ;
}

static inline
void scan_write_zero(buf_t buf, refbuf_t rbuf, ofs_t ofs, len_t len) {
    memset(buf_ofs(refbuf_write(rbuf), ofs), 0, len) ;
}

static inline
void scan_read(buf_t buf, refbuf_t rbuf, ofs_t ofs, len_t len) {
    memcpy(buf, cbuf_ofs(refbuf_read(rbuf), ofs), len) ;
}

void iovec_read_scan(
        iovec_loc_t *loc,
	iovec_t iov,
	ofs_t ofs,
	len_t len,
	buf_t buf
) {
    iovec_scan(loc, iov, ofs, len, buf, scan_read) ;
}

void iovec_write_scan(
        iovec_loc_t *loc,
	iovec_t iov,
	ofs_t ofs,
	len_t len,
	buf_t buf
) {
    iovec_scan(loc, iov, ofs, len, buf, scan_write) ;
}

void iovec_write_zero_scan(
        iovec_loc_t *loc,
	iovec_t iov,
	ofs_t ofs,
	len_t len,
	buf_t buf
) {
    iovec_scan(loc, iov, ofs, len, buf, scan_write_zero) ;
}

void iovec_read(iovec_t iov, ofs_t ofs, len_t len, buf_t buf) {
    iovec_loc_t loc ;
    iovec_loc_init(&loc) ;
    iovec_read_scan(&loc, iov, ofs, len, buf) ;
}

void iovec_write(iovec_t iov, ofs_t ofs, len_t len, buf_t buf) {
    iovec_loc_t loc ;
    iovec_loc_init(&loc) ;
    iovec_write_scan(&loc, iov, ofs, len, buf) ;
}

void iovec_write_zero(iovec_t iov, ofs_t ofs, len_t len) {
    iovec_loc_t loc ;
    iovec_loc_init(&loc) ;
    iovec_write_zero_scan(&loc, iov, ofs, len, NULL) ; /* Hack */
}

iovec_t iovec_concat(iovec_t *iova, len_t len) {
    ofs_t ofs ;
    ofs_t tofs ;
    len_t tlen ;
    iovec_t iov ;
    
    for (tlen=0,ofs=0;ofs<len;ofs++) {
	iovec_check(iova[ofs]) ;
	tlen += iova[ofs]->len ;
    }

    iov = iovec_fresh(tlen) ;

    for (tofs=0,ofs=0;ofs<len;ofs++) {
	iovec_t tmp = iova[ofs] ;
	assert(tofs + tmp->len <= tlen) ;
	memcpy(&iov->body[tofs], &tmp->body[0], tmp->len * sizeof(tmp->body[0])) ;
	tofs += tmp->len ;
	if (tmp->free_me) {
	    sys_free(tmp) ;
	}
    }
    assert(tofs == tlen) ;
    iovec_check(iov) ;
    return iov ;
}

iovec_t iovec_zeroes(len_t len) {
    iovec_t iovec ;
    unsigned nbuf ;
    unsigned i ;
    nbuf = (len + REFBUF_ZERO_LEN - 1) / REFBUF_ZERO_LEN ;
    iovec = iovec_fresh(nbuf) ;
    
    for (i=0;i<nbuf;i++) {
	iovec_body_t *body = &iovec->body[i] ;
	body->rbuf = refbuf_zero() ;
	body->ofs = 0 ;
	if (i < nbuf - 1) {
	    body->len = REFBUF_ZERO_LEN ;
	} else {
	    body->len = len % REFBUF_ZERO_LEN ;
	    if (!body->len) {
		body->len = REFBUF_ZERO_LEN ;
	    }
	}
    }
    assert_eq_uint32(iovec_len(iovec), len) ;
    return iovec ;
}

iovec_t iovec_scratch(len_t len) {
    iovec_t iovec ;
    unsigned nbuf ;
    unsigned i ;
    nbuf = (len + REFBUF_ZERO_LEN - 1) / REFBUF_ZERO_LEN ;
    iovec = iovec_fresh(nbuf) ;
    
    for (i=0;i<nbuf;i++) {
	iovec_body_t *body = &iovec->body[i] ;
	body->rbuf = refbuf_scratch() ;
	body->ofs = 0 ;
	if (i < nbuf - 1) {
	    body->len = REFBUF_ZERO_LEN ;
	} else {
	    body->len = len % REFBUF_ZERO_LEN ;
	    if (!body->len) {
		body->len = REFBUF_ZERO_LEN ;
	    }
	}
    }
    assert_eq_uint32(iovec_len(iovec), len) ;
    return iovec ;
}

inline
iovec_t iovec_of_buf(buf_t buf, ofs_t ofs, len_t len) {
    iovec_t iovec ;
    refbuf_t rbuf ;
    rbuf = refbuf_alloc(buf) ;
    iovec = iovec_alloc(rbuf, ofs, len) ;
    return iovec ;
}

iovec_t iovec_of_buf_copy(cbuf_t buf, ofs_t ofs, len_t len) {
    buf_t tmp = sys_alloc(len) ;
    memcpy(tmp, buf + ofs, len) ;
    return iovec_of_buf(tmp, 0, len) ;
}

inline
iovec_t iovec_alloc_writable(len_t len) {
    return iovec_of_buf(sys_alloc(len), 0, len) ;
}

#ifndef NDEBUG
iovec_t iovec_take(iovec_t iov) {
#if 1
    return iov ;
#elif 0
    iovec_t new = iovec_copy(iov) ;
    iovec_free(iov) ;
    return new ;
#else
    len_t len = iovec_len(iov) ;
    buf_t buf = sys_alloc(len) ;
    iovec_t new ;
    iovec_read(iov, 0, len, buf) ;
    new = iovec_of_buf(buf, 0, len) ;
    iovec_free(iov) ;
    return new ;
#endif
}
#endif

#ifndef MINIMIZE_CODE
void iovec_dump(debug_t debug, iovec_t iov) {
    ofs_t ofs ;
    eprintf("IOVEC:dump name=%s niov=%d len=%d\n", debug, iov->len, iovec_len(iov)) ;
    for (ofs=0;ofs<iov->len;ofs++) {
	iovec_body_t *body = &iov->body[ofs] ;
	eprintf("  %d:ofs=%d len=%d buf=%08lx\n", ofs, body->ofs, body->len, (unsigned long)refbuf_read(body->rbuf)) ;
    }
#ifndef DBD_KERNEL
    if (0) {
	len_t len = iovec_len(iov) ;
	buf_t buf = sys_alloc(len) ;
	string_t s ;
	iovec_flatten_buf(iov, buf, 0, len) ;
	s = hex_of_bin(buf, len) ;
	eprintf("%s\n", s) ;
	string_free(s) ;
    }
#endif
}
#endif

#if 1
void iovec_test(void) {
    int i ;
    for (i=0;i<100000;i++) {
	int j ;
	buf_t buf ;
	ofs_t ofs ;
	len_t len ;
	iovec_t iov ;
	iovec_t iov0 ;
	iovec_t iov1 ;
	iovec_t iovc ;
	len = sys_random(1000) ;
	buf = sys_alloc(len) ;
	
	/* Scramble the buffer.
	 */
	for (ofs=0;ofs<len;ofs++) {
	    ((char*)buf)[ofs] = sys_random(256) ;
	}
	iov = iovec_alloc_writable(len) ;

	/* Repeatedly split buffer, put back together, and
	 * test for equality.
	 */
	for (j=0;j<100;j++) {
	    ofs = sys_random(len + 1) ;
	    iov0 = iovec_sub(iov, 0, ofs) ;
	    iov1 = iovec_sub(iov, ofs, len - ofs) ;
	    
	    if (sys_random(2)) {
		iovc = iovec_append(iov0, iov1) ;
	    } else {
		iovec_loc_t loc0 ;
		iovec_loc_t loc1 ;
		iovc = iovec_alloc_writable(iovec_len(iov0) + iovec_len(iov1)) ;
		iovec_loc_init(&loc0) ;
		iovec_loc_init(&loc1) ;
		iovec_xfer(&loc0, iovc, 0, &loc1, iov0, 0, iovec_len(iov0)) ; 
		iovec_loc_init(&loc1) ;
		iovec_xfer(&loc0, iovc, iovec_len(iov0), &loc1, iov1, 0, iovec_len(iov1)) ; 
	    }
	    if (iovec_len(iov) != iovec_len(iovc) ||
		!iovec_eq(iov, iovc)) {
		eprintf("j=%d\n", j) ;
		iovec_dump("iov", iov) ;
		iovec_dump("iovc", iovc) ;
		iovec_dump("iov0", iov0) ;
		iovec_dump("iov1", iov1) ;
	    }
	    assert_eq(iovec_len(iov), iovec_len(iovc)) ;
	    assert(iovec_eq(iov, iovc)) ;
	    iov = iovc ;
	}
	eprintf("count=%d len=%d nvec=%d\n", i, len, iov->len) ;
    }
}
#endif

void iovec_fill(iovec_t iov, struct iovec *iovec, int *niovec) {
    ofs_t ofs ;
    iovec_check(iov) ;
#if 0
    if (iov->len > *niovec) {
	eprintf("niovec=%u iov->len=%u\n", *niovec, iov->len) ;
    }
#endif
    assert(iov->len <= *niovec) ;
    for (ofs=0;ofs<iov->len;ofs++) {
	iovec[ofs].iov_base = buf_ofs((void*)refbuf_read(iov->body[ofs].rbuf), iov->body[ofs].ofs) ;
	assert(iov->body[ofs].len) ;
	iovec[ofs].iov_len = iov->body[ofs].len ;
    }
    *niovec = iov->len ;
}

void iovec_fill_sub(
        iovec_t iov,
	iovec_loc_t *loc,
	ofs_t ofs,
	len_t len,
	struct iovec *iovec,
	int *niovec
) {
    ofs_t i ;
    ofs_t ofs_lo ;
    ofs_t ofs_hi ;
    ofs_t iov_lo ;
    ofs_t iov_hi ;
    iovec_body_t *body ;

    iovec_bounds(iov, ofs, len) ;
    
    if (!len) {
	*niovec = 0 ;
	return ;
    }

    iovec_check(iov) ;

    ofs_lo = sub_help(iov, loc, ofs, FALSE) ;
    iov_lo = loc->iov ;
    ofs_hi = sub_help(iov, loc, ofs + len, TRUE) ;
    iov_hi = loc->iov ;

    if (iov_lo == iov_hi) {
	body = &iov->body[iov_lo] ;
	iovec[0].iov_base = buf_ofs((void*)refbuf_read(body->rbuf), body->ofs + ofs_lo) ;
	iovec[0].iov_len = ofs_hi - ofs_lo ;
	*niovec = 1 ;
	return ;
    }

    *niovec = iov_hi - iov_lo + 1 ;

    /* Handle beginning.
     */
    body = &iov->body[iov_lo] ;
    iovec[0].iov_base = buf_ofs((void*)refbuf_read(body->rbuf), body->ofs + ofs_lo) ;
    iovec[0].iov_len = body->len - ofs_lo ;
    
    /* Handle end.
     */
    body = &iov->body[iov_hi] ;
    iovec[iov_hi - iov_lo].iov_base = buf_ofs((void*)refbuf_read(body->rbuf), body->ofs) ;
    iovec[iov_hi - iov_lo].iov_len = ofs_hi ;

    /* Middle.
     */
    for (i=1;i<iov_hi - iov_lo;i++) {
	body = &iov->body[iov_lo + i] ;
	iovec[i].iov_base = buf_ofs((void*)refbuf_read(body->rbuf), body->ofs) ;
	iovec[i].iov_len = body->len ;
    }
}
