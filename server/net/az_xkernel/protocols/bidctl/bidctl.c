/* 
 * bidctl.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.22 $
 * $Date: 1993/02/01 22:36:21 $
 */

/* 
 * BIDCTL -- BootId Control Protocol
 */

#include "xkernel.h"
#include "bidctl.h"
#include "bidctl_i.h"
#include "vnet.h"


#ifdef __STDC__

static long		bidctlHdrLoad( VOID *, char *, long, VOID * );
static int		bidctlControl( XObj, int, char *, int );
static xkern_return_t	bidctlDemux( XObj, XObj, Msg * );
static xkern_return_t	bidctlOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t	bidctlOpenEnable( XObj, XObj, XObj, Part * );
#  ifdef XK_DEBUG
static char *		flagStr( int );
#  endif
static int		getHostFromPart( Part *, IPhost * );
static BidctlState *	getState( XObj, IPhost *, int );
static void		getProtlFuncs( XObj );
#  ifdef ALLOW_TIMEOUT_IN_GETSTATE_BLOCKING
static void		openGiveUp( Event, VOID * );
#  endif
static void		processRomFile( void );
static void		verifyPeerBid( BidctlState *bs, BootId );

#else

static long		bidctlHdrLoad();
static xkern_return_t	bidctlDemux();
static xkern_return_t	bidctlOpenDisable();
static xkern_return_t	bidctlOpenEnable();
static char *		flagStr();
static int		getHostFromPart();
static BidctlState *	getState();
static void		getProtlFuncs();
static void		processRomFile();
static void		verifyPeerBid();

#endif __STDC__


static int	willBcastBoot = 
#ifdef BIDCTL_NO_BOOT_BCAST
  				0
#else
				1
#endif
;


#define MIN(x,y) 		((x)>(y) ? (y) : (x))


void
bidctl_init( self )
    XObj	self;
{
    Part	p;
    PState	*ps;
    XObj	llp;

    xTrace0(bidctlp, TR_GROSS_EVENTS, "bidctl Init");
    processRomFile();
    ps = X_NEW(PState);
    self->state = (VOID *)ps;
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xTrace0(bidctlp, TR_ERRORS,
		"bidctl could not get lower protocol -- aborting init");
	return;
    }
    ps->bsMap = mapCreate(BIDCTL_BSMAP_SIZE, sizeof(IPhost));
    ps->myBid = bidctlNewId();
    xTrace1(bidctlp, TR_GROSS_EVENTS, "bidctl using id == %x", ps->myBid);
    getProtlFuncs(self);
    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, 0), &p) == XK_FAILURE ) {
	xTrace0(bidctlp, TR_ERRORS, "openEnable failed in bidctl init");
    }
    if ( willBcastBoot ) {
	bidctlBcastBoot(self);
    }
    evDetach(evSchedule(bidctlTimer, (VOID *)ps, BIDCTL_TIMER_INTERVAL));
}


void
bidctlSemWait( bs )
    BidctlState	*bs;
{
    xTrace1(bidctlp, TR_DETAILED,
	    "BIDCTL blocking thread on state for peer %s",
	    ipHostStr(&bs->peerHost));
    xAssert(bs->fsmState == BIDCTL_QUERY_S || bs->fsmState == BIDCTL_OPEN_S);
    bs->rcnt++;
    bs->waitSemCount++;
    semWait(&bs->waitSem);
    bs->rcnt--;
    xTrace1(bidctlp, TR_DETAILED,
	    "BIDCTL thread blocked on state for peer %s wakes up",
	    ipHostStr(&bs->peerHost));
}


void
bidctlReleaseWaiters( bs )
    BidctlState	*bs;
{
    xTrace1(bidctlp, TR_EVENTS, "bidctlRelease -- waiters: %d",
	    bs->waitSemCount);
    while ( bs->waitSemCount > 0 ) {
	semSignal(&bs->waitSem);
	bs->waitSemCount--;
    }
}


