/*     
 * $RCSfile: udp.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.71 $
 * $Date: 1993/02/01 22:28:30 $
 */

#include "xkernel.h"
#include "ip.h"
#include "udp.h"
#include "udp_internal.h"
#include "gc.h"

int	traceudpp;


/* #define UDP_USE_GC */


/* 
 * Check port range
 */
#define portCheck(_port, name, retval) 					\
	{								\
    	  if ( (_port) < 0 || (_port) > MAX_PORT ) {			\
		xTrace2(udpp, TR_SOFT_ERRORS,				\
		   "%s port %d out of range", (name), (_port));  	\
		return (retval);					\
	  }								\
        }								


#ifdef __STDC__

static void	getproc_sessn(XObj);
static void	getproc_protl(XObj);
static XObj	udpCreateSessn( XObj, XObj, XObj, ActiveId * );
static int	getPorts( Part *, UDPport *, UDPport *, char * );
static xkern_return_t	udpOpenDisableAll( XObj, XObj );
#ifdef UDP_USE_GC
static void	udpDestroySessn( XObj );
#endif

#else

static int	getPorts();
static void	getproc_sessn();
static void	getproc_protl();
static XObj	udpCreateSessn();
#ifdef UDP_USE_GC
static void	udpDestroySessn();
#endif

#endif __STDC__


#define		SESSN_COLLECT_INTERVAL	30 * 1000 * 1000    /* 30 seconds */
#define		ACTIVE_MAP_SIZE 101
#define		PASSIVE_MAP_SIZE 23

static void
dispPseudoHdr(h)
    IPpseudoHdr *h;
{
    xTrace2(udpp, TR_ALWAYS, "   IP pseudo header: src: %s  dst: %s",
	    ipHostStr(&h->src), ipHostStr(&h->dst));
    xTrace3(udpp, TR_ALWAYS, "      z:  %d  p: %d len: %d",
	    h->zero, h->prot, ntohs(h->len));
}


/*
 * udpHdrStore -- write header to potentially unaligned msg buffer.
 * Note:  *hdr will be modified
 */
static void
udpHdrStore(hdr, dst, len, arg)
    VOID *hdr;
    char *dst;
    long int len;
    VOID *arg;
{
    SSTATE *sstate;
    
    xAssert(len == sizeof(HDR));
    xAssert(arg);
    
    ((HDR *)hdr)->ulen = htons(((HDR *)hdr)->ulen);
    ((HDR *)hdr)->sport = htons(((HDR *)hdr)->sport);
    ((HDR *)hdr)->dport = htons(((HDR *)hdr)->dport);
    ((HDR *)hdr)->sum = 0;
    bcopy((char *)hdr, dst, sizeof(HDR));
    sstate = (SSTATE *)((storeInfo *)arg)->s->state;
    if (sstate->useCkSum) {
	u_short sum = 0;
	
	xTrace0(udpp, TR_FUNCTIONAL_TRACE, "Using UDP checksum");
	sstate->pHdr.len = ((HDR *)hdr)->ulen;	/* already in net byte order */
	xIfTrace(udpp, TR_FUNCTIONAL_TRACE) {
	    dispPseudoHdr(&sstate->pHdr);
	}
	sum = inCkSum(((storeInfo *)arg)->m, (u_short *)&sstate->pHdr,
		       sizeof(IPpseudoHdr));
	bcopy((char *)&sum, (char *)&((HDR *)dst)->sum, sizeof(u_short));
	xAssert(! inCkSum(((storeInfo *)arg)->m, (u_short *)&sstate->pHdr,
			  sizeof(IPpseudoHdr)));
    }
}


/*
 * udpHdrLoad -- load header from potentially unaligned msg buffer.
 * Result of checksum calculation will be in hdr->sum.
 */
