/*
 * blast_output.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.31 $
 * $Date: 1993/02/01 22:20:02 $
 */

#include "blast_internal.h"

#ifdef __STDC__

static void	blastSendTimeout( Event, void * );

#else

static void	blastSendTimeout();

#endif __STDC__


static xmsg_handle_t
sendShortMsg(m, state)
    Msg *m;
    SState *state;  
{
    state->short_hdr.len = msgLen(m);
    xIfTrace(blastp, TR_DETAILED) {
	blast_phdr(&state->short_hdr);
    }
    xTrace1(blastp, TR_DETAILED, "in blast_push2 down_s = %x",
	    xGetDown(state->self, 0));
    msgPush(m, blastHdrStore, &state->short_hdr, BLASTHLEN, 0);
    xIfTrace(blastp,TR_DETAILED) {
	xTrace0(blastp, TR_ALWAYS, "message in header");
	blast_phdr(&state->short_hdr);
    }
    xTrace1(blastp, TR_DETAILED, "in blast_push2 msg len = %d", msgLen(m));
    if (xPush(xGetDown(state->self, 0), m) ==  -1) {
	xTrace0(blastp, TR_ERRORS, "blast_push: can't send message");
	return XMSG_ERR_HANDLE;
    }
    return XMSG_NULL_HANDLE;
}


static void
push_fragment(s, frag, h)
    XObj s;
    Msg *frag;
    BLAST_HDR *h;
{
    Msg msgToPush;

    xIfTrace(blastp, TR_DETAILED) {
	xTrace0(blastp, TR_ALWAYS, "Outgoing message ");
	blast_phdr(h);
    }
    msgConstructCopy(&msgToPush, frag);
    msgPush(&msgToPush, blastHdrStore, h, BLASTHLEN, 0);
    if (xPush(s, &msgToPush) ==  -1) {
	xTrace0(blastp, TR_ERRORS, "blast_push: can't send fragment");
    }
    msgDestroy(&msgToPush);
}

    	
xmsg_handle_t
blastPush(s, msg)
    XObj s;
    Msg *msg;
{
    SState	*state;
    PState 	*pstate;
    MSG_STATE  	*mstate;
    BLAST_HDR 	*hdr;
    int 	seq;
    int 	num_frags;
    int 	i;
    XObj	lls;
    
    xTrace0(blastp, TR_EVENTS, "in blast_push");
    xAssert(xIsSession(s));

    pstate = (PState *) s->myprotl->state;
    state = (SState *) s->state;
    xTrace1(blastp, TR_MORE_EVENTS, "blast_push: state = %d", state);
    xTrace1(blastp, TR_MORE_EVENTS,
	    "blast_push: outgoing msg len (no blast hdr): %d",
	    msgLen(msg));

    if (msgLen(msg) > state->fragmentSize * BLAST_MAX_FRAGS ) {
	xTrace2(blastp, TR_SOFT_ERRORS,
		"blast_push: message length %d > max size %d",
	       msgLen(msg), state->fragmentSize * BLAST_MAX_FRAGS);
	return XMSG_ERR_HANDLE;
    }
    
    /* if message is short, by-pass everthing */
    if ( msgLen(msg) < state->fragmentSize ) {
	return sendShortMsg(msg, state);
    }
    
    xTrace0(blastp, TR_DETAILED, "in blast_push3");
    
    /* get new sequence number */
    if(++pstate->max_seq <= 0) {
	/* once every eon the seqnumber with wrap*/
	xError("blast_push: WORLD ENDS!");
	return XMSG_ERR_HANDLE;
    }
    seq = pstate->max_seq;
    
    /* need new message state */
    mstate = blastNewMstate(s);
    /* 
     * Add a reference count for this message
     */
    state->ircnt++;
    
    /* bind mstate to seqence number */
    mstate->binding = mapBind(state->send_map, (char *)&seq, mstate);
    if (mstate->binding == ERR_BIND) {
	xTrace0(blastp, TR_ERRORS, "blast_push: can't bind seqence number");
	if (mstate) xFree((char *)mstate);
	return XMSG_ERR_HANDLE;
    } 
    mstate->state = state;
    
    /* fill in header */
    hdr = &(mstate->hdr);
    *hdr = state->short_hdr;
    hdr->seq = seq;
    xIfTrace(blastp, TR_DETAILED) {
	xTrace0(blastp, TR_ALWAYS, "blasts_push: static header:");
	blast_phdr(hdr);
    }
    
    num_frags = (msgLen(msg) + state->fragmentSize - 1)/(state->fragmentSize);
    hdr->num_frag = num_frags;
    BLAST_MASK_SET_BIT(&hdr->mask, 1);
    
    /* send off fragments */
    xTrace1(blastp, TR_MORE_EVENTS, "blast: outstanding messages (P): %d",
	    pstate->outstanding_messages.count);
    semWait(&pstate->outstanding_messages);
    lls = xGetDown(s, 0);
    for ( i=1; i <= num_frags; i++ ) {
	Msg *packet = &mstate->frags[i];
	msgConstructEmpty(packet);
	msgChopOff(msg, packet, state->fragmentSize);
	xTrace2(blastp, TR_MORE_EVENTS,
		"fragment: len[%d] = %d", i, msgLen(packet));
	/* fill in dynamic parts of header */
	hdr->len = msgLen(packet);
	xTrace1(blastp, TR_MORE_EVENTS, "blast_push: sending frag %d", i);
	push_fragment(lls, packet, hdr);
	BLAST_MASK_SHIFT_LEFT(hdr->mask, 1);
    }
    /* set timer to free message if no NACK is received */
    mstate->wait = SEND_CONST * num_frags;
    xTrace1(blastp, TR_MORE_EVENTS,
	    "scheduling blastSendTimeout for mstate %x", mstate);
    mstate->event = evSchedule(blastSendTimeout, mstate, mstate->wait);
    return seq;
}


