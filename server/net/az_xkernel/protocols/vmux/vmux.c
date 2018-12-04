/*
 * vmux.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:33:22 $
 */


#include "xkernel.h"
#include "vmux.h"
#include "vmux_i.h"

#define VMUX_MAX_PARTS		5
    
/* 
 * Check for a valid participant list
 */
#define partCheck(pxx, name, retval)					\
	{								\
	  if ( ! (pxx) || partLen(pxx) < 1 ||				\
	      			(partLen(pxx) > (VMUX_MAX_PARTS)) ) { 	\
		xTrace1(vmuxp, TR_ERRORS,				\
			"VMUX %s -- bad participants",			\
			(name));  					\
		return (retval);					\
	  }								\
	}

#define savePart(pxx, spxx)	\
    bcopy((char *)(pxx), (char *)(spxx), sizeof(Part) * partLen(p))

#define restorePart(pxx, spxx)	\
    bcopy((char *)(spxx), (char *)(pxx), sizeof(Part) * partLen(p))



int tracevmuxp;



#ifdef __STDC__

static xkern_return_t	vmuxOpenDone( XObj, XObj, XObj, XObj );
static xkern_return_t	vmuxOpenDisable( XObj, XObj, XObj, Part * );
static xkern_return_t	vmuxOpenEnable( XObj, XObj, XObj, Part * );
static int		vmuxControlProtl( XObj, int, char *, int );


#endif



static XObj
vmuxOpen( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part 	*p;
{
    Part	sp[VMUX_MAX_PARTS];
    XObj	llp, lls;
    int		i;
    
    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open");
    partCheck(p, "vmuxOpen", ERR_XOBJ);
    savePart(p, sp);
    for ( i=0; i < self->numdown; i++ ) {
	restorePart(p, sp);
	xTrace1(vmuxp, TR_MAJOR_EVENTS, "VMUX open trying llp %d", i);
	llp = xGetDown(self, i);
	if ( (lls = xOpen(hlpRcv, hlpType, llp, p)) != ERR_XOBJ ) {
	    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open successful");
	    /* 
	     * This session is passing through the open of more than
	     * one protocol, so its reference count is being
	     * artificially increased.  We'll correct for this level
	     * before returning the session.
	     *
	     * Should we be scheduling an event to do an xClose on this
	     * session? 
	     */
	    lls->rcnt--;
	    return lls;
	}
    }
    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open fails");
    return ERR_XOBJ;
}


static xkern_return_t
vmuxOpenEnable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part	*p;
{
    PState	*ps = (PState *)self->state;

    return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
				    ps->llpList, p);
}


static xkern_return_t
vmuxOpenDisable( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PState	*ps = (PState *)self->state;

    xTrace0(vmuxp, TR_MAJOR_EVENTS, "VMUX open disable");
    return defaultVirtualOpenDisable(self, ps->passiveMap, hlpRcv, hlpType,
				     ps->llpList, p);
}


static xkern_return_t
vmuxOpenDone( self, lls, llp, hlpType )
    XObj self, lls, llp, hlpType;
{
    PState	*ps = (PState *)self->state;
    Enable	*e;
    
    xTrace0(vmuxp, 3, "In VMUX OpenDone");
    if ( mapResolve(ps->passiveMap, &hlpType, &e) == XK_FAILURE ) {
	/* 
	 * This shouldn't happen
	 */
	xTrace0(vmuxp, TR_ERRORS,
		"vmuxOpenDone: Couldn't find hlp for new session");
	return XK_FAILURE;
    }
    return xOpenDone(e->hlpRcv, lls, self);
}


static int
vmuxControlProtl( self, opcode, buf, len )
    XObj self;
    int opcode, len;
    char *buf;
{
    /*
     * All opcodes are forwarded to the last protocol
     */
    return xControl(xGetDown(self, self->numdown - 1), opcode, buf, len);
}


void
vmux_init( self )
    XObj self;
{
    int		i;
    PState	*ps;

    xTrace0(vmuxp, TR_GROSS_EVENTS, "VMUX init");
    xAssert(xIsProtocol(self));
    ps = X_NEW(PState);
    self->state = (VOID *)ps;
    for ( i=0; i < self->numdown; i++ ) {
	XObj	llp;

	llp = xGetDown(self, i);
	if ( ! xIsProtocol(llp) ) {
	    xTrace1(vmuxp, TR_ERRORS, "VMUX can't find protocol %d", i);
	    xError("VMUX init errors -- not initializing");
	    return;
	} else {
	    xTrace2(vmuxp, TR_GROSS_EVENTS, "VMUX llp %d is %s", i, llp->name);
	}
	ps->llpList[i] = llp;
    }
    ps->llpList[i] = 0;
    ps->passiveMap = mapCreate(VMUX_MAP_SZ, sizeof(XObj));
    self->control = vmuxControlProtl;
    self->open = vmuxOpen;
    self->openenable = vmuxOpenEnable;
    self->opendisable = vmuxOpenDisable;
    self->opendone = vmuxOpenDone;
}


