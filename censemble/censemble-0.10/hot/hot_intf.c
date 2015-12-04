/**************************************************************/
/* Author: Mark Hayden, 11/1999. */
/* Based on code originally written by Alexey Vaysburd. */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

/* ijh: HP cc has a different way of doing inlines */
#if __hpux
#pragma inline begin_critical, end_critcal
#define inline
#endif

#include "infr/config.h"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef WIN32
void Perror(char *s);
#include <winsock2.h>
#else
#include <sys/time.h>
#define Perror(s) perror(s)
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "hot_sys.h"
#include "hot_error.h"
#include "hot_thread.h"
#include "hot_msg.h"
#include "hot_mem.h"
#include "hot_ens.h"

#ifdef assert			/* Yuck */
#undef assert
#endif

#include "infr/trans.h"
#include "infr/version.h"
#include "infr/appl.h"
#include "infr/domain.h"
#include "infr/sched.h"
#include "infr/unique.h"
#include "infr/sys.h"
#include "trans/real.h"
#include "trans/udp.h"
#include "infr/util.h"
#include "infr/view.h"
#include "infr/endpt.h"
#include "infr/appl_intf.h"
#include "infr/layer.h"

static string_t name = "HOT" ;

static struct {
    uint64_t cast_xmit ;
    uint64_t cast_recv ;
    uint64_t send_xmit ;
    uint64_t send_recv ;
} stats ;

static inline void trace(const char *s, ...) {
#if 0
    va_list args;
    static int debugging = -1 ;

    va_start(args, s);

    if (debugging == -1) {
	debugging = (getenv("ENS_HOT_TRACE") != NULL) ;
    }
  
    if (!debugging) return ;

    fprintf(stderr, "HOT_INTF:");
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
#endif
}

#if 0
static
void timer(char *msg) {
    static struct timeval last ;
    struct timeval tv ;
    int usec ;
    gettimeofday(&tv, NULL) ;
    usec = (tv.tv_usec - last.tv_usec) + (tv.tv_sec - last.tv_sec) * 1000000 ;
    fprintf(stderr, "HOT_INTF:%08d usec:%s\n", usec, msg) ;
    last = tv ;
}
#endif

#if 0
static
void select_calls(void) {
    static int c ;
    c++;
    if ((c % 100)==0)
	fprintf(stderr, "HOT_INTF:#select_calls=%d\n", c) ;
}
#endif

/**************************************************************/

static hot_mem_t
    ch_endpt_array, 
    ch_bool_array, 
    ch_joinops,
    ch_protocol;

/**************************************************************/

/* This describes a group context.
 */
typedef struct hot_gctx {
    hot_ens_cbacks_t conf;	/* application callbacks */
    void *env;			/* application state */
    int group_blocked;		/* is group blocked? */
    int joining ;		/* are we joining? */
    int leaving ;		/* are we leaving? */
    equeue_t actions ;
    endpt_id_array_t view ;
    hot_ens_JoinOps_t *jops ;
    hot_endpt_t *members ;
    nmembers_t nmembers ;
} *state_t ;

/**************************************************************/

/* Global state.
 */
static hot_lock_t global_mutex;	/* global mutex lock  */
static int in_critical;		/* Set when mutex is locked */
static int initialized;		/* Set when hot_c is initialized. */
static int started;		/* Set when Ensemble/OCAML is started */
static etime_t start_time;

/* Live gctx's.  These need to be allocated with sys_alloc
 * in case we are using the garbage collector.
 */
static state_t *alive ;	/* joins */
static unsigned nalive ;
static bool_t pending_joins ;
static alarm_t hot_alarm ;
static domain_t domain ;

/**************************************************************/

static inline void begin_critical(void) {
    hot_err_Check(hot_lck_Lock(global_mutex)) ;
    assert(!in_critical) ;
    in_critical = 1 ;
}

static inline void end_critical(void) {
    assert(in_critical) ;
    in_critical = 0 ;
    hot_err_Check(hot_lck_Unlock(global_mutex)) ;
}

