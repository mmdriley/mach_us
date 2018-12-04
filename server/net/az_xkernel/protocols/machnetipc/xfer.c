/* 
 * xfer.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.13 $
 * $Date: 1993/02/01 22:35:12 $
 */

/* 
 * Mach port transfer protocols
 */

/* 
 * see xfer.h for the interface description
 */


#include "xkernel.h"
#include "machripc_internal.h"
#include "machripc_xfer.h"
#include "xfer.h"
#include "bidctl.h"


typedef struct {
    XferRemFunc	f;
    IPhost	*h;
} CallBackInfo;


#ifdef __STDC__

static int		abortLock( VOID *, VOID *, VOID * );
static int		abortPort( VOID *, VOID *, VOID * );
static int		abortTransfer( VOID *, VOID *, VOID * );
static int		findEntry( VOID *, VOID *, VOID * );
static int		mapIsEmpty( Map );
static xkern_return_t	regBidInterest( XObj, IPhost * );
static void		releaseMapOnReboot( XObj, Map, MapForEachFun,
					    IPhost *, VOID *, char * );
static xkern_return_t	unregBidInterest( XObj, IPhost * );

#else

static int		abortLock();
static int		abortPort();
static int		abortTransfer();
static int		findEntry();
static int		mapIsEmpty();
static xkern_return_t	regBidInterest();
static void		releaseMapOnReboot();
static xkern_return_t	unregBidInterest();

#endif __STDC__


/* 
 * XFER maps -- internal structure:
 *
 * transferMap -- { IPhost -> { msgId -> { (npd *) -> reference count} } }.
 * Contains network ports for rights being transferred to this host.
 *
 * lockedMap -- { IPhost -> { (npd *) -> (VOID *) } }.
 * Contains ports which are locked by remote hosts.  The final bound
 * object (the VOID *) is supplied in the bind and can be returned in
 * a callback when unbound.
 */

/* 
 * Size for the internal maps in the 'transfer' map chain, keyed by
 * msgId.  These maps are closed when the message arrives, so the size
 * need only reflect ports whose messages have yet to arrive.
 */
#define XFER_MSG_MAP_SZ	5

/* 
 * Size for the interior maps in the 'transfer' map chain, 
 * keyed on descriptors for individual ports being transferred
 * in a single message 
 */
#define XFER_XFER_PD_MAP_SZ  1

/* 
 * Size for the interior maps in the 'locked' map chain, 
 * keyed on descriptors for ports locked by a single host.
 */
#define XFER_LOCKED_PD_MAP_SZ  5

/* 
 * Size for the map keyed by hosts currently locking ports.
 */
#define XFER_LOCKED_MAP_SZ	13

/* 
 * Size for the transfer map keyed by hosts currently transferring
 * port rights. 
 */
#define XFER_TRANSFER_MAP_SZ	13


int	tracexferp;


/* 
 * description in xfer.h
 */
XObj
xferOpen( self, dst, bidPtr )
    XObj	self;
    IPhost	*dst;
    BootId	*bidPtr;
{
    Part		p;
    XObj		lls, bidctl, xport;
    BidctlBootMsg	bm;
    BootId		bidSave;

    bidctl = xGetDown(self, XFER_BIDCTL_I);
    xAssert(xIsProtocol(bidctl));
    xport = xGetDown(self, XFER_XPORT_I);
    xAssert(xIsProtocol(xport));
    partInit(&p, 1);
    partPush(p, dst, sizeof(IPhost));
    bm.h = *dst;
    bm.id = 0;
    /* 
     * Compare peer BID's before and after opening the lls to avoid
     * transferring rights immediately after the peer reboots.
     */
    /* 
     * This call won't block in real usage ...
     */
    if ( xControl(bidctl, BIDCTL_GET_PEER_BID_BLOCKING, (char *)&bm,
		  sizeof(bm)) < (int)sizeof(bm) ) {
	xTrace1(xferp, TR_ERRORS,
		"xferOpen couldn't get BID of right recipient %s",
		ipHostStr(&bm.h));
	return ERR_XOBJ;
    }
    bidSave = bm.id;
    bm.id = 0;
    if ( (lls = xOpen(self, self, xport, &p)) == ERR_XOBJ ) {
	xTrace1(xferp, TR_SOFT_ERRORS,
		"xferOpen could not open lls for host %s", ipHostStr(dst));
	return ERR_XOBJ;
    }
    if ( xControl(bidctl, BIDCTL_GET_PEER_BID,
		  (char *)&bm, sizeof(bm)) < (int)sizeof(bm) ) {
	xTrace1(xferp, TR_ERRORS,
		"xferOpen couldn't get BID of right recipient %s",
		ipHostStr(&bm.h));
	xClose(lls);
	return ERR_XOBJ;
    }
    if ( bm.id != bidSave ) {
	xTrace1(xferp, TR_ERRORS,
		"xferOpen -- right recipient %s rebooted before transfer",
		ipHostStr(&bm.h));
	xClose(lls);
	return ERR_XOBJ;
    }
    /* 
     * At this point we know that this channel is talking to the same
     * incarnation of the peer as was alive when the transfer was
     * requested.  If the peer reboots, sends on this channel will fail.
     */
    if ( bidPtr ) {
	*bidPtr = bidSave;
    }
    return lls;
}



