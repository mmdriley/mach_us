/* 
 * ethProtl.c 
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.49 $
 * $Date: 1993/02/02 22:38:38 $
 */

/*
 * The xkernel ethernet driver is structured in two layers.
 *
 * The ethernet protocol layer (this file) is independent of any
 * particular ethernet controller hardware.
 * It comprises the usual xkernel protocol functions,
 * e.g. eth_open, eth_push, etc.).
 * It knows about ethernet addresses and "types,"
 * but nothing about any particular ethernet controller.
 *
 * The device driver, which exports an xkernel interface, sits below
 * this protocol
 */

#include "xkernel.h"
#include "eth.h"
#include "eth_i.h"

typedef struct {
    ETHhdr	hdr;
} SState;

typedef struct {
    ETHhost	host;
    ETHtype	type;
} ActiveId;

typedef struct {
    XObj    	prmSessn;
    ETHhost 	myHost;
    Map		actMap;
    Map 	pasMap;
    int		mtu;
} PState;

typedef ETHtype  PassiveId;

typedef struct {
    Msg		msg;
    XObj	self;
    XObj	llp;
    ETHhdr	hdr;
} RetBlock;

#define	ETH_ACTIVE_MAP_SZ	257
#define	ETH_PASSIVE_MAP_SZ	13

int 	traceethp;

#ifdef XK_DEBUG

static ETHhost	ethBcastHost = BCAST_ETH_AD;

#endif


#ifdef __STDC__

static void		demuxStub( Event, VOID * );
static XObj		ethCreateSessn( XObj, XObj, XObj, ActiveId * );
static void		ethSessnInit( XObj );
static int		ethControlProtl( XObj, int, char *, int );
static xkern_return_t	ethDemux( XObj, XObj, Msg * );
static XObj		ethOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	ethOpenEnable( XObj, XObj, XObj, Part * );
static xkern_return_t	ethOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t	ethOpenDisableAll( XObj, XObj );
static xkern_return_t	ethClose( XObj );
static xmsg_handle_t	ethPush( XObj, Msg * );
static xmsg_handle_t	ethLoopPush( XObj, Msg * );
static xkern_return_t	ethPop( XObj, XObj, Msg *, VOID * );
static int		ethControlSessn( XObj, int, char *, int );
static long		getRelProtNum( XObj, XObj, char * );

#else

static void		demuxStub();
static XObj		ethCreateSessn();
static void		ethSessnInit();
static int		ethControlProtl();
static xkern_return_t	ethDemux();
static XObj		ethOpen();
static xkern_return_t	ethOpenEnable();
static xkern_return_t	ethOpenDisable();
static xkern_return_t	ethOpenDisableAll();
static xkern_return_t	ethClose();
static xmsg_handle_t	ethPush();
static xmsg_handle_t	ethLoopPush();
static xkern_return_t	ethPop();
static int		ethControlSessn();
static long		getRelProtNum();

#endif __STDC__


static long
getRelProtNum( hlp, llp, s )
    XObj	hlp, llp;
    char	*s;
{
    long	n;

    n = relProtNum(hlp, llp);
    if ( n == -1 ) {
	xTrace3(ethp, TR_ERRORS,
		"eth %s could not get prot num of %s relative to %s",
		s, hlp->name, llp->name);
    }
    if ( n < 0 || n > 0xffff ) {
	return -1;
    }
    return n;
}


#define ERROR \
		{ sprintf(errBuf, \
			       "ETH format error in line %d of the rom file",\
			       i + 1);	\
		  xError(errBuf); }

/* 
 * May set the 'mtu' field of the protocol state
 */
static void
processRomFile( self )
    XObj	self;
{
    PState	*ps = (PState *)self->state;
    int 	i;
    char	instStr[80];

    strcpy(instStr, self->name);
    if ( strlen(self->instName) > 0 ) {
	strcat(instStr, "/");
	strcat(instStr, self->instName);
    }
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], instStr) ) {
	    if ( ! rom[i][1] ) {
		ERROR;
		break;
	    }
	    if ( ! strcmp(rom[i][1], "mtu") ) {
		if ( ! rom[i][2] || 
#ifdef XKMACHKERNEL
				    sscanf1
#else
				    sscanf
#endif
					  (rom[i][2], "%d", &ps->mtu) < 1 ) {
		    ERROR;
		    break;
		}
	    }
	}
    }
}
#undef ERROR


