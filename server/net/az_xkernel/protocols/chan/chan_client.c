/*
 * chan_client.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.43 $
 * $Date: 1993/02/01 22:30:43 $
 */

#include "xkernel.h"
#include "chan_internal.h"
#include "chan.h"


#ifdef __STDC__

static void		abortCall( XObj );
static xkern_return_t	chanCall( XObj, Msg *, Msg * );
static xkern_return_t	chanClientClose( XObj );
static int		chanClientControl( XObj, int, char *, int );
static ChanOpenFunc	chanClientOpen;
static xkern_return_t	chanClientPop( XObj, XObj, Msg *, VOID * );
static xkern_return_t	errorCall( XObj, Msg *, Msg * );
static XObj		getAnyIdleSessn( XObj, XObj, XObj, ActiveID * );
static void 		getProcClient( XObj );
static int		returnFirstElem( VOID *, VOID *, VOID * );
static int		zombifyClient( VOID *, VOID * );

#else

static xkern_return_t	addIdleSessn();
static void		abortCall();
static xkern_return_t	chanCall();
static xkern_return_t	chanClientClose();
static int		chanClientControl();
static XObj		chanClientOpen();
static xkern_return_t	chanClientPop();
static xkern_return_t	errorCall();
static XObj		getAnyIdleSessn();
static void 		getProcClient();
static int		returnFirstElem();
static int		zombifyClient();

#endif  __STDC__



/* 
 * chanOpen
 */
XObj 
chanOpen( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    XObj   	s, lls;
    ActiveID 	key;
    IPhost	peer;
    Map		chanMap;
    Channel	*chanPtr;
    
    xTrace0(chanp, TR_MAJOR_EVENTS,
	    "CHAN open ..............................");
    if ( (key.prot_id = chanGetProtNum(self, hlpType)) == -1 ) {
	return ERR_XOBJ;
    }
    /*
     * Open lower session
     */
    lls = xOpen(self, self, xGetDown(self, 0), p);
    if ( lls == ERR_XOBJ) {
	xTrace0(chanp, TR_SOFT_ERRORS,
		"chan_open: can't open lower session");
	return ERR_XOBJ;
    }
    key.lls = lls;
    /* 
     * See if there is an idle channel for this host
     */
    if ( (s = getAnyIdleSessn(self, hlpRcv, hlpType, &key)) != ERR_XOBJ ) {
	xTrace1(chanp, TR_MAJOR_EVENTS, "chanOpen returning idle session %x",
		s);
	xClose(lls);
	return s;
    }
    if ( xControl(lls, GETPEERHOST, (char *)&peer, sizeof(IPhost))
				< (int)sizeof(IPhost) ) {
	xTrace0(chanp, TR_ERRORS,
		"chan_open: can't do GETPEERHOST on lower session");
	return ERR_XOBJ;
    }
    if ( (chanMap = chanGetChanMap(self, &peer)) == 0 ) {
	return ERR_XOBJ;
    }
    /* 
     * Get a new channel number
     */
    if ( mapResolve(chanMap, &key.prot_id, &chanPtr) == XK_FAILURE ) {
	Bind	b;

	xTrace2(chanp, TR_MAJOR_EVENTS, "first channel to peer %s, hlp %d",
		ipHostStr(&peer), key.prot_id);
	key.chan = FIRST_CHAN;
	chanPtr = X_NEW(Channel);
	*chanPtr = FIRST_CHAN + 1;
	b = mapBind(chanMap, &key.prot_id, (int)chanPtr);
	xAssert( b != ERR_BIND );
    } else {
	key.chan = *chanPtr;
	(*chanPtr)++;
    }
    xIfTrace(chanp, TR_MAJOR_EVENTS) chanDispKey(&key);
    s = chanClientOpen(self, hlpRcv, hlpType, &key, START_SEQ);
    xClose(lls);
    return s;
}


/* 
 * Creates a new client session based on the given key and sequence number
 */
static XObj
chanClientOpen( self, hlpRcv, hlpType, key, seq )
    XObj	self, hlpRcv, hlpType;
    ActiveID	*key;
    int		seq;
{
    PSTATE 	*ps = (PSTATE *)self->state;
    XObj	s;

    s = chanCreateSessn(self, hlpRcv, hlpType, key, getProcClient,
			ps->actCliKeyMap, ps->actCliHostMap);
    if ( s != ERR_XOBJ ) {
	CHAN_STATE	*ss = (CHAN_STATE *)s->state;
	ss->cur_state = CLNT_FREE;
	ss->hdr.flags = FROM_CLIENT | USER_MSG;
	ss->hdr.seq = seq;
	ss->waitParam = CLIENT_WAIT_DEFAULT;
	ss->maxWait = CLIENT_MAX_WAIT_DEFAULT;
    }
    return s;
}


