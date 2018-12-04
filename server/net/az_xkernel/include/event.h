/*
 * event.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.18 $
 * $Date: 1993/02/01 22:38:44 $
 */

#ifndef event_h
#define event_h

#include "upi.h"
#include "xtime.h"

/*
 *   Return values for evCancel
 */
typedef enum {
    EVENT_FINISHED = -1,
    EVENT_RUNNING = 0,
    EVENT_CANCELLED = 1,
} EvCancelReturn;

/*
 * This structure is to be opaque.  We have to define it here, but protocol
 * implementors are not allowed to know what it contains.
 */
typedef enum {
    E_PENDING,
    E_SCHEDULED,
    E_RUNNING,
    E_BLOCKED,
    E_FINISHED
} EvState;


#if defined(XK_DEBUG)
#  define XK_THREAD_TRACE
#endif

typedef struct Event {
    struct Event *next, *prev;
    unsigned 	deltat;
    void	(* func)();
    VOID	*arg;
    EvState	state;
    unsigned 	flags;
#ifdef XK_THREAD_TRACE
    XTime	startTime;	/* Time started */
    XTime	stopTime;	/* Time stopped or blocked */
    Bind	bind;
#endif
} *Event;



typedef	void	(* EvFunc)(
#ifdef __STDC__
			   Event,
			   void *
#endif
			   );

/* schedule an event that executes f w/ argument a after delay t usec; */
/* t may equal 0, which implies createprocess */
Event evSchedule(
#ifdef __STDC__
		 EvFunc f, void *a, unsigned t
#endif
		 );


/*
 * releases a handle to an event; as soon f completes, the resources
 * associated with the event are freed
 */
void evDetach(
#ifdef __STDC__
	      Event e
#endif
	      );

/* cancel event e:
 *  returns EVENT_FINISHED if it knows that the event has already executed
 *     to completion
 *  returns EVENT_RUNNING if the event is currently running
 *  returns EVENT_CANCELLED if the event has been successfully cancelled
 */
EvCancelReturn evCancel(
#ifdef __STDC__
	     Event e
#endif
	     );


/* 
 * returns true (non-zero) if an 'evCancel' has been performed on the event
 */
int evIsCancelled(
#ifdef __STDC__		  
		  Event e
#endif
		  );


/* 
 * Displays a 'ps'-style listing of x-kernel threads
 */
void evDump(
#ifdef __STDC__		  
		  void
#endif
		  );


#endif
