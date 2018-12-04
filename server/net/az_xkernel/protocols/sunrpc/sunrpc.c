/* 
 * sunrpc.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.36 $
 * $Date: 1993/02/01 22:28:59 $
 */

#include "xrpc.h"
#include "xkernel.h"
#include "ip.h"
#include "udp.h"
#include "sunrpc.h"
#include "sunrpc_i.h"
#include "xrpc_print.h"
#include "gc.h"


/* global data for RPC; need data structure for assigned/free port nums */

int tracesunrpcp=0;
struct  opaque_auth	sunrpcAuthDummy = {0,0,0};

#ifdef __STDC__

static void	clean_hdr( struct rpc_msg * );
static int	exLong( Part *, long *, char *, int );
static int	extractPart( Part *, long *, long *, long *, char * );
static void	getProcClient( XObj );
static void	getProcProtl( XObj );
static void	put_port( int );
static xkern_return_t	destroySessn( XObj );

#else

static void	clean_hdr();
static int	exLong();
static int	extractPart();
static void	getProcClient();
static void	getProcProtl();
static void	put_port();
static xkern_return_t	destroySessn();

#endif __STDC__

#define MIN(A,B) ((A > B) ? B : A)
#define SESSN_COLLECT_INTERVAL	40 * 1000 * 1000	/* 40 seconds */

void
sunrpc_init(self)
    XObj self;
{
    PSTATE	*pstate;

    xTrace0(sunrpcp, 1, "SUNRPC init");
    getProcProtl(self);
    /*
     * No openenables downward can be done yet.  Wait until we 
     * have the specs on a server, which we get in our 
     * own openenable routine.
     */
    self->state = xMalloc(sizeof(PSTATE));
    pstate = (PSTATE *)self->state;
    pstate->activeMap = mapCreate(100, sizeof(ActiveKey));
    pstate->passiveMap = mapCreate(20, sizeof(PassiveKey));
    pstate->replyMap = mapCreate(50, sizeof(u_long));
    /*
     * SUNRPC server sessions will be cached and collected by the system
     * session garbage collector
     */
      initSessionCollector(pstate->activeMap, SESSN_COLLECT_INTERVAL,
			   (Pfv)destroySessn, "sunrpc");
}


static int
exLong( p, lPtr, s, restore )
    Part	*p;
    long	*lPtr;
    char	*s;
    int		restore;
{
    long	*tmp;

    if ( (tmp = (long *)partPop(*p)) == 0 || tmp == (long *)-1 ) {
	xTrace1(sunrpcp, TR_SOFT_ERRORS,
		"%s -- missing participant fields", s);
	return -1;
    }
    *lPtr = *tmp;
    if ( restore ) {
	partPush(*p, tmp, sizeof(long));
    }
    return 0;
}


static int
extractPart( p, vers, prog, port, s )
    Part	*p;
    long	*vers, *prog, *port;
    char 	*s;
{
    if ( ! p || partLen(p) < 1 ) {
	xTrace1(sunrpcp, TR_SOFT_ERRORS, "%s -- bad participant", s);
	return -1;
    }
    if ( exLong(p, vers, s, 0) ||
	 exLong(p, prog, s, 0) ) {
	return -1;
    }
    if ( port ) {
	if ( exLong(p, port, s, 1) ) {
	    return -1;
	}
    }
    return 0;
}


/* 
 * sunrpcOpen is only called to create client sessions.
 */
