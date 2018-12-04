/*     
 * ip.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.94 $
 * $Date: 1993/02/01 22:22:48 $
 */


#include "xkernel.h"
#include "gc.h"
#include "ip.h"
#include "ip_i.h"
#include "arp.h"
#include "route.h"


#ifdef __STDC__

static void		callRedirect( Event, VOID * );
static XObj		createLocalSessn( XObj, XObj, XObj,
					  ActiveId *, IPhost * );
static void		destroyForwardSessn( XObj s );
static void		destroyNormalSessn( XObj s );
static void		destroySessn( XObj, Map );
static XObj		forwardSessn( XObj, ActiveId *, FwdId * );
static XObj		fwdBcastSessn( XObj, XObj, ActiveId *, FwdId * );
static void		fwdSessnInit( XObj );
static IPhost *		getHost( Part * );
static long		getRelProtNum( XObj, XObj, char * );
static int 		get_ident( XObj );
static xkern_return_t 	ipCloseProtl( XObj );
static xkern_return_t	ipCloseSessn( XObj );
static XObj		ipCreateSessn( XObj, XObj, XObj, Pfv, IPhost * );
static int 		ipHandleRedirect( XObj );
static XObj		ipOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	ipOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t	ipOpenEnable( XObj, XObj, XObj, Part * );
static xmsg_handle_t	ipPush( XObj, Msg * );
static XObj		localPassiveSessn( XObj, ActiveId *, IPhost * );
static void		localSessnInit( XObj );
static int		routeChangeFilter( void *, void *, void * );
extern void		scheduleIpFragCollector( PState * );

#else

static void		callRedirect();
static XObj		createLocalSessn();
static void		destroyForwardSessn();
static void		destroyNormalSessn();
static void		destroySessn();
static XObj		forwardSessn();
static XObj		fwdBcastSessn();
static void		fwdSessnInit();
static IPhost *		getHost();
static long		getRelProtNum();
static int 		get_ident();
static xkern_return_t 	ipCloseProtl();
static xkern_return_t	ipCloseSessn();
static XObj		ipCreateSessn();
static int 		ipHandleRedirect();
static XObj		ipOpen();
static xkern_return_t	ipOpenDisable();
static xkern_return_t	ipOpenEnable();
static xmsg_handle_t	ipPush();
static XObj		localPassiveSessn();
static void		localSessnInit();
static int		routeChangeFilter();
extern void		scheduleIpFragCollector();

#endif __STDC__


int 	traceipp;
IPhost	ipSiteGateway = SITE_IP_GTW ;


#define SESSN_COLLECT_INTERVAL	20 * 1000 * 1000	/* 20 seconds */
#define IP_MAX_PROT	0xff


static long
getRelProtNum( hlp, llp, s )
    XObj	hlp, llp;
    char	*s;
{
    long	n;

    n = relProtNum(hlp, llp);
    if ( n == -1 ) {
	xTrace3(ipp, TR_SOFT_ERROR,
	       "%s: couldn't get prot num of %s relative to %s",
	       s, hlp->name, llp->name);
	return -1;
    }
    if ( n < 0 || n > 0xff ) {
	xTrace4(ipp, TR_SOFT_ERROR,
	       "%s: prot num of %s relative to %s (%d) is out of range",
	       s, hlp->name, llp->name, n);
	return -1;
    }
    return n;
}


static xkern_return_t
ipOpenDisableAll( self, hlpRcv )
    XObj	self, hlpRcv;
{
    PState	*ps = (PState *)self->state;
    
    xTrace0(ipp, TR_MAJOR_EVENTS, "ipOpenDisableAll");
    defaultOpenDisableAll(ps->passiveMap, hlpRcv, 0);
    defaultOpenDisableAll(ps->passiveSpecMap, hlpRcv, 0);
    return XK_SUCCESS;
}


/*
 * ip_init: main entry point to IP
 */
