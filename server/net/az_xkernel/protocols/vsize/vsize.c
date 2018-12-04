/* 
 * vsize.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.23 $
 * $Date: 1993/02/01 22:31:31 $
 */


#include "xkernel.h"
#include "vsize_i.h"
#include "vsize.h"

#ifdef __STDC__

static void	protlFuncInit( XObj );
static void	sessnInit( XObj );
static XObj	vsizeCreateSessn( XObj, XObj, XObj, XObj * );
static XObj	vsizeOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	vsizeOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t	vsizeOpenDone( XObj, XObj, XObj, XObj );
static xkern_return_t	vsizeOpenEnable( XObj, XObj, XObj, Part * );
static xkern_return_t	vsizePop( XObj, XObj, Msg *, VOID * );

#else

static void	protlFuncInit();
static void	sessnInit();
static XObj	vsizeCreateSessn();

#endif __STDC__

int tracevsizep;


void
vsize_init(self)
    XObj self;
{
    PSTATE	*pstate;
    
    xTrace0(vsizep, TR_GROSS_EVENTS, "VSIZE init");
    xAssert(xIsProtocol(self));
    xAssert(self->numdown <= STD_DOWN);
    if ( ! xIsProtocol(xGetDown(self, LPI)) ||
	 ! xIsProtocol(xGetDown(self, SPI)) ) {
	xError("vsize down vector is misconfigured");
	return;
    }
    pstate = X_NEW(PSTATE);
    self->state = (VOID *)pstate;
    protlFuncInit(self);
    pstate->activeMap = mapCreate(11, sizeof(ActiveId));
    pstate->passiveMap = mapCreate(11, sizeof(PassiveId));
}


static XObj
vsizeOpen(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    XObj	s;
    XObj	lls[2];
    Part  	savedPart[3];
    PSTATE	*pstate;
    int		plen;
    
    xTrace0(vsizep, TR_MAJOR_EVENTS, "VSIZE open");
    pstate = (PSTATE *)self->state;
    if ( ! p || partLen(p) < 1 || partLen(p) > (3)) { 
	xTrace0(vsizep, TR_ERRORS, "VSIZE open -- bad participants");
	return ERR_XOBJ;
    }								
    /* 
     * Save the original participants before opening since we need to
     * use it in both opens and it will get munged in the first open
     */
    plen = partLen(p) * sizeof(Part);
    bcopy((char *)p, (char *)savedPart, plen);
    /*
     * Opening a large-packet session.
     */
    lls[LPI] = xOpen(self, hlpType, xGetDown(self, LPI), p);
    if ( lls[LPI] == ERR_XOBJ ) {
	xTrace0(vsizep, TR_ERRORS, "vsize_open: could not open large session");
	return ERR_XOBJ;
    }
    /*
     * Opening a short-packet session.  
     */
    bcopy((char *)savedPart, (char *)p, plen);
    lls[SPI] = xOpen(self, hlpType, xGetDown(self, SPI), p);
    if ( lls[SPI] == ERR_XOBJ ) {
	xTrace0(vsizep, TR_ERRORS, "vsize_open: could not open short session");
	xClose(lls[LPI]);
	return ERR_XOBJ;
    }
    if ( mapResolve(pstate->activeMap, lls, &s) == XK_SUCCESS ) {
	xTrace0(vsizep, TR_MAJOR_EVENTS, "found an existing one");
	xClose(lls[SPI]);
	xClose(lls[LPI]);
    } else {
	xTrace0(vsizep, TR_MAJOR_EVENTS, "creating a new one");
	if ( (s = vsizeCreateSessn(self, hlpRcv, hlpType, lls)) == ERR_XOBJ ) {
	    xClose(lls[SPI]);
	    xClose(lls[LPI]);
	}
    }
    return s;
}


/* 
 * vsizeCreateSessn --
 * Create and initialize a new VSIZE session using the lls's in the array.
 * Assumes no session already exists corresponding to the lls's in the array
 */
