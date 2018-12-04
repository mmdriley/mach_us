/* 
 * sunrpc_client.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.18 $
 * $Date: 1993/02/01 22:29:41 $
 */

#include "xkernel.h"
#include "sunrpc_i.h"
#include "xrpc_print.h"
#include "xkpm.h"

#ifdef __STDC__

static void	rpc_timeout( Event, void * );
static u_long	get_xid( void );

#else

static void	rpc_timeout();
static u_long	get_xid();

#endif __STDC__

extern long yabcopy();
long xkxdr_long_decode();


static void
translate_error(hdr, err)
    struct rpc_msg *hdr;
    struct rpc_err *err;
{
    err->re_status = RPC_SUCCESS;
    if (hdr->rm_direction == CALL) {
	xTrace0(sunrpcp,3,"translate_error: call msg cant be error!");
	return;
    }
    if (hdr->rm_reply.rp_stat == MSG_ACCEPTED) {
	switch(hdr->rm_reply.rp_acpt.ar_stat)   {
	  case SUCCESS:
	    err->re_status = RPC_SUCCESS;
	    break;
	  case PROG_UNAVAIL:
	    err->re_status = RPC_PROGUNAVAIL;
	    break;
	  case PROG_MISMATCH:
	    err->re_status = RPC_PROGVERSMISMATCH;
	    err->re_vers.low = hdr->rm_reply.rp_acpt.ar_vers.low; 
	    err->re_vers.high = hdr->rm_reply.rp_acpt.ar_vers.high; 
	    break;
	  case PROC_UNAVAIL:
	    err->re_status = RPC_PROCUNAVAIL;
	    break;
	  case GARBAGE_ARGS:
	    err->re_status = RPC_CANTDECODEARGS;
	    break;
	  case SYSTEM_ERR:
	    err->re_status = RPC_SYSTEMERROR;
	    break;
	  default:
	    xTrace0(sunrpcp,3,"translate_error: invalid accept status\n");
	    err->re_status = RPC_SYSTEMERROR;
	    break;
	}
    } else {
	if (hdr->rm_reply.rp_rjct.rj_stat = RPC_MISMATCH) {
	    err->re_status = RPC_VERSMISMATCH;
	    err->re_vers.low = hdr->rm_reply.rp_rjct.rj_vers.low; 
	    err->re_vers.high = hdr->rm_reply.rp_rjct.rj_vers.high; 
	} else if (hdr->rm_reply.rp_rjct.rj_stat = AUTH_ERROR) {
	    err->re_status = RPC_AUTHERROR;
	    err->re_why = hdr->rm_reply.rp_rjct.rj_why; 
	} else {
	    xTrace0(sunrpcp,3,"translate_error: invalid reject status\n");
	    err->re_status = RPC_SYSTEMERROR;
	}
    }
}


xkern_return_t
sunrpcCall(s, msg, reply_ptr)
    XObj s;
    Msg *msg;
    Msg *reply_ptr;
{
    SState	*state;
    SunrpcHdr	*hdr;
    char procbuf[sizeof(long)]; XKXDR xdrs; long len, proc;
    
    xTrace0(sunrpcp, 3, "in sunrpcCall");
    state = (SState *)s->state;
    hdr = &state->hdr;
    hdr->rm_xid = get_xid();
    if ((len=msgLen(msg))<sizeof(long))
    { if (len)
      { xTrace1(sunrpcp, TR_ERRORS,
		"sunrpcCall: no procedure; message too short, %d bytes", len);
	return XK_FAILURE;}
      proc = 0;
      xTrace0(sunrpcp, TR_FULL_TRACE,
	      "sunrpcCall: treating 0 length message as ping, procedure 0");}
    else
    { msgPop(msg,yabcopy,procbuf,sizeof(long),0);
      xkxdr_init(xdrs,procbuf,sizeof(long),XDR_DECODE);
      proc = xkxdr_long_decode(&xdrs); /* snarf 1 word from xdr'd message */ };
    hdr->rm_call.cb_proc = proc;
    xIfTrace(sunrpcp,7) {
	prpchdr(*hdr, "sunrpcCall");
    }
    xTrace0(sunrpcp, 3, "in sunrpcCall going to encode header ");
    if (sunrpcEncodeHdr(msg, hdr) == -1) {
	xTrace0(sunrpcp, 3, "sunrpcCall: can't encode header");
	xTrace0(sunrpcp, 9, "error: 1");
	return XK_FAILURE;
    }
    xTrace0(sunrpcp, 3, "in sunrpcCall after encoding header ");
    /*
     * set up retry message
     */
    msgAssign(&state->c_callMsg, msg);
    s->binding = mapBind(((PSTATE *)s->myprotl->state)->replyMap,
			 &hdr->rm_xid, s);
    state->c_replyMsg = reply_ptr;
    /*
     * default result
     */
    state->c_error.re_status = RPC_TIMEDOUT;
    /*
     * push message to udp
     */
    if (xPush(xGetDown(s, 0), msg) != 0) {
	xTrace0(sunrpcp, 1, "sunrpcCall received error from llp push");
	return XK_FAILURE;
    }
    /*
     * Setup timeout stuff
     */
    state->c_tries = state->c_tout / state->c_wait;
    xTrace2(sunrpcp, 5, "rpc_push waiting %d msecs %d times for reply",
	    state->c_wait / 1000, state->c_tries); 
    xDuplicate(s);
    state->c_timeoutEvent = evSchedule(rpc_timeout, s, state->c_wait);
    /*
     * wait for answer
     */
    xTrace0(sunrpcp, 5, "sunrpcCall blocking for reply");
    semWait(&state->c_replySem);
    if (state->c_error.re_status == RPC_SUCCESS) {
	xTrace0(sunrpcp, 3, "SUNRPC SUCCESS");
	return XK_SUCCESS;
    } else {
	xTrace0(sunrpcp, 3, "SUNRPC ERROR");
	return XK_FAILURE;
    }
}


