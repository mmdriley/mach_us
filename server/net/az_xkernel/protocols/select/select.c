/*
 * select.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.12 $
 * $Date: 1993/02/01 22:32:04 $
 */

#include "xkernel.h"
#include "select.h"
#include "select_i.h"


/* 
 * select -- The select protocol determines the upper
 * protocol ID at open/openenable time by calling relProtNum.
 */

#ifdef __STDC__

static void		getProtFuncs( XObj );
static XObj		selectOpen( XObj, XObj, XObj, Part * );
static xkern_return_t	selectOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t 	selectOpenEnable( XObj, XObj, XObj, Part * );

#else

static void		getProtFuncs();
static XObj		selectOpen();
static xkern_return_t	selectOpenDisable();
static xkern_return_t 	selectOpenEnable();

#endif __STDC__


void
select_init( self )
    XObj self;
{
    xTrace0(selectp, TR_GROSS_EVENTS, "SELECT init");
    if ( ! xIsProtocol(xGetDown(self, 0)) ) {
	xTrace0(selectp, TR_ERRORS,
		"SELECT could not find down protocol -- not initializing");
	return;
    }
    getProtFuncs(self);
    selectCommonInit(self);
}


static XObj
selectOpen( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part    	*p;
{
    long	hlpNum;
    
    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT open");
    if ( (hlpNum = relProtNum(hlpType, self)) == -1 ) {
	return ERR_XOBJ;
    }
    return selectCommonOpen(self, hlpRcv, hlpType, p, hlpNum);
}


static xkern_return_t
selectOpenEnable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    *p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT openEnable");
    if ( (hlpNum = relProtNum(hlpType, self)) == -1 ) {
	return XK_FAILURE;
    }
    return selectCommonOpenEnable(self, hlpRcv, hlpType, p, hlpNum);
}


static xkern_return_t
selectOpenDisable( self, hlpRcv, hlpType, p )
    XObj   self, hlpRcv, hlpType;
    Part    *p;
{
    long	hlpNum;

    xTrace0(selectp, TR_MAJOR_EVENTS, "SELECT openDisable");
    if ( (hlpNum = relProtNum(hlpType, self)) == -1 ) {
	return XK_FAILURE;
    }
    return selectCommonOpenDisable(self, hlpRcv, hlpType, p, hlpNum);
}



static void
getProtFuncs( p )
    XObj 	p;
{
    p->control = selectControlProtl;
    p->open = selectOpen;
    p->openenable = selectOpenEnable;
    p->opendisable = selectOpenDisable;
    p->calldemux = selectCallDemux;
    p->demux = selectDemux;
}



