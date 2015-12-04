/**************************************************************/
/* IOVEC.H */
/* Author: Mark Hayden, 11/99 */
/* Unless otherwise mentioned, these functions do not free
 * the reference counts of their arguments and iovecs
 * returned have all been correctly reference counted. */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef IOVEC_H
#define IOVEC_H

#include "infr/trans.h"
#include "infr/util.h"
#include "infr/hash.h"
#include "infr/refbuf.h"
#include "infr/md5.h"

/* Type of iovec arrays.
 */
typedef struct iovec_t *iovec_t ;

typedef array_def(iovec) iovec_array_t ;

/**************************************************************/

typedef struct {
    refbuf_t rbuf ;
    ofs_t ofs ;
    len_t len ;
} iovec_body_t ;

struct iovec_t {
    len_t len ;
    bool_t free_me ;
    iovec_body_t body[1] ;
} ;

/**************************************************************/

/* Refcounting operations: these apply appropriate
 * refbuf operation to all member iovecs.
 */

#ifndef NDEBUG
void iovec_check(iovec_t) ;
#else
static inline
void iovec_check(iovec_t iov) { }
#endif

#ifndef NDEBUG
iovec_t iovec_take(iovec_t iov) ;
#else
static inline
iovec_t iovec_take(iovec_t iov) { return iov ; }
#endif

/* This calculates sizes of iovecs, dealing with
 * the definition using body[0] or body[1].
 */
static inline
len_t iovec_calc_len(len_t niov) {
    iovec_t iov ;
    return sizeof(*iov) -
	sizeof(iov->body) + 
	sizeof(iov->body[0]) * niov ;
}

static inline
iovec_t iovec_fresh(len_t len) {
    iovec_t iov = sys_alloc(iovec_calc_len(len)) ;
    iov->len = len ;
    iov->free_me = TRUE ;
    return iov ;
}

static inline
iovec_t iovec_copy(iovec_t iov) {
    iovec_t new ;
    len_t len ;
    ofs_t ofs ;
    iovec_check(iov) ;
    assert(iov) ;
    len = iov->len ;
    new = iovec_fresh(len) ;
    for (ofs=0;ofs<len;ofs++) {
	new->body[ofs].rbuf = refbuf_copy(iov->body[ofs].rbuf) ;
	new->body[ofs].ofs = iov->body[ofs].ofs ;
	new->body[ofs].len = iov->body[ofs].len ;
    }
    return new ;
}

static inline
void iovec_free(iovec_t iov) {
    ofs_t ofs ;
    len_t len = iov->len ;
    iovec_check(iov) ;
    for (ofs=0;ofs<len;ofs++) {
	refbuf_free(iov->body[ofs].rbuf) ;
    }
    if (iov->free_me) {
	sys_free(iov) ;
    }
}

static inline
len_t iovec_len(iovec_t iov) {
    ofs_t i ;
    len_t cnt ;
    iovec_check(iov) ;
    cnt = 0 ;
    for (i=0;i<iov->len;i++) {
	cnt += iov->body[i].len ;
    }
    return cnt ;
}

/**************************************************************/

/* Calculate total length of an array of iovecs.
 */
len_t iovec_len(iovec_t) ;

/* Check if the contents are equal.  Does not affect references.  
 */
bool_t iovec_eq(iovec_t, iovec_t) ;

/* Flatten an iovec array into a single iovec.  Copying only
 * occurs if the array has more than 1 non-empty iovec.
 */
iovec_t iovec_flatten(iovec_t) ; /* allocates */

/* Fragment iovec array into an array of iovec arrays of
 * some maximum size.  No copying occurs.  The resulting
 * iovec arrays are at most the specified length.
 */
iovec_array_t iovec_fragment(len_t, iovec_t) ;

/* Remove any empty iovecs from the array.
 * Refcounts are not affected.
 */
iovec_t iovec_clean(iovec_t) ;

/* Check if the given ofs and len are valid in
 * the iovec.  Aborts if not.
 */
void iovec_bounds(iovec_t, ofs_t, len_t) ;

/* Read 4 bytes of iovec as a 32-bit integer in *
 * network-byte-order.  
 */
int iovec_read_net_int(iovec_t, ofs_t) ;

len_t iovec_read_net_len(iovec_t, ofs_t) ;

/* Print information about an array of iovecs.  For debugging
 */
