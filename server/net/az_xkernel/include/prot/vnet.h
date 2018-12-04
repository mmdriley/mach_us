/*
 * vnet.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:41:43 $
 */

#ifndef vnet_h
#define vnet_h


typedef enum {
    LOCAL_ADDR_C = 1,	/* An address for the local host */
    REMOTE_HOST_ADDR_C,	/* Remote host directly reachable on a local net */
    REMOTE_NET_ADDR_C,  /* Remote host on a remote network */
    BCAST_LOCAL_ADDR_C, /* 255.255.255.255 -- all local nets broadcast */
    BCAST_NET_ADDR_C,	/* Broadcast address for a single network	*/
    BCAST_SUBNET_ADDR_C /* Broadcast for a network in presence of subnets */
} VnetAddrClass;

typedef union {
    VnetAddrClass	class;
    IPhost		host;
} VnetClassBuf;


#define VNET_HOSTONLOCALNET	(VNET_CTL*MAXOPS + 0)
/* 
 * Input: IPhost.  Determines if the host is directly reachable on a
 * local interface.  When run on a protocol, all interfaces are
 * considered.  When run on a session, only those interfaces associated
 * with the session are considered.  Returns sizeof(IPhost) if the
 * host is reachable, 0 if it is not.
 */

#define VNET_ISMYADDR		(VNET_CTL*MAXOPS + 1)
/* 
 * Input: IPhost.  Is this ip host one of mine? 
 * (i.e., should packets sent to this address be delivered locally
 * (including broadcast))
 */

#define VNET_GETADDRCLASS	(VNET_CTL*MAXOPS + 2)
/* 
 * Input/output: VnetClassBuf.  Determines the VnetAddrClass of the given
 * IP host. 
 */

#define VNET_DISABLEINTERFACE	(VNET_CTL*MAXOPS + 3)
/* 
 * Input: VOID *.  Pushes through this session will not use the
 * specified interface until a corresponding VNET_ENABLEINTERFACE is
 * performed.  Probably only useful for broadcast sessions.
 */

#define VNET_ENABLEINTERFACE	(VNET_CTL*MAXOPS + 4)
/* 
 * Input: VOID *.  Undoes the effect of a VNET_DISABLEINTERFACE
 */

#define VNET_GETINTERFACEID	(VNET_CTL*MAXOPS + 5)
/* 
 * Output: VOID *.  Returns an identifier for this interface which can
 * be used as the argument on VNET_{DISABLE,ENABLE}INTERFACE calls.
 */

#define VNET_GETNUMINTERFACES	(VNET_CTL*MAXOPS + 6)
/* 
 * Output: int.  Returns number of interfaces used by the protocol
 * instantiation.
 */


#  ifdef __STDC__

void	vnet_init( XObj );

#  endif

#endif ! vnet_h
