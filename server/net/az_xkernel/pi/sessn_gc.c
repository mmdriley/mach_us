/*     
 * sessn_gc.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.10 $
 * $Date: 1993/02/01 23:57:17 $
 */

/*
 * This garbage collector collects idle sessions.  See gc.h for the
 * interface.
 */

#include "xkernel.h"
#include "gc.h"

int	tracesessngc = 0;


#ifdef __STDC__

static void	sessnCollect( Event, void * );

#else

static void	sessnCollect();

#endif __STDC__


typedef struct {
    Map		map;
    u_int	interval;
    Pfv		destroy;
    char *	msg;
} CollectInfo;


static void
schedule(c)
    CollectInfo *c;
{
    evDetach( evSchedule( sessnCollect, c, c->interval ) );
}


static int
markIdle(key, value, arg)
    VOID *key;
    int value;
    VOID *arg;
{
    XObj	s = (XObj)value;
    CollectInfo	*c = (CollectInfo *)arg;

    if ( s->rcnt == 0 ) {
	if ( s->idle ) {
	    xTrace2(sessngc, 5, "%s sessn GC closing %x", c->msg, s);
	    c->destroy( s );
	} else {
	    xTrace2(sessngc, 5, "%s sessn GC marking %x idle", c->msg, s);
	    s->idle = TRUE;
	}
    } else {
	xTrace3(sessngc, 7, "%s session GC: %x rcnt %d is not idle",
		c->msg, s, s->rcnt);
    }
    return MFE_CONTINUE;
}


static void
sessnCollect(ev, arg)
    Event	ev;
    VOID 	*arg;
{
    CollectInfo	*c = (CollectInfo *)arg;

    xTrace1(sessngc, 3, "session garbage collector (%s)", c->msg);
    mapForEach(c->map, markIdle, c);
    schedule( c );
    xTrace1(sessngc, 5, "%s sessn GC exits", c->msg);
}


void
initSessionCollector(m, interval, destructor, msg)
    Map m;
    int interval;
    Pfv destructor;
    char *msg;
{
    CollectInfo *c;

    xTrace2(sessngc, 3,
	    "session garbage collector initialized for map %x (%s)",
	    m, msg ? msg : "");
    c = (CollectInfo *) xMalloc( sizeof( CollectInfo ) );
    c->map = m;
    c->interval = interval;
    c->destroy = destructor;
    c->msg = msg ? msg : "";
    schedule( c );
}

