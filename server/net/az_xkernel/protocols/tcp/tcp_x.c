/*
 * tcp_x.c
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.39 $
 * $Date: 1993/02/01 22:25:17 $
 */

#include "xkernel.h"
#include "ip.h"
#include "tcp_internal.h"
#include "tcp_fsm.h"
#include "tcp_seq.h"
#include "tcp_timer.h"
#include "tcp_var.h"
#include "tcpip.h"


char *prurequests[] = {
	"ATTACH",	"DETACH",	"BIND",		"LISTEN",
	"CONNECT",	"ACCEPT",	"DISCONNECT",	"SHUTDOWN",
	"RCVD",		"SEND",		"ABORT",	"CONTROL",
	"SENSE",	"RCVOOB",	"SENDOOB",	"SOCKADDR",
	"PEERADDR",	"CONNECT2",	"FASTTIMO",	"SLOWTIMO",
	"PROTORCV",	"PROTOSEND",
};

XObj            TCP;
#define         IP (xGetDown(TCP,0))

long	tcpIpProtocolNum;

static IPhost   myHost;

#ifdef __STDC__

static int	extractPart( Part *, long *, IPhost **, char * );
static void	tcpSessnInit( XObj );
static XObj	tcp_establishopen( XObj, XObj, XObj, IPhost *, IPhost *,
				   int, int );
static void	tcp_dumpstats( void );
static int		tcpControlProtl( XObj, int, char *, int );
static XObj		tcpOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	tcpOpenEnable( XObj, XObj, XObj, Part * );
static xkern_return_t	tcpOpenDisable( XObj, XObj, XObj, Part * );


#else

static int	extractPart();
static void	tcpSessnInit();
static XObj	tcp_establishopen();
static void	tcp_dumpstats();
static int		tcpControlProtl();
static XObj		tcpOpen();
static xkern_return_t	tcpOpenEnable();
static xkern_return_t	tcpOpenDisable();

#endif __STDC__



/* 
 * Check for a valid participant list
 */
#define partCheck(p, name, max, retval)					\
	{								\
	  if ( ! (p) || partLen(p) < 1 || partLen(p) > (max)) { 	\
		xTrace1(tcpp, TR_ERRORS,				\
			"%s -- bad participants",			\
			(name));  					\
		return (retval);					\
	  }								\
	}


/*************************************************************************/

void
tcp_init(self)
    XObj            self;
{
  Part		p;
  PSTATE	*pstate;
  XObj		ip;

  TCP = self;
  xTrace0(tcpp, TR_GROSS_EVENTS, "TCP init");
  xTrace1(tcpp, TR_GROSS_EVENTS, "ENDIAN = %d", ENDIAN);

  xAssert(xIsProtocol(self));

  pstate = (PSTATE *)xMalloc(sizeof(PSTATE));
  self->state = (char *)pstate;

  self->control = tcpControlProtl;
  self->open = tcpOpen;
  self->openenable = tcpOpenEnable;
  self->opendisable = tcpOpenDisable;
  self->demux = tcp_input;

  pstate->passiveMap = mapCreate(37, sizeof(PassiveId));
  pstate->activeMap = mapCreate(101, sizeof(ActiveId));

  tcb.inp_next = tcb.inp_prev = &tcb;
  tcp_iss = 1;
  ip = xGetProtlByName("ip");
  if ( ! xIsXObj(ip) ) {
      Kabort("TCP can't get IP protocol object");
  }
  if ( (tcpIpProtocolNum = relProtNum(self, ip)) == -1 ) {
      Kabort("TCP can't get protocol number relative to IP");
  }
  xTrace1(tcpp, TR_DETAILED, "TCP uses IP protocol number %d",
	  tcpIpProtocolNum);
  partInit(&p, 1);
  partPush(p, ANY_HOST, 0);
  xOpenEnable(TCP, TCP, IP, &p);
  xControl(IP, GETMYHOST, (char *)&myHost, sizeof(myHost));
  evDetach(evSchedule(tcp_fasttimo, 0, TCP_FAST_INTERVAL));
  evDetach(evSchedule(tcp_slowtimo, 0, TCP_SLOW_INTERVAL));
  tcpPortMapInit();
}

