/*     
 * netmask.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 23:58:13 $
 */

/* 
 * Maintains a table of network masks.  
 */

#include "xkernel.h"
#include "ip_host.h"
#include "netmask.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#endif XKMACHKERNEL

int	tracenetmask;

IPhost	IP_LOCAL_BCAST = { 255, 255, 255, 255 };

#define CLASSA(ad) (~((ad).a) & 0x80)
#define CLASSB(ad) (((ad).a & 0x80) && (~((ad).a) & 0x40))
#define CLASSC(ad) (((ad).a & 0x80) && ((ad).a & 0x40) && (~((ad).a) & 0x20))

static IPhost	classCmask = { 0xff,0xff,0xff,0 };
static IPhost	classBmask = { 0xff,0xff,0,0 };
static IPhost	classAmask = { 0xff,0,0,0 };
static IPhost	ipNull = { 0, 0, 0, 0 };

#define MAX_NET_MASKS 50
#define IPNETMAPSIZE  17  /* should 2 * max number of masks in use */

static	Map	netMaskMap;
static	char	errBuf[150];


#ifdef __STDC__

static int	bcastHost( IPhost *, IPhost * );

#else

static int	bcastHost();

#endif


int
netMaskAdd( iph, newMask )
    IPhost *iph;
    IPhost *newMask;
{
    IPhost		net, defMask, *mask;

    xAssert(netMaskMap);
    netMaskDefault(&defMask, iph);
    IP_AND(net, *iph, defMask);
    xTrace2(netmask, TR_EVENTS, "netmask adding mask %s for net %s",
	    ipHostStr(newMask), ipHostStr(&net));
    if ( mapResolve(netMaskMap, &net, &mask) == XK_SUCCESS ) {
	xTrace1(netmask, TR_EVENTS,
		"netmask add is overriding previous mask %s",
		ipHostStr(mask));
	xFree((char *)mask);
	mapUnbind(netMaskMap, &net);
    }
    mask = X_NEW(IPhost);
    *mask = *newMask;
    if ( mapBind(netMaskMap, &net, (int)mask) == ERR_BIND ) {
	return -1;
    }
    return 0;
}


void
netMaskInit()
{
    int		i;
    IPhost	net, mask;

    netMaskMap = mapCreate(IPNETMAPSIZE, sizeof(IPhost));
    /* 
     * Read in values of net masks from ROM file
     */
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], "netmask") ) {
	    if ( rom[i][1] && ! str2ipHost(&net, rom[i][1]) &&
		 rom[i][2] && ! str2ipHost(&mask, rom[i][2]) ) {
		if ( netMaskAdd(&net, &mask) ) {
#ifndef XKMACHKERNEL
		    sprintf(errBuf,
			"netmask: error adding initial mask %s -> %s to map",
				   ipHostStr(&net), ipHostStr(&mask));
		    xError(errBuf);
#else
		    printf(
			  "netmask: error adding initial mask %s -> %s to map",
			   ipHostStr(&net), ipHostStr(&mask));
#endif XKMACHKERNEL
		}
		continue;
	    }
#ifndef XKMACHKERNEL
	    sprintf(errBuf, "netmask format error on line %d of rom file", i);
	    xError(errBuf);
#else
	    printf("netmask format error on line %d of rom file\n", i);
#endif XKMACHKERNEL
	}
    }
}


void
netMaskDefault( mask, host )
    IPhost	*mask, *host;
{
    /* 
     * Use default network mask
     */
    if (CLASSC(*host)) {
	*mask = classCmask;
    } else if (CLASSB(*host)) {
	*mask = classBmask;
    } else { /* CLASS A */
	*mask = classAmask;
    }
}


void
netMaskFind(mask, host)
    IPhost *mask;
    IPhost *host;
{
    IPhost	*mp;
    IPhost	defMask, net;
    
    xAssert(netMaskMap);
    netMaskDefault(&defMask, host);
    IP_AND(net, *host, defMask);
    if ( mapResolve(netMaskMap, &net, &mp) == XK_FAILURE ) {
	*mask = defMask;
    } else {	
	*mask = *mp;
    }
}


static int
bcastHost( h, m )
    IPhost	*h, *m;
{
    IPhost	mComp;
    IPhost	res;

    IP_COMP(mComp, *m);
    /* 
     * Look for a host component that is all zeroes
     */
    IP_AND(res, *h, mComp);
    if ( IP_EQUAL(res, ipNull) ) {
	return 1;
    }
    /* 
     * Look for a host component that is all ones
     */
    IP_OR(res, *h, *m);
    if ( IP_EQUAL(res, IP_LOCAL_BCAST) ) {
	return 1;
    }
    return 0;
}


int
netMaskIsBroadcast( h )
    IPhost	*h;
{
    IPhost	m;

    netMaskFind(&m, h);
    if ( bcastHost(h, &m) ) {
	return 1;
    }
    /* 
     * See if this is a broadcast address using the default mask
     * (i.e., the original mask was for a subnet)
     */
    netMaskDefault(&m, h);
    if ( bcastHost(h, &m) ) {
	return 1;
    }
    /* 
     * See if the original address is all ones
     */
    if ( IP_EQUAL(*h, IP_LOCAL_BCAST) ) {
	return 1;
    }
    /* 
     * Address is not broadcast
     */
    return 0;
}


/* 
 * Are h1 and h2 on the same subnet?
 */
int
netMaskSubnetsEqual( h1, h2 )
    IPhost	*h1, *h2;
{
    IPhost	m1, m2;

    netMaskFind(&m1, h1);
    netMaskFind(&m2, h2);
    if ( ! IP_EQUAL(m1, m2) ) {
	return 0; 
    }
    IP_AND( m1, *h1, m1 );
    IP_AND( m2, *h2, m2 );
    return IP_EQUAL(m1, m2);
}


/* 
 * Are h1 and h2 on the same net?
 */
int
netMaskNetsEqual( h1, h2 )
    IPhost	*h1, *h2;
{
    IPhost	m, net1, net2;

    netMaskDefault(&m, h1);
    IP_AND( net1, *h1, m );
    IP_AND( net2, *h2, m );
    return IP_EQUAL(net1, net2);
}
