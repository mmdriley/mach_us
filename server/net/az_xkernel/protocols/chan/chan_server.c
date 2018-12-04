/*
 * chan_server.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.43 $
 * $Date: 1993/02/01 22:30:38 $
 */

#include "xkernel.h"
#include "chan_internal.h"

#ifdef __STDC__

static xkern_return_t	chanServerClose( XObj );
static xkern_return_t	chanServerPop( XObj, XObj, Msg *, VOID * );
static void		closeStub( Event, VOID * );
static int		doCallDemux( XObj, CHAN_HDR *, Msg * );
static void 		getProcServer( XObj );
static int		validateOpenEnable( XObj );
static int		zombifyServer( VOID *, VOID * );

#else

static xkern_return_t	chanServerClose();
static xkern_return_t	chanServerPop();
static void		closeStub();
static int		doCallDemux();
static void 		getProcServer();
static int		validateOpenEnable();
static int		zombifyServer();

#endif __STDC__


/*
 * chanSvcOpen
 */
XObj 
chanSvcOpen( self, hlpRcv, hlpType, key, seq )
    XObj	self, hlpRcv, hlpType;
    ActiveID	*key;
    int		seq;
{
    XObj   	s;
    PSTATE 	*ps = (PSTATE *)self->state;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN svc open ........................");
    xIfTrace(chanp, TR_MAJOR_EVENTS) chanDispKey(key);
    s = chanCreateSessn(self, hlpRcv, hlpType, key, getProcServer,
			ps->actSvrKeyMap, ps->actSvrHostMap);
    if ( s != ERR_XOBJ ) {
	CHAN_STATE	*ss = (CHAN_STATE *)s->state;
	ss->cur_state = SVC_IDLE;
	ss->hdr.flags = USER_MSG;
	ss->hdr.seq = seq;
	ss->waitParam = SERVER_WAIT_DEFAULT;
	ss->maxWait = SERVER_MAX_WAIT_DEFAULT;
    }
    return s;
}


xkern_return_t
chanOpenEnable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part 	*p;
{
    PSTATE 	*ps = (PSTATE *)self->state;
    PassiveID 	key;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN open enable ......................");
    if ( (key = chanGetProtNum(self, hlpType)) == -1 ) {
	return XK_FAILURE;
    }
    xTrace1(chanp, TR_MAJOR_EVENTS, "chan_openenable: prot_id = %d", key);
    return defaultOpenEnable(ps->passiveMap, hlpRcv, hlpType, (VOID *)&key);
}


xkern_return_t
chanOpenDisable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part 	*p;
{
    PSTATE 	*ps = (PSTATE *)self->state;
    PassiveID 	key;
    
    xTrace0(chanp, TR_MAJOR_EVENTS, "CHAN openDisable ......................");
    if ( (key = chanGetProtNum(self, hlpType)) == -1 ) {
	return XK_FAILURE;
    }
    xTrace1(chanp, TR_MAJOR_EVENTS, "chanOpenDisable: prot_id = %d", key);
    return defaultOpenDisable(ps->passiveMap, hlpRcv, hlpType, (VOID *)&key);
}



/*
 * chanServerPop
 */
