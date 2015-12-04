/**************************************************************/
/* File: Ensemble.xs */
/* Authors: Mark Hayden, Chet Murthy, 11/00 */
/* Copyright 2000 Mark Hayden.  All rights reserved. */
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

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

static
void main_receive(
        void *env,
	rank_t from,
	is_send_t is_send,
	blocked_t blocked,
	iovec_t iov
) {
    SV * _perl_sv = env ;
	{
	  dSP;                            /* initialize stack pointer      */
	  ENTER;                          /* everything created after here */
	  SAVETMPS;                       /* ...is a temporary variable.   */
	  PUSHMARK(SP);                   /* remember the stack pointer    */
	  XPUSHs(_perl_sv);
	  XPUSHs(sv_2mortal(newSViv(from))); /* push the base onto the stack  */
	  XPUSHs(sv_2mortal(newSViv(is_send))); /* push the base onto the stack  */
	  XPUSHs(sv_2mortal(newSViv(blocked))); /* push the base onto the stack  */
	  {
		len_t len = iovec_len (iov);
		char *buf = sys_alloc (len);
		iovec_flatten_buf (iov,buf,0,len);
		XPUSHs(sv_2mortal(newSVpv(buf,len)));
		sys_free (buf);
	  }
	  PUTBACK;                      /* make local stack pointer global */
	  perl_call_method("receive", G_SCALAR); /* call the function             */
	  SPAGAIN;                        /* refresh stack pointer         */
                                      /* pop the return value from stack */
	  PUTBACK;
	  FREETMPS;                       /* free that return value        */
	  LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
	}
}

static
void main_block(void *env) {
    /*state_t s = env ;
      eprintf("RAND:block\n") ;*/
}

static
void main_heartbeat(void *env, etime_t time) {
    SV * _perl_sv = env ;
	{
	  dSP;                            /* initialize stack pointer      */
	  ENTER;                          /* everything created after here */
	  SAVETMPS;                       /* ...is a temporary variable.   */
	  PUSHMARK(SP);                   /* remember the stack pointer    */
	  XPUSHs(_perl_sv);
	  XPUSHs(sv_2mortal(newSViv(time.usecs))); /* push the base onto the stack  */
	  PUTBACK;                      /* make local stack pointer global */
	  perl_call_method("heartbeat", G_SCALAR); /* call the function             */
	  SPAGAIN;                        /* refresh stack pointer         */
                                      /* pop the return value from stack */
	  PUTBACK;
	  FREETMPS;                       /* free that return value        */
	  LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
	}
}

static
void main_disable(void *env) {
    /*state_t s = env ;
      eprintf("RAND:disable\n") ;*/
}

static
void *main_install(void *env, view_local_t ls, view_state_t vs, equeue_t xmit) {
    SV *_perl_sv = env ;

	{
	  dSP;                            /* initialize stack pointer      */
	  ENTER;                          /* everything created after here */
	  SAVETMPS;                       /* ...is a temporary variable.   */
	  PUSHMARK(SP);                   /* remember the stack pointer    */
	  XPUSHs(_perl_sv);
	  XPUSHs(sv_2mortal(newSViv((long)ls))); /* push the base onto the stack  */
	  XPUSHs(sv_2mortal(newSViv((long)vs))); /* push the base onto the stack  */
	  XPUSHs(sv_2mortal(newSViv((long)xmit))); /* push the base onto the stack  */
	  PUTBACK;                      /* make local stack pointer global */
	  perl_call_method("install", G_SCALAR); /* call the function             */
	  SPAGAIN;                        /* refresh stack pointer         */
                                      /* pop the return value from stack */
	  PUTBACK;
	  FREETMPS;                       /* free that return value        */
	  LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
	  return env;
	}
}

static struct appl_intf_t intf ;

static
void main_exit(void *env) {
    SV *_perl_sv = env ;
	{
	  dSP;                            /* initialize stack pointer      */
	  ENTER;                          /* everything created after here */
	  SAVETMPS;                       /* ...is a temporary variable.   */
	  PUSHMARK(SP);                   /* remember the stack pointer    */
	  XPUSHs(_perl_sv);
	  PUTBACK;                      /* make local stack pointer global */
	  perl_call_method("exit", G_SCALAR); /* call the function             */
	  SPAGAIN;                        /* refresh stack pointer         */
                                      /* pop the return value from stack */
	  PUTBACK;
	  FREETMPS;                       /* free that return value        */
	  LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
	  return env;
	}
}

