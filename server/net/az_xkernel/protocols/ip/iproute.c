/*
 * iproute.c
 * 
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.21 $
 * $Date: 1993/02/01 22:22:16 $
 */

#include "xkernel.h"
#include "eth.h"
#include "ip.h"
#include "ip_i.h"
#include "arp.h"
#include "route.h"
#include "route_i.h"

#ifdef __STDC__

static route *	rt_alloc(void);
static int	rt_hash(IPhost *);
static route *	rt_new(IPhost *, IPhost *, IPhost *, int, int);
static void	rt_timer( Event, void * );

#else

static route *	rt_alloc();
static int	rt_hash();
static route *	rt_new();
static void	rt_timer();

#endif  __STDC__

static rtinfo 	Route;
static route	*Rtable[ROUTETABLESIZE];
static IPhost	ipNull = { 0, 0, 0, 0 };


int
rt_init(pstate, defGw)
    PState *pstate;
    IPhost *defGw;
{
    int i;
    
    xTrace0(ipp,1,"IP rt_init()");
    Route.valid = TRUE;
    semInit(&(Route.mutex),1);
    for( i = 0; i < ROUTETABLESIZE; i++) {
	Rtable[i] = NULL;
    }
    /* allocate buffer pool */
    if ( (Route.bpool = (int) xMalloc(BPSIZE * sizeof(route))) != NULL ) {
	bzero((char *)Route.bpool, BPSIZE * sizeof(route));
	Route.bpoolsize = BPSIZE;
    }
    else {
	xError("XMalloc for buffer pool failed");
	Route.bpool = -1;
	Route.bpoolsize = 0;
    }
    if ( IP_EQUAL(*defGw, ipNull) || rt_add_def(pstate, defGw) ) {
	return -1;
    }
    evDetach( evSchedule( rt_timer, 0, RTTABLEUPDATE * 1000 ) );
    xTrace0(ipp,1,"IP rt_init() done");
    return 0;
}


static route *
rt_alloc()
{
    route *ptr;
    
    if (Route.bpoolsize == 0 ) {
	xTrace0(ipp,5,"IP rt_alloc : allocating new bpool");
	Route.bpool = (int) xMalloc(BPSIZE * sizeof(route));
	bzero((char *)Route.bpool, BPSIZE * sizeof(route));
	Route.bpoolsize = BPSIZE;
    }
    ptr = (route *) Route.bpool;
    Route.bpool += sizeof(route);
    Route.bpoolsize--;
    return ptr;
}


static route *
rt_new(net, mask, gw, metric, ttl)
    IPhost *net;
    IPhost *mask;
    IPhost *gw;
    int metric;
    u_short ttl;
{
    route *ptr;
    
    ptr = rt_alloc();
    ptr->net = *net;
    ptr->mask = *mask;
    ptr->gw = *gw;
    ptr->metric = metric;
    ptr->ttl = ttl;
    ptr->refcount = 1;
    ptr->usecount = 0;
    ptr->next = NULL;
    return ptr;
}


int
rt_add_def( ps, gw )
    PState *ps;
    IPhost *gw;
{
    xTrace1(ipp, 4, "IP default route changes.  New GW: %s", ipHostStr(gw));
    semWait(&Route.mutex);
    if ( ! ipHostOnLocalNet(ps, gw) ) {
	xTrace1(ipp, 3, "ip: rt_add_def couldn't find interface for gw %s",
		ipHostStr(gw));
	semSignal(&Route.mutex);
	return -1;
    }
    Route.defrt = rt_new( &ipNull, &ipNull, gw, 1,  0 );
    semSignal(&Route.mutex);
    /*
     * Re-open the connection for every remote host not connected to the
     * local net.  This is certainly overkill, but:
     * 		-- it is not incorrect
     *		-- the default route shouldn't change often
     *		-- keeping track of which hosts use the default route is a
     *		   pain.
     */
    ipRouteChanged(ps, Route.defrt, ipRemoteNet);
    return 0;
}
  

