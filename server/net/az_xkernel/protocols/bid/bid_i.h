/* 
 * bid_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:37:50 $
 */

/* 
 * Declarations private to the BID protocol
 */


#ifndef bid_i_h
#define bid_i_h

#include "bidctl.h"

typedef struct {
    BootId	srcBid;		/* sender boot ID   */
    BootId	dstBid;		/* receiver boot ID */
    long	hlpNum;
} BidHdr;


typedef struct {
    BootId	myBid;
    Map		activeMap;	/* lls -> bootid sessions 	*/
    Map		passiveMap;	/* hlpNum -> hlps 	 	*/
} PState;



typedef struct {
    BidHdr	hdr;
    IPhost	peer;
} SState;



typedef struct {
    XObj	lls;
    long	hlpNum;
} ActiveKey;

typedef long	PassiveKey;



/* 
 * Configuration constants
 */
#define BID_ACTIVE_MAP_SIZE	101
#define BID_PASSIVE_MAP_SIZE	31


/* 
 * Protocol down vector indices
 */
#define BID_XPORT_I	0
#define BID_CTL_I	1

#endif  ! bootid_i_h
