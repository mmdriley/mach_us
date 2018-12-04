/*
 * icmpReqRep.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.16 $
 * $Date: 1993/02/01 22:21:53 $
 */

/*
 * coordination of ICMP requests and replies
 */

#include "xkernel.h"
#include "icmp.h"
#include "icmp_internal.h"

#ifdef __STDC__

static void 	echoReqTimeout( Event, void * );
static void	signalWaiter( Sstate *st, int res );

#else

static void 	echoReqTimeout();
static void	signalWaiter();

#endif

int
icmpSendEchoReq(s, msgLength)
    XObj s;
    int msgLength;
{
  Msg msg;
  char *b;
  ICMPEcho echoHdr;
  ICMPHeader stdHdr;
  Pstate *pstate = (Pstate *)s->myprotl->state;
  Sstate *sstate = (Sstate *)s->state;
  mapId key;
  
  msgConstructAllocate(&msg, msgLength, &b);
  echoHdr.icmp_id = sstate->sessId;
  echoHdr.icmp_seqnum = sstate->seqNum;
  msgPush(&msg, icmpEchoStore, &echoHdr, sizeof(ICMPEcho), NULL);
  stdHdr.icmp_type = ICMP_ECHO_REQ;
  stdHdr.icmp_code = 0;
  msgPush(&msg, icmpHdrStore, &stdHdr, sizeof(ICMPHeader), &msg);
  key.id = echoHdr.icmp_id;
  key.seq = echoHdr.icmp_seqnum;
  sstate->bind = mapBind(pstate->waitMap, (char *)&key, s);
  xPush(xGetDown(s, 0), &msg);
  sstate->timeoutEvent = evSchedule(echoReqTimeout, s, REQ_TIMEOUT * 1000);
  msgDestroy(&msg);
  semWait(&sstate->replySem);
  return sstate->result;
}


void
icmpPopEchoRep(self, msg)
    XObj self;
    Msg *msg;
{
  Pstate *pstate;
  Sstate *sstate;
  ICMPEcho echoHdr;
  Sessn s;
  mapId key;
  
  xAssert(xIsProtocol(self));
  xTrace0(icmpp, 3, "ICMP echo reply received");
  msgPop(msg, icmpEchoLoad, &echoHdr, sizeof(ICMPEcho), NULL);
  pstate = (Pstate *)self->state;
  key.id = echoHdr.icmp_id;
  key.seq = echoHdr.icmp_seqnum;
  xTrace3(icmpp, 4, "id = %d, seq = %d, data len = %d",
	  key.id, key.seq, msgLen(msg));
  if ( mapResolve(pstate->waitMap, &key, &s) == XK_FAILURE ) {
    xTrace1(icmpp, 3, "ICMP echo reply received for nonexistent session %x",
	    echoHdr.icmp_id);
    return;
  }
  sstate = (Sstate *)s->state;
  if (evCancel(sstate->timeoutEvent) == 1) {
    /*
     * Timeout event will not run
     */
    mapRemoveBinding(pstate->waitMap, sstate->bind);
    signalWaiter(sstate, 0);
  }
}


static void
echoReqTimeout(ev, arg)
    Event	ev;
    VOID 	*arg;
{
  XObj	s = (XObj)arg;    
  Sstate *sstate;
  Pstate *pstate;

  xTrace1(icmpp, 3, "ICMP Request timeout for session %x", s);
  xAssert(xIsSession(s));
  sstate = (Sstate *)s->state;
  pstate = (Pstate *)s->myprotl->state;
  mapRemoveBinding(pstate->waitMap, sstate->bind);
  signalWaiter(sstate, -1);
}


static void
signalWaiter(st, res)
    Sstate *st;
    int res;
{
  st->result = res;
  st->seqNum++;
  semSignal(&st->replySem);
}
