/* infr/config.h.  Generated automatically by configure.  */
/**************************************************************/
/* CONFIG.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

/* Causes malloc'd memory to be initialized to random values.
 */
/*#define ALLOC_SCRAMBLE*/

/* Controls use of fast layer scheduler.
 */
#ifndef __KERNEL__		/* uses too much stack for kernel */
#define LAYER_SCHED_FAST
#endif

/* Controls whether layers should be dynamically loaded.
 */
/*#define LAYER_DYNLINK*/

#define RAND_WAIT_FOR_ALL

/* Size of buffers to request on UDP sockets.
 */
#define SOCK_BUF_SIZE (1<<20)

/* Port number used for IP multicast messages.
 */
#define UDP_MULTICAST_PORT 8453

/* Port number to use for the gossip service.
 */
#define UDP_GOSSIP_PORT 8454

/* Set this to 0 if using IP multicast for gossip communication.  Set
 * it to 1 if using the gossip service.
 */
#define UDP_USE_GOSSIP 0

#if 0
/* Macros controlling use of Hans-Boehm GC library.
 */
/*#define USE_GC*/
/*#define GC_DEBUG*/
/*#define GC_LEAK*/

/*#define USE_MCHECK*/
#endif

/* These control aspects of using the Electric Fence library.
 */
/*#define FENCE_BELOW*/
/*#define FENCE_FREE*/

/* Controls whether we want to debug array lengths.
 */
#define ARRAY_LENGTH_DEBUG

#ifdef __linux__
#define _REENTRANT
#define LINUX_THREADS
#endif

#define HAVE_STDINT_H 0

#define HAVE_SYS_INT_TYPES_H 0

#define HAVE_INTTYPES_H 1

#define HAVE_SYS_TIME_H 1

#define HAVE_UNISTD_H 1

#define HAVE_POLL_H 1

#define HAVE_POLL 1

#define HAVE_VSNPRINTF 1

#define HAVE_VSPRINTF 1

#define HAVE_WRITEV 1

#define HAVE_READV 1

#define HAVE_PREAD64 1

#define HAVE_PWRITE64 1

#define HAVE_LSEEK64 1

#define HAVE_LLSEEK 0

/* The number of bytes in a char.  */
#define SIZEOF_CHAR 1

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a long long.  */
#define SIZEOF_LONG_LONG 8

/* Is this machine big endian or not. */
#define WORDS_BIGENDIAN 1

/* The typedef for off_t. */
/* #undef off_t */

/* The typedef for pid_t. */
/* #undef pid_t */
#endif /* CONFIG_H */
