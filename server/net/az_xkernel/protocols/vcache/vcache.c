/* 
 * vcache.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/01 22:35:54 $
 */

/* 
 * VCACHE -- a session caching protocol.
 */

#include "xkernel.h"
#include "gc.h"
#include "vcache.h"
#include "vcache_i.h"
#include "bidctl.h"

#ifdef __STDC__

static void		addActiveSessn( PState *, XObj );
static void		addIdleSessn( PState *, XObj );
static void		getRomEntries( XObj, int * );
static void		protlFuncInit( XObj );
static void		remActiveSessn( PState *, XObj );
static void		remIdleSessn( PState *, XObj );
static xkern_return_t	restoreIdleServer( XObj );
static void		sessnInit( XObj );
static xkern_return_t	vcacheCall( XObj, Msg * , Msg * );
static xkern_return_t	vcacheCallPop( XObj, XObj, Msg *, VOID *, Msg * );
static void		vcacheDestroy( XObj );
static void		vcacheDestroyIdle( XObj );
static XObj		vcacheCreateSessn( XObj, XObj, XObj, XObj );
static XObj		vcacheOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	vcacheOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t	vcacheOpenDone( XObj, XObj, XObj, XObj );
static xkern_return_t	vcacheOpenEnable( XObj, XObj, XObj, Part * );
static xkern_return_t	vcachePop( XObj, XObj, Msg *, VOID * );
static xkern_return_t	vcacheProtlCallDemux( XObj, XObj, Msg * , Msg * );
static xkern_return_t	vcacheProtlDemux( XObj, XObj, Msg * );
static xmsg_handle_t	vcachePush( XObj, Msg * );
static xkern_return_t	vcacheSessnCallDemux( XObj, XObj, Msg * , Msg * );
static xkern_return_t	vcacheSessnDemux( XObj, XObj, Msg * );

#else

static void		addActiveSessn();
static void		addIdleSessn();
static void		getRomEntries();
static void		protlFuncInit();
static void		remActiveSessn();
static void		remIdleSessn();
static xkern_return_t	restoreIdleServer();
static void		sessnInit();
static xkern_return_t	vcacheCall();
static xkern_return_t	vcacheCallPop();
static void		vcacheDestroy();
static void		vcacheDestroyIdle();
static XObj		vcacheCreateSessn();
static XObj		vcacheOpen();
static xkern_return_t	vcacheOpenDisable();
static xkern_return_t	vcacheOpenDone();
static xkern_return_t	vcacheOpenEnable();
static xkern_return_t	vcacheProtlCallDemux();
static xmsg_handle_t	vcachePush();
static xkern_return_t	vcacheSessnCallDemux();

#endif __STDC__


int tracevcachep;


void
vcache_init( self )
    XObj 	self;
{
    PState	*ps;
    int		collectInterval;
    
    xTrace0(vcachep, TR_GROSS_EVENTS, "VCACHE init");
    xAssert(xIsProtocol(self));
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xError("VCACHE down vector is misconfigured");
	return;
    }
    ps = X_NEW(PState);
    self->state = (VOID *)ps;
    ps->activeMap = mapCreate(VCACHE_ALL_SESSN_MAP_SZ, sizeof(XObj));
    ps->idleMap = mapCreate(VCACHE_ALL_SESSN_MAP_SZ, sizeof(XObj));
    ps->passiveMap = mapCreate(VCACHE_HLP_MAP_SZ, sizeof(XObj));
    ps->symmetric = FALSE;
    collectInterval = VCACHE_COLLECT_INTERVAL;
    getRomEntries(self, &collectInterval);
    xTrace1(vcachep, TR_EVENTS, "VCACHE uses collect interval of %d seconds",
	    collectInterval );
    protlFuncInit(self);
    initSessionCollector(ps->idleMap, collectInterval * 1000 * 1000, 
			 vcacheDestroyIdle, "vcache");
}


