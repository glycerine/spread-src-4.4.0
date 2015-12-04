/**************************************************************/
/* FIFO.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/sys.h"
#include "infr/util.h"
#include "infr/unique.h"
#include "infr/domain.h"
#include "infr/endpt.h"
#include "infr/group.h"
#include "infr/layer.h"
#include "infr/trace.h"
#include "infr/appl.h"
#include "trans/udp.h"
#include "trans/real.h"
#include "trans/netsim.h"

static string_t name = "FIFO" ;

#define NPROCS 2

typedef struct {
    view_local_t ls ;
    view_state_t vs ;
    equeue_t xmit ;
} *state_t ;

static
void do_xmit(state_t s, appl_action_t a) {
    appl_action_t *loc = equeue_add(s->xmit) ;
    *loc = a ;
}

static seqno_t count ;
static uint64_t xmit_count ;
static uint64_t recv_count ;

#define MSG_LEN (1 * 1024)

typedef enum { TOKEN, STORM, MAX } header_type_t ;

static
rank_t random_dest(state_t s) {
    rank_t dest ;
    dest = sys_random(s->vs->nmembers - 1) ;
    if (dest >= s->ls->rank) {
	dest ++ ;
    }
    assert(dest >= 0 && dest < s->vs->nmembers) ;
    return dest ;
}

static
iovec_t pad_msg(iovec_t iov) {
#if 1
#define PADDING MSG_LEN
    iovec_t pad = iovec_zeroes(PADDING) ;
    return iovec_append(iov, pad) ;
#else
    return iov ;
#endif
}
 
static
iovec_t xmit_msg(state_t s) {
    marsh_t marsh = marsh_create(NULL) ;
    marsh_rank(marsh, random_dest(s)) ;
    marsh_enum(marsh, TOKEN, MAX) ;
    return pad_msg(marsh_to_iovec(marsh)) ;
}

static
void main_receive(env_t env, rank_t from, is_send_t is_send, blocked_t blocked, iovec_t iov) {
    state_t s = env ;
    unmarsh_t unmarsh ;
    header_type_t type ;

    recv_count += iovec_len(iov) ;
    unmarsh = unmarsh_of_iovec(iov) ;
    type = unmarsh_enum_ret(unmarsh, MAX) ;

    if (0 && s->ls->rank == 0) {
	eprintf("MAIN:%d:receive from=%d send=%d blocked=%d count=%llu\n", s->ls->rank, from, is_send, blocked, count) ;
    }
    switch (type) {
    case TOKEN: {
	rank_t dest ;
	unmarsh_rank(unmarsh, &dest) ;
	assert(dest <= s->vs->nmembers) ;
	if (dest == s->ls->rank && !blocked) {
	    marsh_t marsh ;
	    marsh = marsh_create(NULL) ;
	    marsh_len(marsh, sys_random(100)) ;
	    marsh_enum(marsh, STORM, MAX) ;
	    do_xmit(s, appl_send1(random_dest(s), pad_msg(marsh_to_iovec(marsh)))) ;
	    do_xmit(s, appl_cast(xmit_msg(s))) ;
	}
    } break ;

    case STORM: {
	uint32_t count ;
	unmarsh_len(unmarsh, &count) ;
	if (count > 1) {
	    uint32_t split ;
	    marsh_t marsh ;
	    assert(count >= 1) ;
	    count -- ;
	    split = sys_random(count) ;
	    marsh = marsh_create(NULL) ;
	    marsh_len(marsh, split) ;
	    marsh_enum(marsh, STORM, MAX) ;
	    do_xmit(s, appl_send1(random_dest(s), pad_msg(marsh_to_iovec(marsh)))) ;
	    marsh = marsh_create(NULL) ;
	    marsh_len(marsh, count - split) ;
	    marsh_enum(marsh, STORM, MAX) ;
	    do_xmit(s, appl_send1(random_dest(s), pad_msg(marsh_to_iovec(marsh)))) ;
	}
    } break ;

    OTHERWISE_ABORT() ;
    }

    count ++ ;
    if ((count % (8 * 1024)) == 0) {
	eprintf("count=%09qd xmit=%06qdMB recv=%06qdMB\n", count, xmit_count/1000000, recv_count/1000000) ;
    }

    unmarsh_free(unmarsh) ;
}

static
void main_block(env_t env) {
    /*state_t s = env ;*/
    eprintf("MAIN:block\n") ;
}

