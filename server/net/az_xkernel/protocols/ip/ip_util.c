/*     
 * ip_util.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:22:56 $
 */

/* 
 * Internal IP utility functions
 */

#include "xkernel.h"
#include "ip_i.h"


int
ipHostOnLocalNet( ps, host )
    PState	*ps;
    IPhost	*host;
{
    int		res;
    XObj	llp;

    llp = xGetDown(ps->self, 0);
    xAssert(xIsProtocol(llp));
    res = xControl(llp, VNET_HOSTONLOCALNET, (char *)host, sizeof(IPhost));
    if ( res > 0 ) {
	return 1;
    }
    if ( res < 0 ) {
	xTrace0(ipp, TR_ERRORS, "ip could not do HOSTONLOCALNET call on llp");
    }
    return 0;
}



/*
 * ismy_addr:  is this IP address one which should reach me
 * (my address or broadcast)
 */
int
ipIsMyAddr( self, h )
    XObj	self;
    IPhost	*h;
{
    XObj	llp = xGetDown(self, 0);
    int		r;
    
    r = xControl(llp, VNET_ISMYADDR, (char *)h, sizeof(IPhost));
    if ( r > 0 ) {
	return 1;
    }
    if ( r < 0 ) {
	xError("ip couldn't do VNET_ISMYADDR on llp");
    }
    return 0;
}