static void
getRomEntries( self, collectInterval )
    XObj	self;
    int		*collectInterval;
{
#define ERROR	\
		sprintf(errBuf, "VCACHE romfile format error in line %d", \
			i+1);	\
		xError(errBuf);	\
		continue;	\

    PState	*ps = (PState *)self->state;
    int		i;
    char	fullName[80];

    sprintf(fullName, "%s%c%s", self->name, *self->instName ? '/' : 0,
	    self->instName);
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], fullName) ) {
	    if ( ! rom[i][1] ) {
		ERROR;
	    }
	    if ( ! strncmp(rom[i][1], "symmetric") ) {
		xTrace1(vcachep, TR_EVENTS, "%s symmetric (romfile)",
			fullName);
		ps->symmetric = TRUE;
	    } else if ( ! strncmp(rom[i][1], "asymmetric") ) {
		xTrace1(vcachep, TR_EVENTS, "%s asymmetric (romfile)",
			fullName);
		ps->symmetric = FALSE;
	    } else if ( ! strncmp(rom[i][1], "interval") ) {
		if ( ! rom[i][2] ) {
		    ERROR;
		}
		sscanf(rom[i][2], "%d", collectInterval);
		xTrace2(vcachep, TR_EVENTS,
			"%s collect interval %d (romfile)",
			fullName, *collectInterval);
	    } else {
		sprintf(errBuf, "VCACHE -- unrecognized romfile option %s",
			rom[i][1]);
		xError(errBuf);
	    }
	}
    }
}


static XObj
vcacheOpen( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part 	*p;
{
    XObj	s, lls;
    PState	*ps = (PState *)self->state;
    
    xTrace0(vcachep, TR_MAJOR_EVENTS, "VCACHE open");
    if ( (lls = xOpen(self, hlpType, xGetDown(self, 0), p)) == ERR_XOBJ ) {
	xTrace0(vcachep, TR_ERRORS, "vcacheOpen: could not open lower sessn");
	return ERR_XOBJ;
    }
    if ( mapResolve(ps->activeMap, &lls, &s) == XK_SUCCESS ) {
	xClose(lls);
	return s;
    }
    if ( mapResolve(ps->idleMap, &lls, &s) == XK_SUCCESS ) {
	xClose(lls);
	xSetUp(s, hlpRcv);
	addActiveSessn(ps, s);
	return s;
    }
    s = vcacheCreateSessn(self, hlpRcv, hlpType, lls);
    ((SState *)s->state)->cachable = ps->symmetric;
    if ( s == ERR_XOBJ ) {
	xClose(lls);
    }
    addActiveSessn(ps, s);
    return s;
}


static XObj
vcacheCreateSessn( self, hlpRcv, hlpType, lls )
    XObj 	self, hlpRcv, hlpType, lls;
{
    XObj	s;
    SState	*ss;
    IPhost	peer;
    
    if ( xControl(lls, GETPEERHOST, (char *)&peer, sizeof(IPhost))
				< (int)sizeof(IPhost) ) {
	xTrace0(vcachep, TR_ERRORS,
		"vcacheCreateSessn could not get peer host");
	return ERR_XOBJ;
    }
    if ( (s = xCreateSessn(sessnInit, hlpRcv, hlpType, self, 1, &lls))
			== ERR_XOBJ ) {
	xTrace0(vcachep, TR_ERRORS, "create sessn fails in vcacheOpen");
	return ERR_XOBJ;
    }
    ss = X_NEW(SState);
    s->state = (VOID *)ss;
    /*
     * The lower sessions' up field is made to point to this
     * vcache session (not the protocol)
     */
    xSetUp(lls, s);
    return s;
}




static int
vcacheControlSessn( s, opcode, buf, len )
    XObj 	s;
    int 	opcode, len;
    char 	*buf;
{
    xTrace0(vcachep, TR_EVENTS, "VCACHE controlsessn");
    /*
     * All opcodes are forwarded to the lower session.  
     */
    return xControl(xGetDown(s, 0), opcode, buf, len);
}


static int
vcacheControlProtl( self, opcode, buf, len )
    XObj 	self;
    int 	opcode, len;
    char 	*buf;
{
	/*
	 * All opcodes are forwarded to the lower protocol
	 */
	return xControl(xGetDown(self, 0), opcode, buf, len);
}


static xkern_return_t
vcacheOpenEnable( self, hlpRcv, hlpType, p )
    XObj 	self, hlpRcv, hlpType;
    Part 	*p;
{
    PState	*ps = (PState *)self->state;
    
    xTrace0(vcachep, TR_MAJOR_EVENTS, "VCACHE open enable");
    return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
				    self->down, p);
}


static xkern_return_t
vcacheOpenDisable( self, hlpRcv, hlpType, p )
    XObj 	self, hlpRcv, hlpType;
    Part 	*p;
{
    PState	*ps = (PState *)self->state;

    return defaultVirtualOpenDisable(self, ps->passiveMap, hlpRcv, hlpType,
				     self->down, p);
}


