/*     
 * $RCSfile: bid.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.17 $
 * $Date: 1993/02/01 22:37:44 $
 */

/*
 * Protocol initialization, Session creation/destruction,
 * header load/store routines, control (registration/deregistration)
 */

#include "xkernel.h"
#include "bidctl.h"
#include "bid_i.h"
#include "bid.h"

#ifdef __STDC__

static void		activateSessn( XObj );
static int		bidControlProtl( XObj, int, char *, int );
static int		bidControlSessn( XObj, int, char *, int );
static XObj		bidCreateSessn( XObj, XObj, XObj, ActiveKey * );
static xkern_return_t	bidDemux( XObj, XObj, Msg * );
static void		bidHdrDump( BidHdr *, char * );
static long		bidHdrLoad( VOID *, char *, long, VOID * );
static void		bidHdrStore( VOID *, char *, long, VOID * );
static XObj		bidOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	bidOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t	bidOpenEnable( XObj, XObj, XObj, Part * );
static xkern_return_t	bidPop( XObj, XObj, Msg *, VOID * );
static xmsg_handle_t	bidPush( XObj, Msg * );
static int		bootidChanged( VOID *, VOID *, VOID * );
static void		getProtlFuncs( XObj );
static void		getSessnFuncs( XObj );

#else

static void		activateSessn();
static int		bidControlProtl();
static int		bidControlSessn();
static XObj		bidCreateSessn();
static xkern_return_t	bidDemux();
static void		bidHdrDump();
static long		bidHdrLoad();
static void		bidHdrStore();
static XObj		bidOpen();
static xkern_return_t	bidOpenDisable();
static xkern_return_t	bidOpenEnable();
static xkern_return_t	bidPop();
static xmsg_handle_t	bidPush();
static int		bootidChanged();
static void		getProtlFuncs();
static void		getSessnFuncs();

#endif __STDC__


int	tracebidp;


void
bid_init( self )
    XObj	self;
{
    Part	p;
    PState	*ps = (PState *)self->state;

    xTrace0(bidp, TR_GROSS_EVENTS, "bid Init");
    ps = X_NEW(PState);
    self->state = (VOID *)ps;
    if ( ! xIsProtocol(xGetDown(self, BID_XPORT_I)) ) {
	xTrace0(bidp, TR_ERRORS,
		"bid could not get transport protocol -- aborting init");
	return;
    }
    if ( ! xIsProtocol(xGetDown(self, BID_CTL_I)) ) {
	xTrace0(bidp, TR_ERRORS,
		"bid could not get control protocol -- aborting init");
	return;
    }
    if ( xControl(xGetDown(self, BID_CTL_I), BIDCTL_GET_LOCAL_BID,
		  (char *)&ps->myBid, sizeof(BootId)) < (int)sizeof(BootId) ) {
	xTrace0(bidp, TR_ERRORS, "bid could not get my bid -- aborting init");
	return;
    }
    xTrace1(bidp, TR_GROSS_EVENTS, "bid using id == %x", ps->myBid);
    ps->activeMap = mapCreate(BID_ACTIVE_MAP_SIZE, sizeof(ActiveKey));
    ps->passiveMap = mapCreate(BID_PASSIVE_MAP_SIZE, sizeof(PassiveKey));
    getProtlFuncs(self);
    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, BID_XPORT_I), &p)
							== XK_FAILURE ) {
	xTrace0(bidp, TR_ERRORS, "openEnable failed in bid init");
    }
}


static XObj
bidOpen( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part	*p;
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    ActiveKey	key;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bidOpen");
    if ( (key.lls = xOpen(self, self, xGetDown(self, 0), p)) == ERR_XOBJ ) {
	xTrace0(bidp, TR_MAJOR_EVENTS, "bid could not open lls");
	return ERR_XOBJ;
    }
    if ( (key.hlpNum = relProtNum(hlpType, self)) == -1 ) {
	xTrace0(bidp, TR_ERRORS, "bid open couldn't get hlpNum");
	xClose(key.lls);
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &key, &s) == XK_SUCCESS ) {
	xTrace0(bidp, TR_MAJOR_EVENTS, "bid open found existing sessn");
	xClose(key.lls);
	return s;
    }
    s = bidCreateSessn(self, hlpRcv, hlpType, &key);
    if ( s == ERR_XOBJ ) {
	xClose(key.lls);
	return ERR_XOBJ;
    }
    xTrace1(bidp, TR_DETAILED, "bidOpen returning %x", s);
    return s;
}