void
ip_init(self)
    XObj self;
{
    PState	*ps;
    Part	part;
    
    xTrace0(ipp, 1, "IP init");
#ifdef IP_SIM_DELAYS
    xError("Warning: IP is simulating delayed packets");
#endif
#ifdef IP_SIM_DROPS
    xError("Warning: IP is simulating dropped packets");
#endif
    ipProcessRomFile();
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xError("No llp configured below IP");
	return;
    }
    /* initialize protocol-specific state */
    ps = X_NEW(PState);
    self->state = (char *) ps;
    ps->self = self;
    ps->activeMap = mapCreate(IP_ACTIVE_MAP_SZ, sizeof(ActiveId));
    ps->fwdMap = mapCreate(IP_FORWARD_MAP_SZ, sizeof(FwdId));
    ps->passiveMap = mapCreate(IP_PASSIVE_MAP_SZ, sizeof(PassiveId));
    ps->passiveSpecMap = mapCreate(IP_PASSIVE_SPEC_MAP_SZ,
				   sizeof(PassiveSpecId));
    ps->fragMap = mapCreate(IP_FRAG_MAP_SZ, sizeof(FragId));
    xTrace1(ipp, 2, "IP has %d protocols below\n", self->numdown);
    /*
     * openenable physical network protocols
     */
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);	
    if ( xOpenEnable(self, self, xGetDown(self, 0), &part) == XK_FAILURE ) {
	xTrace0(ipp, TR_ERRORS, "ip_init : can't openenable net protocols");
    }
    /* 
     * Determine number of interfaces used by the lower protocol --
     * knowing this will simplify some of our routing decisions
     */
    if ( xControl(xGetDown(self, 0), VNET_GETNUMINTERFACES,
		  (char *)&ps->numIfc, sizeof(int)) <= 0 ) {
	xError("Couldn't do GETNUMINTERFACES control op");
	ps->numIfc = 1;
    } else {
	xTrace1(ipp, TR_MAJOR_EVENTS, "llp has %d interfaces", ps->numIfc);
    }
    /*
     * initialize route table and set up default route
     */
    if ( rt_init(ps, &ipSiteGateway) ) {
	xTrace0(ipp, TR_MAJOR_EVENTS, "IP rt_init -- no default gateway");
    }
    /*
     * set up function pointers for IP protocol object
     */
    self->open = ipOpen;
    self->close = ipCloseProtl;
    self->control = ipControlProtl;
    self->openenable = ipOpenEnable;
    self->opendisable = ipOpenDisable;
    self->demux = ipDemux;
    self->opendisableall = ipOpenDisableAll;
    scheduleIpFragCollector(ps);
    initSessionCollector(ps->activeMap, SESSN_COLLECT_INTERVAL,
			 destroyNormalSessn, "ip");
    initSessionCollector(ps->fwdMap, SESSN_COLLECT_INTERVAL,
			 destroyForwardSessn, "ip forwarding");
    xTrace0(ipp, 1, "IP init done");
}


static IPhost *
getHost( p )
    Part	*p;
{
    IPhost	*h;

    if ( !p || (partLen(p) < 1) ) {
	xTrace0(ipp, TR_SOFT_ERRORS, "ipGetHost: participant list error");
	return 0;
    }
    h = (IPhost *)partPop(p[0]);
    if ( h == 0 ) {
	xTrace0(ipp, TR_SOFT_ERRORS, "ipGetHost: empty participant stack");
    }
    return h;
}


/*
 * ipOpen
 */