static xkern_return_t
vcacheClose( s )
    XObj	s;
{
    SState		*ss = (SState *)s->state;
    PState 		*ps = (PState *)xMyProtl(s)->state;

    remActiveSessn(ps, s);
    if ( ss->cachable ) {
	xSetUp(s, 0);
	xTrace1(vcachep, TR_EVENTS, "VCACHE caching session %x", s);
	addIdleSessn(ps, s);
    } else {
	xTrace1(vcachep, TR_MAJOR_EVENTS, "VCACHE destroying session %x", s);
	vcacheDestroy(s);
    }
    return XK_SUCCESS;
}


static void
vcacheDestroyIdle( s )
    XObj	s;
{
    xAssert(s->rcnt == 0);
    remIdleSessn((PState *)xMyProtl(s)->state, s);
    vcacheDestroy(s);
}


static void
vcacheDestroy( s )
    XObj 	s;
{
    XObj	lls;
    
    xTrace1(vcachep, TR_MAJOR_EVENTS, "VCACHE destroy of session %x", s);
    lls = xGetDown(s, 0);
    xAssert(xIsSession(lls));
    xSetUp(lls, xMyProtl(s));
    xClose(lls);
    xDestroy(s);
}


static xkern_return_t
vcacheCall( s, msg, rmsg )
    XObj	s;
    Msg 	*msg, *rmsg;
{
    xTrace0(vcachep, TR_EVENTS, "vcacheCall");
    return xCall(xGetDown(s, 0), msg, rmsg);
}


static xmsg_handle_t
vcachePush( s, msg )
    XObj	s;
    Msg 	*msg;
{
    xTrace0(vcachep, TR_EVENTS, "vcachePush");
    return xPush(xGetDown(s, 0), msg);
}


static xkern_return_t
vcacheOpenDone( self, lls, llp, hlpType )
    XObj	self, lls, llp, hlpType;
{
    XObj	s;
    PState	*ps = (PState *)self->state;
    Enable	*e;
    
    xTrace0(vcachep, TR_MAJOR_EVENTS, "In VCACHE openDone");
    if ( self == hlpType ) {
	xTrace0(vcachep, TR_ERRORS, "self == hlpType in vcacheOpenDone");
	return XK_FAILURE;
    }
    /*
     * check for openEnable
     */
    if ( mapResolve(ps->passiveMap, &hlpType, &e) == XK_FAILURE ) {
	/* 
	 * This shouldn't happen
	 */
	xTrace0(vcachep, TR_ERRORS,
		"vcacheOpenDone: Couldn't find hlp for incoming session");
	return XK_FAILURE;
    }
    if ( (s = vcacheCreateSessn(self, e->hlpRcv, e->hlpType, lls))
							== ERR_XOBJ ) {
	return XK_FAILURE;
    }
    xDuplicate(lls);
    ((SState *)s->state)->cachable = TRUE;
    addIdleSessn(ps, s);
    xTrace0(vcachep, TR_EVENTS,
	    "vcache Passively opened session successfully created");
    return xOpenDone(e->hlpRcv, s, self);
}


static xkern_return_t
vcacheProtlCallDemux( self, lls, m, rmsg )
    XObj	self, lls;
    Msg		*m, *rmsg;
{
    xTrace0(vcachep, TR_ERRORS, "vcacheProtlCallDemux called!!");
    return XK_SUCCESS;
}


static xkern_return_t
vcacheProtlDemux( self, lls, m )
    XObj	self, lls;
    Msg		*m;
{
    xTrace0(vcachep, TR_ERRORS, "vcacheProtlDemux called!!");
    return XK_SUCCESS;
}


static xkern_return_t
restoreIdleServer( s )
    XObj	s;
{
    Enable	*e;
    XObj	hlpType;
    PState	*ps;

    xTrace1(vcachep, TR_EVENTS, "VCACHE restoring session %x", s);
    ps = (PState *)xMyProtl(s)->state;
    hlpType = xHlpType(s);
    if ( mapResolve(ps->passiveMap, &hlpType, &e) == XK_FAILURE ) {	
	xTrace0(vcachep, TR_EVENTS, "vcachePop -- no openEnable!");	
	return XK_FAILURE;
    }								
    remIdleSessn(ps, s);
    addActiveSessn(ps, s);
    xOpenDone(e->hlpRcv, s, xMyProtl(s));
    return XK_SUCCESS;
}