static long
bidHdrLoad( hdr, src, len, arg )
    VOID	*hdr, *arg;
    char 	*src;
    long	len;
{
    xAssert( len == sizeof(BidHdr) );
    bcopy(src, hdr, len);
    ((BidHdr *)hdr)->hlpNum = ntohl(((BidHdr *)hdr)->hlpNum);
    return len;
}


static void
bidHdrStore( hdr, dst, len, arg )
    VOID	*hdr, *arg;
    char 	*dst;
    long	len;
{
    BidHdr	h;

    xAssert( len == sizeof(BidHdr) );
    h.hlpNum = htonl(((BidHdr *)hdr)->hlpNum);
    h.srcBid = ((BidHdr *)hdr)->srcBid;
    h.dstBid = ((BidHdr *)hdr)->dstBid;
    bcopy((char *)&h, dst, len);
}


static XObj
bidCreateSessn( self, hlpRcv, hlpType, key )
    XObj	self, hlpRcv, hlpType;
    ActiveKey	*key;
{
    XObj	s, llpCtl;
    PState	*ps = (PState *)self->state;
    SState	*ss;
    Part	part;
    IPhost	peer;
    
    if ( xControl(key->lls, GETPEERHOST, (char *)&peer, sizeof(IPhost))
		< (int)sizeof(IPhost) ) {
	return ERR_XOBJ;
    }
    s = xCreateSessn(getSessnFuncs, hlpRcv, hlpType, self, 1, &key->lls);
    if ( s == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    llpCtl = xGetDown(self, BID_CTL_I);
    partInit(&part, 1);
    partPush(part, &peer, sizeof(peer));
    ss = X_NEW(SState);
    ss->hdr.hlpNum = key->hlpNum;
    ss->hdr.srcBid = ps->myBid;
    ss->peer = peer;
    s->state = (VOID *)ss;
    /* 
     * Register this protocol's interest in this host with the control protocol
     */
    if ( xOpenEnable(self, self, llpCtl, &part) == XK_SUCCESS ) {
	BidctlBootMsg	msg;
	
	msg.h = peer;
	msg.id = 0;
	if ( xControl(llpCtl, BIDCTL_GET_PEER_BID, (char *)&msg,
		      sizeof(msg)) == (int)sizeof(msg) ) {
	    ss->hdr.dstBid = msg.id;
	    s->binding = mapBind(ps->activeMap, key, (int)s);
	    if ( msg.id != 0 ) {
		activateSessn(s);
	    }
	    xAssert( s->binding != ERR_BIND );
	    return s;
	} else {
	    xTrace1(bidp, TR_ERRORS,
		    "bid CreateSessn: couldn't get peer BID for %s",
		    ipHostStr(&peer));
	}
	partInit(&part, 1);
	partPush(part, &peer, sizeof(peer));
	xOpenDisable(self, self, llpCtl, &part);
    } else {
	xTrace0(bidp, TR_ERRORS, "bidCreateSessn couldn't openEnable BIDCTL");
    }
    xFree((char *)ss);
    xDestroy(s);
    return ERR_XOBJ;
}


static xkern_return_t
bidClose( self )
    XObj	self;
{
    XObj	myProtl = xMyProtl(self);
    PState	*ps = (PState *)myProtl->state;
    SState	*ss = (SState *)self->state;
    XObj	lls;
    Part	part;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bid Close");
    lls = xGetDown(self, 0);

    if ( mapRemoveBinding(ps->activeMap, self->binding) == XK_FAILURE ) {
	xAssert(0);
	return XK_FAILURE;
    }
    partInit(&part, 1);
    partPush(part, &ss->peer, sizeof(ss->peer));
    if ( xOpenDisable(myProtl, myProtl, xGetDown(myProtl, BID_CTL_I), &part)
				== XK_FAILURE ) {
	xTrace0(bidp, TR_ERRORS, "bidClose couldn't openDisable cntrl protl");
    }
    xAssert(xIsSession(lls));
    xClose(lls);
    xDestroy(self);
    return XK_SUCCESS;
}


static xkern_return_t
bidDemux( self, lls, m )
    XObj	self, lls;
    Msg		*m;
{
    PState	*ps = (PState *)self->state;
    BidHdr	hdr;
    XObj	s;
    Enable	*e;
    ActiveKey	key;

    xTrace0(bidp, TR_EVENTS, "bid demux");
    if ( ! msgPop(m, bidHdrLoad, (VOID *)&hdr, sizeof(hdr), 0) ) {
	xTrace0(bidp, TR_ERRORS, "bid demux -- msg pop failed");
	return XK_SUCCESS;
    }
    xIfTrace(bidp, TR_DETAILED) {
	bidHdrDump(&hdr, "INCOMING");
    }
    key.lls = lls;
    key.hlpNum = hdr.hlpNum;
    if ( mapResolve(ps->activeMap, (char *)&key, &s) == XK_FAILURE ) {
	if ( mapResolve(ps->passiveMap, &hdr.hlpNum, &e) == XK_FAILURE ) {
	    xTrace1(bidp, TR_SOFT_ERRORS,
		    "bid demux -- no protl for hlpNum %d ", hdr.hlpNum);
	    return XK_FAILURE;
	} else {
	    xTrace1(bidp, TR_DETAILED,
		    "bid demux -- found enable for hlpNum %d ", hdr.hlpNum);
	}
	s = bidCreateSessn(self, e->hlpRcv, e->hlpType, &key);
	if ( s == ERR_XOBJ ) {
	    return XK_FAILURE;
	}
	xDuplicate(lls);
	xOpenDone(e->hlpRcv, s, self);
    } else {
	xAssert(s->rcnt > 0);
    }
    return xPop(s, lls, m, &hdr);
}


static xkern_return_t
bidOpenEnable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part	*p;
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    hlpNum = relProtNum(hlpType, self);
    xTrace1(bidp, TR_MAJOR_EVENTS, "bid openEnable binding key %d",
	    hlpNum);
    return defaultOpenEnable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}


