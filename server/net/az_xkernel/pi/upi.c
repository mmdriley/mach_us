/* 
 * upi.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.51 $
 * $Date: 1993/02/01 23:57:41 $
 */


#ifndef XKMACHKERNEL
#include <varargs.h>
#else
#include <sys/varargs.h>
#endif XKMACHKERNEL
#include "upi.h"
#include "xk_debug.h"
#include "process.h"
#include "assert.h"
#include "prottbl.h"
#include "platform.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#include "x_libc.h"
#endif XKMACHKERNEL

XObj	xNullProtl;
static	Map	protlMap;
static	Map	safeObjMap;


#ifdef XK_DEBUG
static char *controlops[] = {
  "getmyhost",
  "getmyhostcount",
  "getpeerhost",
  "getpeerhostcount",
  "getbcasthost",
  "getmaxpacket",
  "getoptpacket",
  "getmyproto",
  "getpeerproto",
  "resolve",
  "rresolve",
  "freeresources",
  "getparticipants"
};

#  define CONTROLMSG(n) ((n) < sizeof controlops / sizeof(char *) ? \
	controlops[n] : "non-standard")
#else
#  define CONTROLMSG(n) ""
#endif XK_DEBUG

#ifdef __STDC__

static XObj xCreateXObj(int downc, XObj *downv);
static xkern_return_t xDestroyXObj(XObj s);
static XObj	defaultOpen( XObj, XObj, XObj, Part * );

#else

static XObj xCreateXObj();
static xkern_return_t xDestroyXObj();
static XObj	defaultOpen();

#endif __STDC__

/* 
 * If inline functions are not being used, the upi_inline functions
 * will be instantiated here.
 */

#define UPI_INLINE_INSTANTIATE
#include "upi_inline.h"



/*************************************************
 * Uniform Protocol Interface Operations
 *************************************************/


XObj
xOpen(hlpRcv, hlpType, llp, participants)
    XObj hlpRcv, hlpType, llp;
    Part *participants;
{
    XObj s;
    
    xAssert(xIsProtocol(llp));
    xAssert(xIsProtocol(hlpType));
    xAssert(xIsProtocol(hlpRcv));
    xTrace3(protocol, TR_MAJOR_EVENTS, "Calling open[%s] by (%s,%s)",
	    llp->fullName, hlpRcv->fullName, hlpType->fullName);
    s = (XObj)(*(llp->open))(llp, hlpRcv, hlpType, participants);
    if (xIsXObj(s)) {
	s->rcnt++;
    }
    xTrace5(protocol, TR_MAJOR_EVENTS,
	    "Open[%s] by (%s,%s) returns %x (rcnt == %d)",
	    llp->fullName, hlpRcv->fullName, hlpType->fullName, s,
	    xIsXObj(s) ? s->rcnt : 0);
    return s;
}


xkern_return_t
xOpenEnable( hlpRcv, hlpType, llp, p )
    XObj hlpRcv, hlpType, llp;
    Part *p;
{
    xAssert(xIsProtocol(llp));
    xAssert(xIsXObj(hlpType));
    xAssert(xIsXObj(hlpRcv));
    xTrace3(protocol, TR_MAJOR_EVENTS, "Calling openenable[%s] by (%s,%s)",
	    llp->fullName, hlpRcv->fullName, hlpType->fullName);
    return (*(llp->openenable))(llp, hlpRcv, hlpType, p);
}


xkern_return_t
xOpenDone(hlp, s, llp)
    XObj	hlp, s, llp;
{
    register Pfk od;
    
    xAssert(xIsXObj(s));
    xAssert(xIsXObj(hlp));
    xAssert(xIsXObj(llp));
    xIfTrace(protocol, TR_MAJOR_EVENTS) {
	if ( s->up != hlp ) {
	    xTrace4(protocol, TR_MAJOR_EVENTS,
		    "hlpRcv of session %x(%s) becomes %x(%s)",
		    s, s->fullName, hlp, hlp->fullName);
	  }
    }
    s->up = hlp;
    if (!(od = hlp->opendone)) return XK_SUCCESS;
    xTrace2(protocol, TR_MAJOR_EVENTS, "Calling opendone[%s] by %s",
	    hlp->fullName, llp->fullName);
    xTrace4(protocol, TR_FUNCTIONAL_TRACE, "hlp == %x, lls == %x, llp == %x, hlpType == %x",
	    (int)hlp, (int)s, (int)llp, (int)s->hlpType);
    return (*od)(hlp, s, llp, s->hlpType);
}