void
bidctlTimeout( ev, arg )
    Event	ev;
    VOID	*arg;
{
    BidctlState	*bs = (BidctlState *)arg;

    if ( evIsCancelled(ev) ) {
	return;
    }
    /* 
     * Detach this event
     */
    if ( bs->ev ) {
	evDetach(bs->ev);
	bs->ev = 0;
    }
    switch ( bs->fsmState ) {
      case BIDCTL_OPEN_S:
	bs->timeout = MIN( bs->timeout * BIDCTL_OPEN_TIMEOUT_MULT,
			   BIDCTL_OPEN_TIMEOUT_MAX );
	break;
	
      case BIDCTL_QUERY_S:
	bs->timeout = MIN( bs->timeout * BIDCTL_QUERY_TIMEOUT_MULT,
			   BIDCTL_QUERY_TIMEOUT_MAX );
	break;

      default:
	/* 
	 * This event should have been cancelled!
	 */
	xTrace1(bidctlp, TR_ERRORS,
		"openTimeout -- state == %s event no longer needed",
		bidctlFsmStr(bs->fsmState));
	return;
    }
    xTrace1(bidctlp, TR_EVENTS, "bidctlTimeout -- trying again (%d)",
	    bs->retries);
    xTrace1(bidctlp, TR_EVENTS, "bidctlTimeout -- next timeout in %d msec",
	    bs->timeout / 1000);
    bs->ev = evSchedule(bidctlTimeout, bs, bs->timeout);
    bidctlOutput(bs, BIDCTL_REPEAT_QUERY, 0, 0);
}


/*
 * load header from potentially unaligned msg buffer.
 * Result of checksum calculation will be in hdr->sum.
 */
static long
bidctlHdrLoad( hdr, src, len, arg )
    VOID	*hdr, *arg;
    char 	*src;
    long	len;
{
    xAssert( len == sizeof(BidctlHdr) );
    bcopy(src, hdr, len);
    xTrace1(bidctlp, TR_DETAILED, "original checksum: %x",
	    ((BidctlHdr *)hdr)->sum);
    ((BidctlHdr *)hdr)->sum = ~ ocsum((u_short *)hdr, len/2);
    ((BidctlHdr *)hdr)->flags = ntohs(((BidctlHdr *)hdr)->flags);
    return len;
}


void
bidctlHdrStore( hdr, dst, len, arg )
    VOID	*hdr, *arg;
    char 	*dst;
    long	len;
{
    BidctlHdr	*h = (BidctlHdr *)hdr; 

    xAssert( len == sizeof(BidctlHdr) );
    h->flags = htons(h->flags);
    h->sum = 0;
    h->sum = ~ocsum((u_short *)h, len/2); 
    xAssert( (~ocsum((u_short *)h, len/2) & 0xffff) == 0 );
    xTrace1(bidctlp, TR_DETAILED, "outgoing checksum: %x", h->sum);
    bcopy((char *)h, dst, len);
}


#ifdef ALLOW_TIMEOUT_IN_GETSTATE_BLOCKING

static void
openGiveUp( ev, arg )
    Event	ev;
    VOID	*arg;
{
    BidctlState	*bs = (BidctlState *)arg;
    
    semSignal(&bs->waitSem);
}

#endif


static BidctlState *
getState( self, peer, create )
    XObj	self;
    IPhost	*peer;
    int		create;
{
    PState	*ps = (PState *)self->state;
    BidctlState	*bs;
    XObj	lls;
    Part	part;

    /* 
     * Find the appropriate bidctl state
     */
    xTrace1(bidctlp, TR_MORE_EVENTS,
	    "getBidState -- finding state for host %s", ipHostStr(peer));
    if ( mapResolve(ps->bsMap, (char *)peer, &bs) == XK_FAILURE ) {
	if ( ! create ) {
	    xTrace0(bidctlp, TR_MORE_EVENTS,
		    "getBidState -- declining to create new state");
	    return 0;
	}
	xTrace0(bidctlp, TR_MORE_EVENTS, "getBidState -- creating new state");
	partInit(&part, 1);
	partPush(part, peer, sizeof(peer));
	lls = xOpen(self, self, xGetDown(self, 0), &part);
	if ( lls == ERR_XOBJ ) {
	    xTrace0(bidctlp, TR_ERRORS,
		    "getBidState -- could not open lls");
	    return 0;
	}
	bs = X_NEW(BidctlState);
	bzero((char *)bs, sizeof(BidctlState));
	bs->myProtl = self;
	bs->bind = mapBind(ps->bsMap, peer, (int)bs);
	xAssert( bs->bind != ERR_BIND );
	bs->peerHost = *peer;
	bs->peerBid = 0;
	bs->fsmState = BIDCTL_INIT_S;
	bs->lls = lls;
	bs->hlpMap = mapCreate(BIDCTL_HLPMAP_SIZE, sizeof(XObj));
	semInit(&bs->waitSem, 0);
	/* 
	 * Send an initial query for the remote boot id -- it actually gets
	 * started in the transition out of this state (i.e., not here)
	 */
	xTrace0(bidctlp, TR_MORE_EVENTS, "getBidState -- starting query");
	bs->timeout = BIDCTL_OPEN_TIMEOUT;
	bs->ev = evSchedule(bidctlTimeout, bs, bs->timeout);
    } else {
	xTrace0(bidctlp, TR_MORE_EVENTS,
		"getBidState -- state already exists");
    }
    xTrace2(bidctlp, TR_MORE_EVENTS, "getBidState returns %x (rcnt %d)",
	    bs, bs->rcnt);
    return bs;
}


