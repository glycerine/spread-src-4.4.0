/**************************************************************/
/* RAND.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
#include "infr/trans.h"
#include "infr/util.h"
#include "infr/sys.h"
#include "infr/unique.h"
#include "infr/endpt.h"
#include "infr/group.h"
#include "infr/layer.h"
#include "infr/trace.h"
#include "infr/appl.h"
#include "infr/domain.h"
#include "trans/udp.h"
#include "trans/real.h"
#include "trans/netsim.h"

static string_t name = "RAND" ;

#define RAND_WAIT_FOR_ALL

typedef struct {
    alarm_t alarm ;
    etime_t next ;
    view_local_t ls ;
    view_state_t vs ;
    bool_array_t heard ;
    layer_state_t state ;
    equeue_t xmit ;
} *state_t ;

#if 0
static
iovec_t make_msg(state_t s) {
    marsh_t msg ;
    len_t len = sys_random(1024) ;
    buf_t buf = sys_alloc(len) ;
    md5_t md5_calc ;
    ofs_t ofs ;
    for (ofs=0;ofs<len;ofs++) {
	((char*)buf)[ofs] = sys_random(256) ;
    }
    md5(buf, len, &md5_calc) ;
    msg = marsh_create(NULL) ;
    marsh_buf(msg, buf, len) ;
    marsh_md5(msg, &md5_calc) ;
    sys_free(buf) ;
    return marsh_to_iovec(msg) ;
}

static
void check_msg(state_t s, iovec_t iov) {
    md5_t md5_msg ;
    md5_t md5_calc ;
    buf_t buf ;
    len_t len ;
    unmarsh_t msg = unmarsh_of_iovec(iov) ;
    unmarsh_md5(msg, &md5_msg) ;
    len = unmarsh_length(msg) ;
    buf = sys_alloc(len) ;
    unmarsh_buf(msg, buf, len) ;
    md5(buf, len, &md5_calc) ;
    assert(md5_eq(&md5_msg, &md5_calc)) ;
    unmarsh_free(msg) ;
    sys_free(buf) ;
}
#else
static
iovec_t make_msg(state_t s) {
    return iovec_empty() ;
}

static
void check_msg(state_t s, iovec_t iov) {
    assert(iovec_is_empty(iov)) ;
    iovec_free(iov) ;
}
#endif

static
void do_xmit(state_t s, appl_action_t a) {
    appl_action_t *loc = equeue_add(s->xmit) ;
    *loc = a ;
}

static seqno_t count ;
static rank_t thresh = 5 ;
static nmembers_t nmembers = 7 ;

static
void main_receive(
        env_t env,
	rank_t from,
	is_send_t is_send,
	blocked_t blocked,
	iovec_t iov
) {
    state_t s = env ;
    check_msg(s, iov) ;
    array_set(s->heard, from, TRUE) ;
    count ++ ;
    if ((count % 10000) == 0) {
	eprintf("count=%llu\n", count) ;
    }
}

static
void main_block(env_t env) {
    /*state_t s = env ;
      eprintf("RAND:block\n") ;*/
}

static
void main_heartbeat(env_t env, etime_t time) {
    state_t s = env ;
    /*eprintf("RAND:heartbeat\n") ;*/
    if (!time_ge(time, s->next)) {
	return ;
    }

    s->next = time_add(time, time_of_usecs((uint64_t)sys_random(10000000))) ;
    if (s->vs->nmembers >= thresh &&
#ifdef RAND_WAIT_FOR_ALL
	bool_array_forall(s->heard, s->vs->nmembers) &&
#endif
	sys_random(20) == 0) {
	do_xmit(s, appl_leave()) ;
    } else if (sys_random(2) == 0) {
	do_xmit(s, appl_cast(make_msg(s))) ;
    } else {
	rank_t dest = sys_random(s->vs->nmembers) ;
	if (dest != s->ls->rank) {
	    do_xmit(s, appl_send1(dest, make_msg(s))) ;
	}
    }
}