xkern_return_t
sunrpcClientDemux(self, lls, dg, hdr)
    XObj self;
    XObj lls;
    Msg *dg;
    SunrpcHdr *hdr;
{
    XObj 	rpc_s;
    SState	*state;
    PSTATE	*pstate;

    pstate = (PSTATE *)self->myprotl->state;
    if ( mapResolve(pstate->replyMap, &hdr->rm_xid, &rpc_s) == XK_FAILURE ) {
	xTrace0(sunrpcp, 3, "Reply found for nonexistent session");
	/* punt */
	xTrace0(sunrpcp, 9, "error 3");
	return XK_FAILURE;
    }
    xTrace1(sunrpcp, 3, "SUNRPC found client session %x", rpc_s);
    state = (SState *)rpc_s->state;
    mapRemoveBinding(pstate->replyMap, rpc_s->binding);
    if (state->hdr.rm_xid != hdr->rm_xid) {
	/* old transaction */
	xTrace0(sunrpcp, 0, "rpc_demux: transaction id mismatch");
	return XK_FAILURE;
    }
    /*
     * extract error information from header and put into state
     */
    translate_error(hdr, &state->c_error);
	
    if (hdr->rm_reply.rp_stat == MSG_ACCEPTED) {
	sunrpcAuthFree(&(state->hdr.rm_call.cb_verf));
	state->hdr.rm_call.cb_verf = hdr->acpted_rply.ar_verf;
	hdr->acpted_rply.ar_verf = sunrpcAuthDummy;
    } 
    /*
     * Deposit incoming reply message in the client's
     * state info and then unblock the client.
     */
    xTrace1(sunrpcp,7,"rpc_demux after decode: msg_len= %d\n",
	    msgLen(dg));
    
    if (state->c_error.re_status == RPC_SUCCESS) {
	msgAssign(state->c_replyMsg, dg);
    } else {
	xTrace0(sunrpcp, 0, "SUNRPC CALL FAILED");
    }
    /*
     * remove timeout
     */
    if (evCancel(state->c_timeoutEvent)) {
	/*
	 * Remove reference count associated with the event
	 */
	xTrace0(sunrpcp, 7,
		"sunrpc client demux successfully removed timeout");
	xTrace2(sunrpcp, 7, "sunrpc session %x before close rcnt == %d",
		rpc_s, rpc_s->rcnt);
	xClose(rpc_s);
	xTrace2(sunrpcp, 7, "sunrpc session %x after close rcnt == %d",
		rpc_s, rpc_s->rcnt);
    } else {
	/*
	 * We couldn't cancel the event.  Make sure it doesn't
	 * actually do anything
	 */
	xTrace0(sunrpcp, 7,
		"sunrpc client demux could not remove timeout");
	state->c_evAbort = 1;
    }
    semSignal(&state->c_replySem);
    return XK_SUCCESS;
}


static void
rpc_timeout(ev, arg)
    Event	ev;
    VOID 	*arg;
{
    XObj	s;
    SState	*state;
    Msg 	msg;

    s = (XObj)arg;
    state = (SState *)s->state;

    xTrace2(sunrpcp, 1, "SUNRPC timeout s=%x, s->state=%x", s, state);
    
    /*
     * Detach myself
     */
    evDetach(state->c_timeoutEvent);
    if ( state->c_evAbort ) {
	/*
	 * This event is no longer relevant.  Reduce session reference
	 * count and go away.
	 */
	state->c_evAbort = 0;
	xClose(s);
	return;
    }

    /*
      if (msg_isnull(rpc_state->rtstmsg)) {
      xTrace2(sunrpcp, 1, "SUNRPC timeout s=%x, s->state=%x", s, rpc_state);
      event_register(rpc_timeout, (int)s, 0, EV_REMOVE);
      return(0);
      }
      don't know what this is good for; seems to prevent retries.
      Peter, 12-18  */
    
    /* I don't want xserver to ever timout! - mjk */
    
    /* commenting out this entire check - should never quit trying
       to reconnect after a timeout...
       
       rpc_state->tries = rpc_state->tries - 1;
       if (rpc_state->tries < 0) {
       event_register(rpc_timeout, (int)s, 0, EV_REMOVE);
       * wake up rpc_push for the bad news *
       semSignal(&rpc_state->reply_sem);
       xTrace0(sunrpcp, 1, "SUNRPC timeout: quitting");
       * Setup timeout stuff  (incase someone reuses session) *
       rpc_state->tries = rpc_state->old_tries;
       return;
       }
       
       */
    
    xTrace0(sunrpcp, 0, "SUNRPC timeout: trying again");
    
    msgConstructCopy(&msg, &state->c_callMsg);
    xPush(xGetDown(s, 0), &msg);
    msgDestroy(&msg);
    state->c_timeoutEvent = evSchedule(rpc_timeout, arg, state->c_wait);
}


static u_long
get_xid()
{
    static u_long curxid = 1;
  
    return(curxid++);
}