xkern_return_t
bidctlDestroy( bs )
    BidctlState	*bs;
{
    PState	*ps = (PState *)bs->myProtl->state;

    xTrace0(bidctlp, TR_DETAILED, "bidctlDestroy");
    if ( bs->ev ) {
	evCancel(bs->ev);
    }
    xAssert( bs->rcnt == 0 );
    if ( mapRemoveBinding(ps->bsMap, bs->bind) ) {
	xTrace0(bidctlp, TR_ERRORS, "bidctlDestroy -- mapUnbind fails!");
	return XK_FAILURE;
    }
    xAssert(xIsXObj(bs->lls));
    xClose(bs->lls);
    mapClose(bs->hlpMap);
    xFree((char *)bs);
    return XK_SUCCESS;
}


static xkern_return_t
bidctlDemux( self, lls, m )
    XObj	self, lls;
    Msg		*m;
{
    PState	*ps = (PState *)self->state;
    BidctlHdr	hdr;
    BidctlState	*bs;
    IPhost	peer;

    xTrace0(bidctlp, TR_EVENTS, "bidctl demux");
    if ( ! msgPop(m, bidctlHdrLoad, (VOID *)&hdr, sizeof(hdr), 0) ) {
	xTrace0(bidctlp, TR_ERRORS, "bidctl demux -- msg pop failed");
	return XK_SUCCESS;
    }
    if ( xControl(lls, GETPEERHOST, (char *)&peer, sizeof(IPhost)) < 0 ) {
	return XK_FAILURE;
    }
    xIfTrace(bidctlp, TR_DETAILED) {
	bidctlHdrDump(&hdr, "INCOMING", &peer);
    }
    if ( hdr.sum ) {
	xTrace1(bidctlp, TR_ERRORS, "bidctl demux -- checksum (%x) non-zero",
		hdr.sum);
	return XK_FAILURE;
    }
    xIfTrace(bidctlp, TR_MAJOR_EVENTS) {
	if ( hdr.flags & BIDCTL_BCAST_F ) {
	    xTrace1(bidctlp, TR_ALWAYS,
		    "boot broadcast received from peer %s", ipHostStr(&peer));
	}
    }
    if ( mapResolve(ps->bsMap, (char *)&peer, &bs) == XK_FAILURE ) {
	/* 
	 * Active session does not exist.  
	 */
	bs = getState(self, &peer, ! (hdr.flags & BIDCTL_BCAST_F));
	if ( bs == 0 ) {
	    return XK_FAILURE;
	}
    }
    bs->timer = 0;
    bidctlTransition(bs, &hdr);
    return XK_SUCCESS;
}


static int
getHostFromPart( p, h )
    Part	*p;
    IPhost	*h;
{
    VOID	*ptr;

    if ( partLen(p) < 1 ) {
	return 1;
    }
    if ( (ptr = partPop(*p)) == 0 ) {
	return 1;
    }
    *h = *(IPhost *)ptr;
    return 0;
}


static xkern_return_t
bidctlOpenEnable( self, hlpRcv, hlpType, p )
    XObj   	self, hlpRcv, hlpType;
    Part	*p;
{
    Enable	*e;
    IPhost	peer;
    BidctlState	*bs;
    
    if ( getHostFromPart(p, &peer) ) {
	xTrace0(bidctlp, TR_SOFT_ERRORS, "bidctlOpenEnable participant error");
	return XK_FAILURE;
    }
    xTrace1(bidctlp, TR_EVENTS, "bidctl openEnable, peer %s",
	    ipHostStr(&peer));
    if ( xControl(xGetDown(self, 0), VNET_ISMYADDR, (char *)&peer,
		  sizeof(IPhost)) > 0 ) {
	xTrace0(bidctlp, TR_EVENTS, "bidctlOpenEnable -- local host");
	return XK_SUCCESS;
    }
    if ( (bs = getState(self, &peer, 1)) == 0 ) {
	return XK_FAILURE;
    }
    bidctlTransition(bs, 0);
    if ( mapResolve(bs->hlpMap, &hlpRcv, &e) == XK_FAILURE ) {
	e = X_NEW(Enable);
	e->rcnt = 1;
	e->hlpRcv = hlpRcv;
	e->binding = mapBind(bs->hlpMap, &hlpRcv, (int)e);
	xAssert(e->binding != ERR_BIND);
    } else {
	e->rcnt++;
    }
    bs->rcnt++;
    xTrace2(bidctlp, TR_MORE_EVENTS,
	    "bidctlOpenEnable -- enable rcnt == %d, state rcnt == %d",
	    e->rcnt, bs->rcnt);
    return XK_SUCCESS;
}


