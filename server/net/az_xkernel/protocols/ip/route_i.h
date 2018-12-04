/*
 * route_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:22:05 $
 */

/*
 * Structures and constants internal to the routing subsystem
 */

#ifndef route_i_h
#define route_i_h

#define ROUTETABLESIZE  512
#define BPSIZE		100
#define OK		1
#define RTTABLEDELTA    1	  /* decrement route ttl by 1 every update */
#define RTTABLEUPDATE  	1000*60   /* update route table every minute */

typedef struct rtinfo {
	u_char  valid;		/* is rtinfo valid */
	Semaphore mutex;	/* mutex semaphore */
	route	*defrt;		/* default route */
	int	bpool;		/* route structures buffer pool pointer */
	u_short bpoolsize;	/* number of unallocated structures remaining */
} rtinfo;

#define RTFREE(rt)	xTrace0(ipp, 5, "RTFREE")
/*
#define RTFREE(rt)	if ( --(rt->refcount) == 0 ) \
			    xFree((char *)rt);
*/

#endif ! route_i_h