void
eth_init( self )
     XObj self;
{
    PState	*ps;
    XObj	llp;

    xTrace0(ethp, TR_EVENTS, "eth_init");
    if ( ! xIsProtocol(llp = xGetDown(self, 0)) ) {
	xError("eth can not get driver protocol object");
	return;
    }
    if ( xOpenEnable(self, self, llp, 0) == XK_FAILURE ) {
	xError("eth can not openenable driver protocol");
	return;
    }
    ps = X_NEW(PState);
    self->state = (VOID *)ps;
    ps->actMap = mapCreate(ETH_ACTIVE_MAP_SZ, sizeof(ActiveId));
    ps->pasMap = mapCreate(ETH_PASSIVE_MAP_SZ, sizeof(PassiveId));
    ps->prmSessn = 0;
    ps->mtu = MAX_ETH_DATA_SZ;
    processRomFile( self );
    xTrace1(ethp, TR_MAJOR_EVENTS, "eth using mtu %d", ps->mtu);
    if ( xControl(llp, GETMYHOST, (char *)&ps->myHost, sizeof(ETHhost))
			< (int)sizeof(ETHhost) ) {
	xError("eth_init: can't get my own host");
	return;
    }
    self->control = ethControlProtl;
    self->open = ethOpen;
    self->openenable = ethOpenEnable;
    self->opendisable = ethOpenDisable;
    self->demux = ethDemux;
    self->opendisableall = ethOpenDisableAll;
}


static XObj
ethOpen( self, hlpRcv, hlpType, part )
    XObj	self, hlpRcv, hlpType;
    Part 	*part;
{
    PState	*ps = (PState *)self->state;
    ActiveId  	key;
    XObj 	ethSessn;
    ETHhost	*remoteHost;
    long	protNum;
    
    if ( part == 0 || partLen(part) < 1 ) {
	xTrace0(ethp, TR_SOFT_ERRORS, "ethOpen -- bad participants");
	return ERR_XOBJ;
    }
    remoteHost = (ETHhost *)partPop(*part);
    xAssert(remoteHost);
    key.host = *remoteHost;
    if ( (protNum = getRelProtNum(hlpType, self, "open")) == -1 ) {
	return ERR_XOBJ;
    }
    key.type = protNum;
    xTrace2(ethp, TR_MAJOR_EVENTS, "eth_open: destination address = %s:%4x",
	    ethHostStr(&key.host), key.type);
    key.type = htons(key.type);
    if ( mapResolve(ps->actMap, &key, &ethSessn) == XK_FAILURE ) {
	ethSessn = ethCreateSessn(self, hlpRcv, hlpType, &key);
    }
    xTrace1(ethp, TR_MAJOR_EVENTS, "eth_open: returning %X", ethSessn);
    return (ethSessn);

}


static XObj
ethCreateSessn( self, hlpRcv, hlpType, key )
    XObj	self, hlpRcv, hlpType;
    ActiveId	*key;
{
    XObj	s;
    XObj	llp = xGetDown(self, 0);
    SState	*ss;
    PState	*ps = (PState *)self->state;

    s = xCreateSessn(ethSessnInit, hlpRcv, hlpType, self, 1, &llp);
    if ( ETH_ADS_EQUAL(key->host, ps->myHost) ) {
	xTrace0(ethp, TR_MAJOR_EVENTS,
		"ethCreateSessn -- creating loopback session");
	s->push = ethLoopPush;
    }
    s->binding = (Bind) mapBind(ps->actMap, (char *)key, (int)s);
    if ( s->binding == ERR_BIND ) {
	xTrace0(ethp, TR_ERRORS, "error binding in ethCreateSessn");
	xDestroy(s);
	return ERR_XOBJ;
    }
    ss = X_NEW(SState);
    ss->hdr.dst = key->host;
    ss->hdr.type = key->type;
    ss->hdr.src = ps->myHost;
    s->state = (VOID *)ss;
    return s;
}


static xkern_return_t
ethOpenEnable(self, hlpRcv, hlpType, part)
    XObj self, hlpRcv, hlpType;
    Part *part;
{
    PState	*ps = (PState *)self->state;
    PassiveId	key;
    long	protNum;
    
    if ( (protNum = getRelProtNum(hlpType, self, "openEnable")) == -1 ) {
	return XK_FAILURE;
    }
    xTrace2(ethp, TR_GROSS_EVENTS, "eth_openenable: hlp=%x, protlNum=%x",
	    hlpRcv, protNum);
    key = protNum;
    key = htons(key);
    return defaultOpenEnable(ps->pasMap, hlpRcv, hlpType, (VOID *)&key);
} 


static xkern_return_t
ethOpenDisable(self, hlpRcv, hlpType, part)
    XObj self, hlpRcv, hlpType;
    Part *part;
{
    PState	*ps = (PState *)self->state;
    long	protNum;
    PassiveId	key;
    
    if ( (protNum = getRelProtNum(hlpType, self, "opendisable")) == -1 ) {
	return XK_FAILURE;
    }
    xTrace2(ethp, TR_GROSS_EVENTS, "eth_openenable: hlp=%x, protlNum=%x",
	    hlpRcv, protNum);
    key = protNum;
    key = htons(key);
    return defaultOpenDisable(ps->pasMap, hlpRcv, hlpType, (VOID *)&key);
}