/*************************************************************************/
struct tcpstate *new_tcpstate()
{
  struct tcpstate *tcpstate;
  tcpstate = (struct tcpstate *) xMalloc(sizeof(struct tcpstate));
  bzero((char *)tcpstate, sizeof(struct tcpstate));
  tcpSemInit(&tcpstate->waiting, 0);
  tcpSemInit(&tcpstate->lock, 1);
  tcpstate->closed = 0;
  tcpstate->snd = (struct sb *)xMalloc(sizeof(struct sb));
  /* initialize buffer size to TCPRCVWIN to get things started: */
  tcpstate->rcv_space = TCPRCVWIN;
  tcpstate->rcv_hiwat = TCPRCVWIN;
  return tcpstate;
}

/*************************************************************************/
void delete_tcpstate(tcpstate)
struct tcpstate *tcpstate;
{
  tcpVAll(&tcpstate->waiting);
  xFree((char *)tcpstate);

  /* From RICE 07/03/90 */
  tcpVAll(&tcpstate->lock);
  xFree((char *)tcpstate->snd);
  xFree((char *)tcpstate);
}

/*************************************************************************/
static XObj
tcp_establishopen(self, hlpRcv, hlpType, raddr, laddr, rport, lport)
    XObj self, hlpRcv, hlpType;
    IPhost *raddr, *laddr;
    int rport, lport;
{
  Part          p[2];
  XObj		new;
  struct tcpstate *tcpst;
  struct inpcb *inp;
  ActiveId      ex_id;
  XObj		lls;
  PSTATE	*pstate;

  xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp_establish: on %X", hlpRcv);
  xAssert(xIsProtocol(hlpRcv));

  pstate = (PSTATE *)self->state;
  ex_id.localport = lport;
  ex_id.remoteport = rport;
  ex_id.remoteaddr = *raddr;
  if ( mapResolve(pstate->activeMap, &ex_id, &new) == XK_SUCCESS ) {
    xTrace3(tcpp, TR_GROSS_EVENTS, "tcp_establish: (%d->%X.%d) already open",
	    ex_id.localport, ex_id.remoteaddr, ex_id.remoteport);
    return 0;
  }

  partInit(p, laddr ? 2 : 1);
  partPush(p[0], raddr, sizeof(IPhost));
  if ( laddr ) {
      partPush(p[1], laddr, sizeof(IPhost));
  }
  xTrace0(tcpp, TR_EVENTS, "tcpOpen: opening IP");
  if ( (lls = xOpen(TCP, TCP, IP, p)) == ERR_SESSN ) {
    xTrace0(tcpp, TR_ERRORS, "tcpOpen: cannot open IP session");
    return 0;
  }

  new = xCreateSessn(tcpSessnInit, hlpRcv, hlpType, TCP, 1, &lls);
  xTrace3(tcpp, TR_GROSS_EVENTS, "Mapping %d->%X.%d", lport, raddr, rport);
  new->binding = mapBind(pstate->activeMap, (char *) &ex_id, (int) new);
  if ((int)new->binding == -1) {
    xTrace3(tcpp, TR_GROSS_EVENTS, "tcp_establish: Bind of %d->%X.%d failed", 
      lport, raddr, rport);
    return 0;
  }

  tcpst = new_tcpstate();
  tcpst->hlpType = hlpType;
  new->state = (char *)tcpst;
  xTrace0(tcpp, TR_EVENTS, "tcpOpen: attaching...");
  tcp_attach(new);

  inp = sotoinpcb(new);
  tcpst->inpcb = inp;
  tcpst->tcpcb = (struct tcpcb *)inp->inp_ppcb;
  *(IPhost *)&inp->inp_laddr = myHost;
  inp->inp_lport = ex_id.localport;
  *(IPhost *)&inp->inp_raddr = ex_id.remoteaddr;
  inp->inp_rport = ex_id.remoteport;

  return new;
}


static int
extractPart( p, port, host, s )
    Part	*p;
    long	*port;
    IPhost	**host;
    char	*s;
{
    long	*portPtr;
    IPhost	*hostPtr;

    xAssert(port);
    if ( (portPtr = (long *)partPop(*p)) == 0 ) {
	xTrace1(tcpp, TR_SOFT_ERROR, "Bad participant in %s -- no port", s);
	return -1;
    }
    *port = *portPtr;
    if ( *port > MAX_PORT ) {
	xTrace2(tcpp, TR_SOFT_ERROR,
		"Bad participant in %s -- port %d out of range", s, *port);
	return -1;
    }
    if ( host ) {
	if ( (hostPtr = (IPhost *)partPop(*p)) == 0 ) {
	    xTrace1(tcpp, TR_SOFT_ERROR,
		    "Bad participant in %s -- no host", s);
	    return -1;
	}
	*host = hostPtr;
    }
    return 0;
}