static XObj
vsizeCreateSessn(self, hlpRcv, hlpType, lls)
    XObj self, hlpRcv, hlpType;
    XObj *lls;
{
    XObj	s;
    SSTATE	*sstate;
    PSTATE	*pstate = (PSTATE *)self->state;
    
    if ( (s = xCreateSessn(sessnInit, hlpRcv, hlpType, self, 2, lls))
			== ERR_XOBJ ) {
	xTrace0(vsizep, TR_ERRORS, "create sessn fails in vsizeOpen");
	return ERR_XOBJ;
    }
    sstate = X_NEW(SSTATE);
    s->state = (VOID *)sstate;
    s->binding = mapBind(pstate->activeMap, (char *)lls, (int)s);
    if ( s->binding == ERR_BIND ) {
	xTrace0(vsizep, TR_ERRORS, "mapBind fails in vsizeCreateSessn");
	xFree((char *)sstate);
	return ERR_XOBJ;
    }
    if ( xControl(lls[SPI], GETOPTPACKET, (char *)&sstate->cutoff,
		  sizeof(sstate->cutoff)) <= 0 ) {
	xError("VSIZE could not get lls opt packet size.");
	/* 
	 * Try to recover by using a default
	 */
	sstate->cutoff = DEFAULT_CUTOFF;
    }
    xTrace1(vsizep, TR_EVENTS, "new VSIZE sessn uses cutoff of %d",
	    sstate->cutoff);
    /*
     * The lower sessions' up fields are made to point to this
     * vsize session (not the protocol)
     */
    xSetUp(lls[LPI], s);
    xSetUp(lls[SPI], s);
    xTrace1(vsizep, TR_MAJOR_EVENTS, "VSIZE open returns %x", s);
    return s;
}


static int
vsizeControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
    xTrace0(vsizep, TR_EVENTS, "VSIZE controlsessn");
    /*
     * All opcodes are forwarded to the long-pckt session.  
     */
    return xControl(xGetDown(s, LPI), opcode, buf, len);
}


static int
vsizeControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    /*
     * All opcodes are forwarded to the long-pckt protocol
     */
    return xControl(xGetDown(self, LPI), opcode, buf, len);
}


static xkern_return_t
vsizeOpenEnable( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PSTATE	*ps = (PSTATE *)self->state;
    
    xTrace0(vsizep, TR_MAJOR_EVENTS, "VSIZE open enable");
    return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
				    self->down, p);
}


static xkern_return_t
vsizeOpenDisable(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PSTATE	*ps = (PSTATE *)self->state;

    return defaultVirtualOpenDisable(self, ps->passiveMap, hlpRcv, hlpType,
				     self->down, p);
}


static xkern_return_t
vsizeClose(s)
    XObj s;
{
    PSTATE		*pstate;
    xkern_return_t	res;
    
    xTrace1(vsizep, TR_MAJOR_EVENTS, "VSIZE close of session %x", s);
    xAssert(s->rcnt == 0);
    pstate = (PSTATE *)s->myprotl->state;
    res = mapRemoveBinding(pstate->activeMap, s->binding);
    xAssert( res != XK_FAILURE );
    xSetUp(xGetDown(s, SPI), xMyProtl(s));
    xClose(xGetDown(s, SPI));
    xSetUp(xGetDown(s, LPI), xMyProtl(s));
    xClose(xGetDown(s, LPI));
    xDestroy(s);
    return XK_SUCCESS;
}


static xmsg_handle_t
vsizePush(s, msg)
    XObj s;
    Msg *msg;
{
    SSTATE	*sstate;
    
    xTrace0(vsizep, TR_EVENTS, "in vsize push");
    sstate = (SSTATE *)s->state;
    if ( msgLen(msg) <= sstate->cutoff ) {
	xTrace0(vsizep, TR_MORE_EVENTS,
		"vsize_push: pushing to short session");
	xAssert(xIsSession(xGetDown(s, SPI)));
	return xPush(xGetDown(s, SPI), msg);
    } else {
	xTrace0(vsizep, TR_MORE_EVENTS,
		"vsize_push: pushing to long session");
	xAssert(xIsSession(xGetDown(s, LPI)));
	return xPush(xGetDown(s, LPI), msg);
    }
}


