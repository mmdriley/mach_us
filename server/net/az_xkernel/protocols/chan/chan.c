/*     
 * $RCSfile: chan.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.51 $
 * $Date: 1993/02/01 22:30:59 $
 */

#include "xkernel.h"
#include "chan_internal.h"
#include "bidctl.h"


/* If NO_KILLTICKET is defined, CHAN will not inform the lower protocol when it
 * can free a message.  The lower protocol will then free messages based on
 * its timeouts.
 */
/* #define NO_KILLTICKET */

/*
 * Global data
 */
int 		tracechanp=1;

typedef struct {
    XObj	self;
    IPhost	peer;
} DisableStubInfo;


#ifdef __STDC__

static xkern_return_t	chanBidctlUnregister( XObj, IPhost * );
static void		disableStub( Event, VOID * );
static int		getIdleSeqNum( Map, ActiveID * );
static void 		getProcProtl( XObj );
static XObj		getSpecIdleSessn( XObj, Enable *, ActiveID *,
					  Map, Map );
static void 		killticket( XObj, int * );

#else

static xkern_return_t	chanBidctlUnregister();
static void		disableStub();
static int		getIdleSeqNum();
static void 		getProcProtl();
static XObj		getSpecIdleSessn();
static void 		killticket();

#endif __STDC__



#define HDR ((CHAN_HDR *)hdr)

/*
 * chanHdrStore - Used when calling msgPush
 */
void
chanHdrStore(hdr, dst, len, arg)
    VOID *hdr;
    char *dst;
    long int len;
    VOID *arg;
{
    CHAN_HDR h;
    
    xAssert(len == CHANHLEN);  
    h.chan = htons(HDR->chan);
    h.flags = HDR->flags;
    h.prot_id = htonl(HDR->prot_id);
    h.seq = htonl(HDR->seq);
    h.len = htonl(HDR->len);
    bcopy((char *)(&h), dst, CHANHLEN);
}


/*
 * chanHdrLoad - Used when calling msgPop
 */
static long
chanHdrLoad(hdr, src, len, arg)
    VOID *hdr;
    char *src;
    long int len;
    VOID *arg;
{
    xAssert(len == sizeof(CHAN_HDR));  
    bcopy(src, (char *)hdr, CHANHLEN);
    HDR->chan = ntohs(HDR->chan);
    HDR->prot_id = ntohl(HDR->prot_id);
    HDR->seq = ntohl(HDR->seq);
    HDR->len = ntohl(HDR->len);
    return CHANHLEN;
}

#undef HDR

  
void
chanEventFlush(s)
    XObj s;
{
    CHAN_STATE	*state = (CHAN_STATE *)s->state;

    if ( state->event ) {
	evCancel(state->event);
	state->event = 0;
    }
}


/*
 * chan_init
 */
void
chan_init(self)
    XObj self;
{
    Part	part;
    PSTATE 	*pstate;
    
    xTrace0(chanp, TR_GROSS_EVENTS, "CHAN init");
    
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xTrace0(chanp, TR_ERRORS,
		"CHAN could not get get transport protl -- not initializing");
	return;
    }
    if ( ! xIsProtocol(xGetDown(self, CHAN_BIDCTL_I)) ) {
	xTrace0(chanp, TR_ERRORS,
		"CHAN could not get get bidctl protl -- not initializing");
	return;
    }
    self->state = (VOID *)(pstate = X_NEW(PSTATE));
    getProcProtl(self);
    pstate->idleSvrMap =
      			mapCreate(CHAN_IDLE_SERVER_MAP_SZ, sizeof(IPhost));
    pstate->idleCliMap =
      			mapCreate(CHAN_IDLE_CLIENT_MAP_SZ, sizeof(IPhost));
    pstate->actSvrKeyMap =
      			mapCreate(CHAN_ACTIVE_SERVER_MAP_SZ, sizeof(ActiveID));
    pstate->actSvrHostMap =
      			mapCreate(CHAN_ACTIVE_SERVER_MAP_SZ, sizeof(IPhost));
    pstate->actCliKeyMap =
		      	mapCreate(CHAN_ACTIVE_CLIENT_MAP_SZ, sizeof(ActiveID));
    pstate->actCliHostMap =
      			mapCreate(CHAN_ACTIVE_CLIENT_MAP_SZ, sizeof(IPhost));
    pstate->passiveMap = mapCreate(CHAN_HLP_MAP_SZ, sizeof(PassiveID));
    pstate->newChanMap = mapCreate(CHAN_HLP_MAP_SZ, sizeof(long));
    semInit(&pstate->newSessnLock, 1);
    
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, 0), &part) == XK_FAILURE ) {
	xTrace0(chanp, TR_ERRORS,
		"chan_init: can't openenable transport protocol");
	xFree((char *) pstate);
    }
    xTrace0(chanp, TR_GROSS_EVENTS, "CHAN init done");
}


