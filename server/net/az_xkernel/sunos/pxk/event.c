/*
 * event.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.31 $
 * $Date: 1993/02/01 23:53:38 $
 */

#include "xk_debug.h"
#include "event.h"
#include "event_i.h"
#include "assert.h"
#include "x_libc.h"
#include <lwp/lwp.h>

#define THIS_IS_THE_HEADER ((EvFunc)-42)

/* BIG_N should be a function of the estimated number of scheduled events,
 * such that queues don't get too long, but with as little overhead as
 * possible. 128 is probably good for up to some 500-1000 events. [mats]
 */

#define BIG_N 128 

extern int	insque();
extern int	remque();

static struct Event 	evHead[BIG_N];
static int 		tickmod;

#ifdef XK_THREAD_TRACE

Map		localEventMap;
static Map	threadEventMap;

#endif


void
evInit()
{
    int i;

#ifdef XK_THREAD_TRACE
    localEventMap = mapCreate(BIG_N, sizeof(Event));
    threadEventMap = mapCreate(BIG_N, sizeof(thread_t));
#endif
    for(i=0; i<BIG_N; i++)
      {
	  evHead[i].next = &evHead[i];
	  evHead[i].prev = &evHead[i];
	  evHead[i].func = THIS_IS_THE_HEADER;
      }
    tickmod = 0;
}


static void
e_insque(int q, Event e)
{
  Event a;

  for (a = evHead[q].next; a != &evHead[q]; a = a->next) {
    if (a->deltat < e->deltat) {
      /* E goes after a */
      e->deltat -= a->deltat;
      continue;
    } else {
      /* E goes just before a */
      a->deltat -= e->deltat;
      insque(e, a->prev);
      return;
    }
  }
  /* E goes at the end */
  insque(e, evHead[q].prev);
}


static void
e_remque(Event e)
{
  if (e->next->func != THIS_IS_THE_HEADER) {
    e->next->deltat += e->deltat;
  }
  remque(e);
}


/* 
 * It is safe to call unbindEvent multiple times with the same event.
 * Only the first call will have any effect.
 */
static inline void
unbindEvent( Event e )
{
#ifdef XK_THREAD_TRACE
    xAssert(e);
    if ( e->bind ) {
	mapRemoveBinding(threadEventMap, e->bind);
	e->bind = 0;
    }
#endif
}


static inline void
freeEvent( Event e )
{
#ifdef XK_THREAD_TRACE
    xkern_return_t	xkr;
#endif

    xAssert(e);
#ifdef XK_THREAD_TRACE
    unbindEvent(e);
    xkr = mapUnbind(localEventMap, &e);
    xAssert( xkr == XK_SUCCESS );
#endif
    xFree((char *)e);
}


void
evMarkBlocked( s )
    Semaphore	*s;
{
#ifdef XK_THREAD_TRACE
    Event	evSelf;
    thread_t	thSelf;

    lwp_self(&thSelf);
    if ( mapResolve(threadEventMap, &thSelf, &evSelf) == XK_SUCCESS ) {
	evSelf->state = E_BLOCKED;
	xGetTime(&evSelf->stopTime);
    } else {
	xTrace0(event, TR_ERRORS, "realP -- Couldn't find event structure");
    }
#endif
}


void
evMarkRunning()
{
#ifdef XK_THREAD_TRACE
    Event	evSelf;
    thread_t	thSelf;

    lwp_self(&thSelf);
    if ( mapResolve(threadEventMap, &thSelf, &evSelf) == XK_SUCCESS ) {
	evSelf->state = E_RUNNING;
    } else {
	xTrace0(event, TR_ERRORS, "realP -- Couldn't find event structure");
    }
#endif
}


void
stub(Event e)
{
    if ( e->flags & E_CANCELLED_F ) {
	xTrace1(event, TR_MORE_EVENTS,
		"event stub not starting cancelled event %x", e);
	freeEvent(e);
	return;
    }
#ifdef XK_THREAD_TRACE
    {
	thread_t		self;
	
	lwp_self(&self);
	e->bind = mapBind(threadEventMap, &self, e);
	xAssert( e->bind != ERR_BIND );
	xGetTime(&e->startTime);
    }
#endif
    e->state = E_RUNNING;
    e->func(e, e->arg);
    e->state = E_FINISHED;
    if ( e->flags & E_DETACHED_F ) {
	freeEvent(e);
    } else {
	unbindEvent(e);
#ifdef XK_THREAD_TRACE
	xGetTime(&e->stopTime);
#endif
    }
}
      