/**************************************************************/

static void do_join(state_t) ;

sock_t actions_signal ;
sock_t actions_wait ;
bool_t actions_blocked ;
bool_t actions_got_some ;

static void actions_wait_handler(void *env, sock_t sock) {
    char c ;
    int ret ;
    assert(!env) ;
    assert(sock == actions_wait) ;
    ret = recv(actions_wait, &c, sizeof(c), 0) ;
    if (ret == -1) {
	perror("recv") ;
	exit(1) ;
    }
    assert(ret == 1) ;
    assert(c == 0) ;
}

static void actions_do_signal(void) {
    char c ;
    int ret ;
    c = 0 ;
    ret = send(actions_signal, &c, sizeof(c), 0) ;
    if (ret == -1) {
	perror("send") ;
	exit(1) ;
    }
    assert(ret == 1) ;
    actions_blocked = FALSE ;
}

/* This function does not return.
 */
static void ens_do_init(void *param) {
    static int called_already = 0 ;

    if (called_already) {
	hot_sys_Panic("caml_startup:called twice") ;
    }
    called_already = 1 ;

    {
	int ret ;
	sock_t socks[2] ;
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socks) ;
	if (ret == -1) {
	    perror("socketpair") ;
	    exit(1) ;
	}
	actions_wait = socks[0] ;
	actions_signal = socks[1] ;
    }

    {
	start_time = alarm_gettime(hot_alarm) ;
	alarm_add_sock_recv(hot_alarm, name, actions_wait, actions_wait_handler, NULL) ;

	while(1) {
	    bool_t got_some ;
	    got_some = appl_poll(hot_alarm) ;
	    begin_critical() ;
	    if (pending_joins) {
		ofs_t i ;
		got_some = TRUE ;

		pending_joins = 0 ;

		for(i=0;i<nalive;i++) {
		    if (!alive[i]->jops) {
			continue ;
		    }
		    do_join(alive[i]) ;
		}
	    }

	    got_some |= actions_got_some ;
	    actions_got_some = FALSE ;
	    
	    if (!got_some) {
		actions_blocked = TRUE ;
	    }
	    end_critical() ;

	    if (actions_blocked) {
		appl_block(hot_alarm) ;
	    }
	}
    }
}

/* Initialize the interface. Start Ensemble/OCAML if necessary.
 */ 
static hot_err_t hot_ens_Init(char **argv) {
    hot_err_t err;
    hot_thread_attr_t attr;

#ifdef _WIN32_PRECOMPILED
    if (init_ops != HOT_ENS_INIT_SEPARATE_THREAD) {
    	fprintf(stderr, "HOT_INTF:This pre-compiled WIN32 version of HOT only supports the SEPARATE_THREAD options\n");
    	return hot_err_Create(0, "hot_ens_Init:  unsupported init option") ;
    }
#endif
    
    /* Initialize C side of the interface (preallocate/initialize gctx entries,
     * dncall structures, mutex.
     */
    err = hot_lck_Create(&global_mutex) ;
    if (err != HOT_OK) hot_sys_Panic(hot_err_ErrString(err));
    
#ifdef THREADED_SELECT
    /* These could also be #ifdef'd away like the thread create.
     */
    err = hot_sema_Create(0,&selector_sema) ;
    if (err != HOT_OK) hot_sys_Panic(hot_err_ErrString(err)) ;
    err = hot_sema_Create(0,&ml_sema) ;
    if (err != HOT_OK) hot_sys_Panic(hot_err_ErrString(err)) ;
    err = hot_lck_Create(&blocking_lock) ;
    if (err != HOT_OK) hot_sys_Panic(hot_err_ErrString(err)) ;
    err = hot_thread_Create(selector_thread,NULL,NULL) ;
    if (err != HOT_OK) hot_sys_Panic(hot_err_ErrString(err)) ;
#endif

    {
	unique_t unique ;
	sched_t sched ;
	appl_init() ;
	sched = sched_create(name) ;
	unique = unique_create(sys_gethost(), sys_getpid()) ;
	hot_alarm = real_alarm(sched, unique) ;
	domain = udp_domain(hot_alarm) ;
    }
    
    initialized = 1;
    
    /* Start Ensemble/OCAML.  Don't allow starting Ensemble more than once.
     */    
    begin_critical();
    if (started) {
	end_critical();
	return hot_err_Create(0, "hot_ens_Init:  Ensemble already started");
    }

    ch_endpt_array = hot_mem_AddChannel("endpt_array");
    ch_bool_array = hot_mem_AddChannel("bool_array");
    ch_joinops = hot_mem_AddChannel("join_ops");
    ch_protocol = hot_mem_AddChannel("protocol");

    alive = sys_alloc(0) ;
    nalive = 0 ;
    started = 1 ;
    end_critical() ;
    
    /* MH: BUG: move into critical stuff above? */
    attr.stacksize = 1024 * 1024; /* stack size for Ensemble thread = 1M */
    
    if ((err = hot_thread_Create(ens_do_init, (void*) argv, &attr)) != HOT_OK)
	return err;
    
    return HOT_OK;
}

