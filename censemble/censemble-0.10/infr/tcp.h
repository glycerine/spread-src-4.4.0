/**************************************************************/
/* TCP.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#ifndef TCP_H
#define TCP_H

void tcp_send(sock_t, iovec_t) ;
iovec_t tcp_recv(sock_t) ;

#endif /*TCP_H*/