static XObj
ipOpen(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    XObj	ip_s;
    IPhost      *remoteHost;
    IPhost      *localHost = 0;
    ActiveId    activeid;
    long	hlpNum;
    
    xTrace0(ipp, 3, "IP open");
    if ( (remoteHost = getHost(p)) == 0 ) {
	return ERR_XOBJ;
    }
    if ( (hlpNum = getRelProtNum(hlpType, self, "open")) == -1 ) {
	return ERR_XOBJ;
    }
    if ( partLen(p) > 1 ) {
	/* 
	 * Local participant has been explicitly specified
	 */
	localHost = (IPhost *)partPop(p[1]);
	if ( localHost == (IPhost *)ANY_HOST ) {
	    localHost = 0;
	}
    }
    xTrace2(ipp, 5, "IP sends to %s, %d", ipHostStr(remoteHost), hlpNum);
    
    /*
     * key on hlp prot number, destination addr, and local addr (if given)
     */
    activeid.protNum = hlpNum;
    activeid.remote = *remoteHost;
    if ( localHost ) {
	activeid.local = *localHost;
    }
    ip_s = createLocalSessn( self, hlpRcv, hlpType, &activeid, localHost );
    if ( ip_s != ERR_XOBJ ) {
	ip_s->idle = FALSE;
    }
    xTrace1(ipp, 3, "IP open returns %x", ip_s);
    return ip_s;
}


/* 
 * Create an IP session which sends to remote host key->dest.  The
 * 'rem' and 'prot' fields of 'key' will be used as passed in.
 *
 * 'localHost' specifies the host to be used in the header for
 * outgoing packets.  If localHost is null, an appropriate localHost will
 * be selected and used as the 'local' field of 'key'.  If localHost
 * is non-null, the 'local' field of 'key' will not be modified.
 */
static XObj
createLocalSessn( self, hlpRcv, hlpType, key, localHost )
    XObj	self, hlpRcv, hlpType;
    ActiveId 	*key;
    IPhost 	*localHost;
{
    PState	*ps = (PState *)self->state;
    SState	*ss;
    IPheader	*iph;
    IPhost	host;
    XObj	s;
    
    s = ipCreateSessn(self, hlpRcv, hlpType, localSessnInit, &key->remote);
    if ( s == ERR_XOBJ ) {
	return s;
    }
    /*
     * Determine my host address
     */
    if ( localHost ) {
	if ( ! ipIsMyAddr(self, localHost) ) {
	    xTrace1(ipp, TR_SOFT_ERROR, "%s is not a local IP host",
		    ipHostStr(localHost));
	    return ERR_XOBJ;
	}
    } else {
	if ( xControl(xGetDown(s, 0), GETMYHOST, (char *)&host,
		      sizeof(host)) < (int)sizeof(host) ) {
	    xTrace0(ipp, TR_SOFT_ERROR,
		    "IP open could not get interface info for remote host");
	    destroyNormalSessn(s);
	    return ERR_XOBJ;
	}
	localHost = &host;
	key->local = *localHost;
    }
    s->binding = mapBind(ps->activeMap, (char *)key, (int)s);
    if ( s->binding == ERR_BIND ) {
	xkern_return_t	res;

	xTrace0(ipp, TR_MAJOR_EVENTS, "IP open -- session already existed");
	destroyNormalSessn(s);
	res = mapResolve(ps->activeMap, key, &s);
	xAssert( res == XK_SUCCESS );
	return s;
    }
    ss = (SState *)s->state;
    iph = &ss->hdr;
    iph->source = *localHost;
    /*
     * fill in session template header
     */
    iph->vers_hlen = IPVERS;
    iph->vers_hlen |= 5;	/* default hdr length */
    iph->type = 0;
    iph->time = IPDEFAULTDGTTL;
    iph->prot = key->protNum;
    xTrace1(ipp, 4, "IP open: my ip address is %s",
	    ipHostStr(&iph->source));
    return s;
}


static XObj
ipCreateSessn( self, hlpRcv, hlpType, f, dst )
    XObj 	self, hlpRcv, hlpType;
    Pfv		f;
    IPhost	*dst;
{
    XObj	s;
    SState	*ss;

    s = xCreateSessn(f, hlpRcv, hlpType, self, 0, 0);
    ss = X_NEW(SState);
    s->state = (VOID *)ss;
    bzero((char *)ss, sizeof(SState));
    ss->hdr.dest = *dst;
    if ( ipHandleRedirect(s) ) {
	xTrace0(ipp, 3, "IP open fails");
	destroyNormalSessn(s);
	return ERR_XOBJ;
    }
    return s;
}


