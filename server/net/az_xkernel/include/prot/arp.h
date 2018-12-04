/* 
 * arp.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.11 $
 * $Date: 1993/02/01 22:41:05 $
 */

#ifndef arp_h
#define arp_h

#ifndef upi_h
#include "upi.h"
#endif
#ifndef eth_h
#include "eth.h"
#endif
#ifndef ip_h
#include "ip.h"
#endif

#ifdef __STDC__

void		arp_init( XObj );

#endif

#define ARP_INSTALL 		( ARP_CTL * MAXOPS + 0 )
#define ARP_GETMYBINDING	( ARP_CTL * MAXOPS + 1 )

typedef struct {
    ETHhost	hw;
    IPhost	ip;
} ArpBinding;


/* 
 * kludge to let the sunos simulator's ethernet driver simulate broadcast
 */

#ifdef __STDC__
typedef int	(ArpForEachFunc)( ArpBinding *, VOID * );
#else
typedef int	(ArpForEachFunc)();
#endif __STDC__

typedef struct {
    VOID		*v;
    ArpForEachFunc	*f;
} ArpForEach;

#define ARP_FOR_EACH		( ARP_CTL * MAXOPS + 2 )


#endif