static XObj
sunrpcOpen( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    XObj   	s, lls;
    long	prog, vers;
    SState	*state;
    SunrpcHdr	*hdr;
    
    xTrace0(sunrpcp, 3, "SUNRPC open");
    if ( extractPart(p, &vers, &prog, 0, "SUNRPC open") ) {
	return ERR_XOBJ;
    }
    /* 
     * UDP port remains on the stack of the participants.
     * We do no processing on the local address component (if it exists.)
     */
    if ((lls = xOpen(self, self, xGetDown(self, 0), p)) == ERR_XOBJ) {
	xTrace0(sunrpcp, 3, "rpc_open: cannot open UDP session");
	xTrace0(sunrpcp,9,"error: 1");
	return ERR_XOBJ;
    }
    s = xCreateSessn(getProcClient, hlpRcv, hlpType, self, 1, &lls);
    state = X_NEW(SState);
    s->state = (VOID *)state;
    bzero((char *)state, sizeof(SState));
    state->c_tout = 6 * 1000 * 1000;
    state->c_wait = 1 * 1000 * 1000;
    semInit(&state->c_replySem, 0);
    msgConstructEmpty(&state->c_callMsg);
    hdr = &state->hdr;
    hdr->rm_xid = 0;
    hdr->rm_direction = CALL;
    hdr->rm_call.cb_rpcvers = CUR_RPC_VERS;
    hdr->rm_call.cb_prog = prog;
    hdr->rm_call.cb_vers = vers;
    hdr->rm_call.cb_proc = 0;  /* no procedure yet */
    hdr->rm_call.cb_cred = sunrpcAuthDummy;
    hdr->rm_call.cb_verf = sunrpcAuthDummy;
    
    xTrace1(sunrpcp, 3, "SUNRPC open returns %x", s);
    return s;
}


#define SUNRPC_MAX_PARTS	5

