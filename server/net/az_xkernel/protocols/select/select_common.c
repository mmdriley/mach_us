/*
 * select_common.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.9 $
 * $Date: 1993/02/01 22:32:26 $
 */


/* 
 * select_common.c contains code common to both the 'select' and the
 * 'mselect' (multi-select) protocols.
 */


#include "xkernel.h"
#include "select_i.h"



int traceselectp;


#ifdef __STDC__

static void		getSessnFuncs( XObj );
static long		hdrLoad( VOID *, char *, long, VOID * );
static void		hdrStore( VOID *, char *, long, VOID * );
static void		phdr( SelHdr * );
static xkern_return_t	selectCall( XObj, Msg *, Msg * );
static xmsg_handle_t	selectPush( XObj, Msg * );
static xkern_return_t	selectClose( XObj );
static XObj		selectCreateSessn( XObj, XObj, XObj, ActiveKey * );
static xkern_return_t	selectPop( XObj, XObj, Msg *, VOID * );
static xkern_return_t	selectCallPop( XObj, XObj, Msg *, VOID *, Msg * );

#else

static void	getSessnFuncs();
static long	hdrLoad();
static void	hdrStore();
static void	phdr();
static XObj	selectCreateSessn();

#endif __STDC__


void
selectCommonInit( self )
    XObj self;
{
    Part 	part;
    PState 	*pstate;
    
    xTrace0(selectp, TR_GROSS_EVENTS, "SELECT common init");
    pstate = X_NEW(PState);
    self->state = (VOID *)pstate;
    pstate->passiveMap = mapCreate(101, sizeof(PassiveKey));
    pstate->activeMap = mapCreate(101, sizeof(ActiveKey));
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    if ( xOpenEnable(self, self, xGetDown(self, 0), &part) == XK_FAILURE ) {
	xTrace0(selectp,0,"select_init: could not openenable lower protocol");
    }
}


static long
hdrLoad( hdr, src, len, arg )
    VOID	*hdr, *arg;
    char 	*src;
    long  	len;
{
    xAssert(len == sizeof(SelHdr));
    bcopy( src, hdr, len );
    ((SelHdr *)hdr)->id = ntohl(((SelHdr *)hdr)->id);
    ((SelHdr *)hdr)->status = ntohl(((SelHdr *)hdr)->status);
    return len;
}


static void
hdrStore(hdr, dst, len, arg)
    VOID	*hdr, *arg;
    char 	*dst;
    long  	len;
{
    SelHdr	h;

    xAssert( len == sizeof(SelHdr) );
    h.id = htonl(((SelHdr *)hdr)->id);
    h.status = htonl(((SelHdr *)hdr)->status);
    bcopy( (char *)&h, dst, len );
}


XObj
selectCommonOpen( self, hlpRcv, hlpType, p, hlpNum )
    XObj	self, hlpRcv, hlpType;
    Part    	*p;
    long	hlpNum;
{
    XObj   	s;
    PState 	*pstate = (PState *)self->state;
    ActiveKey	key;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT common open");
    key.id = hlpNum;
    key.lls = xOpen(self, self, xGetDown(self, 0), p);
    if ( key.lls == ERR_XOBJ ) {
	xTrace0(selectp, TR_SOFT_ERRORS, "selectOpen: could not open lls");
	return ERR_XOBJ;
    }
    if ( mapResolve(pstate->activeMap, &key, &s) == XK_FAILURE ) {
	/*
	 * session did not exist -- get an uninitialized one
	 */
	xTrace0(selectp, TR_MAJOR_EVENTS,
		"selectOpen -- no active session existed.  Creating new one.");
	s = selectCreateSessn(self, hlpRcv, hlpType, &key);
    } else {
	xTrace1(selectp, TR_MAJOR_EVENTS, "Active sessn %x already exists", s);
	/*
	 * Undo the lls open
	 */
	xClose(key.lls);  
    }
    return s;
}