xkern_return_t
xCloseDone(s)
    XObj s;
{
  register Pfk cd;

  xAssert(xIsXObj(s));
  if (!(cd = s->up->closedone)) return XK_SUCCESS;
  xTrace2(protocol, TR_MAJOR_EVENTS, "Calling closedone[%s] by %s", s->up->fullName,
	 s->myprotl->fullName);
  return (*cd)(s);
}


xkern_return_t
xOpenDisable( hlpRcv, hlpType, llp, participants )
    XObj hlpRcv, hlpType, llp;
    Part *participants;
{
    xAssert(xIsProtocol(llp));
    xAssert(xIsXObj(hlpRcv));
    xAssert(xIsXObj(hlpType));
    xTrace3(protocol, TR_MAJOR_EVENTS, "Calling opendisable[%s] by (%s,%s)",
	    llp->fullName, hlpRcv->fullName, hlpType->fullName);
    return (*(llp->opendisable))(llp, hlpRcv, hlpType, participants);
}


xkern_return_t
xOpenDisableAll( hlpRcv, llp )
    XObj hlpRcv, llp;
{
    xAssert(xIsProtocol(llp));
    xAssert(xIsXObj(hlpRcv));
    xTrace2(protocol, TR_MAJOR_EVENTS, "Calling openDisableAll[%s] by (%s)",
	    llp->fullName, hlpRcv->fullName);
    return (*(llp->opendisableall))(llp, hlpRcv);
}


xkern_return_t
xClose(s)
    XObj s;
{
  xTrace0(protocol, TR_EVENTS, "xClose: entered");
  /*
   * DEC_REF_COUNT_UNCOND comes from upi_inline.h
   */
  DEC_REF_COUNT_UNCOND(s, "xClose");
  xTrace0(protocol, TR_FULL_TRACE, "xClose returning");
  return XK_SUCCESS;
}


xkern_return_t
xDuplicate(s)
    XObj s;
{
  xTrace1(protocol, TR_EVENTS, "calling duplicate[%s]", s->myprotl->fullName);
  return (*s->duplicate)(s);
}


static xkern_return_t
defaultDuplicate(s)
    XObj s;
{
    s->rcnt++;
    return XK_SUCCESS;
}


static XObj
defaultOpen( hlpRcv, hlpType, llp, p )
    XObj	hlpRcv, hlpType, llp;
    Part	*p;
{
    return ERR_XOBJ;
}


static xkern_return_t
returnFailure()
{
    xTrace0(protocol, TR_SOFT_ERRORS, "default UPI operation is invoked");
    return XK_FAILURE;
}

/*
 * xDemux, xCallDemux, xPush and xCall are defined as macros in upi.h when
 * optimized
 */

#ifdef XK_DEBUG

xkern_return_t
xCall(s, msg, replyMsg)
    XObj s;
    Msg *msg;
    Msg *replyMsg;
{
    xkern_return_t retVal;
    
    xAssert(xIsXObj(s));
    xTrace3(protocol, TR_EVENTS, "Calling call[%s] by %s, %d bytes", s->myprotl->fullName,
	    s->up->fullName, msgLen(msg));
    xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
	xTrace0(protocol, TR_ALWAYS, "       Message:");
	msgShow(msg);
    }
    retVal = (*s->call)(s, msg, replyMsg);
    xTrace3(protocol, TR_EVENTS, "call[%s] returns %d bytes in reply to %s",
	    s->myprotl->fullName, msgLen(replyMsg), s->up->fullName);
    return retVal;
}