static xkern_return_t
sunrpcOpenEnable( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PassiveKey	key;
    PSTATE	*pstate;
    Part	sp[SUNRPC_MAX_PARTS];
    
    xTrace0(sunrpcp, 3, "SUNRPC open enable");
    if ( partLen(p) > SUNRPC_MAX_PARTS ) {
	return XK_FAILURE;
    }
    pstate = (PSTATE *)self->state;
    /*
     * Only servers call openenable
     */
    if ( extractPart(p, &key.p.vers, &key.p.prog, &key.port,
		     "sunrpcOpenEnable") ) {
	return XK_FAILURE;
    }
    xIfTrace(sunrpcp, 5) {
	sunrpcPrPassiveKey(&key, "openenable binding");
    }
    /* 
     * Save the participants to use in opendisable (if necessary)
     */
    bcopy((char *)p, (char *)sp, partLen(p) * sizeof(Part));
    /*
     * Now openenable downward
     */
    xTrace0(sunrpcp, 7, "SUNRPC openenable bound ");
    if ( xOpenEnable(self, self, xGetDown(self, 0), p) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    if ( defaultOpenEnable(pstate->passiveMap, hlpRcv, hlpType, &key)
		== XK_FAILURE ) {
	xOpenDisable(self, self, xGetDown(self, 0), sp);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static xkern_return_t
sunrpcOpenDisable( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PassiveKey	key;
    PSTATE	*pstate;
    
    xTrace0(sunrpcp, TR_MAJOR_EVENTS, "SUNRPC open disable");
    pstate = (PSTATE *)self->state;
    if ( extractPart(p, &key.p.vers, &key.p.prog, &key.port,
		     "sunrpcOpenDisable") ) {
	return XK_FAILURE;
    }
    xIfTrace(sunrpcp, 5) {
	sunrpcPrPassiveKey(&key, "openDisable key");
    }
    if ( defaultOpenDisable(pstate->passiveMap, hlpRcv, hlpType, &key)
		== XK_FAILURE ) {
	return XK_FAILURE;
    }
    /*
     * Now openDisable downward
     */
    return xOpenDisable(self, self, xGetDown(self, 0), p);
}


/* 
 * server sessions are cached and garbage collected, so close does nothing
 */
static xkern_return_t
closeServer(s)
    XObj s;
{
    xTrace1(sunrpcp, 3, "SUNRPC close of server session %x (does nothing)", s);
    xAssert(xIsSession(s));
    xAssert(s->rcnt == 0);
    return XK_SUCCESS;
}


/* 
 * Called immediately for client sessions and by the garbage collector
 * for server sessions
 */
static xkern_return_t
destroySessn(s)
    XObj s;
{
    SState	*state;
    XObj	lls;

    xTrace1(sunrpcp, 3, "SUNRPC destroy session %x", s);
    if (!s) return XK_SUCCESS;
    state = (SState *)s->state;
    lls = xGetDown(s, 0);
    xAssert(xIsSession(lls));
    switch ( state->hdr.rm_direction ) {
      case CALL:
	{
	    long	port;
	    
	    if ( xControl(lls, GETMYPROTO, (char *)&port, sizeof(port))
		<= sizeof(port) ) {
		put_port(port);  
	    }
	    msgDestroy(&state->c_callMsg);
	    clean_hdr(&state->hdr);
	    break;
	}

      case REPLY:
	{
	    xAssert( s->binding );
	    mapRemoveBinding(((PSTATE *)s->myprotl->state)->activeMap,
			     s->binding);
	    sunrpcAuthFree(&state->s_cred);
	    clean_hdr(&state->hdr);
	    break;
	}

      default:
	xError("sunrpc destroy session of unknown type");
    }
    xClose(lls);
    xDestroy(s);
    return XK_SUCCESS;
}


static xkern_return_t
rpc_demux(self, s, dg)
    XObj self;
    XObj s;
    Msg *dg;
{
    SunrpcHdr		hdr;
    int			status;
    xkern_return_t 	retVal;
    
    /*
     * Decode header
     */
    bzero((char *)&hdr,sizeof(hdr));
    if (! msgPop(dg, sunrpcHdrLoad, &hdr, MIN(XDRHDRSIZE, msgLen(dg)),
		 &status)) {
	xTrace0(sunrpcp, 3, "sunrpc_demux: msgPop failed");
    }
    if ( status == -1 ) {
	xTrace0(sunrpcp, 3, "rpc_demux: can't decode header!");
	return XK_FAILURE;
    }
    xIfTrace(sunrpcp,5) {
	prpchdr(hdr, "sunrpcDemux hdr");
    }
    /*
     * Look into header for info about who to send the msg to
     */
    if (hdr.rm_direction == CALL) {
	retVal = sunrpcServerDemux(self, s, dg, &hdr);
    } else if (hdr.rm_direction == REPLY) {
	retVal = sunrpcClientDemux(self, s, dg, &hdr);
    } else {
	xTrace1(sunrpcp, 3,
		"sunrpc demux: unknown direction type %d",
		hdr.rm_direction);
	retVal = XK_FAILURE;
    }
    if (retVal == XK_FAILURE) {
	/*
	 * Drop call messages for which we can't find a server 
	 * that has done an openenablek or for reply messages 
	 * for which we can't find a waiting caller. 
	 */
	xTrace0(sunrpcp, 3, "SUNRPC demux dropping the message");
	clean_hdr(&hdr);
    }
    return retVal;
}


static void
clean_hdr(hdr)
    struct rpc_msg *hdr; 
{
    if (hdr->rm_direction == CALL) {
	sunrpcAuthFree(&(hdr->rm_call.cb_verf));
	sunrpcAuthFree(&(hdr->rm_call.cb_cred));
    } else if (hdr->rm_reply.rp_stat == MSG_ACCEPTED) {
	sunrpcAuthFree(&(hdr->acpted_rply.ar_verf));
    } 
}


static u_short ports[128] = { 0, /* ... */ };
static int top_port=127;

u_short
sunrpcGetPort()
{
    static u_short first=1; 
    int i;
    
    if (first) {
	first=0;
	for (i=0;i<128;i++) ports[i]=112+i;
	top_port=127;
    }
    if (top_port < 0) 
      xError("sunrpc: Out of Ports!");
    
    return(ports[top_port--]);
}


static void
put_port(port)
    u_short port;
{
    ports[++top_port] = port;
}


void
sunrpcAuthFree(auth)
    struct opaque_auth *auth;
{
    switch (auth->oa_flavor) {

      case 0: 
	*auth = sunrpcAuthDummy;
	break;

      default:
	if (auth->oa_base) xFree((char *)auth->oa_base);
	*auth = sunrpcAuthDummy;
	break;
    }
}


static void
getProcProtl(p)
    XObj p;
{
    p->control = sunrpcControlProtl;
    p->open = sunrpcOpen;
    p->openenable = sunrpcOpenEnable;
    p->opendisable = sunrpcOpenDisable;
    p->demux = rpc_demux;
}


static void 
getProcClient(s)
    XObj s;
{
    s->call = sunrpcCall;
    s->control = sunrpcControlSessn;
    s->close = destroySessn;
}

void 
sunrpcGetProcServer(s)
    XObj s;
{
    s->pop = sunrpcPop;
    s->control = sunrpcControlSessn;
    s->close = closeServer;
}