/* 
 * chanCall
 */
static xkern_return_t
chanCall(self, msg, rmsg)
    XObj self;
    Msg *msg;
    Msg *rmsg;
{
    CHAN_STATE   	*state;
    CHAN_HDR 	*hdr;
    int 		packet_len;
    XObj		lls;
    
    xTrace0(chanp, TR_EVENTS, "CHAN call ..............................");
    
    state = (CHAN_STATE *)self->state;
    xTrace1(chanp, TR_MORE_EVENTS, "chan_call: state = %x", state);
    xTrace1(chanp, TR_MORE_EVENTS,
	    "chan_call: outgoing length (no chan hdr): %d", msgLen(msg));
    
    if ((state->cur_state != CLNT_FREE)) {
	xTrace0(chanp, TR_SOFT_ERRORS, "chan_call: incorrect initial state");
	return XK_FAILURE;
    }
    
    msg_flush(state->saved_msg);
    /*
     * Save data (without header) for retransmit
     */
    msgConstructCopy(&state->saved_msg.m, msg);
    msg_valid(state->saved_msg);
    
    /*
     * Fill in header
     */
    state->hdr.seq++;
    hdr 		= &state->hdr;
    hdr->len 	= msgLen(msg);
    
    xIfTrace(chanp, TR_MORE_EVENTS) { 
	pChanHdr(hdr);
    } 
    
    msgPush(msg, chanHdrStore, hdr, CHANHLEN, NULL);
    
    lls = xGetDown(self, 0);
    xAssert(xIsSession(lls));
    xTrace1(chanp, TR_MORE_EVENTS, "chan_call: s->push  = %x", lls->push); 
    xTrace1(chanp, TR_MORE_EVENTS, "chan_call: packet len %d", msgLen(msg)); 
    xTrace1(chanp, TR_MORE_EVENTS, "chan_call: length field: %d", hdr->len);
    xTrace2(chanp, TR_MORE_EVENTS, "chan_call: lls %x, packet = %x",
	    lls, msg); 
    xAssert(rmsg);
    state->answer = rmsg;
    /*
     * Send message
     */
    packet_len    = msgLen(msg);
    state->ticket = xPush(lls, msg);
    if (state->ticket < 0) {
	xTrace0(chanp, TR_ERRORS, "chan_call: can't send message");
	msg_flush(state->saved_msg);
	return XK_FAILURE;
    }
    state->cur_state 	= CLNT_WAIT;
    state->wait 	=  CHAN_CLNT_DELAY(packet_len, state->waitParam);
    xTrace1(chanp, TR_DETAILED, "chan_call: client_wait = %d", state->wait);
    state->event = evSchedule(chanTimeout, self, state->wait);
    /*
     * Wait for reply
     */
    semWait(&state->reply_sem);
    msg_flush(state->saved_msg);
    return state->replyValue;
}


/*
 * chanClientPop
 */
