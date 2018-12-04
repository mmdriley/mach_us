/* 
 * sunrpc_server.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.25 $
 * $Date: 1993/02/01 22:29:46 $
 */

#include "xkernel.h"
#include "sunrpc_i.h"
#include "xrpc_print.h"

#ifdef __STDC__

static XObj	createServerSessn( XObj, ActiveKey *, XObj, SunrpcHdr * );
static int	pushReply( XObj, Msg *, Msg * );

#else

static XObj	createServerSessn();
static int	pushReply();

#endif


static int
pushReply(s, msg, reply_ptr)
    XObj s;
    Msg *msg;
    Msg *reply_ptr;
{
    SState	*state;
    struct rpc_msg *hdr;
    
    xTrace0(sunrpcp, 3, "in sunrpc pushReply");
    state = (SState *)s->state;
    xAssert(state->hdr.rm_direction == REPLY);
    hdr = &state->hdr;
    xIfTrace(sunrpcp, 7) {
	prpchdr(*hdr, "sunrpc pushReply");
    }
    xTrace0(sunrpcp, 3, "in sunrpc pushReply going to encode header ");
    if (sunrpcEncodeHdr(msg, hdr) == -1) {
	xTrace0(sunrpcp, 3, "sunrpc pushReply: can't encode header");
	return -1;
    }
    xTrace0(sunrpcp, 3, "in sunrpc pushReply after encoding header ");
    /*
     * push message to udp
     */
    if (xPush(xGetDown(s, 0), msg) != 0) {
	xTrace0(sunrpcp, 1, "sunrpc pushReply received error from llp push");
	return -1;
    }
    return 0;
}



/*
 * rpc_pop is only done for call messages received by server
 */
xkern_return_t
sunrpcPop(self, s, dg, arg)
    XObj self;
    XObj s;
    Msg *dg;
    VOID *arg;
{
    Msg		reply;
    long	sXid;
    SState	*state;
    
    xTrace0(sunrpcp, 3, "SUNRPC_pop");
    /* 
     * Save the current value of the transaction id.  If this
     * session's xid has changed by the time the reply is ready, the
     * client has given up on this request and we won't send the
     * message. 
     */
    state = (SState *)self->state;
    sXid = state->hdr.rm_xid;
    /* push procedure onto front of message; save time by not using XDR */
    msgPush(dg,(MStoreFun)bcopy,&state->s_proc,sizeof(long),0);
    msgConstructEmpty(&reply);
    if (xCallDemux(self, dg, &reply)==XK_FAILURE)
    { xTrace0(sunrpcp, TR_FULL_TRACE,
	      "SUNRPC pop -- xCallDemux returned failure, not sending reply");}
    else
    { if ( sXid == state->hdr.rm_xid ) { pushReply(self, &reply, 0); }
      else {
	xTrace2(sunrpcp, 3,
		"SUNRPC pop -- xid mismatch (%d vs. %d), not sending reply",
		sXid, state->hdr.rm_xid);
      }; };
    msgDestroy(&reply);
    return XK_SUCCESS;
}


static XObj
createServerSessn( self, aKey, lls, hdr )
    XObj	self, lls;
    ActiveKey	*aKey;
    SunrpcHdr	*hdr;
{
    PassiveKey	pKey;
    Enable	*e;
    XObj	s;
    SState	*state;
    PSTATE	*pstate = (PSTATE *)self->state;

    pKey.p = aKey->p;
    if ( xControl(lls, GETMYPROTO, (char *)&pKey.port,
		  sizeof(pKey.port)) < sizeof(pKey.port) ) {
	xError("SUNRPC could not get prot number of lls");
	return ERR_XOBJ;
    }
    xIfTrace(sunrpcp, 5) {
	sunrpcPrPassiveKey(&pKey, "demux lookup passive key");
    }
    if ( mapResolve(pstate->passiveMap, &pKey, &e) == XK_FAILURE ) {
	/*
	 * punt cant find server session 
	 * will not even try to find out why!
	 */
	sunrpcSendError(PROG_UNAVAIL, lls, hdr->rm_xid, 0);
	return ERR_XOBJ;
    }
    xTrace0(sunrpcp, TR_EVENTS, "SUNRPC found server ");
    s = xCreateSessn(sunrpcGetProcServer, e->hlpRcv, e->hlpType, self, 1,
		     &lls);
    if ( s == ERR_XOBJ ) {
	xTrace0(sunrpcp, TR_EVENTS,
		"SUNRPC demux could not create new session");
	return ERR_XOBJ;
    }
    xDuplicate(lls);
    state = X_NEW(SState);
    s->state = (VOID *)state;
    bzero((char *)state, sizeof(SState));
    /*
     * Set up the RPC header template
     */
    state->s_cred = sunrpcAuthDummy;
    state->hdr.rm_direction = REPLY;
    state->hdr.rm_reply.rp_stat = MSG_ACCEPTED;
    state->hdr.rm_reply.rp_acpt.ar_verf = sunrpcAuthDummy;
    state->hdr.rm_reply.rp_acpt.ar_stat = SUCCESS;
    xOpenDone(e->hlpRcv, s, self);
    xIfTrace(sunrpcp, TR_DETAILED) {
	sunrpcPrActiveKey(aKey, "binding new server session");
    }
    s->binding = mapBind(pstate->activeMap, aKey, s);
    xAssert( s->binding != ERR_BIND );
    return s;
}


xkern_return_t
sunrpcServerDemux(self, lls, dg, hdr)
    XObj self;
    XObj lls;
    Msg *dg;
    SunrpcHdr *hdr;
{
    XObj	rpc_s;
    ActiveKey	aKey;
    SState	*state;
    PSTATE	*pstate;

    xTrace0(sunrpcp, 3, "sunrpcServerDemux");
    pstate = (PSTATE *)self->state;
    /*
     * check if RPC version is valid
     */
    if ((hdr->rm_call.cb_rpcvers > RPC_VERS_HIGH) || 
	(hdr->rm_call.cb_rpcvers < RPC_VERS_LOW)) {
	sunrpcSendError(RPC_MISMATCH, lls, hdr->rm_xid, 0);
	return XK_FAILURE;
    }
    aKey.p.prog = hdr->rm_call.cb_prog;
    aKey.p.vers = hdr->rm_call.cb_vers;
    aKey.lls = lls;
    if ( mapResolve(pstate->activeMap, &aKey, &rpc_s) == XK_FAILURE ) {
	xTrace0(sunrpcp, TR_EVENTS,
		"sunrpcServerDemux, no active sessn, looking for openenable");
	rpc_s = createServerSessn(self, &aKey, lls, hdr);
	if ( rpc_s == ERR_XOBJ ) {
	    return XK_FAILURE;
	}
    } else {
	xTrace0(sunrpcp, 3, "sunrpcServerDemux found existing session");
    }
    state = (SState *)rpc_s->state;
    state->hdr.rm_xid = hdr->rm_xid;
    state->s_proc = hdr->rm_call.cb_proc;  /* save procedure number */
    /* 
     * Save new values of cred and verf from caller's header
     */
    sunrpcAuthFree(&state->hdr.rm_reply.rp_acpt.ar_verf);
    sunrpcAuthFree(&state->s_cred);
    state->hdr.rm_reply.rp_acpt.ar_verf = hdr->rm_call.cb_verf;
    state->s_cred = hdr->rm_call.cb_cred;
    hdr->rm_call.cb_cred = sunrpcAuthDummy;
    hdr->rm_call.cb_verf = sunrpcAuthDummy;
    
    xPop(rpc_s, lls, dg, 0); 
    return XK_SUCCESS;
}