/* 
 * description in xfer.h
 */
int
xferConfirmBid( self, xh )
    XObj	self;
    XferHost	*xh;
{
    BidctlBootMsg	bmsg;
    XObj		bidctl;

    bidctl = xGetDown(self, XFER_BIDCTL_I);
    if ( ! xIsProtocol(bidctl) ) {
	xTrace0(xferp, TR_ERRORS, "xferConfirmBid -- protl is misconfigured");
	return 0;
    }
    bmsg.h = xh->h;
    bmsg.id = xh->bid;
    if ( xControl(bidctl, BIDCTL_GET_PEER_BID_BLOCKING,
		  (char *)&bmsg, sizeof(bmsg)) < (int)sizeof(bmsg) ) {
	xTrace1(xferp, TR_ERRORS,
		"xferConfirm could not get BID of peer %s", ipHostStr(&xh->h));
	return 0;
    }
    xTrace3(xferp, TR_MORE_EVENTS,
	    "xferConfirm BID of host %s -- \n\tmsg == %x, cur == %x",
	    ipHostStr(&xh->h), xh->bid, bmsg.id);
    return xh->bid == bmsg.id;
}



/* 
 * description in xfer.h -- see map structure description above
 */
void
xferLockedMapAdd( self, lockedMap, peer, port, arg )
    XObj	self;
    Map		lockedMap;
    IPhost	*peer;
    mnetport	*port;
    VOID	*arg;
{
    Map		pdMap;
    Bind	bind;

    if ( mapResolve(lockedMap, peer, &pdMap) == XK_FAILURE ) {
	xTrace1(xferp, TR_MORE_EVENTS, "creating lock submap for peer %s",
		ipHostStr(peer));
	pdMap = mapCreate(XFER_LOCKED_PD_MAP_SZ, sizeof(mnetport *));
	bind = mapBind(lockedMap, peer, pdMap);
	xAssert(bind != ERR_BIND);
	regBidInterest(self, peer);
    }
    bind = mapBind(pdMap, &port, arg);
    xAssert(bind != ERR_BIND);
}



/* 
 * description in xfer.h -- see map structure description above
 */
void
xferLockedMapRemove( lockedMap, peer, port, callBack )
    Map			lockedMap;
    IPhost		*peer;
    mnetport		*port;
    LockRemFunc		callBack;
{    
    Map			pdMap;
    xkern_return_t	res;
    VOID		*arg;

    if ( mapResolve(lockedMap, peer, &pdMap) == XK_FAILURE ) {
	xTrace0(xferp, TR_ERRORS, "xfer unlock -- no port map!!");
    } else {
	if ( callBack ) {
	    if ( mapResolve(pdMap, &port, &arg) == XK_SUCCESS ) {
		callBack(port, arg);
	    }
	}
	if ( mapUnbind(pdMap, &port) == XK_FAILURE ) {
	    xTrace0(xferp, TR_ERRORS,
		    "xferLockedMapRemove -- could not unbind");
	}
	if ( mapIsEmpty(pdMap) ) {
	    xTrace1(xferp, TR_EVENTS,
		    "Closing locked map for host %s", ipHostStr(peer));
	    res = mapUnbind(lockedMap, peer);
	    xAssert( res == XK_SUCCESS );
	    mapClose(pdMap);
	} else {
	    xTrace0(xferp, TR_EVENTS, "locked map is not empty");
	}
    }
}



/* 
 * description in xfer.h -- see map structure description above
 */