static xkern_return_t
bidOpenDisable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part	*p;
{
    PState	*ps = (PState *)self->state;
    long	hlpNum;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bid openDisable");
    hlpNum = relProtNum(hlpType, self);
    return defaultOpenDisable(ps->passiveMap, hlpRcv, hlpType, &hlpNum);
}


static xkern_return_t
bidOpenDisableAll( self, hlpRcv )
    XObj	self, hlpRcv;
{
    PState	*ps = (PState *)self->state;

    xTrace0(bidp, TR_MAJOR_EVENTS, "bid openDisableAll");
    return defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
}

static xkern_return_t
bidPop( self, lls, m, inHdr )
    XObj	self, lls;
    Msg		*m;
    VOID	*inHdr;
{
    SState	*ss = (SState *)self->state;
    BidHdr	*hdr = (BidHdr *)inHdr;
    
    xAssert(hdr);
    if ( hdr->dstBid != ss->hdr.srcBid ) {
	Msg	nullMsg;

	xTrace2(bidp, TR_MAJOR_EVENTS,
		"bidPop: mismatch of my bid (%x (hdr) vs. %x (state))",
		hdr->dstBid, ss->hdr.srcBid);
	/* 
	 * Send a null message to my peer so it can ask it's control
	 * protocol to straighten it out.
	 */
	msgConstructEmpty(&nullMsg);
	bidPush(self, &nullMsg);
	msgDestroy(&nullMsg);
	return XK_FAILURE;
    }
    if ( hdr->srcBid != ss->hdr.dstBid ) {
	XObj		llpCtl = xGetDown(xMyProtl(self), BID_CTL_I);
	BidctlBootMsg	msg;

	xTrace2(bidp, TR_MAJOR_EVENTS,
		"bidPop: mismatch of peer bid (%x (hdr) vs. %x (state))",
		hdr->srcBid, ss->hdr.srcBid);
	msg.h = ss->peer;
	msg.id = hdr->srcBid;
	xControl(llpCtl, BIDCTL_GET_PEER_BID, (char *)&msg, sizeof(msg));
	return XK_FAILURE;
    }
    return xDemux(self, m);
}