static long
udpHdrLoad(hdr, src, len, arg)
    VOID *hdr;
    char *src;
    long int len;
    VOID *arg;
{
  xAssert(len == sizeof(HDR));
  /*
   * 0 in the checksum field indicates checksum disabled
   */
  bcopy(src, (char *)hdr, sizeof(HDR));
  if (((HDR *)hdr)->sum) {
    IPpseudoHdr *pHdr = (IPpseudoHdr *)msgGetAttr((Msg *)arg, 0);

    xTrace0(udpp, TR_FUNCTIONAL_TRACE, "UDP header checksum was used");
    xAssert(pHdr);
    xIfTrace(udpp, TR_FUNCTIONAL_TRACE) {
      dispPseudoHdr(pHdr);
    }
    ((HDR *)hdr)->sum = inCkSum((Msg *)arg, (u_short *)pHdr, sizeof(*pHdr));
  } else {
    xTrace0(udpp, TR_FUNCTIONAL_TRACE, "No UDP header checksum was used");
  }
  ((HDR *)hdr)->ulen = ntohs(((HDR *)hdr)->ulen);
  ((HDR *)hdr)->sport = ntohs(((HDR *)hdr)->sport);
  ((HDR *)hdr)->dport = ntohs(((HDR *)hdr)->dport);
  return sizeof(HDR);
}


/*
 * udp_init
 */
void
udp_init(self)
    XObj self;
{
    Part	part;
    PSTATE	*pstate;
    XObj	llp, ip;
    
    xTrace0(udpp, TR_GROSS_EVENTS, "UDP init");
    xAssert(xIsProtocol(self));
    
    getproc_protl(self);
    pstate = X_NEW(PSTATE);
    self->state = (VOID *)pstate;
    pstate->activemap = mapCreate(ACTIVE_MAP_SIZE, sizeof(ActiveId));
    pstate->passivemap = mapCreate(PASSIVE_MAP_SIZE, sizeof(PassiveId)); 
    ip = xGetProtlByName("ip");
    if ( ! xIsXObj(ip) ) {
	Kabort("UDP can't get IP protocol object");
    }
    if ( (pstate->llpProt = relProtNum(self, ip)) == -1 ) {
	Kabort("UDP can't get protocol number relative to IP");
    }
    if ( ! xIsProtocol(llp = xGetDown(self, 0)) ) {
	Kabort("UDP could not get lower protocol");
    }
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    if ( xOpenEnable(self, self, llp, &part) == XK_FAILURE ) {
	xTrace0(udpp, TR_ALWAYS,
		"udp_init: can't openenable transport protocol");
	xFree((char *) pstate);
	return;
    }
#ifdef UDP_USE_GC
    initSessionCollector(pstate->activemap, SESSN_COLLECT_INTERVAL,
			 udpDestroySessn, "udp");
#endif
    udpPortMapInit();
    xTrace0(udpp, TR_GROSS_EVENTS, "UDP init done");
}


/* 
 * getPorts -- extract ports from the participant, checking for validity.
 * If lPort is 0, no attempt to read a local port is made.
 * Returns 0 if the port extraction succeeded, -1 if there were problems. 
 */
static int
getPorts( p, rPort, lPort, str )  
    Part	*p;
    UDPport	*lPort, *rPort;
    char	*str;
{
    long	*port;
    long	freePort;

    xAssert(rPort);
    if ( ! p || partLen(p) < 1 ) {
	xTrace1(udpp, TR_SOFT_ERRORS, "%s -- bad participants", str);
	return -1;
    }
    if ( (port = (long *)partPop(p[0])) == 0 ) {
	xTrace1(udpp, TR_SOFT_ERRORS, "%s -- no remote port", str);
	return -1;
    }
    if (*port != ANY_PORT) {
	portCheck(*port, str, -1);
    } /* if */
    *rPort = *port;
    if ( lPort ) {
	if ( partLen(p) < 2 ||
	    (port = (long *)partPop(p[1])) != 0 && *port == ANY_PORT ) {
	    /* 
	     * No port specified -- find a free one
	     */
	    if ( udpGetFreePort(&freePort) ) {
		sprintf(errBuf, "%s -- no free ports!", str);
		xError(errBuf);
		return -1;
	    }
	    *lPort = freePort;
	} else {
	    /* 
	     * A specific local port was requested
	     */
	    if ( port == 0 ) {
		xTrace1(udpp, TR_SOFT_ERRORS,
			"%s -- local participant, but no local port", str);
		return -1;
	    }
	    portCheck(*port, str, -1);
	    *lPort = *port;
	    udpDuplicatePort(*lPort);
	}
    }
    return 0;
}