/* 
 * vcachePop and vcacheSessnDemux must be used (i.e., they can't be
 * bypassed) for the UPI reference count mechanism to work properly. 
 */
static xkern_return_t
vcacheCallPop( s, lls, msg, h, rmsg )
    XObj 	s, lls;
    Msg 	*msg, *rmsg;
    VOID	*h;
{
    xTrace0(vcachep, TR_EVENTS, "vcache callPop");
    if ( s->rcnt == 1 ) {
	if ( restoreIdleServer(s) == XK_FAILURE ) {
	    return XK_FAILURE;
	}
    }
    return xCallDemux(s, msg, rmsg);
}


static xkern_return_t
vcacheSessnCallDemux( self, lls, msg, rmsg )
    XObj 	self, lls;
    Msg 	*msg, *rmsg;
{
    xTrace0(vcachep, TR_EVENTS, "vcache Session callDemux");
    return xCallPop(self, lls, msg, 0, rmsg);
}


static xkern_return_t
vcachePop( s, lls, msg, h )
    XObj 	s, lls;
    Msg 	*msg;
    VOID	*h;
{
    xTrace0(vcachep, TR_EVENTS, "vcache Pop");
    if ( s->rcnt == 1 ) {
	if ( restoreIdleServer(s) == XK_FAILURE ) {
	    return XK_FAILURE;
	}
    }
    return xDemux(s, msg);
}


static xkern_return_t
vcacheSessnDemux( self, lls, msg )
    XObj 	self, lls;
    Msg 	*msg;
{
    xTrace0(vcachep, TR_EVENTS, "vcache Session Demux");
    return xPop(self, lls, msg, 0);
}


static xkern_return_t
vcacheCloseDone( lls )
    XObj	lls;
{
    XObj	s;	/* vcache session */

    s = xGetUp(lls);
    xAssert(xIsSession(s));
    if ( xGetUp(s) ) {
	return xCloseDone(s);
    } else {
	return XK_SUCCESS;
    }
}


static void
sessnInit( s )
    XObj 	s;
{
    xAssert(xIsSession(s));
    
    s->call = vcacheCall;
    s->push = vcachePush;
    s->callpop = vcacheCallPop;
    s->pop = vcachePop;
    s->close = vcacheClose;
    s->control = vcacheControlSessn;
    /* 
     * VCACHE sessions will look like a protocol to lower sessions, so we
     * need a demux function
     */
    s->calldemux = vcacheSessnCallDemux;
    s->demux = vcacheSessnDemux;
    s->closedone = vcacheCloseDone;
}


static void
protlFuncInit(p)
    XObj p;
{
    xAssert(xIsProtocol(p));

    p->control = vcacheControlProtl;
    p->open = vcacheOpen;
    p->openenable = vcacheOpenEnable;
    p->opendisable = vcacheOpenDisable;
    p->calldemux = vcacheProtlCallDemux;
    p->demux = vcacheProtlDemux;
    p->opendone = vcacheOpenDone;
}


/* 
 * Add the session s to the active map
 */
static void
addActiveSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    XObj	lls;

    lls = xGetDown(s, 0);
    if ( ! xIsSession(lls) ) {
	xError("VCACHE addSession -- bogus lls!");
	xAssert(0);
	return;
    }
    xAssert( mapResolve(ps->activeMap, &lls, 0) == XK_FAILURE );
    s->binding = mapBind(ps->activeMap, &lls, s);
    xAssert(s->binding != ERR_BIND);
}



/* 
 * Add the session s to the idle map
 */
static void
addIdleSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    XObj	lls;

    lls = xGetDown(s, 0);
    if ( ! xIsSession(lls) ) {
	xError("VCACHE addIdleSession -- bogus lls!");
	xAssert(0);
	return;
    }
    /* 
     * Reset the session's garbage-collection 'idle' flag so it isn't
     * immediately collected. 
     */
    s->idle = FALSE;
    xAssert( mapResolve(ps->idleMap, &lls, 0) == XK_FAILURE );
    s->binding = mapBind(ps->idleMap, &lls, s);
    xAssert( s->binding != ERR_BIND );
}


static void
remIdleSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    xkern_return_t	res;

    res = mapRemoveBinding(ps->idleMap, s->binding);
    xAssert( res == XK_SUCCESS );
}


static void
remActiveSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    xkern_return_t	res;

    res = mapRemoveBinding(ps->activeMap, s->binding);
    xAssert( res == XK_SUCCESS );
}
