/*
 * udp_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.10 $
 * $Date: 1993/02/01 22:28:23 $
 */

#include "udp_port.h"

#define	HLEN	(sizeof(HDR))

typedef struct header {
    UDPport 	sport;	/* source port */
    UDPport 	dport;	/* destination port */
    short 	ulen;	/* udp length */
    u_short	sum;	/* udp checksum */
} HDR;

typedef struct pstate {
    long	llpProt;	/* My protocol number relative to llp */
    Map   	activemap;
    Map		passivemap;
} PSTATE;

typedef struct sstate {
    HDR         hdr;
    IPpseudoHdr	pHdr;
    u_char	useCkSum;
} SSTATE;

/*
 * The active map is keyed on the pair of ports and the lower level IP
 * session.
 */
typedef struct {
    UDPport   	localport;
    UDPport  	remoteport;
    Sessn	lls;
} ActiveId;

typedef struct {
    Msg		*m;
    XObj 	s;
} storeInfo;

/*
 * The key for the passive map is just the local UDP port number.
 */
typedef UDPport PassiveId;

#define USE_CHECKSUM_DEF 0