xmsg_handle_t
xPush(s, msg)
    XObj s;
    Msg *msg;
{
    int retVal;
    
    xAssert(xIsXObj(s));
    xTrace3(protocol, TR_EVENTS, "Calling push[%s] by %s, %d bytes", s->myprotl->fullName,
	    s->up->fullName, msgLen(msg));
    xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
	xTrace0(protocol, TR_ALWAYS, "       Message:");
	msgShow(msg);
    }
    retVal = (*s->push)(s, msg);
    xTrace3(protocol, TR_EVENTS, "push[%s] by %s returns %d", s->myprotl->fullName,
	    s->up->fullName, retVal);
    return retVal;
}


xkern_return_t
xDemux(s, msg)
    XObj s;
    Msg *msg;
{
  register Pfk demux;

  xAssert(xIsXObj(s));
  xAssert(xIsXObj(s->up));
  xTrace4(protocol, TR_EVENTS, "Calling demux[%s(%x)] by %s, %d bytes",
	  s->up->fullName, s->up, s->myprotl->fullName, msgLen(msg));
  xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
      xTrace0(protocol, TR_ALWAYS, "       Message:");
      msgShow(msg);
  }
  if (!(demux = s->up->demux)) return XK_SUCCESS;
  return (*demux)(s->up, s, msg);
}


xkern_return_t
xCallDemux(s, msg, replyMsg)
    XObj s;
    Msg *msg;
    Msg *replyMsg;
{
  register Pfk calldemux;
  xkern_return_t	retVal;

  xAssert(xIsXObj(s));
  xTrace3(protocol, TR_EVENTS, "Calling calldemux[%s] by %s, %d bytes", s->up->fullName,
	 s->myprotl->fullName, msgLen(msg));
  xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
    xTrace0(protocol, TR_ALWAYS, "       Message:");
    msgShow(msg);
  }
  if (!(calldemux = s->up->calldemux)) return XK_SUCCESS;
  retVal = (*calldemux)(s->up, s, msg, replyMsg);
  xTrace2(protocol, TR_EVENTS, "calldemux[%s] returns %d bytes",
	  s->up->fullName, msgLen(replyMsg));
  return retVal;
}



#endif XK_DEBUG



int
xControl(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
  register Pfi c;

  int res;
  if (! (c = s->control)) {
    return 0;
  }
  xTrace3(protocol, TR_EVENTS, "Calling control[%s] op %s (%d)",
	 s->myprotl->fullName, CONTROLMSG(opcode), opcode);
  res = c(s, opcode, buf, len);
  xTrace4(protocol, TR_EVENTS, "Control[%s] op %s (%d) returns %d",
	s->myprotl->fullName, CONTROLMSG(opcode), opcode, res);
  return res;
}


xkern_return_t
xShutDown( obj )
    XObj obj;
{
    xAssert(xIsXObj(obj));
    xTrace2(protocol, TR_MAJOR_EVENTS, "Calling shutdown[%s(%x)]",
	    obj->fullName, obj);
    return obj->shutdown(obj);
}



/*************************************************
 * Create and Destroy XObj's Sessions and Protocols 
 *************************************************/

static int
noop()
{
    return 0;
}


XObj
xCreateSessn( f, hlpRcv, hlpType, llp, downc, downv )
    Pfv		f;
    XObj 	hlpRcv, hlpType, llp;
    int downc;
    XObj *downv;
{
    XObj s;
    
    xTrace3(protocol, TR_MAJOR_EVENTS, "xCreateSession:[%s] by [%s,%s]",
	    llp->fullName, hlpRcv->fullName, hlpType->fullName);
    if ((s = xCreateXObj(downc, downv)) == ERR_XOBJ) {
	xTrace0(protocol, TR_ERRORS, "CreateSessn failed at CreateXObj");
	return s;
    }
    s->type = Session;
    s->rcnt = 0;
    s->name = llp->name;
    s->instName = llp->instName;
    s->fullName = llp->fullName;
    s->myprotl = llp;
    s->id = 0;			/* only relevant to protocols */
    s->up = hlpRcv;
    s->hlpType = hlpType;
    s->traceVar = llp->traceVar;
    llp->rcnt++;
/*     hlpRcv->rcnt++; */
    if (f) (*f)(s);
    xTrace1(protocol, TR_EVENTS, "xCreateSession returns %x", s);
    return s;
}


