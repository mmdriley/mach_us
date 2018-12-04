/* 
 * bidctl_timer.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:36:46 $
 */

/* 
 * Timer routines.  Active sessions (rcnt > 0) exchange keepalives and
 * inactive cached sessions eventually time out and are removed from
 * the map. 
 */

#include "xkernel.h"
#include "bidctl_i.h"

#ifdef __STDC__

static void	idleCollect( BidctlState *, Map );
static int	clockTick( VOID *, VOID *, VOID * );

#endif __STDC__


static void
bidctlKeepAlive( bs )
    BidctlState	*bs;
{
    if ( bs->timer && (bs->timer % BIDCTL_KEEPALIVE == 0)
	 && bs->fsmState == BIDCTL_NORMAL_S ) {
	xTrace1(bidctlp, TR_EVENTS,
		"Sending keepalive to %s", ipHostStr(&bs->peerHost));
	bidctlOutput(bs, BIDCTL_NO_QUERY, 0, 0);
    }
}



static void
idleCollect( bs, m )
    BidctlState	*bs;
    Map		m;
{
    xTrace1(bidctlp, TR_DETAILED, "bidctl idleCollect runs for peer %s",
	    ipHostStr(&bs->peerHost));
    if ( bs->timer >= BIDCTL_IDLE_LIMIT ) {
	xTrace1(bidctlp, TR_EVENTS,
		"Removing state for %s", ipHostStr(&bs->peerHost));
	/* 
	 * It's OK for bidctlDestroy to fail (if it couldn't nuke the event,
	 * for example.)  We'll just pick it up on the next clock iteration. 
	 */
	bidctlDestroy(bs);
    }
}



static int
clockTick( key, val, arg )
    VOID	*key, *val, *arg;
{
    BidctlState	*bs = (BidctlState *)val;
    Map		m = (Map)arg;

    bs->timer++;
    xTrace3(bidctlp, TR_DETAILED,
	    "bidctl clockTick runs for peer %s, rcnt %d, timer %d",
	    ipHostStr(&bs->peerHost), bs->rcnt, bs->timer);
    if ( bs->rcnt < 1 || (bs->rcnt == 1 && bs->ev) ) {
	idleCollect(bs, m);
    } else {
	bidctlKeepAlive(bs);
    }
    return MFE_CONTINUE;
}



/* 
 * bidctlTimer -- scan through the list of bidctlStates:
 *
 *	send keepalive messages to peers of idle states
 */
void
bidctlTimer( ev, arg )
    Event	ev;
    VOID	*arg;
{
    PState	*ps = (PState *)arg;

    xTrace0(bidctlp, TR_EVENTS, "bidctl timer runs");
    mapForEach(ps->bsMap, clockTick, ps->bsMap);
    evDetach(evSchedule(bidctlTimer, arg, BIDCTL_TIMER_INTERVAL));
}