/**************************************************************/

static void release_endpt_array(
	hot_mem_t *ch,
	hot_endpt_t *endpts,
	unsigned nendpts
) {
    assert(endpts) ;
    assert(nendpts > 0) ;

    hot_mem_Free(endpts) ;
}

static void release_bool_array(
	hot_mem_t *ch,
	hot_bool_t *bools,
	unsigned nbools
) {
    assert(bools) ;
    assert(nbools > 0) ;

    hot_mem_Free(bools) ;
}

/**************************************************************/

/* Return a HOT message created from the given substring.
 */
static inline hot_msg_t msg_of_iovec(iovec_t iov) {
    hot_msg_t msg ;
    char *buf ;
    char npad ;
    int len = iovec_len(iov) ; 
    hot_err_t err ;
  
    buf = sys_alloc(len) ;
    iovec_flatten_buf(iov, buf, 0, len) ;
    npad = buf[0];

    msg = hot_msg_Create() ;
    err = hot_msg_Write(msg, buf + npad, len - npad) ;
    if (err != HOT_OK)
	hot_sys_Panic(hot_err_ErrString(err));

    return msg;
}

/* Return a msg value corresponding to the given HOT message.
 */
static inline iovec_t iovec_of_msg(hot_msg_t msg) {
    buf_t buf ;
    void *data;
    unsigned size;
    hot_err_t err;
    
    err = hot_msg_GetPos(msg, &size);
    if (err != HOT_OK)
	hot_sys_Panic(hot_err_ErrString(err));
    err = hot_msg_Look(msg, size, &data);
    if (err != HOT_OK)
	hot_sys_Panic(hot_err_ErrString(err));
  
    {
	char pad[HOT_MSG_PADDING];
	char npad = HOT_MSG_PADDING - (size % HOT_MSG_PADDING);
    
	pad[0] = npad;
	buf = sys_alloc(size + npad);
	memcpy(buf, pad, npad);
	memcpy(buf + npad, data, size);
	return iovec_of_buf(buf, 0, size + npad) ;
    }
}

/* Convert a value into a view state.
 */