static int
dispActiveMap( key, val, arg )
    VOID	*key, *val, *arg;
{
    XObj	s = (XObj)val;
    xPrintXObj(s);
    return MFE_CONTINUE;
}


static int
dispPassiveMap( key, val, arg )
    VOID	*key, *val, *arg;
{
#ifdef XK_DEBUG
    Enable	*e = (Enable *)val;
#endif
    xTrace2(ethp, TR_ALWAYS, "Enable object, hlpRcv == %s, hlpType = %s",
	    e->hlpRcv->fullName, e->hlpType->fullName);
    return MFE_CONTINUE;
}


static xkern_return_t
ethOpenDisableAll( self, hlpRcv )
    XObj	self, hlpRcv;
{
    xkern_return_t	xkr;
    PState		*ps = (PState *)self->state;

    xTrace0(ethp, TR_MAJOR_EVENTS, "eth openDisableAll called");

    xTrace0(ethp, TR_ALWAYS, "before passive map contents:");
    mapForEach(ps->pasMap, dispPassiveMap, 0);
    xkr = defaultOpenDisableAll(((PState *)self->state)->pasMap, hlpRcv, 0);
    xTrace0(ethp, TR_ALWAYS, "after passive map contents:");
    mapForEach(ps->pasMap, dispPassiveMap, 0);
    xTrace0(ethp, TR_ALWAYS, "active map contents:");
    mapForEach(ps->actMap, dispActiveMap, 0);
    return XK_SUCCESS;
}


static xkern_return_t
ethDemux( self, llp, msg )
    XObj	self, llp;
    Msg		*msg;
{
    PState	*ps = (PState *)self->state;
    ActiveId 	actKey;
    PassiveId 	pasKey;
    XObj 	s = 0;
    Enable 	*e;
    ETHhdr	*hdr = msgGetAttr(msg, 0);
    
    xTrace0(ethp, TR_EVENTS, "eth_demux");
    xTrace1(ethp, TR_FUNCTIONAL_TRACE, "eth type: %x", hdr->type);
    xTrace2(ethp, TR_FUNCTIONAL_TRACE, "src: %s  dst: %s",
	    ethHostStr(&hdr->src), ethHostStr(&hdr->dst));
    xIfTrace(ethp, TR_DETAILED) msgShow(msg);
    xAssert(hdr);
    if ( ps->prmSessn ) {
	Msg	pMsg;
	
	xTrace0(ethp, TR_EVENTS,
		"eth_demux: passing msg to promiscuous session");
	msgConstructCopy(&pMsg, msg);
	xDemux(ps->prmSessn, &pMsg);
	msgDestroy(&pMsg);
    }
#ifdef XK_DEBUG
    /*
     * verify that msg is for this host
     */
    if ( ! (ETH_ADS_EQUAL(hdr->dst, ps->myHost) ||
	    ETH_ADS_EQUAL(hdr->dst, ethBcastHost))) {
	xError("eth_demux: msg is not for this host");
	return XK_FAILURE;
    }

#  if 0
    /* 
     * Temporary for testing
     */
    {
	static int	count = 0;

	/* 
	 * Every 30 packets there is a burst for 10 packets during
	 * which every other packet is delayed.
	 */
	count++;
	if ( ((count / 10) % 3) &&  ! (count % 2) ) {
	    xError("ethDemux delays packet");
	    Delay(4 * 1000);
	    xError("ethDemux delay returns");
	} else {
	    xTrace1(ethp, TR_EVENTS, "ethDemux does not delay (%d)", count);
	}
    }
#  endif
#endif
    actKey.host = hdr->src;
    actKey.type = hdr->type;
    if ( mapResolve(ps->actMap, &actKey, &s) == XK_FAILURE ) {
	pasKey = actKey.type;
	if ( mapResolve(ps->pasMap, &pasKey, &e) == XK_SUCCESS ) {
	    xTrace1(ethp, TR_EVENTS,
		    "eth_demux: openenable exists for msg type %x",
		    ntohs(pasKey));
	    xAssert( ntohs(hdr->type) == relProtNum(e->hlpType, self) );
	    s = ethCreateSessn(self, e->hlpRcv, e->hlpType, &actKey);
	    if ( s != ERR_XOBJ ) {
		xOpenDone(e->hlpRcv, s, self);
		xTrace0(ethp, TR_EVENTS,
			"eth_demux: sending message to new session");
	    }
	} else {
	    xTrace1(ethp, TR_EVENTS,
		    "eth_demux: openenable does not exist for msg type %x",
		    ntohs(pasKey));
	}
    }
    if ( xIsSession(s) ) {
	xPop(s, llp, msg, 0);
    }
    msgDestroy(msg);
    return XK_SUCCESS;
}