static xkern_return_t
chanServerPop(self, lls, msg, inHdr)
    XObj self;
    XObj lls;
    Msg *msg;
    VOID *inHdr;
{
    CHAN_STATE 	*state;
    CHAN_HDR	*hdr = (CHAN_HDR *)inHdr;
    SEQ_STAT 	status;
    
    xTrace0(chanp, TR_EVENTS, "CHAN pop ...............................");
    
    xAssert(hdr);
    state = (CHAN_STATE *)self->state;
    if ( chanCheckMsgLen(hdr->len, msg) ) {
	/* 
	 * Message was too short -- probably mangled
	 */
	return XK_SUCCESS;
    }
    /* 
     * Check sequence number
     */
    status = chanCheckSeq(state->hdr.seq, hdr->seq);
    if ( status == old ) {
	return XK_SUCCESS;
    }
    xTrace3(chanp, TR_MORE_EVENTS, "state: %s  seq = %d  status: %s",
	    chanStateStr(state->cur_state), state->hdr.seq,
	    chanStatusStr(status));
    xAssert(hdr->chan == state->hdr.chan);
    switch(state->cur_state) {
	
      case SVC_IDLE: 
	
	if ( status == new ) {
	    if ( hdr->flags & USER_MSG ) {
		/*
		 * Fire up the server
		 */
		doCallDemux(self, hdr, msg);
	    } else if ( hdr->flags & ACK_REQUESTED ) {
		chanReply(lls, hdr, NEGATIVE_ACK);
	    }
	} else {
	    xTrace0(chanp, TR_EVENTS, "chan_pop: SVC_IDLE: ignoring msg");
	}
	break;
	
      case SVC_WAIT:
	if ( status == new ) {
	    /*
	     * We can free resources
	     */
	    xTrace0(chanp, TR_EVENTS, "Server received ACK.  Freeing message");
	    chanFreeResources(self);
	    /* 
	     * Remove ref count for this message
	     */
	    xAssert(self->rcnt > 1);
	    xClose(self);
	    state->cur_state = SVC_IDLE;
	    chanServerPop(self, lls, msg, hdr);
	    break;
	}
	xAssert( status == current );
	if ( hdr->flags & NEGATIVE_ACK ) {
	    /*
	     * Client lost reply message -- retransmit
	     */
	    xTrace0(chanp, TR_EVENTS,
		    "chanPop: client lost reply -- retransmitting");
	    chanResend(self, 0, 1);
	}
	break;

      case SVC_EXECUTE:
	if ( status == new ) {
#ifdef CHAN_ALLOW_USER_ABORT
	    state->cur_state = SVC_IDLE;
	    chanServerPop(self, lls, msg);
#endif
	    break;
	}
	xAssert(status == current);
	/*
	 * Send ack if requested
	 */
	if ( hdr->flags & ACK_REQUESTED ) {
	    chanReply(xGetDown(self, 0), hdr, 0);
	}
	break;
	
      default:
	xTrace1(chanp, TR_ERRORS,"chan_pop: invalid state %d",
		state->cur_state);
	break;
    } 
    xTrace2(chanp, TR_MORE_EVENTS, "end chanSvrPop:  state: %s curSeq = %d",
	    chanStateStr(state->cur_state), state->hdr.seq);
    return XK_SUCCESS;
}


/* 
 * validateOpenEnable -- Checks to see if there is still an openEnable for
 * the session and, if so, calls openDone.
 * This is called right before a message is sent up through
 * a session with no external references.  This has to be done
 * because CHAN sessions
 * can survive beyond removal of all external references. 
 *
 * Returns 1 if an openenable exists, 0 if it doesn't.
 */
static int
validateOpenEnable( s )
    XObj	s;
{
    CHAN_STATE	*ss = (CHAN_STATE *)s->state;
    Enable	*e;
    PassiveID	key;
    PSTATE	*ps;

    ps = (PState *)xMyProtl(s)->state;
    key = ss->hdr.prot_id;
    if ( mapResolve(ps->passiveMap, &key, &e) == XK_FAILURE ) {
	xTrace1(chanp, TR_MAJOR_EVENTS,
		"chanValidateOE -- no OE for hlp %d!", key);
	return 0;
    }
    xOpenDone(e->hlpRcv, s, xMyProtl(s));
    return 1;
}
    

/*
 * doCallDemux
 */
static int
doCallDemux(self, inHdr, inMsg)
    XObj self;
    CHAN_HDR *inHdr;
    Msg *inMsg;
{
    CHAN_STATE	*state;
    CHAN_HDR	*hdr;
    Msg		rmsg;
    int		packet_len;
    XObj	lls;
    SeqNum	oldSeq;
    
    state = (CHAN_STATE *)self->state;
    state->cur_state = SVC_EXECUTE;
    hdr = &state->hdr;
    hdr->seq = inHdr->seq;
    lls = xGetDown(self, 0);
    xAssert(xIsSession(lls));
    /*
     * Send ack if requested
     */
    if ( inHdr->flags & ACK_REQUESTED ) {
	chanReply( lls, inHdr, 0 );
    }
    xTrace1(chanp, TR_MORE_EVENTS,
	    "chan_pop: incoming length (no chan hdr): %d", msgLen(inMsg));
    /* 
     * Save current value the sequence number.  We
     * will not send the reply if it has changed by the
     * time the reply is ready.
     */
    oldSeq = state->hdr.seq;
    msgConstructEmpty(&rmsg);
    if ( self->rcnt <= 1 && ! validateOpenEnable(self) ) {
	xTrace0(chanp, TR_EVENTS, "CHAN -- no server process, no reply");
	msgDestroy(&rmsg);
	return 0;
    }
    xCallDemux(self, inMsg, &rmsg);
    xTrace0(chanp, TR_EVENTS, "chan_pop: xCallDemux returns");
    xTrace1(chanp, TR_MORE_EVENTS, "chan_pop: (SVC) outgoing length: %d",
	    msgLen(&rmsg));
    if ( oldSeq != state->hdr.seq ) {
	xTrace0(chanp, TR_EVENTS, "CHAN -- reply no longer relevant.");
	msgDestroy(&rmsg);
	return 0;
    }
    /* 
     * Increase session reference count for this message. 
     */
    xDuplicate(self);
    msgConstructCopy(&state->saved_msg.m, &rmsg);
    msg_valid(state->saved_msg);
    /*
     * Fill in reply header
     */
    hdr->len 	= msgLen(&rmsg); 
    
    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(hdr);
    } 
    msgPush(&rmsg, chanHdrStore, hdr, CHANHLEN, NULL);
    
    xTrace1(chanp, TR_MORE_EVENTS, "chan_pop: packet len %d", msgLen(&rmsg)); 
    xTrace1(chanp, TR_MORE_EVENTS, "chan_pop: length field: %d", hdr->len);
    xTrace2(chanp, TR_MORE_EVENTS, "chan_pop: lls %x packet = %x",
	    xGetDown(self, 0), &rmsg); 
    /*
     * Send reply
     */
    packet_len    = msgLen(&rmsg); 
    state->ticket = xPush(lls, &rmsg);
    msgDestroy(&rmsg);
    if (state->ticket < 0) {
	xTrace0(chanp, TR_SOFT_ERRORS, "chan_pop: (SVC) can't send message");
	msg_flush(state->saved_msg);
	return -1;
    }
    state->cur_state = SVC_WAIT;
    state->wait      = CHAN_SVC_DELAY(packet_len, state->waitParam);
    xTrace1(chanp, TR_DETAILED, "chan_pop: (SVC) server_wait: %d",
	    state->wait);
    state->event = evSchedule(chanTimeout, self, state->wait);
    
    return 0;
}