static void hot_view_state(
	view_local_t ls,
	view_state_t vs,
	hot_view_state_t *ret	/*OUT*/ 
) {
    unsigned i ;
    
    memset(ret, 0, sizeof(*ret)) ;
    ret->rank = ls->rank ;
    ret->groupd = vs->groupd ;
    ret->xfer_view = vs->xfer_view ;
    ret->primary = vs->primary ;
    
    ret->xfer_view = vs->xfer_view ;
    ret->primary   = vs->primary ;

    ret->view_id.ltime = vs->ltime ;
    strncpy(ret->view_id.coord.name,
	    endpt_id_to_string(&array_get(vs->view, 0)),
	    HOT_ENDP_MAX_NAME_SIZE) ;
    ret->view_id.coord.name[HOT_ENDP_MAX_NAME_SIZE-1] = '\0' ;
	    
    strncpy(ret->version, version_id_to_string(&vs->version), 
	    HOT_ENS_MAX_VERSION_LENGTH) ;
    ret->version[HOT_ENS_MAX_VERSION_LENGTH-1] = '\0' ;
    
    strncpy(ret->group_name, group_id_to_string(&vs->group),
	    HOT_ENS_MAX_GROUP_NAME_LENGTH) ;	  
    ret->group_name[HOT_ENS_MAX_GROUP_NAME_LENGTH-1] = '\0' ;
    
    strncpy(ret->protocol, proto_id_to_string(vs->proto),
	    HOT_ENS_MAX_PROTO_NAME_LENGTH) ;
    ret->protocol[HOT_ENS_MAX_PROTO_NAME_LENGTH-1] = '\0' ;
    
    strcpy(ret->params, "") ;	/* BUG */
    
    ret->nmembers = vs->nmembers ;
    ret->members = hot_mem_Alloc(ch_endpt_array, sizeof(ret->members[0]) * ret->nmembers) ;
    ret->clients = hot_mem_Alloc(ch_bool_array, sizeof(ret->clients[0]) * ret->nmembers) ;
    for (i=0;i<vs->nmembers;i++) {
	strncpy(ret->members[i].name, endpt_id_to_string(&array_get(vs->view, i)), 
		HOT_ENDP_MAX_NAME_SIZE) ;
	ret->clients[i] = array_get(vs->clients, i) ;
    }
}
    
static void add_action(
        state_t s,
	appl_action_t action
) {
    appl_action_t *actionp ;
    assert(in_critical) ;
    actionp = equeue_add(s->actions) ;
    *actionp = action ;

    if (actions_blocked) {
	actions_do_signal() ;
    }

    actions_got_some = TRUE ;
}

/**************************************************************/

/* Exported interface:  
 */

/* Initialize/reset an options struct.
 */
void hot_ens_InitJoinOps(hot_ens_JoinOps_t *ops) {
    if (ops == NULL)
	hot_sys_Panic("hot_ens_InitJoinOps: bad argument (NULL)");
  
    memset(ops, 0, sizeof(hot_ens_JoinOps_t));
    ops->heartbeat_rate = HOT_ENS_DEFAULT_HEARTBEAT_RATE;
    strcpy(ops->transports, HOT_ENS_DEFAULT_TRANSPORT);
    strcpy(ops->group_name, HOT_ENS_DEFAULT_GROUP_NAME);
    strcpy(ops->protocol, HOT_ENS_DEFAULT_PROTOCOL);
    strcpy(ops->properties, HOT_ENS_DEFAULT_PROPERTIES);
    ops->use_properties = 0;
    //ops->use_properties = 1;
    strcpy(ops->params, "");
    ops->groupd = 0;
    ops->argv = NULL ;
    ops->env = NULL ;
    strcpy(ops->endpt.name, "") ;
}

/* Join a group.  In case of success, the call returns after the
 * first view is delivered.  The group context is returned in *gctxp.  
 */
