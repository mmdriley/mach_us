/* 
 * bidctl_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.16 $
 * $Date: 1993/02/01 22:36:28 $
 */

#ifndef bidctl_i_h
#define bidctl_i_h

/* 
 * Declarations private to the BIDCTL protocol (private even from BID).
 */


#include "bidctl.h"

/* 
 * Message flags
 */

#define	BIDCTL_QUERY_F		0x1
#define	BIDCTL_RESPONSE_F	0x2
#define BIDCTL_BCAST_F		0x4


typedef enum {
    BIDCTL_NO_QUERY = 0, BIDCTL_NEW_QUERY, BIDCTL_REPEAT_QUERY
} BidctlQuery;


/* 
 * Session finite state machine states
 */
typedef enum {
    BIDCTL_INIT_S,
    BIDCTL_NORMAL_S,
    BIDCTL_QUERY_S,
    BIDCTL_RESET_S,
    BIDCTL_OPEN_S,
    BIDCTL_OPENDONE_S
} FsmState;


/* 
 * A BidCtlState is shared among sessions with the same remote host.
 * It may also exist in the protocol idMap without being used by any
 * session. 
 */
typedef struct {
    IPhost	peerHost;
    FsmState	fsmState;
    BootId	peerBid;
    u_int	timer;	
    u_int	rcnt;
    BootId	reqTag;		/* Unique stamp we used in our request msg */
    Bind	bind;
    XObj	lls;
    XObj	myProtl;
    u_int	retries;
    Event	ev;
    Map		hlpMap;
    Semaphore	waitSem;
    int		waitSemCount;
    int		timeout;
} BidctlState;


typedef struct {
    u_short	flags;
    u_short	sum;
    BootId	srcBid;		/* sender boot ID   */
    BootId	dstBid;		/* receiver boot ID */
    BootId	reqTag;		/* receiver boot ID */
    BootId	rplTag;		/* receiver boot ID */
} BidctlHdr;


typedef struct {
    BootId	myBid;
    Map		bsMap;		/* remote host -> BootidState	    */
} PState;


extern int	tracebidctlp;


/* 
 * Configuration constants
 */
#define BIDCTL_BSMAP_SIZE	101
#define BIDCTL_HLPMAP_SIZE	131
#define BIDCTL_REG_MAP_SIZE	31

#define BIDCTL_OPEN_TIMEOUT	5 * 1000 * 1000		/* 5 seconds */
#define BIDCTL_OPEN_TIMEOUT_MULT	2
#define BIDCTL_OPEN_TIMEOUT_MAX 60 * 1000 * 1000

#define BIDCTL_QUERY_TIMEOUT	5 * 1000 * 1000		/* 5 seconds */
#define BIDCTL_QUERY_TIMEOUT_MULT	2
#define BIDCTL_QUERY_TIMEOUT_MAX 60 * 1000 * 1000

#define BIDCTL_TIMER_INTERVAL	60 * 1000 * 1000
#define BIDCTL_KEEPALIVE	5			/* timer intervals */
#define BIDCTL_IDLE_LIMIT	10 			/* timer intervals */
/* 
 * BIDCTL_BCAST_QUERY_DELAY is low for testing purposes.  We probably
 * want a larger value.
 */
#define BIDCTL_BCAST_QUERY_DELAY	3 * 1000	/* 3 seconds */


#ifdef __STDC__

void	bidctlBcastBoot( XObj );
xkern_return_t	bidctlDestroy( BidctlState * );
void	bidctlDispState( BidctlState * );
char *	bidctlFsmStr( FsmState );
void	bidctlHdrDump( BidctlHdr *h, char *, IPhost * );
void	bidctlHdrStore( VOID *, char *, long, VOID * );
BootId	bidctlNewId( void );
void	bidctlOutput( BidctlState *, BidctlQuery sendQuery, int sendResponse,
		      BidctlHdr * );
void	bidctlReleaseWaiters( BidctlState * );
BootId	bidctlReqTag( void );
void	bidctlSemWait( BidctlState * );
void	bidctlStartQuery( BidctlState *, int );
void	bidctlTimer( Event, VOID * );
void	bidctlTimeout( Event, VOID * );
void	bidctlTransition( BidctlState *, BidctlHdr * );

#else

void	bidctlBcastBoot();
xkern_return_t	bidctlDestroy();
void	bidctlDispState();
char *	bidctlFsmStr();
void	bidctlHdrDump();
void	bidctlHdrStore();
BootId	bidctlNewId();
void	bidctlOutput();
void	bidctlReleaseWaiters();
BootId	bidctlReqTag();
void	bidctlSemWait();
void	bidctlStartQuery();
void	bidctlTimer();
void	bidctlTimeout();
void	bidctlTransition();

#endif

#endif  ! bootid_i_h
