/*     
 * $RCSfile: event_monitor.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 23:58:43 $
 */

/*
 * Code supporting ps-like tracing for x-kernel events.  Some of this
 * support is in the platform-specific event code.
 */

#include "xk_debug.h"
#include "event.h"
#include "event_i.h"
#include "assert.h"
#include "x_libc.h"


#ifdef XK_THREAD_TRACE

static char *	evStateStr( EvState );
static int	xTime2sec( XTime );
static void	dispEvent( Event, XTime );
static int	findEvents( VOID *, VOID *, VOID *);
static int	sortFunc( Event *, Event *);



static char *
evStateStr( s )
    EvState	s;
{
    switch ( s ) {
      case E_PENDING: 	return "PENDING";
      case E_SCHEDULED:	return "SCHED";
      case E_RUNNING: 	return "RUNNING";
      case E_FINISHED: 	return "FINISH";
      case E_BLOCKED: 	return "BLOCKED";
      default:		return "UNKNOWN";
    }
}



static int
xTime2sec( t )
    XTime	t;
{
    t.usec += 500 * 1000;
    t.sec += t.usec / (1000 * 1000);
    return t.sec;
}


static void
dispEvent( ev, now )
    Event	ev;
    XTime	now;
{
    XTime	diff;

    xAssert(ev);
    printf("%8x  ", (int)ev->func);
    printf("%8x\t", (int)ev->arg);
    printf("%s\t", evStateStr(ev->state));
    switch ( ev->state ) {
      case E_PENDING:
	xSubTime(&diff, ev->startTime, now);
	printf("%8d", xTime2sec(diff));
	break;
	
      case E_SCHEDULED:
      case E_RUNNING:
	xSubTime(&diff, now, ev->startTime);
	printf("%8d", xTime2sec(diff));
	break;

      case E_FINISHED:
      case E_BLOCKED:
	xSubTime(&diff, now, ev->stopTime);
	printf("%8d", xTime2sec(diff));
	break;
    }
    printf("\t");
    if ( ev->flags & E_DETACHED_F ) {
	printf("%c", 'D');
    }
    if ( ev->flags & E_CANCELLED_F ) {
	printf("%c", 'C');
    }
    printf("\n");
}


#define MAX_EVENTS	128	/* just a suggestion */


typedef struct {
    Event	*arr;
    int		i;
    int		max;
} EvArray;


static int
findEvents( key, val, arg )
    VOID	*key, *val, *arg;
{
    EvArray	*eva = (EvArray *)arg;
    Event	*newArray;
    int		newSize;

    if ( eva->i >= eva->max ) {
	newSize = eva->max ? eva->max * 2 : MAX_EVENTS;
	newArray = (Event *)xMalloc(sizeof(Event) * newSize);
	bzero((char *)newArray, sizeof(Event) * newSize);
	bcopy((char *)eva->arr, (char *)newArray, sizeof(Event) * eva->max);
	if ( eva->arr ) {
	    xFree((char *)eva->arr);
	}
	eva->arr = newArray;
	eva->max = newSize;
    }
    eva->arr[eva->i++] = (Event)val;
    return TRUE;
}


static int
sortFunc( e1, e2 )
    Event	*e1, *e2;
{
    if ( (*e1)->state != (*e2)->state ) {
	return (u_int)(*e1)->state - (u_int)((*e2)->state);
    }
    if ( (*e1)->func != (*e2)->func ) {
	return (u_int)(*e1)->func - (u_int)(*e2)->func;
    }
    if ( (*e1)->arg != (*e2)->arg ) {
	return (u_int)(*e1)->arg - (u_int)(*e2)->arg;
    }
    return 0;
}


void
evDump()
{
    XTime	now;
    EvArray	evArray;
    int		i;

    xGetTime(&now);
    evArray.arr = 0;
    evArray.max = 0;
    evArray.i = 0;
    mapForEach(localEventMap, findEvents, &evArray);
#ifndef XKMACHKERNEL
    qsort((char *)evArray.arr, evArray.i, sizeof(Event), sortFunc);
#endif
    printf("   Func      arg\tstate\t  seconds\tflags\n\n");
    for ( i=0; i < evArray.i; i++ ) {
	dispEvent(evArray.arr[i], now);
    }
    xFree((char *)evArray.arr);
}

#else	/* ! XK_THREAD_TRACE */

void
evDump()
{
}

#endif XK_THREAD_TRACE