static
void main_heartbeat(env_t env, etime_t time) {
    /*state_t s = env ;
      eprintf("MAIN:heartbeat\n") ;*/
}

static
void main_disable(env_t env) {
    /*state_t s = env ;
      eprintf("MAIN:disable\n") ;*/
}

static
env_t main_install(env_t env, view_local_t ls, view_state_t vs, equeue_t xmit) {
    state_t s = env ;
    if (s->vs) {
	view_state_free(s->vs) ;
	view_local_free(s->ls) ;
    }
    s->ls = ls ;
    s->vs = vs ;
    s->xmit = xmit ;
    eprintf("MAIN:install:nmembers=%d rank=%d\n", vs->nmembers, ls->rank) ;
    if (vs->nmembers > 1 && vs->nmembers >= NPROCS && ls->rank == 0) {
	eprintf("MAIN:xmitting\n") ;
	/*array_singleton(appl_action, appl_cast(iovec_empty()), ret) ;*/
	do_xmit(s, appl_cast(xmit_msg(s))) ;
    }
    return s ;
}

static
void main_exit(env_t env) {
    /*state_t s = env ;*/
    eprintf("MAIN:exit\n") ;
}

static
appl_intf_t main_appl(void) {
    appl_intf_t intf = record_create(appl_intf_t, intf) ;
    intf->receive = main_receive ;
    intf->block = main_block ;
    intf->heartbeat = main_heartbeat ;
    intf->disable = main_disable ;
    intf->install = main_install ;
    intf->exit = main_exit ;
    intf->heartbeat_rate = time_of_secs(1) ;
    return intf ;
}

int main(int argc, char *argv[]) {
    view_state_t vs ;
    unique_t unique ;
    group_id_t group ;
    endpt_id_t endpt ;
    proto_id_t proto ;
    addr_id_t addr ;
    sched_t sched ;
    alarm_t alarm ;
    domain_t domain ;
    int i ;

#if 0
    sys_catch_sigint() ;
#endif
    
    assert(sizeof(uint16_t) == 2) ;
    assert(sizeof(uint32_t) == 4) ;
    assert(sizeof(uint64_t) == 8) ;

    if (0) {
    usage:
	eprintf("usage: fifo [-trace name]\n") ;
	sys_exit(1) ;
    }

    for (i=1;i<argc;i++) {
	if (string_eq(argv[i], "-trace")) {
	    if (++i >= argc) {
		goto usage ;
	    }
	    log_add(argv[i]) ;
	} else {
	    goto usage ;
	}
    }

#if 0
    log_add("PT2PTW") ;
    log_add("FLOW") ;
    log_add("PT2PT") ;
    log_add("MNAK") ;
    log_add("MNAKN") ;
    log_add("HEAL") ;
    log_add("TOP") ;
    log_add("INTER") ;
    log_add("SYNCING") ;
    log_add("SUSPECT") ;
    log_add("SUSPECTP") ;
#endif

    appl_init() ;
    sys_random_seed() ;
    sched = sched_create(name) ;
    unique = unique_create(sys_gethost(), sys_getpid()) ;
    alarm = real_alarm(sched, unique) ;
    if (1) {
	domain = udp_domain(alarm) ;
    } else {
	domain = udp_domain_project(udp_domain_full(alarm, alarm, (void*)alarm_deliver,
				 sys_gethost(), TRUE, FALSE,
				 NULL, 0)) ;
    }
    addr = domain_addr(domain, ADDR_UDP) ;
    group = group_named(name) ;

    proto = proto_id_of_string("BOTTOM:MNAK:PT2PT:PT2PTW:"
			       "TOP_APPL:STABLE:VSYNC:SYNC:"
			       "ELECT:INTRA:INTER:LEAVE:SUSPECT:"
			       "PRESENT:HEAL:TOP") ;

    for (i=0;i<NPROCS;i++) {
	layer_state_t state = record_create(layer_state_t, state) ;
	state_t s = record_create(state_t, s) ;
	state->appl_intf = main_appl() ;
	state->appl_intf_env = s ;
	state->alarm = alarm ;
	state->domain = domain ;
	endpt = endpt_id(unique) ;
	vs = view_singleton(proto, group, endpt, addr, LTIME_FIRST, time_zero()) ;
	s->ls = NULL ;
	s->vs = NULL ;
	layer_compose(state, &endpt, vs) ;
    }
    
    appl_mainloop(alarm) ;
   
    return 0 ;
}
