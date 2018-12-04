/*
 * icmp.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:41:11 $
 */

#ifndef icmp_h
#define icmp_h

#ifndef ip_h
#include "ip.h"
#endif

#define	ICMP_ECHO_REP	0
#define ICMP_DEST_UNRCH	3
#define ICMP_SRC_QUENCH	4
#define ICMP_REDIRECT	5
#define	ICMP_ECHO_REQ	8
#define ICMP_TIMEOUT	11
#define ICMP_SYNTAX	12
#define ICMP_TSTAMP_REQ	13
#define ICMP_TSTAMP_REP	14
#define ICMP_INFO_REQ	15
#define ICMP_INFO_REP	16
#define ICMP_AMASK_REQ	17
#define ICMP_AMASK_REP	18


#define ICMP_NET_UNRCH	0
#define ICMP_HOST_UNRCH	1
#define ICMP_PROT_UNRCH	2
#define ICMP_PORT_UNRCH	3
#define ICMP_CANT_FRAG	4
#define ICMP_SRC_R_FAIL	5

#define ICMP_CANT_HOP	0
#define ICMP_CANT_REASS	1


#define ICMP_NET_REDIRECT	0
#define ICMP_HOST_REDIRECT	1
#define ICMP_TOSNET_REDIRECT	2
#define ICMP_TOSHOST_REDIRECT	3

#define ICMP_ECHO_CTL		(ICMP_CTL * MAXOPS + 0)


#  ifdef __STDC__

void	icmp_init( XObj );

#  endif

#endif icmp_h