/*
 * Retransmit the message referenced by mstate.  1's in 'mask' indicate
 * fragments received, 0's indicate fragments that need to be retransmitted.
 */
static void
retransmit(mask, mstate)
    BlastMask	mask;
    MSG_STATE 	*mstate;
{
    int 	i;
    SState	*state;
    BLAST_HDR 	*hdr;
    XObj	lls;
    
    xTrace2(blastp, REXMIT_T, "blast retransmit: called seq = %d mask = %s",
	    mstate->hdr.seq, blastShowMask(mask));
    if (mstate == 0) {
	xTrace0(blastp, REXMIT_T, "retransmit: no message state");
	return;
    }
    if ((state = (SState *)mstate->state) == 0) {
	xTrace0(blastp, TR_ERRORS, "retransmit: no state");
	return;
    }
    hdr = &(mstate->hdr);
    hdr->op = BLAST_RETRANSMIT;
    /* send off fragments */
    lls = xGetDown(mstate->state->self, 0);
    for ( i=1; i<=hdr->num_frag; i++ ) {
	if ( ! BLAST_MASK_IS_BIT_SET(&mask, i) ) {
	    Msg *packet = &mstate->frags[i];
	    xTrace2(blastp, REXMIT_T,
		    "retransmit: retransmitting fragment %d, msgptr %x",
		    i, packet);
	    /* fill in dynamic parts of header */
	    BLAST_MASK_CLEAR(hdr->mask);
	    BLAST_MASK_SET_BIT(&hdr->mask, i);
	    hdr->len = msgLen(packet);
	    push_fragment(lls, packet, hdr);
	}
    }
}



/*
 * Free the message and state associated with the outgoing message referenced
 * by mstate.  Cancels the timeout event.
 */
static void
freeMstate(mstate)
    MSG_STATE *mstate;
{
    PState	*pstate;
    SState	*sstate;
    int		i;
    XObj	sessn;

    xTrace2(blastp, TR_MORE_EVENTS,
	    "blast output freeMstate, seq == %d, mstate == %x",
	    mstate->hdr.seq, mstate);
    sstate = mstate->state;
    sessn = sstate->self;
    pstate = (PState *)xMyProtl(sstate->self)->state;
    if (mstate->binding) {
	if (mapRemoveBinding(sstate->send_map, mstate->binding)
	    						== XK_FAILURE) {
	    xTrace0(blastp, TR_ERRORS, "free_send_seq: can't unbind!");
	}
    }
    if ( mstate->event ) {
	evCancel(mstate->event);
	mstate->event = 0;
    }
    xTrace1(blastp, TR_MORE_EVENTS, "blast freeMstate: num_frags = %d",
	    mstate->hdr.num_frag);
    for ( i = 1; i <= mstate->hdr.num_frag; i++ ) {
	msgDestroy(&mstate->frags[i]);
    }
    if ( stackPush(pstate->mstateStack, (VOID *)mstate) ) {
	xTrace1(blastp, TR_MORE_EVENTS,
		"free_send_seq: mstate stack is full, freeing %x", mstate);
	xFree((char *)mstate);
    }
    xTrace1(blastp, TR_MORE_EVENTS, "blast: outstanding messages (v): %d",
	    pstate->outstanding_messages.count);
    semSignal(&pstate->outstanding_messages);
    /* 
     * Remove ref count corresponding to this message
     */
    blastDecIrc(sessn);
}


/* 
 * Kill the timeout event and free the outgoing message state
 */
int
blast_freeSendSeq(state, seq)
    SState *state;
    int seq;
{
    MSG_STATE	*mstate;
    
    xTrace1(blastp, TR_MORE_EVENTS, "free_send_msg: begin seq = %d", seq);
    if ( mapResolve(state->send_map, &seq, &mstate) == XK_FAILURE ) {
	xTrace0(blastp, TR_SOFT_ERRORS, "free_send_seq: nothing to free");
	return -1;
    } 
    freeMstate(mstate);
    return 0;
}


/*
 * send_timout garbage collects the storage associated with 
 * a given sequence number. Since blast uses nacks it has 
 * to have some way of getting rid of storage 
 */
static void
blastSendTimeout(ev, arg)
    Event	ev;
    VOID 	*arg;
{
    MSG_STATE	*mstate = (MSG_STATE *)arg;

    xTrace0(blastp, REXMIT_T, "blast: send_timeout: timeout!");
    freeMstate(mstate);
}

    
/* 
 * blastSenderPop -- process a retransmission request from out peer
 */
xkern_return_t
blastSenderPop(s, dg, hdr)
    XObj s;
    Msg *dg;
    BLAST_HDR *hdr;
{
    SState	*state;
    MSG_STATE 	*mstate;
    
    xTrace0(blastp, TR_EVENTS, "BLAST_pop");
    xAssert(xIsSession(s));
    
    state = (SState *)s->state;
    /*
     * look for message state
     */
    if ( mapResolve(state->send_map, &hdr->seq, &mstate) == XK_FAILURE ) {
	xTrace0(blastp, TR_EVENTS, "blast_pop: unmatched nack ");
	return XK_SUCCESS;
    }
    /* 
     * Cancel and restart timeout event
     */
    evCancel(mstate->event);
    retransmit(hdr->mask, mstate);
    xTrace1(blastp, TR_MORE_EVENTS,
	    "rescheduling blastSendTimeout for mstate %x", mstate);
    mstate->event = evSchedule(blastSendTimeout, mstate, mstate->wait);
    return XK_SUCCESS;
} 