int
rt_add( pstate, net, mask, gw, metric, ttl )
    PState *pstate;
    IPhost *net;
    IPhost *mask;
    IPhost *gw;
    int metric;
    u_short ttl;
{
    route 	*ptr, *srt, *prev;
    u_char  	isdup;
    int  	j;
    u_long 	hashvalue;
    
    semWait(&Route.mutex);
    ptr = rt_new(net, mask, gw, metric, ttl);
    
    /* compute sort key - number of set bits in mask 
       so that route are sorted : host, subnet, net */
    for (j = 0; j < 8; j++)
      ptr->key += ((mask->a >> j) & 1) + 
	((mask->b >> j) & 1) +
	  ((mask->c >> j) & 1) +
	    ((mask->d >> j) & 1) ;
    
    prev = NULL;
    hashvalue = rt_hash(net);
    xTrace1(ipp,5,"IP rt_add : hash value is %d",hashvalue);
    isdup = FALSE;
    for ( srt = Rtable[hashvalue]; srt; srt = srt->next ) {
	if ( ptr->key > srt->key )
	  break;
	if ( IP_EQUAL(srt->net, ptr->net) && 
	    IP_EQUAL(srt->mask, ptr->mask) ) {
	    isdup = TRUE;
	    break;
	}
	prev = srt;
    }
    if ( isdup ) {
	route *tmptr;
	if ( IP_EQUAL(srt->gw, ptr->gw) ) {
	    /* update existing route */
	    xTrace0(ipp,5,"IP rt_add : updating existing route");
	    srt->metric = metric;
	    srt->ttl = ttl;
	    RTFREE(ptr);
	    semWait(&Route.mutex);
	    return 0;
	}
	/* otherwise someone else has a route there */
	/*
	 * negative metric indicates unconditional override
	 */
	if ( ptr->metric > 0 && srt->metric <= ptr->metric ) {
	    /* it's no better, just drop it */
	    xTrace0(ipp,5,
		    "IP rt_add : dropping duplicate route with greater metric");
	    RTFREE(ptr);
	    semSignal(&Route.mutex);
	    return 0;
	}
	xTrace0(ipp,5,"IP rt_add : new duplicate route better, deleting old");
	tmptr = srt;
	srt = srt->next;
	RTFREE(tmptr);
    } else {
	xTrace0(ipp,5,"IP rt_add : adding fresh route");
    }
    ipRouteChanged(pstate, ptr, ipSameNet);
    ptr->next = srt;
    if ( prev ) {
	prev->next = ptr;
    } else {
	Rtable[hashvalue] = ptr;
    }
    semSignal(&Route.mutex);
    return 0; 
} /* rt_add */


route *
rt_get( dest )
     IPhost *dest;
{
    route *ptr;
    u_long hashvalue;
    u_long sum;
    IPhost fdest;
    
    semWait(&Route.mutex);
    hashvalue = rt_hash(dest);
    xTrace1(ipp,5,"IP rt_get : hash value is %d",hashvalue);
    for( ptr = Rtable[hashvalue]; ptr; ptr = ptr->next ) {
	if ( ptr->ttl == 0 )
	  continue;
	sum =  *(long *)dest & *(long *)&(ptr->mask);
	fdest = *(IPhost *) &sum;
	if ( IP_EQUAL(fdest,ptr->net) )
	  break;
    }
    if ( ptr == 0 ) {
	ptr = Route.defrt;
    }
    if ( ptr ) {
	ptr->refcount++;
	ptr->usecount++;
	xTrace3(ipp, 5,"IP rt_get : Mapped host %s to net %s, gw %s",
		ipHostStr(dest), ipHostStr(&ptr->net), ipHostStr(&ptr->gw));
    } else {
	xTrace1(ipp, 2, "IP rt_get: Could not find route for host %s!",
		ipHostStr(dest));
    }
    semSignal(&Route.mutex);
    return ptr;
}


void
rt_delete( net, mask )
     IPhost *net, *mask;
{
    route *ptr, *prev;
    u_long  hashvalue; 
    
    semWait(&Route.mutex);
    
    hashvalue = rt_hash(net);
    prev = NULL;
    for ( ptr = Rtable[hashvalue]; ptr; ptr = ptr->next ) {
	if ( IP_EQUAL(*net, ptr->net) &&
	    IP_EQUAL(*mask, ptr->mask) )
	  break;
	prev = ptr;
    }
    if ( ptr == NULL ) {
	semWait(&Route.mutex);
	return;
    }
    if ( prev )
      prev->next = ptr->next;
    else
      Rtable[hashvalue] = ptr->next;
    RTFREE(ptr);
    semSignal(&Route.mutex);
    return;
}


/* hash value is sum of net portions of IP address */
static int
rt_hash( net )
     IPhost *net;
{
    IPhost	mask;
    u_long 	hashvalue;
    
    netMaskFind(&mask, net);
    IP_AND(mask, mask, *net);
    hashvalue = mask.a + mask.b + mask.c + mask.d;
    return (hashvalue % ROUTETABLESIZE);
}


void
rt_free(rt)
    route *rt;
{
    semWait(&Route.mutex);
    RTFREE(rt);
    semSignal(&Route.mutex);
}


static void
rt_timer(ev, arg)
    Event	ev;
    VOID 	*arg;
{
    route *ptr, *prev;
    int i;
    
    xTrace0(ipp,5,"IP rt_timer called");
    semWait(&Route.mutex);
    for ( i = 0; i < ROUTETABLESIZE; i++) {
	if ( Rtable[i] == 0 ) {
	    continue;
	}
	prev = NULL;
	for ( ptr = Rtable[i]; ptr; ) {
	    if ( ptr->ttl != IPROUTE_TTL_INFINITE ) {
		ptr->ttl -= RTTABLEDELTA;
	    }
	    if ( ptr->ttl == 0 ) {
		if ( prev ) {
		    prev->next = ptr->next;
		    RTFREE(ptr);
		    ptr = prev->next;
		} else {
		    Rtable[i] = ptr->next;
		    RTFREE(ptr);
		    ptr = Rtable[i];
		}
		continue;
	    }
	    prev = ptr;
	    ptr = ptr->next;
	}
    }
    semSignal(&Route.mutex);
    /*
     * Reschedule this event
     */
    evDetach( evSchedule( rt_timer, 0, RTTABLEUPDATE * 1000 ) );
}