/*************************************************************************/
static XObj
tcpOpen(self, hlpRcv, hlpType, p)
    XObj            self, hlpRcv, hlpType;
    Part           *p;
{
    XObj	s;
    long	remotePort, localPort = ANY_PORT;
    IPhost	*remoteHost, *localHost = 0;
    
    xTrace0(tcpp, TR_GROSS_EVENTS, "TCP open");
    partCheck(p, "tcpOpen", 2, ERR_XOBJ);
    if ( extractPart(p, &remotePort, &remoteHost, "tcpOpen (remote)") ) {
	return ERR_XOBJ;
    }
    if ( partLen(p) > 1 ) {
	if ( extractPart(p+1, &localPort, &localHost, "tcpOpen (local)") ) {
	    return ERR_XOBJ;
	}
	if ( localHost == (IPhost *)ANY_HOST ) {
	    localHost = 0;
	}
    }
    if ( localPort == ANY_PORT ) {
	/* 
	 * We need to find a free local port
	 */
	long	freePort;
	if ( tcpGetFreePort(&freePort) ) {
	    xError("tcpOpen -- no free ports!");
	    return ERR_XOBJ;
	}
	localPort = freePort;
    } else {
	/* 
	 * A specific local port was requested
	 */
	if ( tcpDuplicatePort(localPort) ) {
	    xTrace1(tcpp, TR_MAJOR_EVENTS,
		    "tcpOpen: requested port %d is already in use",
		    localPort);
	    return ERR_XOBJ;
	}
    }
    s = tcp_establishopen(self, hlpRcv, hlpType, remoteHost, localHost,
			  remotePort, localPort);
    if (!s) {
	tcpReleasePort(localPort);
	s = ERR_SESSN;
    } else {
	xTrace0(tcpp, TR_EVENTS, "tcpOpen: connecting...");
	tcp_usrreq(s, PRU_CONNECT, 0, 0);
	
	xTrace0(tcpp, TR_EVENTS, "tcpOpen: waiting...");
	tcpSemWait(&(sototcpst(s)->waiting));
	if (sototcpcb(s)->t_state != TCPS_ESTABLISHED) {
	    xTrace3(tcpp, TR_EVENTS, "tcpOpen: open (%d->%X.%d) connect failed",
		    localPort,  ipHostStr(remoteHost), remotePort);
	    tcp_destroy(sototcpcb(s));
	    tcpReleasePort(localPort);
	    s = ERR_SESSN;
	}
    }
    xTrace1(tcpp, TR_GROSS_EVENTS, "Return from tcpOpen, session = %x", s);
    return s;
}

