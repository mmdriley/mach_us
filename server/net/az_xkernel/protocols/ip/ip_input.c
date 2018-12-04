/*     
 * ip_input.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:23:16 $
 */


#include "xkernel.h"
#include "ip.h"
#include "ip_i.h"
#include "route.h"

#ifdef __STDC__
static int		onLocalNet( XObj, IPhost * );
static int		validateOpenEnable( XObj );
#else
static int		onLocalNet();
static int		validateOpenEnable();
#endif __STDC__


/*
 * ipDemux
 */
xkern_return_t
ipDemux(self, transport_s, dg)
    XObj self;
    XObj transport_s;
    Msg *dg;
{
    IPheader	hdr;
    XObj        s;
    ActiveId	actKey;
    PState	*pstate = (PState *) self->state;
    int		dataLen;
    char	options[40];
    
    xTrace1(ipp, TR_EVENTS,
	    "IP demux called with datagram of len %d", msgLen(dg));
    if ( ipGetHdr(dg, &hdr, options) ) {
	xTrace0(ipp, TR_SOFT_ERRORS,
		"IP demux : getHdr problems, dropping message\n"); 
	return XK_SUCCESS;
    }
    xTrace3(ipp, TR_MORE_EVENTS,
	    "ipdemux: seq=%d,frag=%d, len=%d", hdr.ident, hdr.frag,
	    msgLen(dg));
    if (GET_HLEN(&hdr) > 5) {
	xTrace0(ipp, TR_SOFT_ERRORS,
		"IP demux: I don't understand options!  Dropping msg");
	return XK_SUCCESS;
    }
    dataLen = hdr.dlen - IPHLEN;
    if ( dataLen < msgLen(dg) ) {
	xTrace1(ipp, TR_MORE_EVENTS,
		"IP demux : truncating right at byte %d", dataLen);
	msgTruncate(dg, dataLen);
    }
    actKey.protNum = hdr.prot;
    actKey.remote = hdr.source;
    actKey.local = hdr.dest;
    if ( mapResolve(pstate->activeMap, &actKey, &s) == XK_FAILURE ) {
	FwdId	fwdKey;
	IPhost	mask;

	netMaskFind(&mask, &hdr.dest);
	IP_AND(fwdKey, mask, hdr.dest);
	if ( mapResolve(pstate->fwdMap, &fwdKey, &s) == XK_FAILURE ) {
	    xTrace0(ipp, TR_EVENTS, "no active session found");
	    s = ipCreatePassiveSessn(self, transport_s, &actKey, &fwdKey);
	    if ( s == ERR_XOBJ ) {
		xTrace0(ipp, TR_EVENTS, "...dropping the message");
		return XK_SUCCESS;
	    }
	}
    }
#ifdef IP_SIM_DROPS
    /*
     * Simulate a dropped packet
     */
    {
	static int c = 0;
	
	if (++c % ( 234 + 4 * hdr.dest.d) == 0) {
	    xTrace0(ipp, TR_ALWAYS, "IP simulates dropped packet");
	    return XK_SUCCESS;
	} else {
	    xTrace2(ipp, TR_FULL_TRACE,
		    "Not dropping packet, c = %d, mod = %d",
		    c, 234 + 4 * hdr.dest.d);
	}
    }
#endif
#ifdef IP_SIM_DELAYS
    /*
     * Simulate a delayed packet
     */
    {
	static int c = 0;
	
	if (++c % ( 234 + 4 * hdr.dest.d) == 0) {
	    xTrace0(ipp, TR_ALWAYS, "IP simulates delayed packet");
	    Delay(50);
	}
    }
#endif
    return xPop(s, transport_s, dg, &hdr);
}


xkern_return_t
ipForwardPop( s, lls, msg, inHdr )
    XObj	s, lls;
    Msg 	*msg;
    VOID	*inHdr;
{
    IPheader		*h = (IPheader *)inHdr;
    xmsg_handle_t	res;

    xTrace0(ipp, TR_EVENTS, "ip forward pop");
    xAssert(h);
    if ( --h->time == 0 ) {
	xTrace0(ipp, TR_EVENTS, "ttl == 0 -- dropping");
	return XK_SUCCESS;
    }
    /* 
     * We need to go through ipSend because the MTU on the outgoing
     * interface might be less than the packet size (and need
     * fragmentation.) 
     */
    res = ipSend(s, xGetDown(s, 0), msg, h);
    return ( res == XMSG_ERR_HANDLE ) ? XK_FAILURE : XK_SUCCESS;
}


static int
onLocalNet( llo, h )
    XObj	llo;
    IPhost	*h;
{
    int	res;

    res = xControl(llo, VNET_HOSTONLOCALNET, (char *)h, sizeof(IPhost));
    if ( res < 0 ) {
	xTrace0(ipp, TR_ERRORS, "ipFwdBcst couldn't do HOSTONLOCALNET on llo");
	return 0;
    }
    return res > 0;
}


