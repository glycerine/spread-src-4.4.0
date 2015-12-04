/**************************************************************/
/* DOMAIN.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#ifndef DOMAIN_H
#define DOMAIN_H

#include "infr/trans.h"
#include "infr/addr.h"
#include "infr/endpt.h"
#include "infr/group.h"
#include "infr/iovec.h"

/* DOMAIN_T: The type of a communication domain.
 */
typedef struct domain_t *domain_t ;

/* DEST: this specifies the destinations to use in xmit and
 * xmitv.  For pt2pt xmits, we give a list of endpoint
 * identifiers to which to send the message.  For
 * multicasts, we give a group address and a boolean value
 * specifying whether loopback is necessary.  If no loopback
 * is requested then the multicast may not be delivered to
 * any other processes on the same host as the sender.
 */

/* Note that significant preprocessing may be done on the
 * destination values.  For optimal performance, apply the
 * destination to an xmit outside of your critical path.
 */

typedef struct domain_dest_t {
    enum { DOMAIN_PT2PT, DOMAIN_MCAST, DOMAIN_GOSSIP } type ;
    union {
	struct {
	    addr_id_array_t addr ;
	    len_t naddr ;
	} pt2pt ;
	struct {
	    group_id_t group ;
	    bool_t loopback ;
	} mcast ;
	group_id_t gossip ;
    } u ;
} domain_dest_t ;

typedef void (*domain_handler_t)(env_t, iovec_t) ;

domain_dest_t domain_dest_pt2pt(addr_id_array_t, len_t) ;
domain_dest_t domain_dest_mcast(group_id_t, bool_t) ;
domain_dest_t domain_dest_gossip(group_id_t) ;

void domain_dest_free(domain_dest_t) ;

typedef env_t domain_info_t ;

/* Operations on domains.
 */
name_t domain_name(domain_t) ;

addr_id_t domain_addr(domain_t, addr_type_t) ;
void domain_disable(domain_t) ;
domain_info_t domain_prepare(domain_t, domain_dest_t) ;
void domain_xmit(domain_t, domain_info_t, marsh_t) ;
void domain_xmit_disable(domain_t, domain_info_t) ;

/* Return string by caller.
 */
string_t domain_dest_to_string(domain_dest_t) ;

/**************************************************************/

typedef addr_id_t (*domain_addr_t)(env_t, addr_type_t) ;
typedef domain_info_t (*domain_prepare_t)(env_t, domain_dest_t) ;
typedef void (*domain_xmit_t)(env_t, domain_info_t, marsh_t) ;
typedef void (*domain_xmit_disable_t)(env_t, domain_info_t) ;
typedef void (*domain_disable_t)(env_t) ;

/* Create a domain.
 */
domain_t domain_create(
        env_t env,
        name_t,
	domain_addr_t,
	domain_prepare_t,
	domain_xmit_t,
	domain_xmit_disable_t,
	domain_disable_t
) ;

#endif /*DOMAIN_H*/
