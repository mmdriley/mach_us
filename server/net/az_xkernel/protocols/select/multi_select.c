/*
 * multi_select.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:32:20 $
 */

/* 
 * multi_select -- The multi-select protocol determines the upper
 * protocol ID at open/openenable time by pulling the ID number off
 * the participant stack.
 */


#include "xkernel.h"
#include "mselect.h"
#include "select_i.h"

#ifdef __STDC__

static int		extractId( Part *, long * );
static void		getProtFuncs( XObj );
static int		mselectControlSessn( XObj, int, char *, int );
static XObj		mselectOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	mselectOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t 	mselectOpenEnable( XObj, XObj, XObj, Part * );

#else

static int		extractId();
static void		getProtFuncs();
static int		mselectControlSessn();
static XObj		mselectOpen();
static xkern_return_t	mselectOpenDisable();
static xkern_return_t 	mselectOpenEnable();

#endif __STDC__


void
mselect_init( self )
    XObj self;
{
    xTrace0(selectp, TR_GROSS_EVENTS, "MSELECT init");
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xTrace0(selectp, TR_ERRORS,
		"SELECT could not find down protocol -- not initializing");
	return;
    }
    getProtFuncs(self);
    selectCommonInit(self);
}


static int
extractId( p, id )
    Part	*p;
    long	*id;
{
    long	*n;

    if ( ! p || partLen(p) < 1 ) {
	xTrace0(selectp, TR_SOFT_ERROR,
		"select extractId -- bad participant");
	return -1;
    }
    n = (long *)partPop(*p);
    if ( n == 0 || n == (long *)-1 ) {
	xTrace0(selectp, TR_SOFT_ERROR,
		"select extractId -- bad participant stack");
	return -1;
    }
    xTrace1(selectp, TR_MORE_EVENTS, "select extractID got id == %d", *n);
    *id = *n;
    return 0;
}


static XObj
mselectOpen( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part    	*p;
{
    long	hlpNum;
    XObj	s;

    xTrace0(selectp, TR_MAJOR_EVENTS, "MSELECT open");
    if ( extractId(p, &hlpNum) ) {
	return ERR_XOBJ;
    }
    s = selectCommonOpen(self, hlpRcv, hlpType, p, hlpNum);
    if ( xIsSession(s) ) {
	s->control = mselectControlSessn;
    }
    return s;
}


static int
mselectControlSessn(self, op, buf, len)
    XObj	self;
    int 	op, len;
    char	*buf;
{

    switch ( op ) {
      case GETPARTICIPANTS:
	{
	    int		retLen;
	    SState	*state = (SState *)self->state;
	    Part	p[2];

	    retLen = xControl(xGetDown(self, 0), op, buf, len);
	    if ( retLen <= 0 ) {
		return -1;
	    }
	    partInternalize(p, buf);
	    partPush(*p, &state->hdr.id, sizeof(long));
	    return (partExternalize(p, buf, &len) == XK_FAILURE) ? -1 : len;
	}

      default:
	return selectCommonControlSessn(self, op, buf, len);
    }
}
  


static xkern_return_t
mselectOpenEnable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    *p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "MSELECT openEnable");
    if ( extractId(p, &hlpNum) ) {
	return XK_FAILURE;
    }
    if ( selectCommonOpenEnable(self, hlpRcv, hlpType, p, hlpNum)
		== XK_SUCCESS ) {
	return xOpenEnable(self, self, xGetDown(self, 0), p);
    } else {
	return XK_FAILURE;
    }
}


static xkern_return_t
mselectOpenDisable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    *p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "MSELECT openDisable");
    if ( extractId(p, &hlpNum) ) {
	return XK_FAILURE;
    }
    if ( selectCommonOpenDisable(self, hlpRcv, hlpType, p, hlpNum)
		== XK_SUCCESS ) {
	return xOpenDisable(self, self, xGetDown(self, 0), p);
    } else {
	return XK_FAILURE;
    }
}


static void
getProtFuncs( p )
    XObj 	p;
{
    p->control = selectControlProtl;
    p->open = mselectOpen;
    p->openenable = mselectOpenEnable;
    p->opendisable = mselectOpenDisable;
    p->calldemux = selectCallDemux;
    p->demux = selectDemux;
}