Event
evSchedule( EvFunc func, void *arg, unsigned time)  /* time in usec */
{
  Event e;
  u_int delt;

  xTrace3(event, TR_GROSS_EVENTS, "evSchedule: f = %x, arg = %x, time = %d",
	  func, arg, time);
  e = (Event)xMalloc(sizeof(struct Event));
  e->func = func;
  e->arg = arg;

#ifdef XK_THREAD_TRACE
  {
      XTime	offset = { time / 1E6, time % (unsigned)1E6 };
      Bind	b;
      
      b = mapBind(localEventMap, &e, e);
      xAssert( b != ERR_BIND );
      e->bind = 0;   /* filled in in 'stub' once a native thread is assigned */
      xGetTime(&e->startTime);
      xAddTime(&e->startTime, e->startTime, offset);
  }
#endif
  
  /*
   * EVENT_INTERVAL, time between scans of the event list, is also in
   * usec.
   */
  delt = (time + EVENT_INTERVAL / 2) / EVENT_INTERVAL;
  if (delt == 0 && time != 0) {
      delt = 1;
  }
  xTrace1(event, TR_EVENTS, "event requires %d clock ticks", delt);
  e->deltat = delt / BIG_N + 1;
  e->flags = 0;
  if (delt == 0) {
    e->state = E_SCHEDULED;
    /* createprocess(stub, e); */
    CreateKernelProcess(stub, STD_PRIO, 1, e);
  } else {
    xTrace1(event, TR_FUNCTIONAL_TRACE, "Placing event in queue %d", (tickmod+delt)%BIG_N);
    e_insque((tickmod + delt) % BIG_N, e);
    e->state = E_PENDING;
  }
  xTrace1(event, TR_MAJOR_EVENTS, "evSchedule returns event %x", e);
  return e;
}


void
evDetach( Event e )
{
    xTrace1(event, TR_GROSS_EVENTS, "evDetach: event = %x", e);
    if ( e->state == E_FINISHED ) {
	freeEvent(e);
    } else {
	e->flags |= E_DETACHED_F;
    }
}


/* 
 * See description in event.h
 */
EvCancelReturn
evCancel(Event e)
{
  int ans;

  xTrace1(event, TR_GROSS_EVENTS, "evCancel: event = %x", e);
  switch ( e->state ) {

    case E_SCHEDULED:
      e->flags |= ( E_DETACHED_F | E_CANCELLED_F );
      /* 
       * The stub routine will catch this event before it gets a chance to
       * run
       */
      ans = EVENT_CANCELLED;
      break;

    case E_BLOCKED:
    case E_RUNNING:
      e->flags |= ( E_DETACHED_F | E_CANCELLED_F );
      ans = EVENT_RUNNING;
      break;

    case E_FINISHED:
      ans = EVENT_FINISHED;
      freeEvent(e);
      break;

    case E_PENDING:
      e_remque(e);
      ans = EVENT_CANCELLED;
      freeEvent(e);
      break;
  }

  return ans;
}
  

int
evIsCancelled( e )
    Event	e;
{
    xAssert(e);
    return e->flags & E_CANCELLED_F;
}


void
clock_ih()
{
    Event e;
    Event head;
    
    xTrace0(event, TR_GROSS_EVENTS, "event clock interrupt");
    head = &evHead[tickmod];
    e = head->next;
    if (e != head) {
	e->deltat--;
	xTrace2(event, TR_EVENTS, "lead event in queue %d has %d interrupts to go",
		tickmod, e->deltat);
	xIfTrace(event, TR_FUNCTIONAL_TRACE) {
	    int i;
	    for (i=0; e != head; e = e->next) {
		i++;
	    }
	    e = head->next;
	    xTrace2(event, TR_FUNCTIONAL_TRACE, "%d events in queue %d", i, tickmod);
	}
	for (; e != head && e->deltat == 0; e = e->next) {
	    /*
	     * Fire this event and remove it from the queue. No adjusting
	     * of the following deltat is required.
	     */
	    xTrace1(event, TR_GROSS_EVENTS, "Event %x expired", e->func);
	    remque(e);
	    e->state = E_SCHEDULED;
#ifdef XK_THREAD_TRACE
	    xGetTime(&e->startTime);
#endif
	    /* createprocess(stub, e); */
	    CreateProcess1((Pfi)stub, STD_PRIO, (int)e);
	}
    } else {
	xTrace1(event, TR_FUNCTIONAL_TRACE, "0 events in queue %d", tickmod);
    }
    tickmod = (tickmod + 1 ) % BIG_N;
    xTrace0(event, TR_GROSS_EVENTS, "end event clock interrupt");
}