static void
localSessnInit(self)
     XObj self;
{
    self->push = ipPush;
    self->pop = ipStdPop;
    self->control = ipControlSessn;
    self->close = ipCloseSessn;
}


static void
fwdSessnInit(self)
     XObj self;
{
    self->pop = ipForwardPop;
}


/*
 * ipOpenEnable
 */
static xkern_return_t
ipOpenEnable(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PState 	*pstate = (PState *)self->state;
    IPhost	*localHost;
    long	protNum;
    
    xTrace0(ipp, TR_MAJOR_EVENTS, "IP open enable");
    if ( (localHost = getHost(p)) == 0 ) {
	return XK_FAILURE;
    }
    if ( (protNum = getRelProtNum(hlpType, self, "ipOpenEnable")) == -1 ) {
	return XK_FAILURE;
    }
    if ( localHost == (IPhost *)ANY_HOST ) {
	xTrace1(ipp, TR_MAJOR_EVENTS, "ipOpenEnable binding protocol %d",
		protNum);
	return defaultOpenEnable(pstate->passiveMap, hlpRcv, hlpType,
				 &protNum);
    } else {
	PassiveSpecId	key;

	if ( ! ipIsMyAddr(self, localHost) ) {
	    xTrace1(ipp, TR_MAJOR_EVENTS,
		    "ipOpenEnable -- %s is not one of my hosts",
		    ipHostStr(localHost));
	    return XK_FAILURE;
	}
	key.host = *localHost;
	key.prot = protNum;
	xTrace2(ipp, TR_MAJOR_EVENTS,
		"ipOpenEnable binding protocol %d, host %s",
		key.prot, ipHostStr(&key.host));
	return defaultOpenEnable(pstate->passiveSpecMap, hlpRcv, hlpType,
				 &key);
    }
}


/*
 * ipOpenDisable
 */
static xkern_return_t
ipOpenDisable(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PState      *pstate = (PState *)self->state;
    IPhost	*localHost;
    long	protNum;
    
    xTrace0(ipp, 3, "IP open disable");
    xAssert(self->state);
    xAssert(p);

    if ( (localHost = getHost(p)) == 0 ) {
	return XK_FAILURE;
    }
    if ( (protNum = getRelProtNum(hlpType, self, "ipOpenDisable")) == -1 ) {
	return XK_FAILURE;
    }
    if ( localHost == (IPhost *)ANY_HOST ) {
	xTrace1(ipp, TR_MAJOR_EVENTS,
		"ipOpenDisable unbinding protocol %d", protNum);
	return defaultOpenDisable(pstate->passiveMap, hlpRcv, hlpType,
				  &protNum);
    } else {
	PassiveSpecId	key;

	key.host = *localHost;
	key.prot = protNum;
	xTrace2(ipp, TR_MAJOR_EVENTS,
		"ipOpenDisable unbinding protocol %d, host %s",
		key.prot, ipHostStr(&key.host));
	return defaultOpenDisable(pstate->passiveSpecMap, hlpRcv, hlpType,
				  &key);
    }
}


/*
 * ipCloseSessn
 */
static xkern_return_t
ipCloseSessn(s)
    XObj s;
{
  xTrace1(ipp, 3, "IP close of session %x (does nothing)", s);
  xAssert(xIsSession(s));
  xAssert( s->rcnt == 0 );
  return XK_SUCCESS;
}


static void
destroyForwardSessn(s)
    XObj s;
{
    PState 	*ps = (PState *)(xMyProtl(s))->state;

    destroySessn(s, ps->fwdMap);
}    
  

static void
destroySessn(s, map)
    XObj 	s;
    Map		map;
{
    int		i;
    XObj	lls;
    
    xTrace1(ipp, 3, "IP DestroySessn %x", s);
    xAssert(xIsSession(s));
    if ( s->binding && s->binding != ERR_BIND ) {
	mapRemoveBinding(map, s->binding);
    }
    for (i=0; i < s->numdown; i++ ) {
	lls = xGetDown(s, i);
	if ( xIsSession(lls) ) {
	    xClose(lls);
	}
    }
    xDestroy(s);
}    
  