/* 
 * Adds an entry to the '{ peer -> { hlp -> newChannel } }' map for
 * the given peer if it doesn't already exist.  If it doesn't exist,
 * CHAN's interest in this host is communicated to BIDCTL.
 */
Map
chanGetChanMap( self, h )
    XObj	self;
    IPhost	*h;
{
    Map		hlpMap;
    Bind	b;
    PState	*ps = (PState *)self->state;

    if ( mapResolve(ps->newChanMap, h, &hlpMap) == XK_FAILURE ) {
	xTrace1(chanp, TR_EVENTS, "creating new 'newChanMap' for host %s",
		ipHostStr(h));
	if ( chanBidctlRegister(self, h) == XK_FAILURE ) {
	    return 0;
	}
	hlpMap = mapCreate(CHAN_HLP_MAP_SZ, sizeof(IPhost));
	b = mapBind(ps->newChanMap, (char *)h, (int)hlpMap);
	xAssert(b != ERR_BIND);
    }
    return hlpMap;
}


/* 
 * chanCreateSessn -- create a new channel session using the given
 * XObjects and the information in 'key'.  The session is bound into
 * 'keyMap' with 'key' and is also bound into the mapchain rooted at
 * 'hostMap.'
 */
XObj 
chanCreateSessn( self, hlpRcv, hlpType, key, initFunc, keyMap, hostMap )
    XObj 	self, hlpRcv, hlpType;
    ActiveID	*key;
    Pfv		initFunc;
    Map		keyMap, hostMap;
{
    XObj   	s;
    CHAN_STATE 	*ss;
    CHAN_HDR 	*hdr;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN createSessn ......................");
    xIfTrace(chanp, TR_MAJOR_EVENTS) chanDispKey(key);
    ss = X_NEW(CHAN_STATE);
    bzero((char *)ss, sizeof(CHAN_STATE));
    /*
     * Fill in  state
     */
    msg_clear(ss->saved_msg);
    semInit(&ss->reply_sem, 0);
    if ( xControl(key->lls, GETPEERHOST, (char *)&ss->peer, sizeof(IPhost))
				< (int)sizeof(IPhost) ) {
	xTrace0(chanp, TR_ERRORS,
		"chan_open: can't do GETPEERHOST on lower session");
	return ERR_XOBJ;
    }
    if ( chanGetChanMap(self, &ss->peer) == 0 ) {
	return ERR_XOBJ;
    }
    /*
     * Fill in header
     */
    hdr = &ss->hdr;
    hdr->chan = key->chan;
    hdr->prot_id = key->prot_id;
    hdr->flags = USER_MSG;
    /*
     * Create session and bind to address
     */
    xDuplicate(key->lls);
    s = xCreateSessn(initFunc, hlpRcv, hlpType, self, 1, &key->lls);
    s->state = (char *) ss;
    s->binding 	= (Bind) mapBind(keyMap, (char *) key, (int)s);
    /*
     * Just to be paranoid
     */
    if ( s->binding == ERR_BIND ) {
	xTrace0(chanp, TR_ERRORS, "chanCreateSessn: could not bind session"); 
	xTrace3(chanp, TR_ERRORS,
		"chanCreateSessn: lls = %x, chan = %d, prot_id = %d",
		key->lls, (int)key->chan, key->prot_id);
	xClose(s);
	return ERR_XOBJ;
    }
    chanMapChainAddObject((VOID *)s, hostMap,
			  &ss->peer, key->prot_id, key->chan);
    xTrace1(chanp, TR_MAJOR_EVENTS, "chanCreateSessn returns %x", s);
    return s;
}