static xkern_return_t
chanClientPop(self, lls, msg, inHdr)
    XObj self;
    XObj lls;
    Msg *msg;
    VOID *inHdr;
{
    CHAN_STATE 	*state;
    CHAN_HDR	*hdr = (CHAN_HDR *)inHdr;
    u_int 	seq;
    SEQ_STAT 	status;
    
    xTrace0(chanp, TR_EVENTS, "CHAN Client Pop");
    xAssert(hdr);
    state = (CHAN_STATE *)self->state;
    seq = hdr->seq;
    status = chanCheckSeq(state->hdr.seq, seq);
    xTrace4(chanp, TR_MORE_EVENTS,
	    "state: %s  cur_seq = %d, hdr->seq = %d  status: %s",
	    chanStateStr(state->cur_state), state->hdr.seq, hdr->seq,
	    chanStatusStr(status));
    if ( chanCheckMsgLen(hdr->len, msg) ) {
	return XK_SUCCESS;
    }
    xAssert(hdr->chan == state->hdr.chan);
    switch(state->cur_state) {
	
      case CLNT_WAIT:
	if (status != current) {
	    xTrace0(chanp, TR_SOFT_ERRORS,
		    "chan_pop: CLNT_WAIT: wrong seqence number");
	    break;
	}
	if ( hdr->flags & USER_MSG ) {
	    xTrace0(chanp, TR_EVENTS, "chan_pop: CLNT_WAIT: Received reply"); 
	    chanFreeResources(self);
	    /*
	     * Return results
	     */
	    state->replyValue = XK_SUCCESS;
	    msgAssign(state->answer, msg);
	    semSignal(&state->reply_sem);
	    state->cur_state = CLNT_FREE;
	    xTrace0(chanp, TR_MORE_EVENTS, "chan_pop: CLNT_WAIT: returns OK");
	    /* 
	     * Fall-through to CLNT_FREE to handle ACK (if appropriate)
	     */
	} else {
	    chanEventFlush(self);
	    if ( hdr->flags & NEGATIVE_ACK ) {
		state->ticket = chanResend(self, FROM_CLIENT, 1);
		state->event = evSchedule(chanTimeout, self, state->wait);
	    } else {
		/* 
		 * Msg is an ACK of the current request
		 */
		if ( hdr->flags & ACK_REQUESTED ) {
		    /* 
		     * We missed the reply
		     */
		    chanReply(lls, hdr, FROM_CLIENT | NEGATIVE_ACK );
		} else {
		    xTrace0(chanp, TR_MORE_EVENTS,
			    "chan_pop: CLNT_WAIT: received SVC_EXPLICIT_ACK");
		}
	    }
	    break;
	}
	
      case CLNT_FREE:
	chanClientIdleRespond(hdr, xGetDown(self, 0), state->hdr.seq);
	break;
	
     default:
	xTrace1(chanp, TR_ERRORS, "chan_pop: invalid state %d",
		state->cur_state);
	break;
    } 
    xTrace2(chanp, TR_MORE_EVENTS, "end chanClientPop:  state: %s curSeq = %d",
	    chanStateStr(state->cur_state), state->hdr.seq);
    return XK_SUCCESS;
}


/* 
 * A message came in for a dormant client channel.  'seq' is the current
 * sequence number for this channel.  The message header may be modified.
 */
void
chanClientIdleRespond( hdr, lls, seq )
    CHAN_HDR	*hdr;
    XObj	lls;
    u_int	seq;
{
    xAssert( xIsXObj(lls) );
    xAssert( ! (hdr->flags & FROM_CLIENT) );
    if ( hdr->seq < seq ) {
	xTrace0(chanp, TR_EVENTS, "idle client channel receives old seq num");
	return;
    }
    if ( hdr->seq > seq ) {
	xTrace0(chanp, TR_ERRORS,
		"chan idle client receives new sequence number!");
	return;
    }
    if ( hdr->flags & ACK_REQUESTED ) {
	xTrace0(chanp, TR_MORE_EVENTS, "Idle client sending explicit ACK");
	hdr->seq++;
	chanReply(lls, hdr, FROM_CLIENT);
    }
}


/* 
 * Unblock the current calling thread and have it return with a
 * failure code.  The client session goes into 'idle' state.
 */
static void
abortCall( self )
    XObj	self;
{

    CHAN_STATE	*ss = (CHAN_STATE *)self->state;

    xTrace0(chanp, TR_EVENTS, "chan clientAbort");
    if ( ss->cur_state != CLNT_WAIT ) {
	xTrace1(chanp, TR_SOFT_ERRORS,
		"chan client abort fails, state is %s",
		chanStateStr(ss->cur_state));
	return;
    }
    chanEventFlush(self);
    ss->replyValue = XK_FAILURE;
    ss->cur_state = CLNT_FREE;
    semSignal(&ss->reply_sem);
}


typedef struct {
    int		seq;
    Channel	chan;
} IdleMapEntry;


static int
returnFirstElem( key, val, arg )
    VOID	*key, *val, *arg;
{
    IdleMapEntry	*e = (IdleMapEntry *)arg;

    e->chan = *(Channel *)key;
    e->seq = (int)val;
    return 0;
}


/* 
 * Recreates any idle session for the appropriate host, unbinding it from
 * the idle client map and binding it to the active client map.
 * 'key->prot_id' and 'key->lls'
 * should be valid.  'key->chan' will be filled in if an appropriate
 * session is found.
 */
