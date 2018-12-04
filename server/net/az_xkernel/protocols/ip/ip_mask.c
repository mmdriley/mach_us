/*     
 * ip_mask.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.13 $
 * $Date: 1993/02/01 22:22:27 $
 */

/* 
 * Maintains a table of network masks.  See ip_i.h for the interface.
 */

#include "xkernel.h"
#include "ip.h"
#include "ip_i.h"
#include "route.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#endif XKMACHKERNEL

static IPhost	classCmask = { 0xff,0xff,0xff,0 };
static IPhost	classBmask = { 0xff,0xff,0,0 };
static IPhost	classAmask = { 0xff,0,0,0 };

#define MAX_NET_MASKS 50
#define IPNETMAPSIZE  17  /* should 2 * max number of masks in use */

typedef struct {
    IPhost	host;
    IPhost	mask;
    int		metric;
} NetMaskStruct;

static	Map	netMaskMap;
static	char	errBuf[150];


static int
addMask(net, mask)
    IPhost *net;
    IPhost *mask;
{
    NetMaskStruct	*nms;
    int			i;

    nms = (NetMaskStruct *)mapResolve(netMaskMap, net);
    if ( nms  != (NetMaskStruct *) -1 ) {
	xFree((char *)nms);
	mapUnbind(netMaskMap, net);
    }
    nms = X_NEW(NetMaskStruct);
    bzero((char *)&nms->host, sizeof(IPhost));
    nms->mask = *mask;
    nms->metric = 0;
    /* 
     * Metric counts the number of 1's in the mask
     */
    for (i = 0; i < 8; i++) {
	nms->metric += ((mask->a >> i) & 1) + 
		       ((mask->b >> i) & 1) +
	     	       ((mask->c >> i) & 1) +
		       ((mask->d >> i) & 1) ;
    }
    if ( mapBind(netMaskMap, net, (int)nms) == ERR_BIND ) {
	return -1;
    }
    return 0;
}


void
initNetMaskMap()
{
    int		i;
    IPhost	net, mask;

    netMaskMap = mapCreate(IPNETMAPSIZE, sizeof(IPhost));
    /* 
     * Read in values of net masks from ROM file
     */
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], "ip") ) {
	    if ( rom[i][1] && ! strcmp(rom[i][1], "netmask") ) {
		if ( rom[i][2] && ! str2ipHost(&net, rom[i][2]) &&
		     rom[i][3] && ! str2ipHost(&mask, rom[i][3]) ) {
		    if ( addMask(&net, &mask) ) {
#ifndef XKMACHKERNEL
			xError(sprintf(errBuf,
			       "IP: error adding initial mask %s -> %s to map",
			       ipHostStr(&net), ipHostStr(&mask)));
#else
			printf(
			       "IP: error adding initial mask %s -> %s to map",
			       ipHostStr(&net), ipHostStr(&mask));
#endif XKMACHKERNEL
		    }
		    continue;
		}
	    }
#ifndef XKMACHKERNEL
	    xError(sprintf(errBuf,"IP format error on line %d of rom file",
			   i));
#else
	    printf("IP format error on line %d of rom file",
			   i);
#endif XKMACHKERNEL
	}
    }
}


/* 
 * 'arg (accNms)' contains the host submitted in the query and the best mask
 * and metric found so far.  'mapNms' is the value from this map entry
 * being considered.
 */
static int
findMask(key, value, arg)
    VOID *key;
    int value;
    VOID *arg;
{
    NetMaskStruct	*accNms = (NetMaskStruct *)arg;
    NetMaskStruct	*mapNms = (NetMaskStruct *)value;
    IPhost		*net = (IPhost *)key;
    IPhost		tmp;

    IP_AND(tmp, accNms->host, mapNms->mask);
    /* 
     * Save this index if the host corresponds to the network and
     * if we haven't already found a corresponding network with a
     * higher metric (corresponding to the number of bits in the mask) 
     */
    if ( IP_EQUAL(tmp, *net) && mapNms->metric > accNms->metric ) {
	accNms->metric = mapNms->metric;
	accNms->mask = mapNms->mask;
    }
    return TRUE;
}


void
ipNetMask(mask, host)
    IPhost *mask;
    IPhost *host;
{
    NetMaskStruct	nms;
    
    xAssert(netMaskMap);
    nms.host = *host;
    nms.metric = 0;
    mapForEach(netMaskMap, findMask, &nms);
    if ( nms.metric > 0 ) {
	/* 
	 * We found an applicable mask in the table
	 */
	*mask = nms.mask;
    } else {
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
}


