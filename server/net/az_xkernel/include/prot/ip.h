/* 
 * ip.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.11 $
 * $Date: 1993/02/01 22:41:21 $
 */

#ifndef ip_h
#define ip_h

#include "upi.h"
#include "ip_host.h"


typedef struct ippseudohdr {
  IPhost src;
  IPhost dst;
  u_char zero;
  u_char prot;
  u_short len;
} IPpseudoHdr;

/*
 * IP control opcodes
 */
#define IP_MYNET	(IP_CTL*MAXOPS+0)
#define IP_REDIRECT	(IP_CTL*MAXOPS+1)
#define IP_GETRTINFO    (IP_CTL*MAXOPS+2)

#define IP_LOCAL_BCAST_HOST	{ 255, 255, 255, 255 }


#  ifdef __STDC__

void	ip_init( XObj );

#  endif


#endif