static
void main_disable(env_t env) {
    /*state_t s = env ;
      eprintf("RAND:disable\n") ;*/
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
    s->next = time_zero() ;
    s->xmit = xmit ;
    if (s->heard) {
	array_free(s->heard) ;
    }
    s->heard = bool_array_setone(vs->nmembers, s->ls->rank) ;
    if (s->ls->rank == 0) {
#ifndef MINIMIZE_CODE
#if 1
	string_t tmp = endpt_id_array_to_string(vs->view, vs->nmembers) ;
	eprintf("RAND:install:nmembers=%d rank=%d ltime=%llu xfer=%d prim=%d view=%s\n",
		vs->nmembers, ls->rank, s->vs->ltime, s->vs->xfer_view, s->vs->primary, tmp) ;
	string_free(tmp) ;
#else
	string_t tmp = endpt_id_to_string(&array_get(vs->view, 0)) ;
	eprintf("RAND:install:nmembers=%d rank=%d ltime=%llu view=%s\n",
		vs->nmembers, ls->rank, s->vs->ltime, tmp) ;
	string_free(tmp) ;
#endif
#endif
    }
    if (0 && vs->nmembers == nmembers && ls->rank == 0) {
	eprintf("xmitting\n") ;
	/*array_singleton(appl_action_t, appl_cast(iovec_empty()), ret) ;*/
	do_xmit(s, appl_send1((s->ls->rank + 1) % s->vs->nmembers, make_msg(s))) ;
    }
    if (s->vs->xfer_view) {
	do_xmit(s, appl_xfer_done()) ;
    }
    return s ;
}

static struct appl_intf_t intf ;

static appl_intf_t main_appl(void) ;

static
void main_exit(env_t env) {
    state_t s = env ;
    endpt_id_t endpt ;
    ltime_t ltime ;
    /*eprintf("RAND:exit\n") ;*/
    ltime = s->vs->ltime ;
    endpt = endpt_id(alarm_unique(s->alarm)) ;
    assert(s->vs) ;
    assert(s->ls) ;
    {
	view_state_t vs ;
	vs = view_singleton(s->vs->proto, s->vs->group, endpt,
			    s->ls->addr, s->vs->ltime + 1, s->vs->uptime) ;
	vs->quorum = nmembers / 2 + 1 ;
	layer_compose(s->state, &endpt, vs) ;
    }
}

static
appl_intf_t main_appl(void) {
    intf.receive = main_receive ;
    intf.block = main_block ;
    intf.heartbeat = main_heartbeat ;
    intf.disable = main_disable ;
    intf.install = main_install ;
    intf.exit = main_exit ;
    intf.heartbeat_rate = time_of_secs(1) ;
    return &intf ;
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
    ofs_t i ;

#if 0
    {
	extern void qsort_test(void) ;
	qsort_test() ;
    }
#endif

    assert(sizeof(net_uint16_t) == sizeof(uint16_t)) ;

#if 0
    sys_catch_sigint() ;
#endif

    if (0) {
    usage:
	eprintf("usage: rand [-trace name]\n") ;
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

    iovec_test() ;
#if 0
    priq_test() ;
    iovec_test() ;
    iq_test() ;
    log_add("INTER") ;
    log_add("TOP") ;
    log_add("PT2PTW") ;
    log_add("VSYNC") ;
    log_add("HEAL") ;
    log_add("SUSPECT") ;
    log_add("INTER") ;
    log_add("SYNCING") ;
    log_add("XFER") ;
#endif
    sched = sched_create(name) ;
    unique = unique_create(sys_gethost(), sys_getpid()) ;
    alarm = netsim_alarm(sched, unique) ;
    /*alarm = real_alarm(sched, unique) ;*/
    appl_init() ;
    /*sys_random_seed() ;*/
    group = group_named(name) ;
#if 0
    domain = udp_domain(alarm) ;
    addr = domain_addr(domain, ADDR_UDP) ;
#else
    domain = netsim_domain(alarm) ;
    addr = domain_addr(domain, ADDR_NETSIM) ;
#endif

#if 0
    //proto = proto_id_of_string("BOTTOM:MNAK:PT2PT:"
    //proto = proto_id_of_string("BOTTOM:DROP:MNAK:PT2PT:PT2PTW:CHK_FIFO:CHK_SYNC:"
#endif
    proto = proto_id_of_string(
			       "BOTTOM:CHK_TRANS:MNAK:PT2PT:CHK_FIFO:CHK_SYNC:LOCAL:"
			       "TOP_APPL:STABLE:VSYNC:SYNC:"
			       "ELECT:INTRA:INTER:LEAVE:SUSPECT:"
			       "PRESENT:PRIMARY:XFER:HEAL:TOP"
			       ) ;

    for (i=0;i<nmembers;i++) {
	layer_state_t state = record_create(layer_state_t, state) ;
	state_t s = record_create(state_t, s) ;
	s->alarm = alarm ;
	s->ls = NULL ;
	s->vs = NULL ;
	s->heard = NULL ;
	s->state = state ;
	state->appl_intf = main_appl() ;
	state->appl_intf_env = s ;
	state->alarm = alarm ;
	state->domain = domain ;
	endpt = endpt_id(unique) ;
	vs = view_singleton(proto, group, endpt, addr, LTIME_FIRST, time_zero()) ;
	vs->quorum = nmembers / 2 + 1 ;
	layer_compose(state, &endpt, vs) ;
    }
    
    appl_mainloop(alarm) ;
   
    return 0 ;
}