/* 
 * Used for ipFwdBcast sessions, sessions which receive network broadcasts
 * in a subnet  environment.  Depending on the incoming interface, the
 * message may  need to be forwarded on other interfaces and locally
 * accepted, or it may be dropped.  See RFC 922.
 *
 * This is not very efficient and is further complicated by the hiding
 * of interfaces in VNET.
 */
xkern_return_t
ipFwdBcastPop( s, llsIn, msg, inHdr )
    XObj	s, llsIn;
    Msg 	*msg;
    VOID	*inHdr;
{
    IPheader		*h = (IPheader *)inHdr;
    xmsg_handle_t	res;
    route		*rt;
    Msg			msgCopy;
    VOID		*ifcId;
    XObj		lls;

    xTrace0(ipp, TR_EVENTS, "ip forward bcast pop");
    xAssert(h);
    if ( --h->time == 0 ) {
	xTrace0(ipp, TR_EVENTS, "ttl == 0 -- dropping");
	return XK_SUCCESS;
    }
    /* 
     * Did this packet come in on the interface we would use to reach
     * the source?  If not, drop the message
     */
    if ( ! onLocalNet(llsIn, &h->source) ) {
	if ( onLocalNet(xGetDown(xMyProtl(s), 0), &h->source ) ) {
	    xTrace0(ipp, TR_EVENTS,
	          "ipFwdBcast gets packet not on direct connection, dropping");
	    return XK_SUCCESS;
	}
	rt = rt_get( &h->source );
	if ( ! onLocalNet(llsIn, &rt->gw) ) {
	    xTrace0(ipp, TR_EVENTS,
		    "ipFwdBcast receives packet from dup gw, dropping");
	    return XK_SUCCESS;
	}
    }
    /* 
     * If this is a session used for local delivery (i.e., if the
     * down[0] is valid), send up for local processing (reassemble first)
     */
    if ( xIsSession(xGetDown(s, 0)) ) {
	msgConstructCopy(&msgCopy, msg);
	ipStdPop(s, llsIn, &msgCopy, h);
	msgDestroy(&msgCopy);
    }
    /* 
     * Send this message back out on all appropriate interfaces except
     * the one on which it was received.
     */
    lls = xGetDown(s, 1);
    xAssert(xIsSession(lls));
    xControl(llsIn, VNET_GETINTERFACEID, (char *)&ifcId, sizeof(VOID *));
    if ( xControl(lls, VNET_DISABLEINTERFACE, (char *)&ifcId, sizeof(VOID *))
		< 0 ) {
	xTrace0(ipp, TR_ERRORS,
		"ipFwdBcastPop could not disable lls interface");
	return XK_SUCCESS;
    }
    res = ipSend(s, lls, msg, h);
    xControl(lls, VNET_ENABLEINTERFACE, (char *)&ifcId, sizeof(VOID *));
    return ( res == XMSG_ERR_HANDLE ) ? XK_FAILURE : XK_SUCCESS;
}


/* 
 * validateOpenEnable -- Checks to see if there is still an openEnable for
 * the session and, if so, calls openDone.
 * This is called right before a message is sent up through
 * a session with no external references.  This has to be done
 * because IP sessions
 * can survive beyond removal of all external references. 
 *
 * Returns 1 if an openenable exists, 0 if it doesn't.
 */
static int
validateOpenEnable( s )
    XObj	s;
{
    SState	*ss = (SState *)s->state;
    Enable	*e;

    e = ipFindEnable(xMyProtl(s), ss->hdr.prot, &ss->hdr.source);
    if ( e == ERR_ENABLE ) {
	xTrace1(ipp, TR_MAJOR_EVENTS, "ipValidateOE -- no OE for hlp %d!",
		ss->hdr.prot);
	return 0;
    }
    xOpenDone(e->hlpRcv, s, xMyProtl(s));
    return 1;
}


xkern_return_t
ipMsgComplete(s, lls, dg, inHdr)
    XObj	s, lls;
    Msg 	*dg;
    VOID	*inHdr;
{
    IPheader *h = (IPheader *)inHdr;
    IPpseudoHdr ph;
    
    if ( s->rcnt == 1 && ! validateOpenEnable(s) ) {
	return XK_SUCCESS;
    }
    xAssert(h);
    ph.src = h->source;
    ph.dst = h->dest;
    ph.zero = 0;
    ph.prot = h->prot;
    ph.len = htons( msgLen(dg) );
    msgSetAttr(dg, 0, (VOID *)&ph, sizeof(IPpseudoHdr));
    xTrace1(ipp, TR_EVENTS, "IP pop, length = %d", msgLen(dg));
    xAssert(xIsSession(s));
    return xDemux(s, dg);
}



xkern_return_t
ipStdPop( s, lls, dg, hdr )
    XObj	s, lls;
    Msg		*dg;
    VOID	*hdr;
{
    s->idle = FALSE;
    if (COMPLETEPACKET(*(IPheader *)hdr)) {
	return ipMsgComplete(s, lls, dg, hdr);
    } else {
	return ipReassemble(s, lls, dg, hdr);
    }
}


