/*
 * event.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.32 $
 * $Date: 1993/02/02 00:05:45 $
 */

/*
 * Event library for Mach 3.0 platform [mats]
 */


#include <mach/message.h>

#ifdef XKMACHKERNEL
#include <kern/thread.h>
#include <kern/lock.h>
#include <kern/syscall_subr.h>
#else
#include <cthreads.h>
#include <mach.h>
#include <mach/mach_host.h>
#endif XKMACHKERNEL

#include "xk_debug.h"
#include "upi.h"
#include "platform.h"
#include "assert.h"
#include "event.h"
#include "event_i.h"
#include "xk_mach.h"

typedef int (*intFunc)();

#define THIS_IS_THE_HEADER ((EvFunc)-42)

extern int	insque();
extern int	remque();

/* BIG_N should be a function of the estimated number of scheduled events,
 * such that queues don't get too long, but with as little overhead as
 * possible. 128 is probably good for up to some 500-1000 events. [mats]
 */

#define BIG_N 128 

static struct Event evhead[BIG_N];

static mach_port_t	evClock_port;

#ifdef XKMACHKERNEL
#  define THREAD_T	thread_t
#  define THREAD_SELF()	thread_self()
#else
#  define THREAD_T	cthread_t
#  define THREAD_SELF()	cthread_self()
#endif XKMACHKERNEL

static THREAD_T		evClock_thread;


#ifdef XKMACHKERNEL
simple_lock_data_t	 eLock;
#define EVENT_LOCK()     simple_lock( &eLock )
#define EVENT_UNLOCK()   simple_unlock( &eLock )
#else
mutex_t			 event_mutex;
#define EVENT_LOCK()     mutex_lock(event_mutex)
#define EVENT_UNLOCK()   mutex_unlock(event_mutex)
#endif XKMACHKERNEL


extern int event_granularity;

int tickmod;
int tracetick;
int traceevent;

#ifdef XK_THREAD_TRACE

Map		localEventMap;
static Map	threadEventMap;

#endif


void evClock(int); /* Forward */

/*
 * evInit -- initialize the timer queues and start the clock thread.
 *
 */
void
evInit(interval)
    int interval;
{
  int i;
  xTrace0(event,TR_GROSS_EVENTS,"evInit enter");

#ifdef XK_THREAD_TRACE
    localEventMap = mapCreate(BIG_N, sizeof(Event));
    threadEventMap = mapCreate(BIG_N, sizeof(THREAD_T));
#endif

  for(i=0;i<BIG_N;i++)
    {
      evhead[i].next = &evhead[i];
      evhead[i].prev = &evhead[i];
      evhead[i].func = THIS_IS_THE_HEADER;
    }
  tickmod = 0;
 
/* initialize timers and semaphores */
#ifdef XKMACHKERNEL
  simple_lock_init( &eLock );
#else
  event_mutex = mutex_alloc();
  (void) mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
			    &evClock_port);
  (void) mach_port_insert_right(mach_task_self(),
				evClock_port, evClock_port,
				MACH_MSG_TYPE_MAKE_SEND);
#endif XKMACHKERNEL
  
/* Since evClock does not use any xkernel routines, this should be 
   allowed to remain a cthread, in spite of the sledgehammer concurrency
   control otherwise enforced in the xkernel.
 */

  xTrace0(event,TR_EVENTS,"evInit starting evClock thread");

#ifdef XKMACHKERNEL
  /* 
   * "kernel_thread" seems broken; does not get the third parameter
   *  onto the thread stack, so evClock doesn't really use it.
   */
  evClock_thread = kernel_thread( kernel_task, evClock, (char *)interval );
#else
  evClock_thread = cthread_fork((any_t)evClock, (void *)interval);
  cthread_detach(evClock_thread);
#endif XKMACHKERNEL

  xTrace0(event,TR_EVENTS,"evInit exit");
}


/*
 *  e_insque  -- insert an event into an event bucket, ordered by
 *               increasing timeout value
 */

static void
e_insque(q,e)
    int q;
    Event e;
{
  Event a;

  xTrace0(event,TR_FULL_TRACE,"e_insque enter");
  EVENT_LOCK();
  for (a = evhead[q].next; a != &evhead[q]; a = a->next) {
    if (a->deltat < e->deltat) {
      /* E goes after a */
      e->deltat -= a->deltat;
      continue;
    } else {
      /* E goes just before a */
      a->deltat -= e->deltat;
      insque(e, a->prev);
      EVENT_UNLOCK();
      return;
    }
  }
  /* E goes at the end */
  insque(e, evhead[q].prev);
  EVENT_UNLOCK();
}

/*
 *  e_remque  --- remove an event from an event bucket
 */

