/**************************************************************/
/* UDP.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef UDP_H
#define UDP_H

#include "infr/alarm.h"
#include "infr/domain.h"


domain_t udp_domain(alarm_t alarm) ;

/**************************************************************/

typedef struct udp_domain_t *udp_domain_t ;

#define UDP_MAX_GOSSIP (5)

udp_domain_t udp_domain_full(
        alarm_t alarm,
	env_t handler_env,
	domain_handler_t handler,
	inet_t inet,
	bool_t bind_all,
	bool_t enable_multicast,
	inet_t gossip[],
	len_t ngossip
) ;

domain_t udp_domain_project(udp_domain_t) ;

#endif /* UDP_H */