static void
destroyNormalSessn(s)
    XObj s;
{
    PState 	*ps = (PState *)(xMyProtl(s))->state;

    destroySessn(s, ps->activeMap);
}    
  


/*
 * ipCloseProtl
 */
static xkern_return_t
ipCloseProtl(self)
    XObj self;
{
  PState        *pstate;
  
  xAssert(xIsProtocol(self));
  xAssert(self->rcnt==1);
  
  pstate = (PState *) self->state;
  mapClose(pstate->activeMap);
  mapClose(pstate->passiveMap);
  mapClose(pstate->fragMap);
  xFree((char *) pstate);
  xDestroy(self);
  return XK_SUCCESS;
}

/*
 * ipPush
 */
static xmsg_handle_t
ipPush(s, msg)
    XObj s;
    Msg *msg;
{
    SState	*sstate;
    IPheader	hdr;
    
    xAssert(xIsSession(s));
    sstate = (SState *) s->state;
    
    hdr = sstate->hdr;
    hdr.ident = get_ident(s);
    hdr.dlen = msgLen(msg) + (GET_HLEN(&hdr) * 4);
    return ipSend(s, xGetDown(s, 0), msg, &hdr);
}


/*
 * Send the msg over the ip session's down session, fragmenting if necessary.
 * All header fields not directly related to fragmentation should already
 * be filled in.  We only reference the 'mtu' field of s->state (this
 * could be a forwarding session with a vestigial header in s->state,
 * so we use the header passed in as a parameter.)
 */
xmsg_handle_t
ipSend(s, lls, msg, hdr)
    XObj	s, lls;
    Msg 	*msg;
    IPheader 	*hdr;
{
    int 	hdrLen;
    int		len;
    SState	*sstate;

    sstate = (SState *)s->state;
    len = msgLen(msg);
    hdrLen = GET_HLEN(hdr);
    if ( len + hdrLen * 4 <= sstate->mtu ) {
	/*
	 * No fragmentation
	 */
	xTrace0(ipp,5,"IP send : message requires no fragmentation");
	msgPush(msg, ipHdrStore, hdr, hdrLen * 4, NULL);
	xIfTrace(ipp,5) {
	    xTrace0(ipp,5,"IP send unfragmented datagram header: \n");
	    ipDumpHdr(hdr);
	}
	return xPush(lls, msg);
    } else {
	/*
	 * Fragmentation required
	 */
	int 	fragblks;
	int	fragsize;
	Msg	fragmsg;
	int	offset;
	int	fraglen;
	xmsg_handle_t handle = XMSG_NULL_HANDLE;
	
	if ( hdr->frag & DONTFRAGMENT ) {
	    xTrace0(ipp,5,
		    "IP send: fragmentation needed, but NOFRAG bit set");
	    return XMSG_NULL_HANDLE;  /* drop it */
	}
	fragblks = (sstate->mtu - (hdrLen * 4)) / 8;
	fragsize = fragblks * 8;
	xTrace0(ipp,5,"IP send : datagram requires fragmentation");
	xIfTrace(ipp,5) {
	    xTrace0(ipp,5,"IP original datagram header :");
	    ipDumpHdr(hdr);
	}
	/*
	 * fragmsg = msg;
	 */
	xAssert(xIsXObj(lls));
	msgConstructEmpty(&fragmsg);
	for( offset = 0; len > 0; len -= fragsize, offset += fragblks) {
	    IPheader  	hdrToPush;
	    
	    hdrToPush = *hdr;
	    fraglen = len > fragsize ? fragsize : len;
	    msgChopOff(msg, &fragmsg, fraglen);
	    /*
	     * eventually going to need to selectively copy options
	     */
	    hdrToPush.frag += offset;
	    if ( fraglen != len ) {
		/*
		 * more fragments
		 */
		hdrToPush.frag |= MOREFRAGMENTS;
	    }
	    hdrToPush.dlen = hdrLen * 4 + fraglen;
	    xIfTrace(ipp,5) {
		xTrace0(ipp,5,"IP datagram fragment header: \n");
		ipDumpHdr(&hdrToPush);
	    }
	    msgPush(&fragmsg, ipHdrStore, &hdrToPush, hdrLen * 4, NULL);
	    if ( (handle =  xPush(lls, &fragmsg)) == XMSG_ERR_HANDLE ) {
		break;
	    }
	}
	msgDestroy(&fragmsg);
	return ( handle == XMSG_ERR_HANDLE ) ? handle : XMSG_NULL_HANDLE;
    }
}