static xkern_return_t
bidctlOpenDisable( self, hlpRcv, hlpType, p )
    XObj   	self, hlpRcv, hlpType;
    Part	*p;
{
    IPhost	peer;
    Enable	*e;
    BidctlState	*bs;
    
    if ( getHostFromPart(p, &peer) ) {
	xTrace0(bidctlp, TR_SOFT_ERRORS,
		"bidctlOpenDisable participant error");
	return XK_FAILURE;
    }
    xTrace1(bidctlp, TR_EVENTS, "bidctl openDisable, peer %s",
	    ipHostStr(&peer));
    if ( xControl(xGetDown(self, 0), VNET_ISMYADDR, (char *)&peer,
		  sizeof(IPhost)) > 0 ) {
	xTrace0(bidctlp, TR_EVENTS, "bidctlOpenDisable -- local host");
	return XK_SUCCESS;
    }
    if ( (bs = getState(self, &peer, 0)) == 0 ) {
	return XK_FAILURE;
    }
    if ( mapResolve(bs->hlpMap, &hlpRcv, &e) == XK_FAILURE ) {
	xTrace0(bidctlp, TR_SOFT_ERRORS,
		"bidOpenDisable -- no enable object found");
	return XK_FAILURE;
    }
    bs->rcnt--;
    e->rcnt--;
    xTrace2(bidctlp, TR_MORE_EVENTS, 
	    "bidctlOpenDisable -- enable rcnt == %d, state rcnt == %d",
	    e->rcnt, bs->rcnt);
    if ( e->rcnt == 0 ) {
	mapRemoveBinding(bs->hlpMap, e->binding);
    }
    return XK_SUCCESS;
}