hot_err_t hot_ens_Join(
        hot_ens_JoinOps_t *jops,
	state_t *gctxp	/*OUT*/ 
) {
    hot_err_t err = HOT_OK ;
    state_t s ;

    /* Initialize global state if not done so already.
   */
    if (!initialized) {
	err = hot_ens_Init(jops->argv); 
	if (err != HOT_OK)
	    return err;
    }
  
    begin_critical();    
  
  /* Allocate a new group context 
   * Initialize the group record.
   */
    s = sys_alloc(sizeof(*s)) ;
    s->joining = 1 ;
    s->leaving = 0 ;
    s->conf = jops->conf;
    s->env = jops->env;
    s->jops = hot_mem_Alloc(ch_joinops, sizeof(*s->jops)) ;
    *s->jops = *jops ;
    s->actions = NULL ;
    s->view = NULL ;
    s->members = NULL ;
    s->nmembers = 0 ;

    {
	state_t *new ;
	ofs_t ofs ;
	new = sys_alloc(sizeof(new[0]) * (nalive + 1)) ;
	for (ofs=0;ofs<nalive;ofs++) {
	    new[ofs] = alive[ofs] ;
	}
	new[nalive] = s ;
	nalive ++ ;
	sys_free(alive) ;
	alive = new ;
	pending_joins = TRUE ;
    }

    /* We actually set the "return" value here so that
     * it is available when the accepted view call arrives.
     * This is somewhat of a hack.
     */
    *gctxp = s ;
  
    end_critical();
    return HOT_OK;
}

/* Leave a group.  This should be the last call made to a given gctx.
 * No more events will be delivered to this gctx after the call
 * returns.  
 */
hot_err_t hot_ens_Leave(state_t s) {
    begin_critical();
    trace("leave") ;
    eprintf("HOT_INTF:leave\n") ;

    if (s->leaving) {
	hot_sys_Panic("hot_ens_Leave:this member is already leaving") ;
    }

    s->leaving = 1 ;
  
    /* Enqueue the leave request.
     */
    add_action(s, appl_leave()) ;

    /* No more actions are now allowed.
     */
    s->actions = NULL ;
  
    end_critical();

    return HOT_OK;
}

/* Send a multicast message to the group.
 */
hot_err_t hot_ens_Cast(
        state_t s,
	hot_msg_t msg, 
	hot_ens_MsgSendView *send_view	/*OUT*/
) {
    iovec_t iov ;
    
    begin_critical() ;
    stats.cast_xmit ++ ;
#if 0
    eprintf("cast(%llu->%llu) send(%llu->%llu)\n",
	    stats.cast_xmit, stats.cast_recv, 
	    stats.send_xmit, stats.send_recv) ;
#endif
    /*timer("hot_ens_Cast") ;*/

    if (send_view) {
	*send_view = s->group_blocked
	    ? HOT_ENS_MSG_SEND_NEXT_VIEW
	    : HOT_ENS_MSG_SEND_CURRENT_VIEW ;
    }

    iov = iovec_of_msg(msg) ;
    add_action(s, appl_cast(iov)) ;
    
    end_critical() ;
    return HOT_OK ;
}

/* Send a point-to-point message to the specified group member.
 */
hot_err_t hot_ens_Send(
        state_t s, 
	hot_endpt_t *dest,
	hot_msg_t msg,
	hot_ens_MsgSendView *send_view /*OUT*/
) {
    iovec_t iov ;
    rank_t peer ;

    begin_critical() ;
    stats.send_xmit ++ ;
    
    if (send_view) {
	*send_view = s->group_blocked
	    ? HOT_ENS_MSG_SEND_NEXT_VIEW 
	    : HOT_ENS_MSG_SEND_CURRENT_VIEW ;
    }
    
    iov = iovec_of_msg(msg) ;

    for (peer=0;peer<s->nmembers;peer++) {
	if (!strncmp(dest->name, s->members[peer].name,
		     HOT_ENDP_MAX_NAME_SIZE)) {
	    break ;
	}
    }
    assert(peer < s->nmembers) ;

    add_action(s, appl_send1(peer, iov)) ;
    
    end_critical() ;
    return HOT_OK ;
}

/* Report group members as failure-suspected.
 * 
 * NB:  In the initial implementation, this downcall will not be supported.
 *      (if invoked, an exeption will be raised by OCAML).
 */
#if 0
hot_err_t hot_ens_Suspect(
	state_t stx,
	hot_endpt_t *suspects, 
	int nsuspects
) {
    dncall_t *dn;
    int size ;

    begin_critical();    
  
  /* Enqueue the suspect request.
   */
    dn = alloc_dn(s,DNCALL_SUSPECT);
  
    size = sizeof(hot_endpt_t) * nsuspects ;
    dn->suspects = (hot_endpt_t*) hot_mem_Alloc(ch_endpt_array,size);
    memcpy(dn->suspects, suspects, size);
    dn->nsuspects = nsuspects;
  
    end_critical();
    return HOT_OK;
}