static
appl_intf_t
make_appl_intf(int hb) {
    appl_intf_t intf = malloc(sizeof(*intf));

    intf->receive = main_receive ;
    intf->block = main_block ;
    intf->heartbeat = main_heartbeat ;
    intf->disable = main_disable ;
    intf->install = main_install ;
    intf->exit = main_exit ;
    intf->heartbeat_rate = time_of_secs(hb) ;
    return intf ;
}

static int
not_here(char *s)
{
    croak("%s not implemented on this architecture", s);
    return -1;
}

static double
constant(char *name, int len, int arg)
{
    errno = 0;
    if (strEQ(name + 0, "APPL_INTF_H")) {	/*  removed */
#ifdef APPL_INTF_H
	return APPL_INTF_H;
#else
	errno = ENOENT;
	return 0;
#endif
    }
    errno = EINVAL;
    return 0;
}

MODULE = Ensemble		PACKAGE = Ensemble		

MODULE = Ensemble		PACKAGE = Ensemble::Internal

sched_t
sched_create(name)
char *	name
	CODE:
		RETVAL = sched_create(name);
	OUTPUT:
		RETVAL

unique_t
unique_create()
	CODE:
		RETVAL = unique_create(sys_gethost(), sys_getpid());
	OUTPUT:
		RETVAL

alarm_t
netsim_alarm(sched, unique)
sched_t		sched
unique_t	unique
	CODE:
		RETVAL = netsim_alarm(sched,unique);
	OUTPUT:
		RETVAL

alarm_t
real_alarm(sched, unique)
sched_t		sched
unique_t	unique
	CODE:
		RETVAL = real_alarm(sched,unique);
	OUTPUT:
		RETVAL

void
appl_init()

group_id_t *
group_named(name)
char *		name
	CODE:
		{
			RETVAL = malloc(sizeof(*RETVAL));
			*RETVAL = group_named(name);
		}
	OUTPUT:
		RETVAL

void
group_id_free(group)
group_id_t *	group
	CODE:
		free (group);

domain_t
netsim_domain(alarm)
alarm_t		alarm
	CODE:
		RETVAL = netsim_domain(alarm);
	OUTPUT:
		RETVAL

domain_t
udp_domain(alarm)
alarm_t		alarm
	CODE:
		RETVAL = udp_domain(alarm);
	OUTPUT:
		RETVAL

addr_id_t *
domain_addr(domain, ty)
domain_t	domain
int		ty
	CODE:
		{
			RETVAL = malloc(sizeof(*RETVAL));
			*RETVAL = domain_addr(domain,ty);
		}
	OUTPUT:
		RETVAL

void
addr_id_free(addr)
addr_id_t *	addr
	CODE:
		free (addr);

proto_id_t
proto_id_of_string(stack)
char *		stack
	CODE:
		RETVAL = proto_id_of_string(stack);
	OUTPUT:
		RETVAL

layer_state_t
layer_state_create(appl_intf, domain, alarm)
SV *		appl_intf
domain_t	domain
alarm_t		alarm
	CODE:
		{
		
		RETVAL = record_create(layer_state_t, RETVAL);
		RETVAL->appl_intf = make_appl_intf(1);
		RETVAL->appl_intf_env = appl_intf;
		SvREFCNT_inc (appl_intf);
		RETVAL->alarm = alarm;
		RETVAL->domain = domain;
		}
	OUTPUT:
		RETVAL

endpt_id_t *
endpt_id(unique)
unique_t	unique
	CODE:
		{
		RETVAL = malloc(sizeof(*RETVAL));
		*RETVAL = endpt_id(unique);
		}
	OUTPUT:
		RETVAL

void
endpt_id_free(endpt)
endpt_id_t *	endpt
	CODE:
		free (endpt);

void
layer_compose(state,eidp,vs)
layer_state_t	state
endpt_id_t *	eidp
view_state_t	vs
	CODE:
		layer_compose(state,eidp,vs);

view_state_t
view_singleton(proto,group,endpt,addr,ltime,etime)
proto_id_t		proto
group_id_t *	group
endpt_id_t *	endpt
addr_id_t *		addr
ltime_t			ltime
etime_t			etime
	CODE:
		RETVAL = view_singleton(proto,*group,*endpt,*addr,ltime,etime);
	OUTPUT:
		RETVAL

view_state_t
view_state_copy(vs)
view_state_t	vs

