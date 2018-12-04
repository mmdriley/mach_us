/* 
 * veth_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:33:11 $
 */


#ifndef vnet_h
#include "vnet.h"
#endif

typedef struct {
    IPhost	host;
    IPhost	mask;
    XObj	llp;
    XObj	arp;
    int		subnet;	 /* does this interface use subnetting? */
} Interface;

#define VNET_MAX_INTERFACES 	10
#define VNET_MAX_PARTS	 	5

typedef struct {
    Map		activeMap;
    Map 	passiveMap;
    Map		bcastMap;
    Interface	ifc[VNET_MAX_INTERFACES];
    XObj	llpList[VNET_MAX_INTERFACES+1];
    u_int	numIfc;
} PState;

typedef enum { VNET_BCAST_SESSN, VNET_NORMAL_SESSN } VnetSessnType;

typedef struct {
    VnetSessnType	type;
    struct {
	Interface	*ifc;
	int		active;
    } ifcs[VNET_MAX_INTERFACES];
    	/* 
     	 * There may be more than one interface for a broadcast session
     	 */
    Map			map;	/* Which map am I bound in */
} SState;

typedef	XObj	PassiveKey;
typedef XObj	ActiveKey;

#define VNET_BCAST_MAP_SZ	11
#define VNET_ACTIVE_MAP_SZ	23
#define VNET_PASSIVE_MAP_SZ	11