static XObj
getAnyIdleSessn( self, hlpRcv, hlpType, key )
    XObj	self, hlpRcv, hlpType;
    ActiveID	*key;
{
    PSTATE		*ps = (PSTATE *)self->state;
    Map			map;
    IdleMapEntry	e;
    XObj		s;
#ifdef XK_DEBUG
    int			seqCheck;
#endif

    map = chanGetMap(ps->idleCliMap, key->lls, key->prot_id);
    if ( ! map ) {
	xTrace0(chanp, TR_EVENTS, "chanGetAnyIdleSessn -- no map");
	return ERR_XOBJ;
    }
    e.seq = -1;
    mapForEach(map, returnFirstElem, &e);
    if ( e.seq == -1 ) {
	return ERR_XOBJ;
    } 
    /* 
     * Transfer from idle to active map
     */
    xAssert( mapResolve(map, &e.chan, &seqCheck) == XK_SUCCESS && 
	     seqCheck == e.seq );
    mapUnbind(map, (char *)&e.chan);
    key->chan = e.chan;
    s = chanClientOpen(self, hlpRcv, hlpType, key, e.seq);
    if ( s == ERR_XOBJ ) {
	/* 
	 * This shouldn't happen
	 */
	mapBind(map, (char *)&e.chan, e.seq);
    }
    return s;
}


static xkern_return_t
chanClientClose( s )
    XObj	s;
{
    CHAN_STATE	*ss = (CHAN_STATE *)s->state;
    PSTATE	*ps = (PSTATE *)xMyProtl(s)->state;

    xTrace1(chanp, TR_MAJOR_EVENTS, "chan client close of sessn %x", s);
    switch ( ss->cur_state ) {
      case CLNT_WAIT:
	/* 
	 * This shouldn't happen
	 */
	xTrace0(chanp, TR_ERRORS,
		"chanClientClose called in CLNT_WAIT state!!!");
	chanEventFlush(s);
	abortCall(s);
	break;

      case CLNT_FREE:
	xTrace0(chanp, TR_MAJOR_EVENTS, "chanClientClose marking sessn idle");
	return chanAddIdleSessn(ps->actCliHostMap, ps->actCliKeyMap,
				ps->idleCliMap, s);

      case DISABLED:
	xTrace0(chanp, TR_MAJOR_EVENTS, "chanClientClose destroying sessn");
	chanDestroy(s);
	break;

      default:
	xTrace1(chanp, TR_ERRORS,
		"chanClientClose called with unhandled state type %s",
		chanStateStr(ss->cur_state));
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static int
chanClientControl( self, op, buf, len )
    XObj	self;
    int		op, len;
    char 	*buf;
{
    switch ( op ) {
	
      case CHAN_ABORT_CALL:
#ifdef CHAN_ALLOW_USER_ABORT
	abortCall(self);
#endif
	return 0;

      default:
	return chanControlSessn( self, op, buf, len );
    }
}


/* 
 * Used as the 'call' function for disabled sessions
 */
static xkern_return_t
errorCall( s, m, rm )
    XObj	s;
    Msg		*m, *rm;
{
    return XK_FAILURE;
}


/* 
 * Disable the session 'val'.  It is removed from the map, it's caller
 * is released with failure, and any subsequent calls will fail.
 * This function must not block.
 */
static int
zombifyClient( key, val )
    VOID	*key;
    VOID	*val;
{
    XObj	s = (XObj)val;
    PState	*ps;
    CHAN_STATE	*ss;
    xkern_return_t	res;
    
    xAssert(xIsSession(s));
    ss = (CHAN_STATE *)s->state;
    ps = (PState *)xMyProtl(s)->state;
    xTrace1(chanp, TR_EVENTS,
	    "chanZombifyClient -- disabling session %x", s);
    if ( ss->cur_state == CLNT_WAIT ) {
	xTrace0(chanp, TR_EVENTS, "chanZombifyClient -- releasing caller");
	abortCall(s);
    } else {
	xAssert( ss->cur_state == CLNT_FREE );
    }
    ss->cur_state = DISABLED;
    /* 
     * Remove session from 'key' map.  It will be removed from the
     * 'host' map by mapForEach.
     */
    xAssert(s->binding);
    res = mapRemoveBinding(ps->actCliKeyMap, s->binding);
    xAssert( res == XK_SUCCESS );
    s->binding = 0;
    s->call = errorCall;
    return MFE_CONTINUE | MFE_REMOVE;
}


/* 
 * In response to notification that a peer rebooted, destroy idle
 * client sessions and disable active client sessions to that host.
 * This function must not block.
 */
void
chanClientPeerRebooted( ps, peer )
    PSTATE	*ps;
    IPhost	*peer;
{
    xTrace0(chanp, TR_EVENTS, "removing client sessions for rebooted host");
    chanMapChainApply(ps->idleCliMap, peer, chanMapRemove);
    chanMapChainApply(ps->actCliHostMap, peer, zombifyClient);
}


static void 
getProcClient(s)
    XObj s;
{
    xAssert(xIsSession(s));
    s->close 	= chanClientClose;
    s->call 	= chanCall;
    s->pop 	= chanClientPop; 
    s->control 	= chanClientControl;
}