void
chanDestroy( s )
    XObj 	s;
{
    CHAN_STATE	*sstate;
    PSTATE 	*pstate;
    XObj	lls;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN Destroy ........................");
    xTrace1(chanp, TR_MAJOR_EVENTS, "Of session %x", s);
    xAssert( xIsSession(s) );
    xAssert(s->rcnt == 0);
    xAssert(s->binding == 0);
    pstate  = (PSTATE *)s->myprotl->state;
    sstate  = (CHAN_STATE *)s->state;
    chanEventFlush(s);
    /*
     * Free chan state
     */
    if (sstate) {
	msg_flush(sstate->saved_msg); 
	lls = xGetDown(s, 0);
	if ( lls != ERR_XOBJ ) {
	    xClose(lls);
	}
    }
    xDestroy(s);
    return;
}


/*
 * chanDemux
 */
static xkern_return_t
chanDemux(self, lls, msg)
    XObj self;
    XObj lls;
    Msg *msg;
{
    CHAN_HDR 	hdr;
    XObj   	s;
    ActiveID 	actKey;
    PassiveID 	pasKey;
    PSTATE 	*ps = (PSTATE *)self->state;
    Enable	*e;
    Map		map;
    
    xTrace0(chanp, TR_EVENTS, "CHAN demux .............................");

    if ( ! msgPop(msg, chanHdrLoad, (VOID *)&hdr, CHANHLEN, 0) ) {
	xError("chanDemux: msgPop returned false");
	return XK_FAILURE;
    }
    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(&hdr);
    } 
    /*
     * Check for active channel
     */
    actKey.chan		= hdr.chan;
    actKey.prot_id 	= hdr.prot_id;
    actKey.lls 	= lls;
    xIfTrace(chanp, TR_DETAILED) {
	chanDispKey( &actKey );
    }
    map = (hdr.flags & FROM_CLIENT) ? ps->actSvrKeyMap : ps->actCliKeyMap;
    if ( mapResolve(map, &actKey, &s) == XK_SUCCESS ) {
	/*
	 * Pop to active channel
	 */
	xTrace1(chanp, TR_MORE_EVENTS, "chanDemux: existing channel %s", s);
	return xPop(s, lls, msg, &hdr);
    } 
    /* 
     * Look for an idle/dormant session
     */
    if ( ! (hdr.flags & FROM_CLIENT) ) {
	int	seq;

	seq = getIdleSeqNum(ps->idleCliMap, &actKey);
	if ( seq == -1 ) {
	    /* 
	     * We never heard of this channel -- we'll drop the msg
	     */
	    xTrace0(chanp, TR_SOFT_ERRORS,
		    "spurious msg for non-existent channel");
	} else {
	    chanClientIdleRespond(&hdr, actKey.lls, seq);
	}
    } else {
	/*
	 * Find openenable
	 */
	pasKey = actKey.prot_id;
	if ( mapResolve(ps->passiveMap, &pasKey, &e) == XK_FAILURE ) {
	    xTrace1(chanp, TR_EVENTS,
		    "chanDemux -- no openenable for hlp %d", hdr.prot_id);
	} else {
	    /* 
	     * Try to create a session
	     */
	    semWait(&ps->newSessnLock);
	    /* 
	     * Look again ...
	     */
	    if ( mapResolve(map, &actKey, &s) == XK_FAILURE ) {
		s = getSpecIdleSessn(self, e, &actKey, 
				     ps->idleSvrMap, ps->actSvrKeyMap);
		if ( s == ERR_XOBJ ) {
		    s = chanSvcOpen(self, e->hlpRcv, e->hlpType, &actKey, 0);
		}
	    } else {
		/*
		 * Pop to active channel
		 */
		xTrace1(chanp, TR_MORE_EVENTS, "chanDemux: new channel %s created by someone else", s);
	    } 
	    semSignal(&ps->newSessnLock);
	    if ( s != ERR_XOBJ ) {
		xPop(s, lls, msg, &hdr);
	    } else {
		xTrace0(chanp, TR_SOFT_ERRORS,
			"chanDemux: can't create session ");
	    }
	}
    }
    return XK_SUCCESS;
}


/* 
 * chanCheckMsgLen -- checks the length field of the header with the
 * actual length of the message, truncating it if necessary.
 *
 * returns 0 if the message is long enough and
 * 	  -1 if the message is too short.
 */
int
chanCheckMsgLen( hdrLen, m )
    u_int 	hdrLen;
    Msg 	*m;
{
    u_int	dataLen;

    dataLen = msgLen(m);
    xTrace2(chanp, TR_DETAILED,
	    "chan checkLen: hdr->len = %d, msg_len = %d", hdrLen,
	    dataLen);
    if (hdrLen < dataLen) {
	xTrace0(chanp, TR_MORE_EVENTS, "chan_pop: truncating msg");
	msgTruncate(m, hdrLen);
    } else if (hdrLen > dataLen) {
	xTrace0(chanp, TR_SOFT_ERRORS, "chan_pop: message too short!");
	return -1;
    }
    return 0;
}


