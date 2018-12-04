/* 
 * arp_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/01 22:19:20 $
 */

#ifndef arp_i_h
#define arp_i_h

#include "ip.h"
#include "eth.h"
#include "arp.h"

#define	ARP_HRD	  1
#define	ARP_PROT  0x0800	/* doing IP addresses only */
#define	ARP_HLEN  28		/* the body is null */
#define	ARP_TAB   100		/* arp table size */
#define ARP_TIME  2000		/* 2 seconds */
#define ARP_RTRY  2		/* retries for arp request */
#define ARP_RRTRY 5		/* retries for rarp request */
#define INIT_RARP_DELAY	 5000	/* msec to delay between failed self rarps */

#define	ARP_REQ   1
#define	ARP_RPLY  2
#define	ARP_RREQ  3
#define	ARP_RRPLY 4

#define ARP_MAXOP 4


typedef enum { ARP_ARP, ARP_RARP } ArpType;
typedef enum { ARP_FREE, ARP_ALLOC, ARP_RSLVD } ArpStatus;

typedef struct {
    short  arp_hrd;
    short  arp_prot;
    char   arp_hlen;
    char   arp_plen;
    short  arp_op;
    ETHhost  arp_sha;
    IPhost arp_spa;
    ETHhost  arp_tha;
    IPhost arp_tpa;
} ArpHdr;


/*
 * An arpWait represents an outstanding request
 */
typedef struct {
    ArpStatus	*status;
    int 	tries;
    Event	event;
    Semaphore 	s;
    int 	numBlocked;
    XObj	self;		/* ARP protocol */
    ArpHdr	reqMsg;		/* ARP requests only */
} ArpWait;


typedef struct ARPprotlstate {
    XObj		rarp;
    struct arpent	*tbl;
    XObj		arpSessn;
    XObj		rarpSessn;
    ArpHdr		hdr;
} PSTATE;


#ifdef __STDC__

void		arpPlatformInit( XObj );
void		arpSendRequest( ArpWait * );
void		newArpWait( ArpWait *, XObj, IPhost *, ArpStatus * );
void		newRarpWait( ArpWait *, XObj, ETHhost *, ArpStatus * );

#else

void		arpPlatformInit();
void		arpSendRequest();
void		newArpWait();
void		newRarpWait();

#endif


extern int	tracearpp;

#endif arp_i_h