/*************************************************************************/
static xkern_return_t
tcpOpenEnable(self, hlpRcv, hlpType, p)
    XObj	self, hlpRcv, hlpType;
    Part	*p;
{
    PassiveId	key;
    long	protNum;
    PSTATE	*pstate;
    Enable	*e;
    
    partCheck(p, "tcpOpenEnable", 1, XK_FAILURE);
    if ( extractPart(p, &protNum, 0, "tcpOpenEnable") ) {
	return XK_FAILURE;
    }
    key = protNum;
    pstate = (PSTATE *)self->state;
    xTrace2(tcpp, TR_MAJOR_EVENTS, "tcpOpenEnable mapping %d->%x", key,
	    hlpRcv);
    if ( mapResolve(pstate->passiveMap, &key, &e) == XK_SUCCESS ) {
	if ( e->hlpRcv == hlpRcv && e->hlpType == hlpType ) {
	    e->rcnt++;
	    return XK_SUCCESS;
	}
	return XK_FAILURE;
    }
    tcpDuplicatePort(key);
    e = (Enable *)xMalloc(sizeof(Enable));
    e->hlpRcv = hlpRcv;
    e->hlpType = hlpType;
    e->rcnt = 1;
    e->binding = mapBind(pstate->passiveMap, (char *) &key, (int) e);
    if ( e->binding == ERR_BIND ) {
	xFree((char *)e);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}

/*************************************************************************/
static xkern_return_t
tcpOpenDisable(self, hlpRcv, hlpType, p)
    XObj	self, hlpRcv, hlpType;
    Part        *p;
{
    PassiveId	key;
    long	protNum;
    Enable	*e;
    PSTATE	*pstate;

    pstate = (PSTATE *)self->state;
    partCheck(p, "tcpOpenDisable", 1, XK_FAILURE);
    if ( extractPart(p, &protNum, 0, "tcpOpenEnable") ) {
	return XK_FAILURE;
    }
    key = protNum;
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp_disable removing %d", key);
    if ( mapResolve(pstate->passiveMap, &key, &e) == XK_FAILURE ||
		e->hlpRcv != hlpRcv || e->hlpType != hlpType ) {
	return XK_FAILURE;
    }
    if (--(e->rcnt) == 0) {
	mapRemoveBinding(pstate->passiveMap, e->binding);
	xFree((char *)e);
	tcpReleasePort(key);
    }
    return XK_SUCCESS;
}

/*************************************************************************/
/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
static xkern_return_t
tcpClose(so)
    XObj	so;
{
    register struct tcpcb *tp;
    register struct tcpstate *tcpst;
    
    xTrace1(tcpp, TR_FUNCTIONAL_TRACE, "tcpClose(so=%08x)", so);

    tcpst = sototcpst(so);
    tcpst->closed |= 1;
    
    tp = sototcpcb(so);
    tp = tcp_usrclosed(tp);
    if (tp) tcp_output(tp);

    xTrace2(tcpp, TR_MAJOR_EVENTS, "tcpClose: tp=%08x, tcpst->closed=%x",
	    tp, tcpst->closed);
    
    if (tcpst->closed == 1) {
	/*
	 * This was a user close before the network close
	 */
	xTrace0(tcpp, TR_EVENTS, "tcpClose: waiting...");
	tcpSemWait(&tcpst->waiting);
	xTrace0(tcpp, TR_EVENTS, "tcpClose: done");
    }
    return XK_SUCCESS;
}


/*************************************************************************/
static xmsg_handle_t
tcpPush(so, msg)
     XObj so;
     Msg *msg;
{
    int error;
    int space;
    Msg pushMsg;
    register struct tcpstate *tcpst = sototcpst(so);
    register struct tcpcb *tp = sototcpcb(so);

    xTrace2(tcpp, TR_MAJOR_EVENTS, "tcpPush: session %X, msg %d bytes",
	    so, msgLen(msg));
    if (so->rcnt < 1) {
	xTrace1(tcpp, TR_GROSS_EVENTS, "tcpPush: session ref count = %d",
		so->rcnt);
	return XMSG_ERR_HANDLE;
    }
    if (tcpst->closed) {
	xTrace1(tcpp, TR_SOFT_ERROR, "tcpPush: session already closed %d",
		tcpst->closed);
	return XMSG_ERR_HANDLE;
    }

#undef CHUNKSIZE (sbhiwat(tcpst->snd) / 2)
#define CHUNKSIZE (tp->t_maxseg)

    if ((tcpst->flags & TCPST_NBIO) &&
	(sbspace(tcpst->snd) < MIN(msgLen(msg), CHUNKSIZE)))
    {
	return XMSG_ERR_WOULDBLOCK;
    } /* if */

    msgConstructEmpty(&pushMsg);
    while ( msgLen(msg) != 0 ) {
	xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
		"tcpPush: msgLen = <%d>", msgLen(msg));
	msgChopOff(msg, &pushMsg, CHUNKSIZE);
	xTrace2(tcpp, TR_FUNCTIONAL_TRACE,
		"tcpPush: after break pushMsg = <%d> msg = <%d>",
		msgLen(&pushMsg), msgLen(msg));
	while ((space = sbspace(tcpst->snd)) < msgLen(&pushMsg)) {
	    xTrace1(tcpp, TR_GROSS_EVENTS,
		    "tcpPush: waiting for space (%d available)", space);
	    xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
		    "tcpPush: msgLen == %d before blocking",
		    msgLen(msg));
	    tcpSemWait(&tcpst->waiting);
	    if (tcpst->closed) {
		xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
			"tcpPush: session already closed %d", tcpst->closed);
		msgDestroy(&pushMsg);
		return XMSG_ERR_HANDLE;
	    }
	}
	xTrace1(tcpp, TR_FUNCTIONAL_TRACE,
		"tcppush about to append to sb.  orig msgLen == %d",
		msgLen(msg));
	sbappend(tcpst->snd, &pushMsg);
	error = tcp_output(sototcpcb(so));
	if (error) {
	    xTrace1(tcpp, TR_ERRORS, "tcpPush failed with code %d", error);
	    msgDestroy(&pushMsg);
	    return XMSG_ERR_HANDLE;
	}
    }
    msgDestroy(&pushMsg);
    return XMSG_NULL_HANDLE;
}

