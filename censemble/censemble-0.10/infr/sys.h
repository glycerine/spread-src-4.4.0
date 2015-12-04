/**************************************************************/
/* SYS.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef SYS_H
#define SYS_H

#include "infr/trans.h"
#include "infr/array.h"

#define SYS_UNIX_PATH_MAX 64

typedef struct {
    inet_t inet ;
    net_port_t port ;
} sys_addr_t ;

/* Tells sys module to enter infinite loop rather than exit()ing or
 * abort()ing.
 */
void sys_dont_really_exit(void) ;

void sys_exit(int i) NORETURN() ;

uint64_t sys_gettime(void) ;

inet_t sys_gethost(void) ;

string_t sys_gethostname(void) ;

uint32_t sys_getpid(void) ;

sock_t sys_sock_dgram(void) ;

void sys_sock_close(sock_t) ;

sock_t sys_sock_stream(void) ;

void sys_pipe(sock_t *send, sock_t *recv) ;

len_t sys_pipe_recv_peek(sock_t) ;

void sys_listen(sock_t sock) ;

void sys_accept(sock_t sock, sock_t *client) ;

void sys_connect(sock_t sock, sys_addr_t *addr) ;

bool_t sys_connect_nonblock(sock_t, sys_addr_t *, bool_t *complet) ;

bool_t sys_connect_nonblock_complete(sock_t) ;

void sys_sockopt_reuse(sock_t) ;

void sys_sockopt_tcp_no_delay(sock_t) ;

void sys_sock_nonblock(sock_t) ;

void sys_sockopt_bsdcompat(sock_t) ;

void sys_sockopt_multicast_loop(sock_t) ;

/* Neither sys_sockopt_join nor sys_socket_leave fail when there is a
 * problem, although a warning is printed.  
 */
void sys_sockopt_join(sock_t, inet_t interface, inet_t ipmc) ;

void sys_sockopt_leave(sock_t, inet_t interface, inet_t ipmc) ;

bool_t sys_inet_multicast(inet_t) ;

inet_t sys_inet_any(void) ;

net_port_t sys_bind_any(sock_t, inet_t) ;

void sys_bind(sock_t, sys_addr_t *) ;

sock_t sys_sock_unix_stream(void) ;

#if 0
void sys_sock_unix_bind(sock_t, sys_addr_t *) ;
#endif

void sys_setsock_sendbuf(sock_t, len_t) ;

void sys_setsock_recvbuf(sock_t, len_t) ;

void sys_setsock_no_udp_checksum(sock_t) ;

string_t sys_inet_to_string(inet_t) ;

bool_t sys_string_to_inet(string_t, inet_t *) ;

void sys_catch_sigint(void) ;

void *sys_alloc(len_t) ;

void *sys_alloc_try(len_t) ;	/* may fail */

bool_t sys_string_to_uint64(string_t, uint64_t *) ;

bool_t sys_string_to_uint32(string_t, uint32_t *) ;

#if 0
void *sys_alloc_atomic(len_t) ;
#else
#define sys_alloc_atomic(len) sys_alloc(len)
#endif

void sys_free(const void *obj) ;

len_t sys_recv(sock_t, buf_t, len_t) ;

len_t sys_recv_stream(sock_t, buf_t, len_t, bool_t *error) ;

bool_t sys_addr_eq(const sys_addr_t *, const sys_addr_t *) ;

string_t sys_addr_to_string(const sys_addr_t *) ;

len_t sys_recvfrom(sock_t, buf_t, len_t, sys_addr_t *) ;

len_t sys_recv_peek(sock_t) ;

len_t sys_send_peek(sock_t) ;

len_t sys_send(sock_t, buf_t, len_t) ;

void sys_pipe_send(sock_t, buf_t, len_t) ;

void sys_pipe_recv(sock_t, buf_t, len_t) ;

void sys_sendto(sock_t, buf_t, len_t, sys_addr_t *) ;

void sys_eprintf(const char *fmt, ...) FORMAT(printf, 1, 2) ;

string_t sys_sprintf(const char *fmt, ...) FORMAT(printf, 1, 2) ;

/*void sys_sscanf(char *s, const char *fmt, ...) FORMAT(sscanf, 1 2) ;*/

#ifdef MINIMIZE_CODE
#define sys_panic(args) sys_do_abort()
#define sys_panic_perror(args) sys_do_abort()
#else
void sys_do_panic(const char *fmt, ...) FORMAT(printf, 1, 2) NORETURN() ;
void sys_do_panic_perror(const char *fmt, ...) FORMAT(printf, 1, 2) NORETURN() ;
#define sys_panic_perror(args) sys_do_panic_perror args
#define sys_panic(args) sys_do_panic args
#endif

void sys_do_abort(void) NORETURN() ;
#ifdef MINIMIZE_CODE
#define sys_abort() sys_do_abort()
#else
#define sys_abort() sys_do_panic("CENSEMBLE:sys_abort:file=%s line=%d", __FILE__, __LINE__)
#endif

uint32_t sys_random(uint32_t max) ;
uint64_t sys_random64(uint64_t max) ;

void sys_random_seed(void) ;

string_t sys_strerror(void) ;

void sys_suicide(int argc, char *argv[]) ;

#endif /*SYS_H*/