Enable *
ipFindEnable( self, hlpNum, localHost )
    XObj	self;
    int		hlpNum;
    IPhost	*localHost;
{
    PState		*ps = (PState *)self->state;
    Enable		*e = ERR_ENABLE;
    PassiveId		key = hlpNum;
    PassiveSpecId	specKey;

    if ( mapResolve(ps->passiveMap, &key, &e) == XK_SUCCESS ) {
	xTrace1(ipp, TR_MAJOR_EVENTS,
		"Found an enable object for prot %d", key);
    } else {
	specKey.prot = key;
	specKey.host = *localHost;
	if ( mapResolve(ps->passiveSpecMap, &specKey, &e) == XK_SUCCESS ) {
	    xTrace2(ipp, TR_MAJOR_EVENTS,
		    "Found an enable object for prot %d host %s",
		    specKey.prot, ipHostStr(&specKey.host));
	}
    }
    return e;
}


static XObj
localPassiveSessn( self, actKey, localHost )
    XObj 	self;
    ActiveId 	*actKey;
    IPhost	*localHost;
{
    Enable		*e;

    e = ipFindEnable(self, actKey->protNum, localHost);
    if ( e == ERR_ENABLE ) {
	return ERR_XOBJ;
    }
    return createLocalSessn(self, e->hlpRcv, e->hlpType, actKey, localHost); 
    /* 
     * openDone will get called in validateOpenEnable
     */
}


static XObj
fwdBcastSessn( self, llsIn, actKey, fwdKey )
    XObj 	self, llsIn;
    ActiveId 	*actKey;
    FwdId	*fwdKey;
{
    XObj	s;
    Part	p;
    XObj	lls;
    IPhost	localHost;

    xTrace0(ipp, TR_MAJOR_EVENTS, "creating forward broadcast session");
    if ( xControl(llsIn, GETMYHOST, (char *)&localHost, sizeof(IPhost)) < 0 ) {
	return ERR_XOBJ;
    }
    if ( (s = localPassiveSessn(self, actKey, &localHost)) == ERR_XOBJ ) {
	/* 
	 * There must not have been an openenable for this msg type --
	 * this will just be a forwarding session
	 */
	if ( (s = forwardSessn(self, actKey, fwdKey)) == ERR_XOBJ ) {
	    return ERR_XOBJ;
	}
	xSetDown(s, 1, xGetDown(s, 0));
	xSetDown(s, 0, 0);
    } else {
	/* 
	 * This will be a local session with an extra down session for
	 * the forwarding of broadcasts
	 */
	partInit(&p, 1);
	partPush(p, &actKey->local, sizeof(IPhost));
	if ( (lls = xOpen(self, self, xGetDown(self, 0), &p)) == ERR_XOBJ ) {
	    xTrace0(ipp, TR_ERRORS, "ipFwdBcastSessn couldn't open lls");
	    return ERR_XOBJ;
	}
	xSetDown(s, 1, lls);
    }
    s->pop = ipFwdBcastPop;
    return s;
}


static XObj
forwardSessn( self, actKey, fwdKey )
    XObj	self;
    ActiveId	*actKey;
    FwdId	*fwdKey;
{
    PState	*ps = (PState *)self->state;
    XObj	s;

    xTrace2(ipp, TR_MAJOR_EVENTS,
	    "creating forwarding session to net %s (host %s)",
	    ipHostStr(fwdKey), ipHostStr(&actKey->local));
    s = ipCreateSessn(self, xNullProtl, xNullProtl, fwdSessnInit,
		      &actKey->local);
    if ( s == ERR_XOBJ ) {
	return s;
    }
    s->binding = mapBind(ps->fwdMap, (char *)fwdKey, (int)s);
    xAssert( s->binding != ERR_BIND );
    return s;
}