/*
 * udpOpen
 */
static XObj
udpOpen( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    XObj	udp_s;
    XObj	lls;
    ActiveId 	key;
    PSTATE	*pstate;
    
    xTrace0(udpp, TR_MAJOR_EVENTS, "UDP open");
    pstate = (PSTATE *)self->state;
    if ( getPorts(p, &key.remoteport, &key.localport, "udpOpen") ) {
	return ERR_XOBJ;
    }
    xTrace2(udpp, TR_MAJOR_EVENTS, "UDP open: from port %d to port %d",
	    key.localport, key.remoteport);
    udp_s = ERR_XOBJ;	
    lls = xOpen(self, self, xGetDown(self, 0), p);
    if ( lls != ERR_XOBJ ) {
	key.lls = lls;
	if ( mapResolve(pstate->activemap, &key, &udp_s) == XK_FAILURE ) {
	    xTrace0(udpp, TR_MAJOR_EVENTS, "udpOpen creates new session");
	    udp_s = udpCreateSessn(self, hlpRcv, hlpType, &key);
	    if ( udp_s != ERR_XOBJ ) {
		/* 
		 * A successful open!
		 */
		xTrace1(udpp, TR_MAJOR_EVENTS, "UDP open returns %x", udp_s);
		return udp_s;
	    } 
	} else {
	    /*
	     * We don't allow multiple opens of the same UDP session.  If
	     * the refcount is zero, the session is just being idle
	     * awaiting garbage collection
	     */
	    if ( udp_s->rcnt > 0 ) {
		xTrace0(udpp, TR_MAJOR_EVENTS, "udpOpen ERROR -- found existing session!");
		udp_s = ERR_XOBJ;
	    } else {
		udp_s->idle = FALSE;
	    }
	}	
	xClose(lls);
    }
    return udp_s;
}


static XObj
udpCreateSessn(self, hlpRcv, hlpType, key)
    XObj self, hlpRcv, hlpType;
    ActiveId *key;
{
    XObj	s;
    SSTATE	*sstate;
    PSTATE	*pstate;
    HDR		*udph;

    pstate = (PSTATE *)self->state;
    s = xCreateSessn(getproc_sessn, hlpRcv, hlpType, self, 1, &key->lls);
    s->binding = mapBind(pstate->activemap, (char *)key, (int) s);
    sstate = (SSTATE *)xMalloc(sizeof(SSTATE));
    if ( xControl(key->lls, GETMYHOST, (char *)&sstate->pHdr.src,
		  sizeof(IPhost)) == -1 ) {
	xTrace0(udpp, TR_MAJOR_EVENTS,
		"UDP create sessn could not get local host from lls");
	xFree((char *)sstate);
	return ERR_XOBJ;
    }
    if (xControl(key->lls, GETPEERHOST, (char *)&sstate->pHdr.dst,
		 sizeof(IPhost)) == -1) {
	xTrace0(udpp, TR_MAJOR_EVENTS,
		"UDP createSessn could not get remote host from lls");
	xFree((char *)sstate);
	return ERR_XOBJ;
    }
    sstate->pHdr.zero = 0;
    sstate->pHdr.prot = pstate->llpProt;
    sstate->useCkSum = USE_CHECKSUM_DEF;
    s->state = (char *)sstate;
    udph = &(sstate->hdr);
    udph->sport = key->localport;
    udph->dport = key->remoteport;
    udph->ulen = 0;
    udph->sum = 0;
    return s;
}


/*
 * udpControlSessn
 */