static void
e_remque(e)
    Event e;
{
    xTrace0(event,TR_FULL_TRACE,"e_remque enter");
    xAssert(e);
    if (e->next->func != THIS_IS_THE_HEADER) {
	e->next->deltat += e->deltat;
    }
    remque(e);
}



#ifdef XK_THREAD_TRACE

/* 
 * It is safe to call unbindEvent multiple times with the same event.
 * Only the first call will have any effect.
 */
#define unbindEvent( _e )					\
do {								\
    xAssert(_e);						\
    if ( (_e)->bind ) {						\
	mapRemoveBinding(threadEventMap, (_e)->bind);		\
	(_e)->bind = 0;						\
    }								\
} while(0)

#define freeEvent( _e )					\
do {							\
    xAssert(_e);					\
    unbindEvent(_e);					\
    mapUnbind(localEventMap, &(_e));			\
    xFree((char *)(_e));				\
} while (0)


#else ! XK_THREAD_TRACE

#define unbindEvent( _e ) do { } while (0)
#define freeEvent(_e)	xFree((char *)(_e))

#endif XK_THREAD_TRACE




void
evMarkBlocked( s )
    Semaphore	*s;
{
#ifdef XK_THREAD_TRACE
    Event	evSelf;
    THREAD_T	thSelf;

    thSelf = THREAD_SELF();
    if ( mapResolve(threadEventMap, &thSelf, &evSelf) == XK_SUCCESS ) {
	evSelf->state = E_BLOCKED;
	xGetTime(&evSelf->stopTime);
    } else {
	xTrace0(event, TR_ERRORS, "evMarkBlocked -- Couldn't find event structure");
    }
#endif
}


void
evMarkRunning()
{
#ifdef XK_THREAD_TRACE
    Event	evSelf;
    THREAD_T	thSelf;

    thSelf = THREAD_SELF();
    if ( mapResolve(threadEventMap, &thSelf, &evSelf) == XK_SUCCESS ) {
	evSelf->state = E_RUNNING;
    } else {
	xTrace0(event, TR_ERRORS, "realP -- Couldn't find event structure");
    }
#endif
}




/*
 *  stub -- all scheduled events start here
 *
 *  CreateProcess will invoke stub, and stub will apply the function
 *  pointer to the event object and the argument
 *
 */
static void
stub(e)
    Event e;
{
  xTrace0(event,TR_FULL_TRACE,"event stub enter");
  if ( e->flags & E_CANCELLED_F ) {
      xTrace1(event, TR_MORE_EVENTS,
	      "event stub not starting cancelled event %x", e);
      freeEvent(e);
      return;
  }
#ifdef XK_THREAD_TRACE
    {
	THREAD_T		self;
	
	self = THREAD_SELF();
	e->bind = mapBind(threadEventMap, &self, e);
	xAssert( e->bind != ERR_BIND );
	xGetTime(&e->startTime);
    }
#endif
  e->state = E_RUNNING;
  xTrace1(event,TR_FULL_TRACE, "starting event at addr %x", e->func);
  e->func(e, e->arg);
  e->state = E_FINISHED;
  EVENT_LOCK();
  if ( e->flags & E_DETACHED_F ) {
      freeEvent(e);
  } else {
      unbindEvent(e);
#ifdef XK_THREAD_TRACE
      xGetTime(&e->stopTime);
#endif
  }
  EVENT_UNLOCK();
}

/*
 *  evSchedule  -- prepare an event for later scheduling
 *
 *
 */

Event evSchedule( func, arg, time ) /* Time in us */
    EvFunc 	func;
    VOID	*arg;
    unsigned 	time; 	/* Time in us */
{
  Event e;
  int delt;
  bool  ret;

  xTrace0(event,TR_FULL_TRACE,"evSchedule enter");
  e = (Event)xMalloc(sizeof(struct Event));
  xAssert(e);
  xTrace1(event, TR_EVENTS, "evSchedule allocates event %x", e);
  e->func = func;
  e->arg = arg;

#ifdef XK_THREAD_TRACE
  {
      XTime	offset;
      Bind	b;
      
      xTrace1(event, TR_MORE_EVENTS, "evSchedule adds event %x to maps", e);
      offset.sec = time / (1000 * 1000);
      offset.usec = time % (unsigned)(1000 * 1000);
      xAssert(localEventMap);
      b = mapBind(localEventMap, &e, e);
      xAssert( b != ERR_BIND );
      e->bind = 0;   /* filled in in 'stub' once a native thread is assigned */
      xGetTime(&e->startTime);
      xTrace2(event, TR_DETAILED, "event start time: %d %d",
	      e->startTime.sec, e->startTime.usec);
      xAddTime(&e->startTime, e->startTime, offset);
  }
#endif

  delt = (time+500*event_granularity)/(event_granularity*1000);
               /* time in us, delt in ticks, event_granularity in ms/tick */
  if (delt == 0 && time != 0) {
      delt = 1;
  }
  e->deltat = delt/BIG_N; /* If BIG_N is a power of 2, this could be a shift*/
  e->flags = 0;
  if (delt == 0) {
    e->state = E_SCHEDULED;
/*    cthread_detach(cthread_fork(stub, e)); */
/* Disallowed, use sledgehammer instead. */
    xTrace0(event,TR_EVENTS,"evSchedule starting event");
    ret = CreateProcess(stub,STD_PRIO,1,e);
  } else {
    /* e_insque will take care of locking */
    e_insque((tickmod+delt)%BIG_N,e); /* Mod could be AND if BIG_N power of 2*/
    e->state = E_PENDING;
  }
  xTrace0(event,TR_EVENTS,"evSchedule exit");
  return e;
}


