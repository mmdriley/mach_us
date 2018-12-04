/*
 * route.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.11 $
 * $Date: 1993/02/01 22:22:22 $
 */

/*
 * Interface to the routing subsystem
 */

#ifndef route_h
#define route_h

#define IPROUTE_TTL_INFINITE	0xffff
#define RTDEFAULTTTL    60*24	  /* default ttl for a route - 1 day */

typedef struct route {
	IPhost	net; 		/* net for this route */
	IPhost  mask;		/* mask for this net */
 	IPhost  gw;		/* gateway for hop */
	int 	metric; 	/* distance metric */
	Sessn 	netSessn; 	/* cached network session */
	u_short key;		/* sort key */
	u_short ttl;		/* time to live */
	struct route *next;	/* next for this hash */
/* stats */
	u_long  refcount;	/* current reference count */
	u_long  usecount;	/* use count */
} route;


#include "ip_i.h"

#ifdef __STDC__

/* 
 * Initialize the routing tables and set the default router to be the
 * given IP host.  If the IP host is all zeroes or if it is not
 * directly reachable, there will be no default router and rt_init
 * will return -1.
 */
extern int	rt_init( PState *, IPhost * );

extern int 	rt_add( PState *, IPhost *, IPhost *, IPhost *, int, int );
extern int	rt_add_def( PState *, IPhost * );
extern route *	rt_get( IPhost * );
extern void	rt_free( route * );
extern void	rt_delete( IPhost *, IPhost * );

#else

extern int	rt_init();
extern int 	rt_add();
extern int	rt_add_def();
extern route *	rt_get();
extern void	rt_free();
extern void	rt_delete();

#endif __STDC__

#endif  ! route_h