static int
udpControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
  SSTATE        *sstate;
  PSTATE        *pstate;
  HDR           *hdr;
  
  xAssert(xIsSession(s));
  
  sstate = (SSTATE *) s->state;
  pstate = (PSTATE *) s->myprotl->state;
  
  hdr = &(sstate->hdr);
  switch (opcode) {
    
  case UDP_DISABLE_CHECKSUM:
    sstate->useCkSum = 0;
    return 0;

  case UDP_ENABLE_CHECKSUM:
    sstate->useCkSum = 1;
    return 0;

  case GETMYPROTO:
    checkLen(len, sizeof(long));
    *(long *)buf = sstate->hdr.sport;
    return sizeof(long);

  case GETPEERPROTO:
    checkLen(len, sizeof(long));
    *(long *)buf = sstate->hdr.dport;
    return sizeof(long);

   case GETPARTICIPANTS:
     {
	 Part	*p = (Part *)buf;
	 int 	retLen;
	 long	*port;

	 retLen = xControl(xGetDown(s, 0), opcode, buf, len);
	 if ( retLen >= sizeof(Part) ) {
	     port = X_NEW(long);
	     *port = sstate->hdr.dport;
	     partPush(p[0], port, sizeof(long));
	 }
	 if ( retLen >= 2 * sizeof(Part) ) {
	     port = X_NEW(long);
	     *port = sstate->hdr.sport;
	     partPush(p[1], port, sizeof(long));
	 }
	 return retLen;
     }

  case GETMAXPACKET:
  case GETOPTPACKET:
    {
	checkLen(len, sizeof(int));
	if ( xControl(xGetDown(s, 0), opcode, buf, len) < sizeof(int) ) {
	    return -1;
	}
	*(int *)buf -= sizeof(HDR);
	return sizeof(int);
    }
    
  default:
    return xControl(xGetDown(s, 0), opcode, buf, len);;
  }
}


/*
 * udpControlProtl
 */
static int
udpControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    long port;

    xAssert(xIsProtocol(self));
    
    switch (opcode) {
	
      case UDP_GETFREEPROTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	if (udpGetFreePort(&port)) {
	    return -1;
	} /* if */
	*(long *)buf = port;
	return 0;

      case UDP_RELEASEPROTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	udpReleasePort(port);
	return 0;

      default:
	return xControl(xGetDown(self, 0), opcode, buf, len);
	
    }
}


/*
 * udpOpenEnable
 */
static xkern_return_t
udpOpenEnable( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PSTATE 	*pstate;
    Enable 	*e;
    PassiveId	key;
    
    xTrace0(udpp, TR_MAJOR_EVENTS, "UDP open enable");
    pstate = (PSTATE *)self->state;
    if ( getPorts(p, &key, 0, "udpOpenEnable") ) {
	return XK_FAILURE;
    }
    xTrace1(udpp, TR_MAJOR_EVENTS, "Port number %d", key);
    if ( mapResolve(pstate->passivemap, &key, &e) != XK_FAILURE ) {
	if ( e->hlpRcv == hlpRcv ) {
	    e->rcnt++;
	    return XK_SUCCESS;
	}
	return XK_FAILURE;
    }
    udpDuplicatePort(key);
    e = (Enable *)xMalloc(sizeof(Enable));
    e->hlpRcv = hlpRcv;
    e->hlpType = hlpType;
    e->rcnt = 1;
    e->binding = mapBind(pstate->passivemap, (char *)&key, (int) e);
    if (e->binding == ERR_BIND) {
      xFree((char *)e);
      return XK_FAILURE;
    }
    return XK_SUCCESS;
}


#ifdef UDP_USE_GC

/*
 * udpCloseSessn
 */
static xkern_return_t
udpCloseSessn(s)
    XObj s;
{
    SSTATE	*sstate;

    sstate = (SSTATE *)s->state;

    xAssert(xIsSession(s));
    xTrace1(udpp, TR_MAJOR_EVENTS, "UDP close of session %x", s);
    xAssert(s->rcnt == 0);
    udpReleasePort(sstate->hdr.sport);
    return XK_SUCCESS;
}


/* 
 * This function is udpDestroySessn if we are using garbage
 * collection, udpCloseSessn if we are not.
 */
static void
udpDestroySessn( s )

#else

static xkern_return_t
udpCloseSessn( s )