static xkern_return_t
chanServerClose( s )
    XObj	s;
{
    CHAN_STATE	*ss = (CHAN_STATE *)s->state;
    PState	*ps;

    ps = (PState *)xMyProtl(s)->state;
    xTrace1(chanp, TR_MAJOR_EVENTS, "chan server close of sessn %x", s);

    switch ( ss->cur_state ) {
      case SVC_EXECUTE:
      case SVC_WAIT:
	/* 
	 * This shouldn't happen.
	 */
	xTrace1(chanp, TR_ERRORS, "chanServerClose called in %s!!",
		chanStateStr(ss->cur_state));
	/* 
	 * fall-through
	 */

      case SVC_IDLE:
	chanAddIdleSessn(ps->actSvrHostMap, ps->actSvrKeyMap, ps->idleSvrMap,
			 s);
	break;

      case DISABLED:
	chanDestroy(s);
	break;

      default:
	xTrace1(chanp, TR_ERRORS, "chanServerClose -- unknown state %s",
		chanStateStr(ss->cur_state));
	break;
    }

    return XK_SUCCESS;
}
    

static void
closeStub( ev, arg )
    Event	ev;
    VOID	*arg;
{
    xAssert( xIsSession((XObj)arg) );
    xClose( (XObj)arg );
}


/* 
 * Response to a peer reboot notification.  If this session has the
 * appropriate peer, put it in a
 * 'zombie' state where it is not in any map and is only waiting to be
 * shut down.
 *
 * This function must not block.
 */
static int
zombifyServer( key, val )
    VOID	*key;
    VOID	*val;
{
    XObj	s = (XObj)val;
    CHAN_STATE	*ss;
    PSTATE	*ps;
    FsmState	oldState;
    xkern_return_t	res;

    xAssert(xIsSession(s));
    ps = (PSTATE *)xMyProtl(s)->state;
    ss = (CHAN_STATE *)s->state;
    xTrace1(chanp, TR_EVENTS,
	    "chan serverDisableSessn -- zombifying session %x", s);
    /* 
     * Remove session from 'key' map.  It will be removed from the
     * 'host' map by mapForEach.
     */
    xAssert(s->binding);
    res = mapRemoveBinding(ps->actSvrKeyMap, s->binding);
    xAssert( res == XK_SUCCESS );
    s->binding = 0;

    oldState = ss->cur_state;
    ss->cur_state = DISABLED;
    ss->hdr.seq++;	/* Make sure replies don't go out */
    chanEventFlush(s);
    msg_flush(ss->saved_msg);
    if ( oldState == SVC_WAIT ) {
	evDetach(evSchedule(closeStub, (VOID *)s, 0));
    }
    return MFE_CONTINUE | MFE_REMOVE;
}



/* 
 * In response to notification that a peer rebooted, destroy idle
 * server sessions and disable active server sessions to that host.
 * This function must not block.
 */
void
chanServerPeerRebooted( ps, peer )
    PSTATE	*ps;
    IPhost	*peer;
{
    xTrace0(chanp, TR_EVENTS, "removing server sessions for rebooted host");

    chanMapChainApply(ps->idleSvrMap, peer, chanMapRemove);
    chanMapChainApply(ps->actSvrHostMap, peer, zombifyServer);
}


static void 
getProcServer(s)
    XObj s;
{
    xAssert(xIsSession(s));
    s->close 	= chanServerClose;
    s->pop 	= chanServerPop; 
    s->control 	= chanControlSessn;
}