static XObj
selectCreateSessn( self, hlpRcv, hlpType, key )
    XObj	self, hlpRcv, hlpType;  		
    ActiveKey	*key;
{
    XObj 	s;
    SState	*state;
    PState	*pstate = (PState *)self->state;
    SelHdr 	*hdr;
    
    xTrace0(selectp, TR_FUNCTIONAL_TRACE, "selectCreateSessn");
    state = X_NEW(SState);
    bzero((char *)state, sizeof(SState));
    hdr = &state->hdr;
    hdr->status = SEL_OK;
    hdr->id = key->id;
    s = xCreateSessn(getSessnFuncs, hlpRcv, hlpType, self, 1, &key->lls);
    s->binding = mapBind(pstate->activeMap, key, (int)s);
    if ( s->binding == ERR_BIND ) {
	xTrace0(selectp, TR_ERRORS, "selectCreateSessn -- bind failed!");
	xClose(s);
	return ERR_XOBJ;
    }
    s->state = (VOID *)state;
    return s;
}


xkern_return_t
selectCommonOpenEnable( self, hlpRcv, hlpType, p, hlpNum )
    XObj   	self, hlpRcv, hlpType;
    Part	*p;
    long	hlpNum;
{
    PassiveKey	key;
    PState 	*pstate = (PState *)self->state;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT common open enable");
    key = hlpNum;
    return defaultOpenEnable(pstate->passiveMap, hlpRcv, hlpType, &key);
}


xkern_return_t
selectCommonOpenDisable( self, hlpRcv, hlpType, p, hlpNum )
    XObj   	self, hlpRcv, hlpType;
    Part    	*p;
    long	hlpNum;
{
    PassiveKey	key;
    PState 	*pstate = (PState *)self->state;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT open disable");
    key = hlpNum;
    return defaultOpenEnable(pstate->passiveMap, hlpRcv, hlpType, &key);
}


static xkern_return_t
selectClose(s)
    XObj   s;
{
    PState		*ps = (PState *)xMyProtl(s)->state;
    xkern_return_t	res;

    xTrace1(selectp, TR_EVENTS, "select_close of session %x", s);
    xAssert(xIsSession(s));
    xAssert(s->rcnt <= 0);
    xClose(xGetDown(s, 0));
    res = mapRemoveBinding(ps->activeMap, s->binding);
    xAssert( res == XK_SUCCESS );
    xDestroy(s);
    return XK_SUCCESS;
}