/* Request a protocol change.
 */
hot_err_t hot_ens_ChangeProtocol(
	state_t stx,
	char *protocol
) {
    dncall_t *dn;

    begin_critical();

    /* Enqueue the ChangeProtocol request.
   */ 
    dn = alloc_dn(s, DNCALL_PROTOCOL);

    assert(!dn->protocol) ;
    dn->protocol = hot_mem_Alloc(ch_protocol,HOT_ENS_MAX_PROTO_NAME_LENGTH) ;
    if (strlen(protocol) >= sizeof(dn->protocol))
	hot_sys_Panic("hot_ens_AddChangeProperties: properties too large") ;
    strcpy(dn->protocol, protocol);

    end_critical();
    return HOT_OK;
}

/* Request a protocol change specifying properties.
 */
hot_err_t hot_ens_ChangeProperties(
	state_t stx,
	char *properties
) {
    dncall_t *dn;
    assert(properties) ;

    begin_critical();

  /* Enqueue the ChangeProperties request.
   */ 
    dn = alloc_dn(s,DNCALL_PROPERTIES);

    assert(!dn->protocol) ;
    dn->protocol = hot_mem_Alloc(ch_protocol,HOT_ENS_MAX_PROTO_NAME_LENGTH) ;
    if (strlen(properties) >= sizeof(dn->protocol))
	hot_sys_Panic("hot_ens_AddChangeProperties: properties too large") ;
    strcpy(dn->protocol, properties);

    end_critical();
    return HOT_OK;
}
#endif

/* Request a new view to be installed.
 */
hot_err_t hot_ens_RequestNewView(
	state_t stx
) {
    begin_critical();
    
    /* Enqueue the ChangeProtocol request.
     */
    //dn = alloc_dn(s,DNCALL_PROMPT);
    
    end_critical();
    return HOT_OK;
}

static void
do_receive(
        void *env,
	rank_t from,
	is_send_t is_send,
	blocked_t blocked,
	iovec_t iov
) {
    state_t s = env ;
    hot_endpt_t origin ;
    hot_msg_t msg ;
    hot_ens_ReceiveSend_cback receive_cback ;

    msg = msg_of_iovec(iov) ;

    begin_critical();
    trace("receive") ;

    assert(from >= 0 && from < s->nmembers) ;
    origin = s->members[from] ;

    if (is_send) {
	stats.send_recv ++ ;
	receive_cback = s->conf.receive_send ;
    } else {
	stats.cast_recv ++ ;
	receive_cback = s->conf.receive_cast;
    }
    end_critical();

    receive_cback(s, s->env, &origin, msg) ;
    hot_err_Check(hot_msg_Release(&msg)) ;
}

/* Accepted a new view.
 */