static xkern_return_t
ethClose( s )
    XObj	s;
{
    PState	*ps = (PState *)xMyProtl(s)->state;

    xTrace1(ethp, TR_MAJOR_EVENTS, "eth closing session %x", s);
    xAssert( xIsSession( s ) );
    xAssert( s->rcnt <= 0 );
    mapRemoveBinding( ps->actMap, s->binding );
    xDestroy( s );
    return XK_SUCCESS;
}


static void
demuxStub(ev, arg)
    Event	ev;
    VOID 	*arg;
{
    RetBlock	*b = (RetBlock *)arg;

    ethDemux(b->self, b->llp, &b->msg);
    xFree((char *)arg);
}


static xmsg_handle_t
ethLoopPush( s, m )
    XObj	s;
    Msg		*m;
{
    RetBlock	*b;

    b = X_NEW(RetBlock);
    msgConstructCopy(&b->msg, m);
    b->hdr = ((SState *)s->state)->hdr;
    msgSetAttr(&b->msg, 0, (VOID *)&b->hdr, sizeof(b->hdr));
    b->self = s->myprotl;
    b->llp = xGetDown(s->myprotl, 0);
    evDetach( evSchedule(demuxStub, b, 0) );
    return XMSG_NULL_HANDLE;
}


static xmsg_handle_t
ethPush( s, msg )
    XObj	s;
    Msg 	*msg;
{
    xTrace0(ethp, TR_EVENTS, "eth_push");
    msgSetAttr(msg, 0, &((SState *)s->state)->hdr, sizeof(ETHhdr));
    xPush(xGetDown(s, 0), msg);
    return XMSG_NULL_HANDLE;
}


static xkern_return_t
ethPop( s, llp, m, h)
    XObj	s, llp;
    Msg 	*m;
    VOID	*h;
{
    return xDemux(s, m);
} 


static int
ethControlSessn(s, op, buf, len)
    XObj	s;
    int 	op, len;
    char 	*buf;
{
    SState	*ss = (SState *)s->state;
    
    xAssert(xIsSession(s));
    switch (op) {

      case GETMYHOST:
      case GETMAXPACKET:
      case GETOPTPACKET:
	return ethControlProtl(xMyProtl(s), op, buf, len);
	
      case GETPEERHOST:
	checkLen(len, sizeof(ETHhost));
	bcopy((char *) &ss->hdr.dst, buf, sizeof(ETHhost));
	return (sizeof(ETHhost));
	
      case GETMYHOSTCOUNT:
      case GETPEERHOSTCOUNT:
	checkLen(len, sizeof(int));
	*(int *)buf = 1;
	return sizeof(int);
	
      case GETMYPROTO:
      case GETPEERPROTO:
	checkLen(len, sizeof(long));
	*(long *) buf = ss->hdr.type;
	return sizeof(long);
	
      case GETPARTICIPANTS:
	{
	    Part	p;

	    partInit(&p, 1);
	    /* 
	     * Remote host
	     */
	    partPush(p, &ss->hdr.dst, sizeof(ETHhost));
	    return (partExternalize(&p, buf, &len) == XK_FAILURE) ? -1 : len;
	}

      case ETH_SETPROMISCUOUS:
	{
	    PState	*ps = (PState *)xMyProtl(s)->state;
	    checkLen(len, sizeof(int));
	    ps->prmSessn = s;
	    /* 
	     * Tell the device driver to go into promiscuous mode
	     */
	    return xControl(xGetDown(s, 0), op, buf, len);
	}

      default:
	return -1;
    }
}


static int
ethControlProtl( self, op, buf, len )
    XObj	self;
    int 	op, len;
    char 	*buf;
{
    PState	*ps = (PState *)self->state;

    xAssert(xIsProtocol(self));
  
    switch (op) {

      case GETMAXPACKET:
      case GETOPTPACKET:
	checkLen(len, sizeof(int));
	*(int *) buf = ps->mtu;
	return (sizeof(int));
	
      case GETMYHOST:
	checkLen(len, sizeof(ETHhost));
	bcopy((char *) &ps->myHost, buf, sizeof(ETHhost));
	return (sizeof(ETHhost));

      default:
	return xControl(xGetDown(self, 0), op, buf, len);
  }
  
}


static void
ethSessnInit(s)
    XObj s;
{
  s->push = ethPush;
  s->pop = ethPop;
  s->close = ethClose;
  s->control = ethControlSessn;
}