static xkern_return_t
vsizeOpenDone(self, lls, llp, hlpType)
    XObj self, lls, llp, hlpType;
{
    XObj	s;
    Part 	p[2];
    char	partBuf[100];
    PSTATE	*pstate;
    XObj	llsArray[2];
    int		llsIndex, otherIndex;
    Enable	*e;
    
    xTrace0(vsizep, TR_MAJOR_EVENTS, "In VSIZE openDone");
    if ( self == hlpType ) {
	xTrace0(vsizep, TR_ERRORS, "self == hlpType in vsizeOpenDone");
	return XK_FAILURE;
    }
    pstate = (PSTATE *)self->state;
    /* 
     * Figure out which of my lower protocols owns the lower session
     */
    if ( llp == xGetDown(self, SPI) ) {
	llsIndex = SPI;
	otherIndex = LPI;
    } else if ( llp == xGetDown(self, LPI) ) {
	llsIndex = LPI;
	otherIndex = SPI;
    } else {
	xError("Impossible lower level session in vsize openDone");
	return XK_FAILURE;
    }
    /*
     * check for open enables
     */
    if ( mapResolve(pstate->passiveMap, &hlpType, &e) == XK_FAILURE ) {
	/* 
	 * This shouldn't happen
	 */
	xTrace0(vsizep, TR_ERRORS,
		"vsizeOpenDone: Couldn't find hlp for incoming session");
	return XK_FAILURE;
    }
    /* 
     * Open the lls for the other protocol (not the one which owns
     * lls) using participants from a control op on lls
     */
    if ( xControl(lls, GETPARTICIPANTS, partBuf, sizeof(partBuf)) <= 0 ) {
	xTrace0(vsizep, TR_ERRORS,
		"Could not get participants from lls in vsizeOpenDone");
	return XK_FAILURE;
    }
    partInternalize(p, partBuf);
    llsArray[llsIndex] = lls;
    llsArray[otherIndex] = xOpen(self, hlpType, xGetDown(self, otherIndex), p);
    if ( llsArray[otherIndex] == ERR_XOBJ ) {
	xTrace0(vsizep, TR_ERRORS, "vsizeOpenDone couldn't open other lls");
	return XK_FAILURE;
    }
    xDuplicate(lls);
    if ( (s = vsizeCreateSessn(self, e->hlpRcv, e->hlpType, llsArray))
		== ERR_XOBJ ) {
	xClose(llsArray[LPI]);
	xClose(llsArray[SPI]);
	return XK_FAILURE;
    }
    xTrace0(vsizep, TR_EVENTS,
	    "vsize Passively opened session successfully created");
    return xOpenDone(e->hlpRcv, s, self);
}


static xkern_return_t
vsizeProtlDemux( self, lls, m )
    XObj	self, lls;
    Msg		*m;
{
    xTrace0(vsizep, TR_ERRORS, "vsizeProtlDemux called!!");
    return XK_SUCCESS;
}


/* 
 * vsizePop and vsizeSessnDemux must be used (i.e., they can't be
 * bypassed) for the UPI reference count mechanism to work properly. 
 */
static xkern_return_t
vsizePop(self, lls, msg, h)
    XObj self;
    XObj lls;
    Msg *msg;
    VOID *h;
{
    xTrace0(vsizep, TR_EVENTS, "vsize Pop");
    return xDemux(self, msg);
}


static xkern_return_t
vsizeSessnDemux(self, lls, msg)
    XObj self;
    XObj lls;
    Msg *msg;
{
    xTrace0(vsizep, TR_EVENTS, "vsize Session Demux");
    return xPop(self, lls, msg, 0);
}


static void
sessnInit(s)
    XObj s;
{
    xAssert(xIsSession(s));
    
    s->push = vsizePush;
    s->pop = vsizePop;
    s->close = vsizeClose;
    s->control = vsizeControlSessn;
    /* 
     * VSIZE sessions will look like a protocol to lower sessions, so we
     * need a demux function
     */
    s->demux = vsizeSessnDemux;
}


static void
protlFuncInit(p)
    XObj p;
{
    xAssert(xIsProtocol(p));

    p->control = vsizeControlProtl;
    p->open = vsizeOpen;
    p->openenable = vsizeOpenEnable;
    p->opendisable = vsizeOpenDisable;
    p->demux = vsizeProtlDemux;
    p->opendone = vsizeOpenDone;
}