static void *
do_install(
        void *env,
	view_local_t ls,
	view_state_t vs,
	equeue_t xmit
) {
    state_t s = env ;
    hot_view_state_t view_state;
    hot_ens_AcceptedView_cback accepted_view;
    unsigned i ;
  
    trace("install") ;
    begin_critical();

    accepted_view = s->conf.accepted_view;
    env = s->env ;
  
    /* The group is unblocked now.
     */
    s->group_blocked = 0;
    
    /* Setup the new view.
     */
    hot_view_state(ls, vs, &view_state) ;

    if (s->members) {
	hot_mem_Free(s->members) ;
    }

    /* Move entries from the interim queue to the new one.
     * The main thing here is to update the ranks of the
     * send destinations in the queue.
     */
    if (s->actions) {
	while(!equeue_empty(s->actions)) {
	    appl_action_t *ap = equeue_take(s->actions) ; 
	    appl_action_t a = *ap ;
	    assert(a->type != APPL_SEND) ;
	    if (a->type == APPL_LEAVE) {
		eprintf("HOT_INTF:ADVANCING LEAVE\n") ;
	    }
	    if (a->type != APPL_SEND1) {
		ap = equeue_add(xmit) ;
		*ap = a ;
	    } else {
		rank_t rank ;
		for (rank=0;rank<vs->nmembers;rank++) {
		    if (endpt_id_eq(&array_get(vs->view, rank),
				    &array_get(s->view, a->u.send1.dest))) {
			break ;
		    }
		}

		if (rank == vs->nmembers) {
		    /* Not in new view.
		     */
		    appl_action_free(a) ;
		} else {
		    a->u.send1.dest = rank ;
		    ap = equeue_add(xmit) ;
		    *ap = a ;
		}
	    }
	}

	array_free(s->view) ;
	equeue_free(s->actions) ;
    }

    s->actions = xmit ;
    s->view = endpt_id_array_copy(vs->view, vs->nmembers) ;
    s->nmembers = vs->nmembers ;
    s->members = hot_mem_Alloc(ch_endpt_array, sizeof(s->members[0]) * s->nmembers) ;
    memset(s->members, 0, sizeof(s->members[0]) * s->nmembers) ;
    for (i=0;i<vs->nmembers;i++) {
	strncpy(s->members[i].name, endpt_id_to_string(&array_get(vs->view, i)), 
		HOT_ENDP_MAX_NAME_SIZE) ;
    }

    end_critical();
    
    if (accepted_view) {
	(*accepted_view)(s, env, &view_state);
    }
    
    begin_critical();
	
    if (view_state.members) {
	release_endpt_array(&ch_endpt_array, view_state.members, view_state.nmembers);
    }
    
    if (view_state.clients) {
	release_bool_array(&ch_bool_array, view_state.clients, view_state.nmembers);
    }
    
    s->joining = 0 ;
    
    end_critical() ;

    return s ;
}

/* Got a heartbeat event.
 */
static void
do_heartbeat(void *env, etime_t time) {
    state_t s = env ;
    hot_ens_Heartbeat_cback heartbeat ;

    trace("heartbeat") ;
    begin_critical() ;
    heartbeat = s->conf.heartbeat ;
    env = s->env ;
    end_critical() ;
    
    if (heartbeat) {
	etime_t time = alarm_gettime(hot_alarm) ;
	heartbeat(s, env, time_to_msecs(time_sub(time, start_time))) ;
    }
}

/* The group is about to block.
 */
static void
do_block(void *env) {
    state_t s = env ;
    hot_ens_Block_cback block;
  
    /* Set the "group_blocked" flag in the corresp. gctx.
     * The flag is cleared when the next view is delivered.
     */
    trace("block") ;
    begin_critical();
    env = s->env;
    block = s->conf.block;
    s->group_blocked = 1;
    
    /* Switch to using an interim queue.  This is done
     * before the callback so that the "next_view"
     * stuff is done right.
     */
    s->actions = equeue_create(name, sizeof(appl_action_t)) ;
    end_critical();
    
    if (block) {
	block(s, env);
    }
}

static void 
do_disable(void *env) {
    /* NO-OP */
}

/* The member has left the group.
 */