XObj
xCreateProtl(f, name, instName, downc, downv)
    Pfv f;
    char *name, *instName;
    int downc;
    XObj *downv;
{
  XObj s;
  int  id;

  xTrace1(protocol, TR_MAJOR_EVENTS, "xCreateProtocol:[%s]", name);
  if ( (id = protTblGetId(name)) == -1 ) {
    xTrace1(protocol, TR_ERRORS, "CreateProtl could not find protocol id for %s", name);
      return ERR_XOBJ;
  }
  if ((s = xCreateXObj(downc, downv)) == ERR_XOBJ) {
    xTrace0(protocol, TR_ERRORS, "CreateProtl failed at CreateXObj");
    return ERR_XOBJ;
  }
  s->close = (Pfk)noop;
  s->type = Protocol;
  s->rcnt = 1;
  s->name = xMalloc(strlen(name) + 1);
  strcpy(s->name, name);
  s->instName = xMalloc(strlen(instName) + 1);
  strcpy(s->instName, instName);
  s->fullName = xMalloc(strlen(name) + strlen(instName) + 2);
  sprintf(s->fullName, "%s%c%s", name, (*instName ? '/' : '\0'), instName);
  s->myprotl = s;
  s->id = id;
  s->up = ERR_XOBJ;		/* only relevant to sessions */
  if (f) (*f)(s);
  return s;
}
		     

static XObj
xCreateXObj(downc, downv)
    int downc;
    XObj *downv;
{
  XObj s;
  XObj *dv;
  int  i;
  Bind	b;

  s = (struct xobj *) xMalloc(sizeof(struct xobj));
  if (! s) {
    xTrace0(protocol, TR_ERRORS, "xCreateObj malloc failure(1)");
    return ERR_XOBJ;
  }
  bzero((char *)s, sizeof(struct xobj));
  s->numdown = downc;
  if (downc > STD_DOWN) {
    s->downlistsz = (((downc - STD_DOWN) / STD_DOWN) + 1) * STD_DOWN;
    s->downlist = (XObj *) xMalloc(s->downlistsz * sizeof(XObj));
    if (! s->downlist) {
      xFree((char *)s);
      xTrace0(protocol, TR_ERRORS, "xCreateObj malloc failure(2)");
      return ERR_XOBJ;
    }
  } else {
    s->downlistsz = 0;
  }
  dv = s->down;
  for (i=0; i < downc; i++, dv++) {
    if (i == STD_DOWN) {
      dv = s->downlist;
    }
    *dv = downv[i];
  }
  s->idle = FALSE;
  s->open = defaultOpen;
  s->close = (Pfk)returnFailure;
  s->closedone = (Pfk)noop;
  s->openenable = (Pfk)returnFailure;
  s->opendisable = (Pfk)returnFailure;
  s->opendisableall = (Pfk)noop;
  s->opendone = (Pfk)noop;
  s->demux = (Pfk)returnFailure;
  s->calldemux = (Pfk)returnFailure;
  s->callpop = (Pfk)returnFailure;
  s->pop = (Pfk)returnFailure;
  s->push = (Pfh)returnFailure;
  s->call = (Pfk)returnFailure;
  s->control = (Pfi)returnFailure;
  s->shutdown = (Pfk)noop;
  s->duplicate = defaultDuplicate;
  b = mapBind(safeObjMap, &s, s);
  xAssert( b != ERR_BIND );
  return (XObj)s;
}


static xkern_return_t
xDestroyXObj(s)
    XObj s;
{
    xkern_return_t xkr = XK_SUCCESS;
    
    xkr = mapUnbind(safeObjMap, &s);
    xAssert(xkr == XK_SUCCESS);
    if (s->state) {
	xkr = xFree((char *)s->state) ? XK_FAILURE : XK_SUCCESS;
    }
    return xFree((char *)s) ? XK_FAILURE : xkr;
}


xkern_return_t
xDestroy(s)
    XObj s;
{
    xTrace2(protocol, TR_EVENTS, "xDestroy[%s(%x)]", s->fullName, s);
    if ( s->type == Session ) {
	xClose(s->myprotl);
    } 
    return xDestroyXObj(s);
}



/*************************************************
 * Utility Routines
 *************************************************/



