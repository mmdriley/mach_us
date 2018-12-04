/* 
 * udp.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:40:48 $
 */

#ifndef udp_h
#define udp_h

#ifndef ip_h
#include "ip.h"
#endif

#define UDP_ENABLE_CHECKSUM (UDP_CTL * MAXOPS + 0)
#define UDP_DISABLE_CHECKSUM (UDP_CTL * MAXOPS + 1)
#define UDP_GETFREEPROTNUM	(UDP_CTL * MAXOPS + 2)
#define UDP_RELEASEPROTNUM	(UDP_CTL * MAXOPS + 3)

#  ifdef __STDC__

void	udp_init( XObj );

#  endif


#endif