/*************************************************************************/
static int
tcpControlSessn(s, opcode, buf, len)
    Sessn           s;
    char           *buf;
    int             opcode;
    int             len;
{
  struct tcpstate *ts;
  struct tcpcb   *tp;
  u_short size;

  ts = (struct tcpstate *) s->state;
  tp = ts->tcpcb;
  switch (opcode) {

   case GETMYPROTO:
     checkLen(len, sizeof(long));
     *(long *)buf = tp->t_template->ti_sport;
     return sizeof(long);

   case GETPEERPROTO:
     checkLen(len, sizeof(long));
     *(long *)buf = tp->t_template->ti_dport;
     return sizeof(long);

   case TCP_PUSH:
     tp->t_force = 1;
     tcp_output(tp);
     tp->t_force = 0;
     return 0;

   case TCP_GETSTATEINFO:
     *(int *) buf = tp->t_state;
     return (sizeof(int));

   case GETOPTPACKET:
   case GETMAXPACKET:
     if ( xControl(xGetDown(s, 0), opcode, buf, len) < sizeof(int) ) {
	 return -1;
     }
     *(int *)buf -= sizeof(struct tcphdr);
     return sizeof(int);
     
   case GETPARTICIPANTS:
     {
	 Part	*p = (Part *)buf;
	 int	retLen;
	 long	*lPtr;

	 retLen = xControl(xGetDown(s, 0), opcode, buf, len);
	 if ( retLen < sizeof(Part) ) {
	     return -1;
	 }
	 lPtr = (long *)xMalloc(sizeof(long));
	 *lPtr = tp->t_template->ti_dport;
	 partPush(p[0], (VOID *)lPtr, sizeof(long));
	 if ( retLen >= 2 * sizeof(Part) ) {
	     lPtr = (long *)xMalloc(sizeof(long));
	     *lPtr = tp->t_template->ti_sport;
	     partPush(p[1], (VOID *)lPtr, sizeof(long));
	 }
	 return retLen;
     }

   case SETNONBLOCKINGIO:
     xControl(xGetDown(s, 0), opcode, buf, len);
     if (*(int*)buf) {
	 ts->flags |= TCPST_NBIO;
     } else {
	 ts->flags &= ~TCPST_NBIO;
     } /* if */
     return 0;

   case TCP_SETRCVBUFSPACE:
     checkLen(len, sizeof(u_short));
     size = *(u_short *)buf;
     ts->rcv_space = size;
     /* after receiving message possibly send window update: */
     tcp_output(tp);
     return 0;

   case TCP_GETSNDBUFSPACE:
     checkLen(len, sizeof(u_short));
     *(u_short*)buf = sbspace(ts->snd);
     return sizeof(u_short);

   case TCP_SETRCVBUFSIZE:
     checkLen(len, sizeof(u_short));
     size = *(u_short *)buf;
     ts->rcv_hiwat = size;
     return 0;

   case TCP_SETSNDBUFSIZE:
     checkLen(len, sizeof(u_short));
     sbhiwat(ts->snd) = *(u_short *)buf;
     return 0;

   case TCP_SETOOBINLINE:
     checkLen(len, sizeof(int));
     if (*(int*)buf) {
	 ts->flags |= TCPST_OOBINLINE;
     } else {
	 ts->flags &= ~TCPST_OOBINLINE;
     } /* if */
     return 0;

   case TCP_GETOOBDATA:
     {     
	 char peek;

	 checkLen(len, sizeof(char));

	 peek = *(char*)buf;
	 if ((tp->t_oobflags & TCPOOB_HADDATA) ||
	     !(tp->t_oobflags & TCPOOB_HAVEDATA))
	 {
	     return 0;
	 } /* if */
	 *(char*)buf = tp->t_iobc;
	 if (!peek) {
	     tp->t_oobflags ^= TCPOOB_HAVEDATA | TCPOOB_HADDATA;
	 } /* if */
	 return 1;
     }

   case TCP_OOBPUSH:
     {
	 Msg *m;

	 checkLen(len, sizeof(Msg*));

	 m = (Msg*)buf;
	 sbappend(ts->snd, m);
	 tp->snd_up = tp->snd_una + sblength(ts->snd);
	 tp->t_force = 1;
	 tcp_output(tp);
	 tp->t_force = 0;
	 return 0;
     }

   default:
     return xControl(xGetDown(s, 0), opcode, buf, len);
  }
}