#endif    
    XObj	s;
{
    PSTATE	*pstate;
    SSTATE	*sstate;
    
    xAssert(xIsSession(s));
    xTrace1(udpp, TR_MAJOR_EVENTS, "UDP destroy session %x", s);
    xAssert(s->rcnt == 0);
    pstate = (PSTATE *)s->myprotl->state;
    sstate = (SSTATE *)s->state;
    mapRemoveBinding(pstate->activemap, s->binding);
    xClose(xGetDown(s, 0));
#ifndef UDP_USE_GC
    udpReleasePort(sstate->hdr.sport);
#endif
    xDestroy(s);
#ifndef UDP_USE_GC
    return XK_SUCCESS;
#endif
}


/*
 * udpCloseProtl
 */
static xkern_return_t
udpCloseProtl(self)
    XObj self;
{
  PSTATE        *pstate;
  
  xAssert(xIsProtocol(self));
  
  if (((self->rcnt)) <= 0) {
      pstate = (PSTATE *) self->state;
      mapClose(pstate->activemap);
      mapClose(pstate->passivemap);
      xFree((char *) pstate);
  }
  return XK_SUCCESS;
}


/*
 * udpPush
 */
/*ARGSUSED*/
static xmsg_handle_t
udpPush(s, msg)
    XObj s;
    Msg *msg;
{
  SSTATE        *sstate;
  HDR           hdr;
  storeInfo	info;
  
  xTrace0(udpp, TR_EVENTS, "in udp push");
  xAssert(xIsSession(s));
  sstate = (SSTATE *) s->state;
  hdr = sstate->hdr;
  hdr.ulen = msgLen(msg) + HLEN;
  info.s = s;
  info.m = msg;
  xTrace2(udpp, TR_EVENTS, "sending msg len %d from port %d",
	 msgLen(msg), hdr.sport);
  xTrace5(udpp, TR_EVENTS, "  to port %d @ %d.%d.%d.%d", hdr.dport,
	 sstate->pHdr.dst.a, sstate->pHdr.dst.b, sstate->pHdr.dst.c,
	 sstate->pHdr.dst.d);
  msgPush(msg, udpHdrStore, &hdr, HLEN, &info);
  return xPush(xGetDown(s, 0), msg);
}


/*
 * udpDemux
 */
static xkern_return_t
udpDemux(self, lls, dg)
    XObj self;
    XObj lls;
    Msg *dg;
{
    HDR   	h;
    XObj   	s;
    ActiveId  	activeid;
    PassiveId	passiveid;
    PSTATE      *pstate;
    Enable	*e;
    
    pstate = (PSTATE *)self->state;
    xTrace0(udpp, TR_EVENTS, "UDP Demux");
    msgPop(dg, udpHdrLoad, &h, HLEN, dg);

    xTrace1(udpp, TR_FUNCTIONAL_TRACE, "Sending host: %s",
	    ipHostStr(&((IPpseudoHdr *)msgGetAttr(dg, 0))->src));
    xTrace1(udpp, TR_FUNCTIONAL_TRACE, "Destination host: %s",
	    ipHostStr(&((IPpseudoHdr *)msgGetAttr(dg, 0))->dst));
    xTrace2(udpp, TR_EVENTS, "sport = %d, dport = %d", h.sport, h.dport);

    if (h.sum) {
	xTrace1(udpp, TR_MAJOR_EVENTS, "udpDemux: bad hdr checksum (%x)! dropping msg",
		h.sum);
	return XK_SUCCESS;
    }
    if ((h.ulen - HLEN) < msgLen(dg)) {
	msgTruncate(dg, (int) h.ulen);
    }
    xTrace2(udpp, TR_FUNCTIONAL_TRACE, " h->ulen = %d, msg_len = %d", h.ulen,
	    msgLen(dg));
    activeid.localport = h.dport;
    activeid.remoteport = h.sport;
    activeid.lls = lls;
    if ( mapResolve(pstate->activemap, &activeid, &s) == XK_FAILURE ) {
	passiveid = h.dport;
	if ( mapResolve(pstate->passivemap, &passiveid, &e) == XK_FAILURE ) {
	    xTrace0(udpp, TR_MAJOR_EVENTS, "udpDemux dropping the message");
	    return XK_SUCCESS;
	}
	xTrace1(udpp, TR_MAJOR_EVENTS, "Found an open enable for prot %d",
		e->hlpRcv);
	s = udpCreateSessn(self, e->hlpRcv, e->hlpType, &activeid);
	if (s == ERR_XOBJ) {
	    xTrace0(udpp, TR_ERRORS, "udpDemux could not create session");
	    return XK_SUCCESS;
	}
	xDuplicate(lls);
	udpDuplicatePort(activeid.localport);
#ifndef UDP_USE_GC
	xOpenDone(e->hlpRcv, s, self);
#endif  ! UDP_USE_GC
    } else {
	xTrace1(udpp, TR_EVENTS, "Popping to existing session %x", s);
    }
#ifdef UDP_USE_GC
    /* 
     * Since UDP sessions don't go away when the external ref count is
     * zero, we need to check for openEnables when rcnt == 0.
     */
    if ( s->rcnt == 0 ) {
	passiveid = h.dport;
	if ( mapResolve(pstate->passivemap, &passiveid, &e) == XK_FAILURE ) {
	    xTrace0(udpp, TR_MAJOR_EVENTS, "udpDemux dropping the message");
	    return XK_SUCCESS;
	}
	xOpenDone(e->hlpRcv, s, self);
    }
#endif  UDP_USE_GC
    xAssert(xIsSession(s));
    return xPop(s, lls, dg, 0);
}