void iovec_dump(debug_t, iovec_t) ;

/* Flatten an iovec array into a string buffer.
 */
len_t iovec_flatten_buf(iovec_t, buf_t, ofs_t, len_t) ;

/* An empty iovec array.
 */
iovec_t iovec_empty(void) ;
bool_t iovec_is_empty(iovec_t) ;

/* Append two iovec arrays.  Takes the reference count.
 */
iovec_t iovec_append(iovec_t, iovec_t) ;

/* Catenate a list of iovec arrays.  Takes the reference count.
 */
#if 0
iovec_t iovec_concat(iovec_array_t) ;
#else
iovec_t iovec_concat(iovec_t *, len_t) ;
#endif

/* Prepend a single iovec to an iovec array.  Does not
 * affect refcounts.  
 */
/*void iovec_prependi( iovec.t -> t -> t*/
/*void iovec_appendi(iovec_t, iovec.t -> t) ;*/
		   
/* Get a subset of an iovec.  The ofs is relative
 * to the iovec.  The returned object is properly
 * reference counted.
 */
iovec_t iovec_sub(iovec_t, ofs_t, len_t) ;

void iovec_split(iovec_t, ofs_t, iovec_t*, iovec_t*) ;

/* Read/write the data from the given range into/from the buffer.
 * The buffer must be at least of the specified length.
 */
void iovec_read(iovec_t, ofs_t, len_t, buf_t) ;
void iovec_write(iovec_t, ofs_t, len_t, buf_t) ;

void iovec_write_zero(iovec_t, ofs_t, len_t) ;

/* Same as above, but also frees the input iovec.
 */
iovec_t iovec_sub_take(iovec_t, ofs_t, len_t) ;

/* This is used for reading iovecs from left to right.
 * The loc objects prevent the sub operation from O(n^2)
 * behavior when repeatedly sub'ing an iovec.  loc0 is
 * then beginning of the iovec.
 */
typedef struct {
    ofs_t iov ;
    ofs_t ofs ;
} iovec_loc_t ;

static inline
void iovec_loc_init(iovec_loc_t *loc) {
    loc->iov = 0 ;
    loc->ofs = 0 ;
}

iovec_t iovec_sub_scan(iovec_loc_t *, iovec_t, ofs_t, len_t) ;

void iovec_read_scan(iovec_loc_t *, iovec_t, ofs_t, len_t, buf_t) ;

void iovec_write_scan(iovec_loc_t *, iovec_t, ofs_t, len_t, buf_t) ;

void iovec_xfer(
        iovec_loc_t *loc_dst,
	iovec_t iovec_dst,
	ofs_t ofs_dst,
        iovec_loc_t *loc_src,
	iovec_t iovec_src,
	ofs_t ofs_src,
	len_t len
) ;

void iovec_md5_update(iovec_t, MD5Context *) ;

void iovec_hash(iovec_t, hash_t *) ;

/* Allocate an iovec.  The reference count is not incremented.
 */
iovec_t iovec_alloc(refbuf_t, ofs_t, len_t) ;

iovec_t iovec_alloc_writable(len_t) ;

iovec_t iovec_zeroes(len_t) ;

iovec_t iovec_scratch(len_t) ;

/* Intern a buf into an iovec.  The buf will be freed when the last
 * reference is dropped.
 */
iovec_t iovec_of_buf(buf_t, ofs_t, len_t) ;

/* Copy the buf and make an iovec out of it.
 */
iovec_t iovec_of_buf_copy(cbuf_t, ofs_t, len_t) ;

/* Allocate an uninitialized iovec.
 */
iovec_t iovec_fresh(len_t len) ;

#ifdef IOVEC_FILL_WANTED
void iovec_fill(iovec_t, struct iovec *, int *niovec) ;
void iovec_fill_sub(iovec_t, iovec_loc_t *, ofs_t, len_t, struct iovec *, int *niovec) ;
#endif

static inline
void iovec_of_buf_init1(
        iovec_t iovec,
	refbuf_t refbuf,
	ofs_t ofs,
	len_t len
) {
    iovec->len = 1 ;
    iovec->free_me = FALSE ;
    iovec->body[0].rbuf = refbuf ;
    iovec->body[0].ofs = ofs ;
    iovec->body[0].len = len ;
}

void iovec_test(void) ;
#endif /* IOVEC_H */
