/* 
 * bidctl.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:41:53 $
 */

/* 
 * Public non-standard interface to bidctl
 */


#ifndef bidctl_h
#define bidctl_h
   
#  ifdef __STDC__

void		bidctl_init( XObj );

#  endif


typedef	u_int	BootId;


/* 
 * Protocols which previously registered are informed of reboots using
 * a BidctlRebootMsg as the buffer in a BIDCTL_PEER_REBOOTED control call.
 * These protocols will also be informed by BIDCTL when an initial
 * BootId is learned via a BIDCTL_FIRST_CONTACT control call.
 */

typedef struct {
    IPhost	h;
    BootId	id;
} BidctlBootMsg;
   
#define BIDCTL_PEER_REBOOTED		( BIDCTL_CTL * MAXOPS + 0 )
#define BIDCTL_GET_LOCAL_BID		( BIDCTL_CTL * MAXOPS + 1 )
#define BIDCTL_GET_PEER_BID		( BIDCTL_CTL * MAXOPS + 2 )
#define BIDCTL_GET_PEER_BID_BLOCKING	( BIDCTL_CTL * MAXOPS + 4 )
#define BIDCTL_FIRST_CONTACT		( BIDCTL_CTL * MAXOPS + 3 )


/* 
 * Temporary session control ops for testing
 */

#define BIDCTL_TEST_SEND_BAD_BID	( BIDCTL_CTL * MAXOPS + 10 )
#define BIDCTL_TEST_SEND_QUERY		( BIDCTL_CTL * MAXOPS + 11 )
#define BIDCTL_DUMP_STATE		( BIDCTL_CTL * MAXOPS + 12 )
#define BIDCTL_TEST_SEND_BCAST		( BIDCTL_CTL * MAXOPS + 13 )

#endif  ! bidctl_h