static xkern_return_t
selectCall( self, msg, rMsg )
    XObj	self;
    Msg     	*msg, *rMsg;
{
    SState	*state = (SState *)self->state;
    SelHdr 	rHdr;
    
    xTrace0(selectp, TR_EVENTS, "in selectCall");
    xIfTrace(selectp, TR_DETAILED) {
	phdr(&state->hdr);
    }
    msgPush(msg, hdrStore, &state->hdr, sizeof(SelHdr), 0);
    if ( xCall(xGetDown(self, 0), msg, rMsg) == XK_FAILURE ) {
	return XK_FAILURE;
    }
    if ( msgPop(rMsg, hdrLoad, &rHdr, sizeof(SelHdr), 0) == FALSE ) {
	xTrace0(selectp, TR_ERRORS, "selectCall -- msgPop failed");
	return XK_FAILURE;
    }
    if ( rHdr.status == SEL_FAIL ) {
	xTrace0(selectp, TR_ERRORS, "selectCall -- reply indicates failure ");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static xmsg_handle_t
selectPush( self, msg )
    XObj	self;
    Msg     	*msg;
{
    SState	*state = (SState *)self->state;
    
    xTrace0(selectp, TR_EVENTS, "in selectPush");
    xIfTrace(selectp, TR_DETAILED) {
	phdr(&state->hdr);
    }
    msgPush(msg, hdrStore, &state->hdr, sizeof(SelHdr), 0);
    return xPush(xGetDown(self, 0), msg);
}


xkern_return_t
selectCallDemux( self, lls, msg, rmsg )
    XObj	self, lls;
    Msg		*msg, *rmsg;
{
    SelHdr 	hdr;
    PassiveKey 	pKey;
    ActiveKey	aKey;
    XObj   	s;
    Enable	*e;
    PState 	*pstate = (PState *)self->state;
    
    xTrace0(selectp, TR_EVENTS , "selectDemux");
    if ( msgPop(msg, hdrLoad, &hdr, sizeof(hdr), 0) == FALSE ) {
	xTrace0(selectp, TR_SOFT_ERRORS, "selectDemux -- msgPop failed!");
	return XK_FAILURE;
    }
    xIfTrace(selectp, TR_DETAILED) {
	phdr(&hdr);
    }
    aKey.id = hdr.id;
    aKey.lls = lls;
    if ( mapResolve(pstate->activeMap, &aKey, &s) == XK_FAILURE ) {
	/*
	 * no active session exists -- check for openenable
	 */
	pKey = aKey.id;
	xTrace1(selectp, TR_EVENTS, "Checking for enable using id: %d", pKey);
	if ( mapResolve(pstate->passiveMap, &pKey, &e) == XK_FAILURE ) {
	    xTrace0(selectp, TR_EVENTS, "selectDemux: no openenable done");
	} else {
	    xTrace0(selectp, TR_MAJOR_EVENTS,
		    "select_demux creates new server session");
	    s = selectCreateSessn(self, e->hlpRcv, e->hlpType, &aKey);
	    if ( s != ERR_XOBJ ) {
		xDuplicate(lls);
		xOpenDone(e->hlpRcv, s, self);
	    }
	}
    } else {
	xTrace0(selectp, TR_MORE_EVENTS, "selectDemux found existing sessn");
    }
    if ( rmsg ) {
	/* 
	 * This really is a call demux
	 */
	if ( s != ERR_XOBJ && xCallPop(s, lls, msg, 0, rmsg) == XK_SUCCESS ) {
	    hdr.status = SEL_OK;
	} else {
	    xTrace0(selectp, TR_EVENTS,
		    "selectDemux -- sending failure reply");
	    hdr.status = SEL_FAIL;
	}
	msgPush(rmsg, hdrStore, &hdr, sizeof(hdr), 0);
    } else {
	/* 
	 * This is really a demux, not a calldemux
	 */
	xPop(s, lls, msg, 0);
    }
    return XK_SUCCESS;
}


xkern_return_t
selectDemux( s, lls, msg )
    XObj	s, lls;
    Msg		*msg;
{
    xTrace0(selectp, TR_EVENTS, "SELECT demux");
    return selectCallDemux(s, lls, msg, 0);
}


static xkern_return_t
selectCallPop( s, lls, msg, hdr, rMsg )
    XObj	s, lls;
    Msg     	*msg, *rMsg;
    VOID	*hdr;
{
    xTrace0(selectp, TR_EVENTS, "SELECT callpop");
    return xCallDemux(s, msg, rMsg);
}


static xkern_return_t
selectPop( s, lls, msg, hdr )
    XObj	s, lls;
    Msg     	*msg;
    VOID	*hdr;
{
    xTrace0(selectp, TR_EVENTS, "SELECT_pop");
    return xDemux(s, msg);
}


int
selectControlProtl(self, op, buf, len)
    XObj	self;
    int 	op, len;
    char	*buf;
{
    return xControl(xGetDown(self, 0), op, buf, len);
}
  

int
selectCommonControlSessn(self, op, buf, len)
    XObj	self;
    int 	op, len;
    char	*buf;
{
    switch ( op ) {
      case GETMAXPACKET:
      case GETOPTPACKET:
	if ( xControl(xGetDown(self, 0), op, buf, len) < sizeof(int) ) {
	    return -1;
	}
	*(int *)buf -= sizeof(SelHdr);
	return (sizeof(int));
	
      default:
	return xControl(xGetDown(self, 0), op, buf, len);
    }
}
  

static void
phdr( h )
    SelHdr	*h;
{
    xTrace2(selectp, TR_ALWAYS, "SELECT header: id == %d  status == %s",
	    h->id,
	    (h->status == SEL_OK) ? "SEL_OK" :
	    (h->status == SEL_FAIL) ? "SEL_FAIL" :
	    "UNKNOWN");
}



static void
getSessnFuncs( s )
    XObj	s;
{
    s->call = selectCall;
    s->push = selectPush;
    s->callpop = selectCallPop;
    s->pop = selectPop;
    s->control = selectCommonControlSessn;
    s->close = selectClose;
}