XObj
ipCreatePassiveSessn( self, lls, actKey, fwdKey )
    XObj 	self, lls;
    ActiveId	*actKey;
    FwdId	*fwdKey;
{
    PState		*ps = (PState *)self->state;
    VnetClassBuf	buf;
    XObj		s = ERR_XOBJ;

    buf.host = actKey->local;
    if ( xControl(xGetDown(self, 0), VNET_GETADDRCLASS,
		  (char *)&buf, sizeof(buf)) < (int)sizeof(buf) ) {
	xTrace0(ipp, TR_ERRORS,
		"ipCreatePassiveSessn: GETADDRCLASS failed");
	return ERR_XOBJ;
    }
    switch( buf.class ) {
      case LOCAL_ADDR_C:
	/* 
	 * Normal session 
	 */
	s = localPassiveSessn(self, actKey, &actKey->local);
	break;

      case REMOTE_HOST_ADDR_C:
      case REMOTE_NET_ADDR_C:
	s = forwardSessn(self, actKey, fwdKey);
	break;
	    
      case BCAST_SUBNET_ADDR_C:
	if ( ps->numIfc > 1 ) {
	    /* 
	     * Painfully awkward forward/local consideration session
	     */
	    s = fwdBcastSessn(self, lls, actKey, fwdKey);
	    break;
	}
	/* 
	 * Else fallthrough
	 */

      case BCAST_LOCAL_ADDR_C:
      case BCAST_NET_ADDR_C:
	{
	    IPhost	localHost;

	    /* 
	     * Almost a normal session -- need to be careful about our
	     * source address 
	     */
	    if ( xControl(lls, GETMYHOST, (char *)&localHost, sizeof(IPhost))
		< 0 ) {
		return ERR_XOBJ;
	    }
	    s = localPassiveSessn(self, actKey, &localHost);
	}
	break;

    }
    return s;
}


/*
 * ipHandleRedirect -- called when the ip session's lower session needs
 * to be (re)opened.  This could be when the ip session is first created
 * and the lower session is noneistent, or when a redirect is received
 * for this session's remote network.  The router is
 * consulted for the best interface.  The new session is assigned to
 * the first position in the ip session's down vector.  The old session,
 * if it existed, is freed.
 *
 * Note that the local IPhost field of the header doesn't change even
 * if the route changes.
 * 
 * preconditions: 
 * 	s->state should be allocated
 * 	s->state->hdr.dest should contain the ultimate remote address or net
 *
 * return values:
 *	0 if lower session was succesfully opened and assigned
 *	1 if lower session could not be opened -- old lower session is
 *		not affected
 */
static int
ipHandleRedirect(s)
    XObj s;
{
    XObj	ip = xMyProtl(s);
    XObj	llp = xGetDown(ip, 0);
    XObj 	lls, llsOld;
    SState	*ss = (SState *)s->state;
    route	*rt;
    Part	p;
    int		res;
    
    /*
     * 'host' is the remote host to which this session sends packets,
     * not necessarily the final destination
     */
    xAssert(xIsSession(s));
    partInit(&p, 1);
    partPush(p, &ss->hdr.dest, sizeof(IPhost));
    if ( (lls = xOpen(ip, ip, llp, &p)) == ERR_XOBJ ) {
	xTrace0(ipp, TR_EVENTS,
		"ipHandleRedirect could not get direct lower session");
	if ( (rt = rt_get(&ss->hdr.dest)) == 0 ) {
	    xTrace0(ipp, TR_SOFT_ERRORS,
		    "ipHandleRedirect could not find route");
	    return 1;
	}
	partInit(&p, 1);
	partPush(p, &rt->gw, sizeof(IPhost));
	if ( (lls = xOpen(ip, ip, llp, &p)) == ERR_XOBJ ) {
	    xTrace0(ipp, TR_ERRORS,
		    "ipHandleRedirect could not get gateway lower session");
	    return 1;
	}
    }
    xTrace0(ipp, 5, "Successfully opened lls");
    /*
     * Determine mtu for this interface
     */
    res = xControl(lls, GETMAXPACKET, (char *)&ss->mtu, sizeof(int));
    if (res < 0 || ss->mtu <= 0) {
	xTrace0(ipp, 3, "Could not determine interface mtu");
	ss->mtu = IPOPTPACKET;
    }
    if ( xIsSession(llsOld = xGetDown(s, 0)) ) {
	xClose(llsOld);
    }
    xSetDown(s, 0, lls);
    return 0;
}