void
xferTransferMapAdd( self, xferMap, peer, msgId, port )
    XObj	self;
    Map		xferMap;
    IPhost	*peer;
    mnetport	*port;
    MsgId	msgId;
{   
    Bind	bind;
    Map		pdMap, msgMap;
    VOID	*rcnt;

    if ( mapResolve(xferMap, peer, &msgMap) == XK_FAILURE ) {
	xTrace1(xferp, TR_MORE_EVENTS, "creating transfer submap for peer %s",
		ipHostStr(peer));
	msgMap = mapCreate(XFER_MSG_MAP_SZ, sizeof(MsgId));
	bind = mapBind(xferMap, peer, msgMap);
	xAssert(bind != ERR_BIND);
	regBidInterest(self, peer);
    }
    if ( mapResolve(msgMap, &msgId, &pdMap) == XK_FAILURE ) {
	xTrace1(xferp, TR_MORE_EVENTS, "creating message submap for msg %d",
		msgId);
	pdMap = mapCreate(XFER_XFER_PD_MAP_SZ, sizeof(mnetport *));
	bind = mapBind(msgMap, &msgId, pdMap);
	xAssert(bind != ERR_BIND);
    }
    if ( mapResolve(pdMap, &port, &rcnt) == XK_SUCCESS ) {
	mapUnbind(pdMap, &port);
	mapBind(pdMap, &port, (VOID *)(((int)rcnt) + 1));
	xTrace1(xferp, TR_EVENTS,
		"increasing port transfer refcount to %d",
		((int)rcnt) + 1);
    } else {
	xTrace0(xferp, TR_EVENTS, "adding new entry for this port to msg map");
	mapBind(pdMap, &port, (VOID *)1);
    }
}


/* 
 * description in xfer.h -- see map structure description above
 */
void
xferTransferMapRemove( xferMap, peer, msgId )
    Map		xferMap;
    IPhost	*peer;
    MsgId	msgId;
{    
    Map			msgMap, pdMap;
    xkern_return_t	res;

    if ( mapResolve(xferMap, peer, &msgMap) == XK_FAILURE ) {
	xTrace1(xferp, TR_ERRORS,
		"transfer complete notification -- no host map for msg %s",
		ipHostStr(peer));
	return;
    }
    if ( mapResolve(msgMap, &msgId, &pdMap) == XK_FAILURE ) {
	xTrace1(xferp, TR_ERRORS,
		"transfer complete notification -- no port map for msg %d",
		msgId);
	return;
    }
    xTrace1(xferp, TR_EVENTS,
	    "xferTransferMapRemove closing map for message %d",
	    msgId);
    res = mapUnbind(msgMap, &msgId);
    xAssert( res == XK_SUCCESS );
    mapClose(pdMap);
    if ( mapIsEmpty(msgMap) ) {
	xTrace1(xferp, TR_EVENTS,
		"Closing empty xfer map for host %s", ipHostStr(peer));
	res = mapUnbind(xferMap, peer);
	xAssert( res == XK_SUCCESS );
	mapClose(msgMap);
    } else {
	xTrace0(xferp, TR_EVENTS, "xfer map is not empty");
    }
}


/* 
 * This is a MapForEachFun applied to the last map in the
 * 'transferMap' map chain:
 *	key == mnetport *
 *	val == reference count
 *	arg == abort function pointer
 *
 * The port (the key) is, in fact, not going to be
 * transferred.   We need to tell the port manager to remove the port
 * right by calling the abort function.
 */
static int
abortPort( key, val, arg )
    VOID	*key, *val, *arg;
{
    int			i;
    XferRemFunc	cb = (XferRemFunc)arg;

    xAssert(cb);
    xTrace2(xferp, TR_EVENTS, "port number %d, refs == %d",
	    (*(mnetport **)key)->net_port_number, (int)val);
    for ( i=0; i < (int)val; i++ ) {
	cb(*(mnetport **)key);
    }
    return 1;
}


/* 
 * This is a mapForEachFun applied to the middle map in the
 * 'transferMap' map chain:
 *	key == msgId *
 *	val == interior map
 *
 * The rights in the interior map are to be deallocated and the
 * interior map closed.
 */
static int
abortTransfer( key, val, arg )
    VOID	*key, *val, *arg;
{
    xIfTrace(xferp, TR_EVENTS) {
	xTrace0(xferp, TR_EVENTS, "aborting transfer of ports:");
	mapForEach((Map)val, abortPort, arg);
    }
    mapClose((Map)val);
    return MFE_CONTINUE;
}



/* 
 * This is a mapForEachFun applied to the interior map in the
 * 'lockedMap' map chain:
 *	key == mnetport *
 *
 * We unlock this port.
 */