void
view_state_free(vs)
view_state_t	vs

int
view_get_quorum(vs)
view_state_t	vs
	CODE:
		RETVAL = vs->quorum;
	OUTPUT:
		RETVAL

int
view_get_nmembers(vs)
view_state_t	vs
	CODE:
		RETVAL = vs->nmembers;
	OUTPUT:
		RETVAL

bool_t
view_get_xfer_view(vs)
view_state_t	vs
	CODE:
		RETVAL = vs->xfer_view;
	OUTPUT:
		RETVAL

bool_t
view_get_primary(vs)
view_state_t	vs
	CODE:
		RETVAL = vs->primary;
	OUTPUT:
		RETVAL

ltime_t
view_get_ltime(vs)
view_state_t	vs
	CODE:
		RETVAL = vs->ltime;
	OUTPUT:
		RETVAL

etime_t
view_get_uptime(vs)
view_state_t	vs
	CODE:
		RETVAL = vs->uptime;
	OUTPUT:
		RETVAL

proto_id_t
view_get_proto(vs)
view_state_t	vs
	CODE:
		RETVAL = vs->proto;
	OUTPUT:
		RETVAL

group_id_t *
view_get_group(vs)
view_state_t	vs
	CODE:
		{
		  RETVAL = malloc(sizeof(*RETVAL));
		  *RETVAL = vs->group;
		}
	OUTPUT:
		RETVAL

void
view_set_quorum(vs,n)
view_state_t	vs
int				n
	CODE:
		vs->quorum = n;

SV *
view_state_members_to_string(vs)
view_state_t	vs
	CODE:
		{
		  string_t buf = endpt_id_array_to_string(vs->view, vs->nmembers);
		  RETVAL = newSVpv (buf,string_length (buf));
		  string_free (buf);
		}
	OUTPUT:
		RETVAL

SV *
view_state_to_string(vs)
view_state_t	vs
	CODE:
		{
		  string_t buf = view_state_to_string(vs);
		  RETVAL = newSVpv (buf,string_length (buf));
		  string_free (buf);
		}
	OUTPUT:
		RETVAL

void
view_local_free(ls)
view_local_t	ls

int
view_local_get_rank(ls)
view_local_t	ls
	CODE:
		RETVAL = ls->rank;
	OUTPUT:
		RETVAL

addr_id_t *
view_local_get_addr(ls)
view_local_t	ls
	CODE:
		{
		  RETVAL = malloc(sizeof(*RETVAL));
		  *RETVAL = ls->addr;
		}
	OUTPUT:
		RETVAL

ltime_t
LTIME_FIRST()
	CODE:
		RETVAL = LTIME_FIRST;
	OUTPUT:
		RETVAL

etime_t
time_zero()

int
sys_random(n)
int		n

void
appl_mainloop(alarm)
alarm_t		alarm

void
log_add(s)
char *		s

void
equeue_add_xfer_done(eq)
equeue_t	eq
	CODE:
		appl_action_t *loc = equeue_add(eq) ;
		*loc = appl_xfer_done() ;

void
equeue_add_leave(eq)
equeue_t	eq
	CODE:
		appl_action_t *loc = equeue_add(eq) ;
		*loc = appl_leave() ;

void
equeue_add_cast(eq,sv)
equeue_t	eq
SV *		sv
	CODE:
		{
		  appl_action_t *loc = equeue_add(eq) ;
		  marsh_t msg = marsh_create(NULL) ;
		  STRLEN len;
		  char *buf = SvPV (sv,len);
		  marsh_buf (msg,buf,len);
		  *loc = appl_cast(marsh_to_iovec (msg)) ;
		}

void
equeue_add_send1(eq,dest,sv)
equeue_t	eq
int			dest
SV *		sv
	CODE:
		{
		  appl_action_t *loc = equeue_add(eq) ;
		  marsh_t msg = marsh_create(NULL) ;
		  STRLEN len;
		  char *buf = SvPV (sv,len);
		  marsh_buf (msg,buf,len);
		  *loc = appl_send1(dest,marsh_to_iovec (msg)) ;
		}

SV *
md5(sv)
SV *	sv
	CODE:
		{
		  STRLEN len;
		  char *buf = SvPV (sv,len);
		  md5_t md5_calc ;
		  md5(buf, len, &md5_calc) ;
		  RETVAL = newSVpv (&md5_calc,sizeof (md5_calc));
		}
	OUTPUT:
		RETVAL