/* 
 * Misc. routines 
 */
static int
get_ident( s )
    XObj	s;
{
    static int n = 1;
    return n++;
}


static void
callRedirect(ev, s)
    Event	ev;
    VOID	*s;
{
    xTrace1(ipp, 4, "ip: callRedirect runs with session %x", s);
    ipHandleRedirect((XObj) s);
    xClose((XObj)s);
    return;
}


typedef struct {
    int 	(* affected)(
#ifdef __STDC__
			     PState *, IPhost *, route *
#endif
			     );
    route	*rt;
    PState	*pstate;
} RouteChangeInfo;

/* 
 * ipRouteChanged -- For each session in the active map, determine if a
 * change in the given route affects that session.  If it does, the
 * session is reconfigured appropriately.
 */
void
ipRouteChanged(pstate, rt, routeAffected)
    PState *pstate;
    route *rt;
    int (*routeAffected)(
#ifdef __STDC__
			 PState *, IPhost *, route *
#endif			 
			 );
{
    RouteChangeInfo	rInfo;

    rInfo.affected = routeAffected;
    rInfo.rt = rt;
    rInfo.pstate = pstate;
    mapForEach(pstate->activeMap, routeChangeFilter, &rInfo);
    mapForEach(pstate->fwdMap, routeChangeFilter, &rInfo);
}
  

static int
routeChangeFilter(key, value, arg)
    VOID *key, *value, *arg;
{
    RouteChangeInfo	*rInfo = (RouteChangeInfo *)arg;
    XObj		s = (XObj)value;
    SState		*state;
    
    xAssert(xIsSession(s));
    state = (SState *)s->state;
    xTrace3(ipp, 4, "ipRouteChanged does net %s affect ses %x, dest %s?",
	    ipHostStr(&rInfo->rt->net), s, ipHostStr(&state->hdr.dest));
    if ( rInfo->affected(rInfo->pstate, &state->hdr.dest, rInfo->rt) ) {
	xTrace1(ipp, 4,
		"session %x affected -- reopening lower session", s);
	xDuplicate(s);
	evDetach( evSchedule(callRedirect, s, 0) );
    } else {
	xTrace1(ipp, 4, "session %x unaffected by routing change", s);
    }
    return MFE_CONTINUE;
}



	/*
	 * Functions used as arguments to ipRouteChanged
	 */
/*
 * Return true if the remote host is not connected to the local net
 */
int
ipRemoteNet( ps, dest, rt )
    PState	*ps;
    IPhost 	*dest;
    route 	*rt;
{
    return ! ipHostOnLocalNet(ps, dest);
}


/*
 * Return true if the remote host is on the network described by the
 * route.
 */
int
ipSameNet(pstate, dest, rt)
    PState *pstate;
    IPhost *dest;
    route *rt;
{
    return ( (dest->a & rt->mask.a) == (rt->net.a & rt->mask.a) &&
	     (dest->b & rt->mask.b) == (rt->net.b & rt->mask.b) &&
	     (dest->c & rt->mask.c) == (rt->net.c & rt->mask.c) &&
	     (dest->d & rt->mask.d) == (rt->net.d & rt->mask.d) );
}