static int
abortLock( key, val, arg )
    VOID	*key, *val, *arg;
{
    mnetport		*np = *(mnetport **)key;
    LockRemFunc	cb = (LockRemFunc)arg;

    xAssert( np );
    xAssert( cb );
    xTrace1(xferp, TR_EVENTS, "XFER aborting lock on port %d",
	    np->net_port_number);
    cb(np, val);
    return MFE_CONTINUE;
}


/* 
 * 'topMap' is either a transferMap or a lockedMap.  'host' is a peer
 * that rebooted.  If the top-level map has locked or transferred
 * ports, appropriate 'release' actions will be taken.  
 */
static void
releaseMapOnReboot( self, topMap, f, peer, arg, txt )
    XObj		self;
    Map			topMap;
    IPhost		*peer;
    MapForEachFun	f;
    VOID		*arg;
    char		*txt;
{
    Map			subMap;
    xkern_return_t	res;

    if ( mapResolve(topMap, peer, &subMap) == XK_SUCCESS ) {
	xTrace1(xferp, TR_EVENTS, "XFER reboot handler -- %s", txt);
	mapForEach(subMap, f, arg);
	mapClose(subMap);
	res = mapUnbind(topMap, peer);
	xAssert( res == XK_SUCCESS );
	/* 
	 * XXX -- Schedule this
	 */
	unregBidInterest(self, peer);
    }
}


/* 
 * description in xfer.h
 */
void
xferPeerRebooted( self, peer, lockedMap, transferMap, xrf, lrf )
    XObj	self;
    IPhost	*peer;
    Map		lockedMap, transferMap;
    XferRemFunc	xrf;
    LockRemFunc	lrf;
{
    /* 
     * Unlock ports which were locked by the rebooted host. 
     */
    releaseMapOnReboot(self, lockedMap, abortLock, peer, (VOID *)lrf, 
		       "aborting locks");
    /* 
     * Release rights which were being transferred to 
     * this host from the rebooted host
     */
    releaseMapOnReboot(self, transferMap, abortTransfer, peer, (VOID *)xrf,
		       "aborting transfers");
}    


/* 
 * Unregister 'protl's interest in 'peer's reboot by openDisabling the
 * BootId protocol
 */
static xkern_return_t
unregBidInterest( protl, peer )
    XObj	protl;
    IPhost	*peer;
{
    XObj	llp;
    Part	p;
    
    xAssert(xIsProtocol(protl));
    xTrace1(xferp, TR_MAJOR_EVENTS,
	    "xfer unregistering interest in peer %s with bidctl",
	    ipHostStr(peer));
    llp = xGetDown(protl, XFER_BIDCTL_I);
    xAssert(xIsProtocol(llp));
    partInit(&p, 1);
    partPush(p, (VOID *)peer, sizeof(IPhost));
    return xOpenDisable(protl, protl, llp, &p);
}



/* 
 * Register 'protl's interest in 'peer's reboot by openEnabling the
 * BootId protocol
 */
static xkern_return_t
regBidInterest( protl, peer )
    XObj	protl;
    IPhost	*peer;
{
    XObj	llp;
    Part	p;
    
    xAssert(xIsProtocol(protl));
    xTrace1(xferp, TR_MAJOR_EVENTS,
	    "xfer registering interest in peer %s with bidctl",
	    ipHostStr(peer));
    llp = xGetDown(protl, XFER_BIDCTL_I);
    xAssert(xIsProtocol(llp));
    partInit(&p, 1);
    partPush(p, (VOID *)peer, sizeof(IPhost));
    return xOpenEnable(protl, protl, llp, &p);
}


/* 
 * description in xfer.h
 */
void
xferCreateMaps( lockedMap, xferMap )
    Map	*lockedMap, *xferMap;
{
    *lockedMap = mapCreate(XFER_LOCKED_MAP_SZ, sizeof(IPhost));
    *xferMap = mapCreate(XFER_TRANSFER_MAP_SZ, sizeof(IPhost));
}



char *
xferHostStr( rh )
    XferHost	*rh;
{
    static char	buf[80];

    sprintf(buf, "%s  (BID == %x)", ipHostStr(&rh->h), rh->bid);
    return buf;
}



static int
findEntry( key, val, arg )
    VOID	*key, *val, *arg;  
{
    xTrace0(xferp, TR_FULL_TRACE, "xfer findEntry");
    *(VOID **)arg = key;
    return 0;
}


static int
mapIsEmpty( m )
    Map	m;
{
    VOID	*key;

    key = 0;
    mapForEach(m, findEntry, &key);
    return key == 0;
}