static int
bidctlControl( self, op, buf, len )
    XObj	self;
    int		op, len;
    char	*buf;
{
    PState	*ps = (PState *)self->state;

    switch ( op ) {

      case BIDCTL_GET_LOCAL_BID:
	checkLen(len, sizeof(BootId));
	*(BootId *)buf = ps->myBid;
	return sizeof(BootId);
	
      case BIDCTL_GET_PEER_BID:
      case BIDCTL_GET_PEER_BID_BLOCKING:
	{
	    BidctlBootMsg	*m = (BidctlBootMsg *)buf;
	    BidctlState		*bs;

	    checkLen(len, sizeof(BidctlBootMsg));
	    xTrace2(bidctlp, TR_EVENTS, "BIDCTL_GET_PEER_BID%s peer %s",
		    (op == BIDCTL_GET_PEER_BID) ? "" : "_BLOCKING",
		    ipHostStr(&m->h));
	    if ( xControl(xGetDown(self, 0), VNET_ISMYADDR, (char *)&m->h,
			  sizeof(IPhost)) > 0 ) {
		m->id = ps->myBid;
		return sizeof(BidctlBootMsg);
	    }
	    if ( (bs = getState(self, &m->h, 1)) == 0 ) {
		return 0;
	    }
	    /* 
	     * Transition to take us from INIT to OPEN state if necessary
	     */
	    bidctlTransition(bs, 0);
	    if ( op == BIDCTL_GET_PEER_BID && m->id != 0 ) {
		/* 
		 * The caller has offered a suggested BootId
		 */
		if ( m->id != bs->peerBid &&
		     bs->fsmState == BIDCTL_NORMAL_S ) {
		    /* 
		     * The caller has reason to believe that the input
		     * bootid is correct.  We'll check it out.
		     */
		    bidctlStartQuery(bs, 1);
		}
	    } else {
		/* 
		 * BLOCKING operation
		 */
		if ( m->id == 0 ) {
		    /* 
		     * Caller will simply wait on any BID
		     */
		    if ( bs->peerBid == 0 ) {
			bidctlSemWait(bs);
		    }
		} else {
		    verifyPeerBid(bs, m->id);
		}
	    }
	    m->id = bs->peerBid;
	    return sizeof(BidctlBootMsg);
	}

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}


/* 
 * Verify -- caller wants verification that the
 * suggested BID is still reasonable or
 * confirmation (via peer handshake *after* the request)
 * that it is incorrect.  This condition holds when we return.
 */
static void
verifyPeerBid( bs, sugBid )
    BidctlState	*bs;
    BootId	sugBid;
{
    if ( sugBid == bs->peerBid ) {
	return;
    }
    xTrace2(bidctlp, TR_EVENTS,
	    "BIDCTL verify -- suggested bid %x != state bid %x",
	    sugBid, bs->peerBid);
    switch ( bs->fsmState ) {
      case BIDCTL_QUERY_S:
      case BIDCTL_OPEN_S:
	/* 
	 * We are already waiting on someone else's query. 
	 */
	bidctlSemWait(bs);
	if ( bs->fsmState == BIDCTL_QUERY_S ) {
	    /* 
	     * We're now waiting on (another) someone else's query, but
	     * we know that it took place after the request.
	     */
	    bidctlSemWait(bs);
	    break;
	} 
	xAssert( bs->fsmState == BIDCTL_NORMAL_S );
	if ( sugBid == bs->peerBid ) {
	    break;
	}
	/* 
	 * Fallthrough -- send our own request
	 */
	
      case BIDCTL_NORMAL_S:
	/* 
	 * The caller has reason to believe that
	 * the input bootid is correct.  We'll check
	 * it out.
	 */
	xTrace0(bidctlp, TR_EVENTS, "starting user-initiated query sequence");
	xTrace2(bidctlp, TR_EVENTS, "\t(cur == %x, supplied == %x)",
		bs->peerBid, sugBid);
	bidctlStartQuery(bs, 1);
	bidctlSemWait(bs);
	break;
	
      default:
	xAssert(0);
    }
}


#ifdef XK_DEBUG

static char *
flagStr( f )
    int	f;
{
    static char	s[80];

    if ( f == 0 ) {
	return "NONE ";
    }
    s[0] = 0;
    if ( f & BIDCTL_QUERY_F ) {
	strcat(s, "QUERY ");
    }
    if ( f & BIDCTL_RESPONSE_F ) {
	strcat(s, "RESPONSE ");
    }
    if ( f & BIDCTL_BCAST_F ) {
	strcat(s, "BCAST ");
    }
    if ( s[0] == 0 ) {
	return "UNRECOGNIZED";
    }
    return s;
}

#endif XK_DEBUG


void
bidctlHdrDump( h, str, peer )
    BidctlHdr	*h;
    char	*str;
    IPhost	*peer;
{
    xTrace2(bidctlp, TR_ALWAYS, "Bidctl %s header (peer %s):",
	    str, ipHostStr(peer));
    xTrace4(bidctlp, TR_ALWAYS,
	   "flags: %s  sum: %.4x  srcBid: %.8x  dstBid: %.8x",
	    flagStr(h->flags), h->sum, h->srcBid, h->dstBid);
    xTrace2(bidctlp, TR_ALWAYS, "reqTag: %.8x  rplTag: %.8x",
	    h->reqTag, h->rplTag);
}


static void
getProtlFuncs( p )
    XObj	p;
{
    p->demux = bidctlDemux;
    p->openenable = bidctlOpenEnable;
    p->opendisable = bidctlOpenDisable;
    p->control = bidctlControl;
}

void
bidctlDispState( bs )
    BidctlState	*bs;
{
    xTrace1(bidctlp, TR_ALWAYS, "bidctl state for peer %s",
	    ipHostStr(&bs->peerHost));
    xTrace3(bidctlp, TR_ALWAYS, "%s, rcnt %d, peerbid %x",
	    bidctlFsmStr(bs->fsmState), bs->rcnt, 
	    bs->peerBid);
    xTrace2(bidctlp, TR_ALWAYS, "timer %d, retries %d",
	    bs->timer, bs->retries);
}



#define ERROR { sprintf(errBuf,	 "BIDCTL ROM file format error in line %d", \
			i + 1); \
		xError(errBuf); }

static void
processRomFile()
{
    int	i;

    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], "bidctl") ) {
	    if ( ! rom[i][1] ) {
		ERROR;
		continue;
	    }
	    if ( ! strncmp(rom[i][1], "bcast") ) {
		xTrace0(bidctlp, TR_EVENTS, "rom file broadcast specified");
		willBcastBoot = 1;
		continue;
	    }
	    if ( ! strncmp(rom[i][1], "nobcast") ) {
		xTrace0(bidctlp, TR_EVENTS, "rom file no broadcast specified");
		willBcastBoot = 0;
		continue;
	    }
	    ERROR;
	}
    }
}