static int
chanControlProtl( self, op, buf, len )
    XObj 	self;
    int 	op, len;
    char 	*buf;
{
    PSTATE		*ps = (PSTATE *)self->state;
    BidctlBootMsg	*msg;

    switch ( op ) {

      case BIDCTL_FIRST_CONTACT:
	return 0;

      /* 
       * This handler must not block
       */
      case BIDCTL_PEER_REBOOTED:
	{
	    DisableStubInfo	*dsInfo;
	    Map			hlpMap;
	    xkern_return_t	res;

	    msg = (BidctlBootMsg *)buf;
	    xTrace1(chanp, TR_MAJOR_EVENTS,
		    "chan receives notification that peer %s rebooted",
		    ipHostStr(&msg->h));
	    if ( mapResolve(ps->newChanMap, &msg->h, &hlpMap) == XK_FAILURE ) {
		/* 
		 * We shouldn't have been notified of this reboot
		 */
		xTrace1(chanp, TR_SOFT_ERRORS,
			"CHAN receives spurious notification of %s reboot",
			ipHostStr(&msg->h));
	    } else {
		chanClientPeerRebooted(ps, &msg->h);
		chanServerPeerRebooted(ps, &msg->h);
		mapClose(hlpMap);
		res = mapUnbind(ps->newChanMap, &msg->h);
		xAssert(res == XK_SUCCESS);
		dsInfo = X_NEW(DisableStubInfo);
		dsInfo->peer = msg->h;
		dsInfo->self = self;
		evDetach( evSchedule( disableStub, (VOID *)dsInfo, 0));
	    }
	}
	return 0;

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}
  

/*
 * chanControlSessn
 */
int
chanControlSessn( self, opcode, buf, len )
    XObj 	self;
    int 	opcode, len;
    char 	*buf;
{
    CHAN_STATE *sstate;
    
    xTrace0(chanp, TR_EVENTS, "CHAN controlsessn ......................");
    xTrace1(chanp, TR_EVENTS, "Of session=%x", self); 
    
    sstate = (CHAN_STATE *)self->state;
    
    switch (opcode) {
	
      case GETMYPROTO:
      case GETPEERPROTO:
	checkLen(len, sizeof(long));
	*(long *)buf = sstate->hdr.prot_id;
	return sizeof(long);

      case GETMAXPACKET:
      case GETOPTPACKET:
	checkLen(len, sizeof(int));
	if ( xControl(xGetDown(self, 0), opcode, buf, len) <= 0 ) {
	    return -1;
	}
	*(int *)buf -= sizeof(CHAN_HDR);
	return sizeof(int);
	
      case GETPEERHOST:
	checkLen(len, sizeof(IPhost));
	*(IPhost *)buf = sstate->peer;
	return sizeof(IPhost);

      case CHAN_SET_TIMEOUT:
	checkLen(len, sizeof(int));
	sstate->waitParam = *(int *)buf;
	xTrace1(chanp, TR_EVENTS, "channel session timeout set to %d",
		sstate->waitParam);
	return 0;
	
      case CHAN_SET_MAX_TIMEOUT:
	checkLen(len, sizeof(int));
	sstate->maxWait = *(int *)buf;
	xTrace1(chanp, TR_EVENTS, "channel session max timeout set to %d",
		sstate->maxWait);
	return 0;
	
      case CHAN_GET_TIMEOUT:
	checkLen(len, sizeof(int));
	*(int *)buf = sstate->waitParam;
	return sizeof(int);

      case CHAN_GET_MAX_TIMEOUT:
	checkLen(len, sizeof(int));
	*(int *)buf = sstate->maxWait;
	return sizeof(int);

      default:
	return xControl(xGetDown(self, 0), opcode, buf, len);
    }
}


/*
 * killticket: get rid of any dangling fragments 
 */
static void
killticket(s, t_ptr)
    XObj s;
    int *t_ptr;
{
#ifndef NO_KILLTICKET
    if (*t_ptr) {
	xAssert(xIsSession(s));
	xControl(s, FREERESOURCES, (char *)t_ptr, sizeof(int));
	*t_ptr = 0;
    }
#endif
}


/* 
 * Release timeout resources currently held (timeout events, saved
 * messages, tickets for lower messages).  Works for both client and
 * server
 */
void
chanFreeResources(self)
    XObj self;
{
    XObj	lls;
    CHAN_STATE	*state;

    state = (CHAN_STATE *)self->state;
    xAssert(state);
    lls = xGetDown(self, 0);
    xAssert(xIsSession(lls));
    killticket(lls, &state->ticket);
    msg_flush(state->saved_msg);
    chanEventFlush(self);
}


/*
 * chanReply -- send a control reply (no user data) to lower session
 * 's' using the given header and flags.
 */
void
chanReply( s, hdr, flags )
    XObj 	s;
    CHAN_HDR 	*hdr;
    int 	flags;
{
    CHAN_HDR 	hdr_copy;
    Msg 	msg;
    
    xAssert(xIsSession(s));
    hdr_copy 	 = *hdr;
    hdr_copy.flags = flags;
    hdr_copy.len 	 = 0;
    xTrace0(chanp, TR_EVENTS, "chan_pop: Sending reply");
    xIfTrace(chanp, TR_MORE_EVENTS) {
	pChanHdr(&hdr_copy);
    }
    msgConstructEmpty(&msg);
    msgPush(&msg, chanHdrStore, &hdr_copy, CHANHLEN, NULL);
    xPush(s, &msg);
    msgDestroy(&msg);
}


/*
 * chanCheckSeq -- determine the relation of the new sequence number 
 * 'new_seq' to 'cur_seq'.  
 */
SEQ_STAT 
chanCheckSeq(cur_seq, new_seq)
    unsigned int cur_seq;
    unsigned int new_seq;
{
    if (cur_seq == new_seq) {
	xTrace0(chanp, TR_MORE_EVENTS, "chanPop: current sequence number");
	return(current);
    }
    if (cur_seq < new_seq) {
	xTrace0(chanp, TR_MORE_EVENTS, "chanPop: new sequence number");
	return(new);
    }
    xTrace0(chanp, TR_MORE_EVENTS, "chanPop: old sequence number");
    return(old);
}


/* 
 * chanGetProtNum -- determine the protocol number of 'hlp'
 * relative to this protocol
 */
long
chanGetProtNum( self, hlp )
    XObj	self, hlp;
{
    long n;

    if ( (n = relProtNum(hlp, self)) == -1 ) {
	xTrace1(chanp, TR_ERRORS,
		"chan could not get relative protocol number of %s",
		hlp->name);
    }
    return n;
}
  


/* 
 * getProcProtl
 */
static void 
getProcProtl(s)
    XObj s;
{
    xAssert(xIsProtocol(s));
    s->control 	= chanControlProtl;
    s->open   	= chanOpen;
    s->openenable = chanOpenEnable;
    s->demux 	= chanDemux;
}



/* 
 * Register this protocol's interest in 'peer' with BIDCTL
 */
xkern_return_t
chanBidctlRegister( self, peer )
    XObj	self;
    IPhost	*peer;
{
    XObj	llp;
    Part	p;
    
    xAssert(xIsProtocol(self));
    xTrace1(chanp, TR_MAJOR_EVENTS,
	    "chan registering interest in peer %s with bidctl",
	    ipHostStr(peer));
    llp = xGetDown(self, CHAN_BIDCTL_I);
    xAssert(xIsProtocol(llp));
    partInit(&p, 1);
    partPush(p, (VOID *)peer, sizeof(IPhost));
    return xOpenEnable(self, self, llp, &p);
}


/* 
 * Register this protocol's interest in 'peer' with BIDCTL
 */
static xkern_return_t
chanBidctlUnregister( self, peer )
    XObj	self;
    IPhost	*peer;
{
    XObj	llp;
    Part	p;
    
    xAssert(xIsProtocol(self));
    xTrace1(chanp, TR_MAJOR_EVENTS,
	    "chan unregistering interest in peer %s with bidctl",
	    ipHostStr(peer));
    llp = xGetDown(self, CHAN_BIDCTL_I);
    xAssert(xIsProtocol(llp));
    partInit(&p, 1);
    partPush(p, (VOID *)peer, sizeof(IPhost));
    return xOpenDisable(self, self, llp, &p);
}


static void
disableStub( ev, arg )
    Event	ev;
    VOID 	*arg;
{
    DisableStubInfo	*dsInfo = (DisableStubInfo *)arg;

    xAssert(dsInfo);
    xTrace1(chanp, TR_MAJOR_EVENTS, "chan disable stub runs for host %s",
	    ipHostStr(&dsInfo->peer));
    chanBidctlUnregister(dsInfo->self, &dsInfo->peer);
    xFree((char *)dsInfo);
}


/* 
 * Remove the session 's' from the active maps 'keyMap' and 'hostMap'.
 * The session must actually be in the maps.
 */
void
chanRemoveActive( s, keyMap, hostMap )
    XObj	s;
    Map		keyMap, hostMap;
{
    xkern_return_t	res;
    Map			chanMap;
    SState		*ss = (SState *)s->state;
#ifdef XK_DEBUG
    XObj		oldSessn;
#endif

    /* 
     * Remove from active key map
     */
    xAssert(s->binding);
    res = mapRemoveBinding(keyMap, s->binding);
    xAssert( res == XK_SUCCESS );
    s->binding = 0;
    chanMap = chanMapChainFollow(hostMap, &ss->peer, ss->hdr.prot_id);
    xAssert( chanMap );
    xAssert( mapResolve(chanMap, &ss->hdr.chan, &oldSessn) == XK_SUCCESS
	     && oldSessn == s );
    res = mapUnbind(chanMap, (VOID *)&ss->hdr.chan);
    xAssert( res == XK_SUCCESS );
}


/* 
 * Removes the session from the 'fromMap' and binds it in the idleMap
 * series starting at 'toMap', using the peer, prot_id and channel
 * found in the session state.
 */
xkern_return_t
chanAddIdleSessn( hostMap, fromMap, toMap, s )
    Map		hostMap, fromMap, toMap;
    XObj	s;
{
    IPhost	peerHost;
    CHAN_STATE	*ss = (CHAN_STATE *)s->state;

    if ( xControl(xGetDown(s, 0), GETPEERHOST, (char *)&peerHost,
		  sizeof(IPhost)) < (int)sizeof(IPhost) ) {
	xTrace0(chanp, TR_ERRORS, "chanClientClose could not get peer host");
	return XK_FAILURE;
    }
    chanRemoveActive(s, fromMap, hostMap);
    /* 
     * We don't need to store the session if it's sequence number is
     * the default.
     */
    if ( ss->hdr.seq > START_SEQ ) {
	chanMapChainAddObject((VOID *)ss->hdr.seq, toMap, &peerHost,
			      ss->hdr.prot_id, ss->hdr.chan);
    }
    chanDestroy(s);
    return XK_SUCCESS;
}


/* 
 * Looks for a dormant session corresponding to the active key,
 * looking in the map chain starting at idleMap.  If
 * such a session is found, the corresponding sequence number is
 * returned.  The sequence number is not unbound from the map.
 * If no dormant session is found, -1 is returned.
 */
static int
getIdleSeqNum( idleMap, key )
    ActiveID	*key;
    Map		idleMap;
{
    Map		map;
    Channel	chan;
    int		seq;

    if ( ! (map = chanGetMap(idleMap, key->lls, key->prot_id)) ) {
	xTrace0(chanp, TR_EVENTS, "chanGetIdleSeqNum -- no map");
	return -1;
    }
    chan = key->chan;
    if ( mapResolve(map, &chan, &seq) == XK_FAILURE ) {
	seq = -1;
    }
    return seq;
}


/* 
 * Restores the appropriate dormant idle session if it exists.
 * The session is removed from the idle map and placed in the active map.
 */
static XObj
getSpecIdleSessn( self, e, key, idleMap, actMap )
    XObj	self;
    ActiveID	*key;
    Enable	*e;
    Map		idleMap, actMap;
{
    Map			map;
    Channel		chan;
    xkern_return_t	res;
    int			seq;

    if ( ! (map = chanGetMap(idleMap, key->lls, key->prot_id)) ) {
	xTrace0(chanp, TR_EVENTS, "chanGetSpecIdleSessn -- no map");
	return ERR_XOBJ;
    }
    chan = key->chan;
    if ( mapResolve(map, &chan, &seq) == XK_SUCCESS ) {
	XObj	s;

	xTrace1(chanp, TR_EVENTS,
		"chanGetSpecIdleSessn -- found stored sequence #%d", seq);
	s = chanSvcOpen(self, e->hlpRcv, e->hlpType, key, seq);
	if ( s == ERR_XOBJ ) {
	    xTrace1(chanp, TR_ERRORS,
		    "chan getSpecIdleSessn could not reopen chan %d",
		    key->chan);
	    return ERR_XOBJ;
	}
	/* 
	 * Remove this session from the idle map
	 */
	res = mapUnbind(map, (char *)&chan);
	xAssert(res == XK_SUCCESS);
	return s;
    } else {
	xTrace0(chanp, TR_EVENTS,
		"chanGetSpecIdleSessn -- found no appropriate session");
	return ERR_XOBJ;
    }
}


int
chanMapRemove( key, val )
    VOID	*key;
    VOID	*val;
{
    return MFE_CONTINUE | MFE_REMOVE;
}


/* 
 * Follows the map chain starting at 'm', looking for the map
 * corresponding to the given lls and hlp number.
 */
Map
chanGetMap( m, lls, prot )
    Map		m;
    XObj	lls;
    long	prot;
{
    IPhost	peerHost;

    if ( xControl(lls, GETPEERHOST, (char *)&peerHost,
		  sizeof(IPhost)) < (int)sizeof(IPhost) ) {
	xTrace0(chanp, TR_ERRORS, "chanGetIdleMap could not get peer host");
	return 0;
    }
    return chanMapChainFollow(m, &peerHost, prot);
}


/* 
 * Send a message to the lower session of 's' using 'flags.'  The
 * saved user message will be sent if the 'forceUsrMsg' parameter is
 * non-zero or if the user message will fit in a single packet.
 *
 * The saved user message must be valid.
 *
 * Returns a handle on the outgoing message.
 */
xmsg_handle_t
chanResend( s, flags, forceUsrMsg )
    XObj	s;
    int		flags, forceUsrMsg;
{
    CHAN_HDR		hdr;
    SState		*ss = (SState *)s->state;
    Msg			packet;
    xmsg_handle_t	ticket;
    int			size;

    hdr = ss->hdr;
    hdr.flags = flags;
    xAssert( ! msg_isnull(ss->saved_msg));
    xAssert( (flags & ACK_REQUESTED) || forceUsrMsg );
    if ( ! forceUsrMsg ) {
	if ( xControl(s, GETOPTPACKET, (char *)&size, sizeof(int))
	    						< (int)sizeof(int) ) {
	    xTrace0(chanp, TR_ERRORS, "chanResend couldn't GETOPTPKT");
	    size = 0;
	}
    }
    if ( forceUsrMsg || msgLen(&ss->saved_msg.m) < size ) {
	msgConstructCopy(&packet, &ss->saved_msg.m);
	hdr.flags |= USER_MSG;
    } else {
	msgConstructEmpty(&packet);
	hdr.len = 0;
    }
    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(&hdr);
    } 
    msgPush(&packet, chanHdrStore, &hdr, CHANHLEN, NULL);
    /*
     * Send message
     */
    xAssert(xIsSession(xGetDown(s, 0)));
    ticket = xPush(xGetDown(s, 0), &packet);
    msgDestroy(&packet);
    return ticket;
}