/*
 * evDetach -- free the storage used by event, or mark it
 *             as detachable after the associate thread finishes
 *
 */

void evDetach(e)
    Event e;
{
  xTrace0(event, TR_FULL_TRACE, "evDetach");
  EVENT_LOCK();
  if ( e->state == E_FINISHED ) {
    xTrace1(event, 5, "evDetach frees event %x", e);
    freeEvent(e);
  } else {
    xTrace1(event, 5, "evDetach marks event %x", e);
    e->flags |= E_DETACHED_F;
  }
  EVENT_UNLOCK();
  xTrace0(event, TR_EVENTS, "evDetach exits");
}


/* 
 * See description in event.h
 */
EvCancelReturn evCancel(e)
    Event e;
{
    int ans;
    
    xTrace1(event, TR_FULL_TRACE, "evCancel: event = %x", e);
    xAssert(e);
    EVENT_LOCK();
    
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
    EVENT_UNLOCK();
    return ans;
}


int
evIsCancelled( e )
    Event	e;
{
    int	res;

    xAssert(e);
    EVENT_LOCK();
    res = e->flags & E_CANCELLED_F;
    EVENT_UNLOCK();
    return res;
}



/*
 * evClock --- update the event queues periodically
 *
 * This routine is not under the master lock.
 * It decrements the tick count for each event bucket; events
 * whose time has expired are scheduled for execution via the
 * master lock.  The events are stored in the bucket in order
 * of increasing time to expiration.  Each event's "deltat" field
 * indicates the time by which it exceeds the previous event's
 * timeout value.
 *
 * the "kernel_thread" routine for the Mach inkernel version did
 * not seem to pass the interval argument to this thread, so
 * it uses the global "event_granualarity" directly.
 *
 */
int tracetickshort = 0;

void evClock(interval)
    int interval;
{
  Event e;
  bool  ret;
#ifndef XKMACHKERNEL
  mach_msg_header_t m;
  cthread_set_name(cthread_self(), "evClock");
  cthread_wire();
  max_cthread_priority();
#endif XKMACHKERNEL

  xTrace0(event,TR_FULL_TRACE,"evClock enter");
  while(TRUE)
    {
#ifdef XKMACHKERNEL
      thread_will_wait_with_timeout( current_thread(), event_granularity );
      thread_block((void (*)()) 0);
      if (1)
#else /* XKMACHKERNEL */
      cthread_yield();
      if (mach_msg(&m, MACH_RCV_MSG|MACH_RCV_TIMEOUT,
		   0, sizeof m, evClock_port,
		   interval, MACH_PORT_NULL) == MACH_RCV_TIMED_OUT)
#endif XKMACHKERNEL
	{
	  if (tracetickshort) { xIfTrace(event,TR_EVENTS) {printf("{tick}");} ;}
	  else { xTrace0(event,TR_EVENTS,"evClock tick"); };
	  EVENT_LOCK();
	  evhead[tickmod].next->deltat--;
	  tickmod = (tickmod+1)%BIG_N; /* optimize if BIG_N is power of 2*/
	  for (e = evhead[tickmod].next; e != &evhead[tickmod] 
	       && e->deltat == 0; e = e->next) {
	    remque(e);
	    e->state = E_SCHEDULED;
#ifdef XK_THREAD_TRACE
	    xGetTime(&e->startTime);
#endif
/*	    cthread_detach(cthread_fork(stub, e)); */
/* Disallowed, use sledgehammer instead. */
	    xTrace0(event,TR_EVENTS,"evClock starting event");
	    ret = CreateProcess(stub,STD_PRIO, 1, e);
	  }
	  EVENT_UNLOCK();
	}
#ifndef XKMACHKERNEL
      else
	{
	  printf("evClock: received a message!!!\n");
	  /* If we get a message. In this version we shouldn't! [mats] */
	}
#endif XKMACHKERNEL      
    }
}