static int
bidControlProtl( self, op, buf, len )
    XObj	self;
    int		op, len;
    char 	*buf;
{
    PState	*ps = (PState *)self->state;

    switch ( op ) {

      case BIDCTL_FIRST_CONTACT:
      case BIDCTL_PEER_REBOOTED:
	mapForEach(ps->activeMap, bootidChanged, (BidctlBootMsg *)buf);
	return 0;

      case GETMAXPACKET:
      case GETOPTPACKET:
	if ( xControl(xGetDown(self, 0), op, buf, len) < (int)sizeof(int) ) {
	    return -1;
	}
	*(int *)buf -= sizeof(BidHdr);
	return (sizeof(int));

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}


static int
bootidChanged( key, val, arg )
    VOID	*key, *val, *arg;
{
    XObj		s = (XObj)val;
    BidctlBootMsg	*m = (BidctlBootMsg *)arg;
    SState		*ss;

    xAssert(xIsSession(s));
    xAssert(arg);
    ss = (SState *)s->state;
    if ( ! IP_EQUAL(ss->peer, m->h) ) {
	xTrace3(bidp, TR_EVENTS,
		"bid sessn %x (host %s) ignoring reboot message for %s",
		s, ipHostStr(&ss->peer), ipHostStr(&m->h));
	return 1;
    }
    activateSessn(s);
    xTrace3(bidp, TR_MAJOR_EVENTS, "bid session %x resetting BID of %s to %x",
	    s, ipHostStr(&m->h), m->id);
    ss->hdr.dstBid = m->id;
    return MFE_CONTINUE;
}


static int
bidControlSessn( self, op, buf, len )
    XObj	self;
    int		op, len;
    char 	*buf;
{
    switch ( op ) {

      case GETMAXPACKET:
      case GETOPTPACKET:
	if ( xControl(xGetDown(self, 0), op, buf, len) < (int)sizeof(int) ) {
	    return -1;
	}
	*(int *)buf -= sizeof(BidHdr);
	return (sizeof(int));

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}


static xmsg_handle_t
bidPush( self, m )
    XObj	self;
    Msg		*m;
{
    SState	*ss = (SState *)self->state;

    xTrace0(bidp, TR_EVENTS, "bidPush");
    xIfTrace(bidp, TR_DETAILED) {
	bidHdrDump(&ss->hdr, "OUTGOING");
    }
    msgPush(m, bidHdrStore, &ss->hdr, sizeof(BidHdr), 0);
    return xPush(xGetDown(self, 0), m);
}


static void
bidHdrDump( h, str )
    BidHdr	*h;
    char	*str;
{
    xTrace1(bidp, TR_ALWAYS, "Bid %s header:", str);
    xTrace3(bidp, TR_ALWAYS,
	    "srcBid: %.8x  dstBid: %.8x  hlpNum: %d",
	    h->srcBid, h->dstBid, h->hlpNum);
}


static void
getProtlFuncs( p )
    XObj	p;
{
    p->open = bidOpen;
    p->control = bidControlProtl;
    p->demux = bidDemux;
    p->openenable = bidOpenEnable;
    p->opendisable = bidOpenDisable;
    p->opendisableall = bidOpenDisableAll;
}


static void
activateSessn( s )
    XObj	s;
{
    s->push = bidPush;
    s->pop = bidPop;
}


static void
getSessnFuncs( s )
    XObj	s;
{
    s->control = bidControlSessn;
    s->close = bidClose;
}