/* 
 * Run on inactive channels, asks for ACK
 */
void
chanTimeout( ev, arg )
    Event	ev;
    VOID 	*arg;
{
    XObj		s = (XObj)arg;
    CHAN_STATE		*ss;
    xmsg_handle_t	ticket;
    int			flags;
    
    xAssert(xIsSession(s));
    ss = (CHAN_STATE *)s->state; 
    xTrace1(chanp, TR_EVENTS, "CHAN: %sTimeout: timeout!",
	    ss->hdr.flags & FROM_CLIENT ? "client" : "server");
    if ( ss->hdr.flags & FROM_CLIENT ) {
	flags = ACK_REQUESTED | FROM_CLIENT;
    } else {
	flags = ACK_REQUESTED;
    }
    ticket = chanResend(s, flags, 0);
    if ( evIsCancelled(ev) ) {
	xTrace0(chanp, TR_EVENTS, "chanTimeout cancelled");
	return;
    }
    if ( ticket != XMSG_NULL_HANDLE ) {
	ss->ticket = ticket;
    }
    /* 
     * Detach myself
     */
    evDetach(ss->event);
    ss->wait = MIN(2*ss->wait, ss->maxWait);
    xTrace1(chanp, TR_MORE_EVENTS, "new timeout value: %d", ss->wait);
    ss->event = evSchedule(chanTimeout, s, ss->wait);
}