/*************************************************************************/
static int
tcpControlProtl(self, opcode, buf, len)
    XObj	    self;
    int             opcode, len;
    char           *buf;
{
    long port;

    switch (opcode) {

      case TCP_DUMPSTATEINFO:
	tcp_dumpstats();
	return 0;

      case TCP_GETFREEPROTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	if (tcpGetFreePort(&port)) {
	    return -1;
	} /* if */
	*(long *)buf = port;
	return 0;

      case TCP_RELEASEPROTNUM:
	checkLen(len, sizeof(long));
	port = *(long *)buf;
	tcpReleasePort(port);
	return 0;

      default:
	return xControl(xGetDown(self,0), opcode, buf, len);
    }
}

/*************************************************************************/
static void
tcpSessnInit(s)
    XObj s;
{
    xAssert(xIsSession(s));
    s->push = tcpPush;
    s->close = tcpClose;
    s->control = tcpControlSessn;
    s->demux = tcp_input;
    s->pop = tcpPop;
}

/*************************************************************************/
void
sorwakeup(so)
XObj so;
{
  xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp: sorwakeup on %X", so);
}
    
/*************************************************************************/
void
sowwakeup(so)
XObj so;
{
  xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp: sowwakeup on %X", so);
  tcpSemSignal(&sototcpst(so)->waiting);
}

/*************************************************************************/
/*ARGSUSED*/
int
soreserve(so, send, recv)
XObj so;
int send, recv;
{
  struct tcpstate *tcpst = sototcpst(so);
  sbinit(tcpst->snd);
  return 0;
}

/*************************************************************************/
void
socantsendmore(so)
    XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  socantsendmore on %X", so);
}

/*************************************************************************/
void
soisdisconnected(so)
    XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisdisconnected on %X", so);
    if (sototcpst(so)->closed) {
	/* It was already closed, either by the network or the user */
    } else {
	/* deliver close done after we're done handling current message: */
	xCloseDone(so);
    } /* if */
    tcpVAll(&sototcpst(so)->waiting);
} /* soisdisconnected */

/*************************************************************************/
void
soabort(so)
XObj so;
{
}

/*************************************************************************/
void
socantrcvmore(so)
    XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  socantrcvmore on %X", so);
    
    sototcpst(so)->closed |= 2;
    if (sototcpst(so)->closed & 1) {
	/* do nothing, the user already knows that it is closed */
    } else {
	xCloseDone(so);
    }
}

/*************************************************************************/
void
soisconnected(so)
    XObj so;
{
    struct tcpstate	*state = sototcpst(so);
    TcpSem 		*s = &state->waiting;

    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisconnected on %X", so);
    if (s->waitCount) {
	xTrace1(tcpp, TR_EVENTS, "tcp:  waking up opener on %X", so);
	tcpSemSignal(s);
    } else {
	/*
	 * I think that this is the time for a open done
	 */
	xOpenDone(xGetUp(so), so, xMyProtl(so));
    }
}

/*************************************************************************/
void
soisconnecting(so)
    XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisconnecting on %X", so);
}

/*************************************************************************/
void
soisdisconnecting(so)
XObj so;
{
    xTrace1(tcpp, TR_MAJOR_EVENTS, "tcp:  soisdisconnecting on %X", so);
}

/*************************************************************************/
/* 
 * sonewconn is called by the input routine to establish a passive open
 */
XObj
sonewconn(self, so, hlpType, src, dst, sport, dport)
    XObj self, so, hlpType;
    IPhost *src, *dst;
    int sport, dport;
{
  XObj new;
  xAssert(xIsProtocol(so));
  xTrace1(tcpp, TR_MAJOR_EVENTS, "sonewconn on %X", so);
  new = tcp_establishopen(self, so, hlpType, src, dst, sport, dport);
  if ( new ) {
      tcpDuplicatePort(dport);
  }
  return new;
}