XObj
xGetProtlByName(name)
    char *name;
{
  register XObj p;
  register int i;

  for (i = 0; (p = protl_tab[i]); i++) {
    if (*p->name == *name && !strcmp(p->name, name)) return p;
  }
  return ERR_XOBJ;
}


xkern_return_t
xSetDown( s, i, obj )
    XObj s;
    int i;
    XObj obj;
{
    XObj *newdl;
    int  newsz, n;
    
    if (i < STD_DOWN) {
	s->down[i] = obj;
    } else {
	n = i - STD_DOWN;
	if (n >= s->downlistsz) {
	    /* 
	     * Make newsz the smallest sufficient multiple of STD_DOWN
	     */
	    newsz = ((n / STD_DOWN) + 1) * STD_DOWN;
	    newdl = (XObj *) xMalloc(newsz * sizeof(XObj));
	    if ( s->downlist ) {
		bcopy((char *)s->downlist, (char *)newdl,
		      s->downlistsz * sizeof(XObj));
		xFree((char *)s->downlist);
	    }
	    s->downlist = newdl;
	    s->downlistsz = newsz;
	}
	s->downlist[n] = obj;
    }
    if (i + 1 > s->numdown) {
	s->numdown = i + 1;
    }
    return XK_SUCCESS;
}


XObj
xGetDown( s, i )
    XObj s;
    int i;
{
    xAssert( xIsXObj(s) );
    if (i + 1 > s->numdown) {
	return ERR_XOBJ;
    }
    return (i < STD_DOWN) ? s->down[i] : s->downlist[i - STD_DOWN];
}



/*************************************************
 * Miscellaneous Routines
 *************************************************/


bool
xIsValidXObj( obj )
    XObj	obj;
{
    return (mapResolve(safeObjMap, &obj, 0) == XK_SUCCESS);
}


void
xPrintXObj(p)
    XObj p;
{
    int 	i;
    
    if ( !p ) {
	xTrace0(protocol, TR_ALWAYS, "XOBJ NULL");
	return;
    }
    xTrace2(protocol, TR_ALWAYS, "XOBJ %x %s", p, p->fullName);
    xTrace1(protocol, TR_ALWAYS, "type = %s",
	    p->type == Protocol ? "Protocol" : "Session");
    xTrace4(protocol, TR_ALWAYS, "myprotocol = %x %s %s %s",
	    p->myprotl, p->myprotl->name, p->myprotl->instName,
	    p->myprotl->fullName);
    xTrace1(protocol, TR_ALWAYS, "id = %d", p->id);
    xTrace1(protocol, TR_ALWAYS, "rcnt = %d", p->rcnt);
    xTrace1(protocol, TR_ALWAYS, "up = %x ", p->up);
    if ( xIsXObj(p->up) ) {
	xTrace1(protocol, TR_ALWAYS, "up->type = %s ",
		p->type == Protocol ? "Protocol" : "Session");
	xTrace1(protocol, TR_ALWAYS, "up->fullName = %s\n", p->up->fullName);
    }
    xTrace1(protocol, TR_ALWAYS, "numdown = %d ", p->numdown);
    for ( i=0; i < p->numdown; i++ ) {
	xTrace2(protocol, TR_ALWAYS, "down[%d] = %x", i, xGetDown(p, i));
    }
}



void
upiInit()
{
    xTrace0(init, TR_EVENTS, "upi init");
    protlMap = mapCreate(37, sizeof(XObj));
    safeObjMap = mapCreate(511, sizeof(XObj));
    xNullProtl = xCreateProtl((Pfv)noop, "null", "", 0, 0);
}


#if 0

static int
closeProtls( key, val, arg )
    VOID	*key, *val, *arg;
{
    if ( xIsProtocol((XObj)val) ) {
	xClose((XObj)val);
    }
    return TRUE;
}

#endif


static int
shutdownProtls( key, val, arg )
    VOID	*key, *val, *arg;
{
    if ( xIsProtocol((XObj)val) ) {
	xShutDown((XObj)val);
    }
    return TRUE;
}


void
xRapture()
{
/*    mapForEach(safeObjMap, closeProtls, 0); */
    mapForEach(safeObjMap, shutdownProtls, 0);
}