static void 
do_exit(void *env) {
    state_t s = env ;
    hot_ens_Exit_cback exit_cb;
    
    /* Phase 1: get info for callback.
     */
    trace("exit") ;
    begin_critical();  
  
    if (!s->leaving) {
	hot_sys_Panic("hot_ens_Exit_cbd: mbr state is not leaving");
    }
    
    env = s->env;
    exit_cb = s->conf.exit;
    
    end_critical();
    
    if (exit_cb) {
	exit_cb(s, env);
    }  
  
    /* Phase 2: cleanup.
     */
    begin_critical();  
    {
	unsigned ofs ;

	for (ofs=0;ofs<nalive;ofs++) {
	    if (alive[ofs] == s) break ;
	}
	assert(ofs < nalive) ;
	for (ofs++;ofs<nalive;ofs++) {
	    assert(alive[ofs] != s) ;
	    alive[ofs-1] = alive[ofs] ;
	}
	alive[ofs-1] = NULL ;
	nalive -- ;

	assert(!s->jops) ;
	if (s->members) {
	    hot_mem_Free(s->members) ;
	    s->members = NULL ;
	}
	
	if (s->view) {
	    array_free(s->view) ;
	    s->view = NULL ;
	}

	if (s->actions) {
	    while(!equeue_empty(s->actions)) {
		appl_action_t *ap = equeue_take(s->actions) ; 
		appl_action_t a = *ap ;
		appl_action_free(a) ;
	    }
	    array_free(s->actions) ;
	    s->actions = NULL ;
	}
	
	sys_free(s) ;
    }
    
    end_critical();  
}

static void
do_join(state_t s) {
    hot_ens_JoinOps_t *jops = s->jops ;
    view_state_t vs ;
    appl_intf_t appl_intf = record_create(appl_intf_t, appl_intf) ;
    layer_state_t state = record_create(layer_state_t, state) ;
    endpt_id_t endpt ;
    view_local_t ls ;

    assert(jops) ;
    s->jops = NULL ;

    trace("join") ;

    appl_intf->heartbeat_rate = time_of_usecs(((uint64_t)jops->heartbeat_rate) * 1000) ;
    appl_intf->exit = do_exit ;
    appl_intf->install = do_install ;
    appl_intf->block = do_block ;
    appl_intf->receive = do_receive ;
    appl_intf->disable = do_disable ;
    appl_intf->heartbeat = do_heartbeat ;

    endpt = endpt_id(alarm_unique(hot_alarm)) ;
    
    /* BUG: need to handle transports.
     */
    //rt[JOPS_TRANSPORTS] = hot_ml_copy_string(ops->transports);

    vs = view_singleton(
			/* BUG: need to handle properties properly.
			 */
			proto_id_of_string(jops->protocol),
			
			//rt[JOPS_PROTOCOL] = hot_ml_copy_string(ops->use_properties ? "" : ops->protocol);
			//rt[JOPS_PROPERTIES] = hot_ml_copy_string(ops->use_properties ? ops->properties : "");
			//rt[JOPS_USE_PROPERTIES] = Val_bool(ops->use_properties);
			
			group_named(jops->group_name),
			endpt,
			domain_addr(domain, ADDR_UDP), /* BUG */
			LTIME_FIRST,	/* ltime */
			alarm_gettime(hot_alarm)) ;
    assert(vs->ltime > 0) ;

    vs->groupd = jops->groupd ;
    vs->clients = bool_array_create_init(1, jops->client) ;

    //eprintf("HOT:view=%s\n", view_state_to_string(vs)) ;

    assert(domain) ;
    state->alarm = hot_alarm ;
    state->domain = domain ;
    state->appl_intf = appl_intf ;
    state->appl_intf_env = s ;
    ls = view_local(&endpt, vs) ;
    layer_compose(state, &endpt, vs) ;

    /* BUG ?? 
     */
    //rt[JOPS_DEBUG] = Val_bool(ops->debug);
}


#ifdef WIN32

void
Perror(char *s) {
    char error[256];
    int errno;
  
    errno = GetLastError();
    (void) FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM ,
			 NULL,	// message format
			 errno,
			 0,	// language
			 error,
			 256,	// buffer size, ignored for ALLOC_BUF
			 NULL	// args for msg format
			 );
    printf("%s: (%d) %s\n", s, errno, error);
    //LocalFree(error);
}

#endif



hot_err_t hot_ens_MLPrintOverride(
        void (*handler)(char *msg)
) {
    return HOT_OK ;
}

hot_err_t hot_ens_MLUncaughtException(
	void (*handler)(char *info)
) {
    return HOT_OK ;
}