/*************************************************************************/
/* 
 * sohasoutofband is called by the input routine to signal the presence
 * of urgent (out-of-band) data
 */
void
sohasoutofband(so, oobmark)
     XObj so;
     u_int oobmark;
{
    void *buf[2];

    buf[0] = so;
    buf[1] = (void*) oobmark;
    xControl(xGetUp(so), TCP_OOBMODE, (char*) buf, sizeof(buf));
} /* sohasoutofband */

/*************************************************************************/

void
tcpSemWait( ts )
    TcpSem	*ts;
{
    ts->waitCount++;
    semWait(&ts->s);
    ts->waitCount--;
}


void
tcpSemSignal( ts )
    TcpSem	*ts;
{
    semSignal(&ts->s);
}


void
tcpVAll( ts )
    TcpSem	*ts;
{
    int i, n;

    n=ts->waitCount;
    for ( i=0; i < n; i++ ) {
	semSignal(&ts->s);
    }
}


void
tcpSemInit( ts, n )
    TcpSem	*ts;
    int		n;
{
    semInit(&ts->s, n);
    ts->waitCount = 0;
}

/*************************************************************************/
static void
tcp_dumpstats()
{
  printf("tcps_badsum %d\n", tcpstat.tcps_badsum);
  printf("tcps_badoff %d\n", tcpstat.tcps_badoff);
  printf("tcps_hdrops %d\n", tcpstat.tcps_hdrops);
  printf("tcps_badsegs %d\n", tcpstat.tcps_badsegs);
  printf("tcps_unack %d\n", tcpstat.tcps_unack);
  printf("connections initiated %d\n", tcpstat.tcps_connattempt);
  printf("connections accepted %d\n", tcpstat.tcps_accepts);
  printf("connections established %d\n", tcpstat.tcps_connects);
  printf("connections dropped %d\n", tcpstat.tcps_drops);
  printf("embryonic connections dropped %d\n", tcpstat.tcps_conndrops);
  printf("conn. closed (includes drops) %d\n", tcpstat.tcps_closed);
  printf("segs where we tried to get rtt %d\n", tcpstat.tcps_segstimed);
  printf("times we succeeded %d\n", tcpstat.tcps_rttupdated);
  printf("delayed acks sent %d\n", tcpstat.tcps_delack);
  printf("conn. dropped in rxmt timeout %d\n", tcpstat.tcps_timeoutdrop);
  printf("retransmit timeouts %d\n", tcpstat.tcps_rexmttimeo);
  printf("persist timeouts %d\n", tcpstat.tcps_persisttimeo);
  printf("keepalive timeouts %d\n", tcpstat.tcps_keeptimeo);
  printf("keepalive probes sent %d\n", tcpstat.tcps_keepprobe);
  printf("connections dropped in keepalive %d\n", tcpstat.tcps_keepdrops);
  printf("total packets sent %d\n", tcpstat.tcps_sndtotal);
  printf("data packets sent %d\n", tcpstat.tcps_sndpack);
  printf("data bytes sent %d\n", tcpstat.tcps_sndbyte);
  printf("data packets retransmitted %d\n", tcpstat.tcps_sndrexmitpack);
  printf("data bytes retransmitted %d\n", tcpstat.tcps_sndrexmitbyte);
  printf("ack-only packets sent %d\n", tcpstat.tcps_sndacks);
  printf("window probes sent %d\n", tcpstat.tcps_sndprobe);
  printf("packets sent with URG only %d\n", tcpstat.tcps_sndurg);
  printf("window update-only packets sent %d\n", tcpstat.tcps_sndwinup);
  printf("control (SYN|FIN|RST) packets sent %d\n", tcpstat.tcps_sndctrl);
  printf("total packets received %d\n", tcpstat.tcps_rcvtotal);
  printf("packets received in sequence %d\n", tcpstat.tcps_rcvpack);
  printf("bytes received in sequence %d\n", tcpstat.tcps_rcvbyte);
  printf("packets received with ccksum errs %d\n", tcpstat.tcps_rcvbadsum);
  printf("packets received with bad offset %d\n", tcpstat.tcps_rcvbadoff);
  printf("packets received too short %d\n", tcpstat.tcps_rcvshort);
  printf("duplicate-only packets received %d\n", tcpstat.tcps_rcvduppack);
  printf("duplicate-only bytes received %d\n", tcpstat.tcps_rcvdupbyte);
  printf("packets with some duplicate data %d\n", tcpstat.tcps_rcvpartduppack);
  printf("dup. bytes in part-dup. packets %d\n", tcpstat.tcps_rcvpartdupbyte);
  printf("out-of-order packets received %d\n", tcpstat.tcps_rcvoopack);
  printf("out-of-order bytes received %d\n", tcpstat.tcps_rcvoobyte);
  printf("packets with data after window %d\n", tcpstat.tcps_rcvpackafterwin);
  printf("bytes rcvd after window %d\n", tcpstat.tcps_rcvbyteafterwin);
  printf("packets rcvd after \"close\" %d\n", tcpstat.tcps_rcvafterclose);
  printf("rcvd window probe packets %d\n", tcpstat.tcps_rcvwinprobe);
  printf("rcvd duplicate acks %d\n", tcpstat.tcps_rcvdupack);
  printf("rcvd acks for unsent data %d\n", tcpstat.tcps_rcvacktoomuch);
  printf("rcvd ack packets %d\n", tcpstat.tcps_rcvackpack);
  printf("bytes acked by rcvd acks %d\n", tcpstat.tcps_rcvackbyte);
  printf("rcvd window update packets %d\n", tcpstat.tcps_rcvwinupd);


  tcpstat.tcps_badsum = 0;
  tcpstat.tcps_badoff = 0;
  tcpstat.tcps_hdrops = 0;
  tcpstat.tcps_badsegs = 0;
  tcpstat.tcps_unack = 0;
  tcpstat.tcps_connattempt = 0;
  tcpstat.tcps_accepts = 0;
  tcpstat.tcps_connects = 0;
  tcpstat.tcps_drops = 0;
  tcpstat.tcps_conndrops = 0;
  tcpstat.tcps_closed = 0;
  tcpstat.tcps_segstimed = 0;
  tcpstat.tcps_rttupdated = 0;
  tcpstat.tcps_delack = 0;
  tcpstat.tcps_timeoutdrop = 0;
  tcpstat.tcps_rexmttimeo = 0;
  tcpstat.tcps_persisttimeo = 0;
  tcpstat.tcps_keeptimeo = 0;
  tcpstat.tcps_keepprobe = 0;
  tcpstat.tcps_keepdrops = 0;
  tcpstat.tcps_sndtotal = 0;
  tcpstat.tcps_sndpack = 0;
  tcpstat.tcps_sndbyte = 0;
  tcpstat.tcps_sndrexmitpack = 0;
  tcpstat.tcps_sndrexmitbyte = 0;
  tcpstat.tcps_sndacks = 0;
  tcpstat.tcps_sndprobe = 0;
  tcpstat.tcps_sndurg = 0;
  tcpstat.tcps_sndwinup = 0;
  tcpstat.tcps_sndctrl = 0;
  tcpstat.tcps_rcvtotal = 0;
  tcpstat.tcps_rcvpack = 0;
  tcpstat.tcps_rcvbyte = 0;
  tcpstat.tcps_rcvbadsum = 0;
  tcpstat.tcps_rcvbadoff = 0;
  tcpstat.tcps_rcvshort = 0;
  tcpstat.tcps_rcvduppack = 0;
  tcpstat.tcps_rcvdupbyte = 0;
  tcpstat.tcps_rcvpartduppack = 0;
  tcpstat.tcps_rcvpartdupbyte = 0;
  tcpstat.tcps_rcvoopack = 0;
  tcpstat.tcps_rcvoobyte = 0;
  tcpstat.tcps_rcvpackafterwin = 0;
  tcpstat.tcps_rcvbyteafterwin = 0;
  tcpstat.tcps_rcvafterclose = 0;
  tcpstat.tcps_rcvwinprobe = 0;
  tcpstat.tcps_rcvdupack = 0;
  tcpstat.tcps_rcvacktoomuch = 0;
  tcpstat.tcps_rcvackpack = 0;
  tcpstat.tcps_rcvackbyte = 0;
  tcpstat.tcps_rcvwinupd = 0;
}