/*
 * udpPop
 */
/*ARGSUSED*/
static xkern_return_t
udpPop(s, ds, dg)
    XObj s;
    XObj ds;
    Msg *dg;
{
  xTrace1(udpp, TR_EVENTS, "UDP pop, length = %d", msgLen(dg));
  xAssert(xIsSession(s));
  return xDemux(s, dg);
}


static xkern_return_t
udpOpenDisable(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PassiveId	key;
    PSTATE   	*pstate;
    Enable	*e;
    
    xTrace0(udpp, TR_MAJOR_EVENTS, "UDP open disable");
    pstate = (PSTATE *)self->state;
    if ( getPorts(p, &key, 0, "udpOpenDisable") ) {
	return XK_FAILURE;
    }
    xTrace1(udpp, TR_MAJOR_EVENTS, "port %d", key);
    if ( mapResolve(pstate->passivemap, &key, &e) == XK_FAILURE ||
	e->hlpRcv == hlpRcv && e->hlpType == hlpType ) {
	if (--(e->rcnt) == 0) {
	    mapRemoveBinding(pstate->passivemap, e->binding);
	    xFree((char *)e);
	    udpReleasePort(key);
	}
	return XK_SUCCESS;
    } else {
	return XK_FAILURE;
    }
}


static void
callUnlockPort( key, e )
    VOID	*key;
    Enable	*e;
{
    xTrace1(udpp, TR_FUNCTIONAL_TRACE,
	    "UDP callUnlockPort called with key %d", (int)*(PassiveId *)key);
    udpReleasePort(*(PassiveId *)key);
}


static xkern_return_t
udpOpenDisableAll( self, hlpRcv )
    XObj	self, hlpRcv;
{
    xTrace0(udpp, TR_MAJOR_EVENTS, "udpOpenDisableAll");
    return defaultOpenDisableAll(((PSTATE *)self->state)->passivemap,
				 hlpRcv, callUnlockPort);
}


static void
getproc_protl(s)
    XObj s;
{
  xAssert(xIsProtocol(s));
  s->close = udpCloseProtl;
  s->control = udpControlProtl;
  s->open = udpOpen;
  s->openenable = udpOpenEnable;
  s->opendisable = udpOpenDisable;
  s->demux = udpDemux;
  s->opendisableall = udpOpenDisableAll;
}


static void
getproc_sessn(s)
    XObj s;
{
  xAssert(xIsSession(s));
  s->push = udpPush;
  s->pop = udpPop;
  s->control = udpControlSessn;
  s->close = udpCloseSessn;
}
