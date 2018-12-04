/*
 * event_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.9 $
 * $Date: 1993/02/01 23:53:27 $
 */

/* 
 * Event definitions for modules which work very closely with the
 * event subsystem.  
 */

#ifndef event_i
#define event_i

#include "process.h"

/*
 * EVENT_INTERVAL -- number of usec between scans of the event list
 * (event granularity).
 *
 * sparc verions needs this to be longer due to apparent
 *  bug in lwp_sleep (should be ~ 10000.)  cjt
 */
#define EVENT_INTERVAL 500000

#define E_DETACHED_F	(1 << 0)
#define E_CANCELLED_F 	(1 << 1)


#ifdef XK_THREAD_TRACE

#  ifdef __STDC__

/* 
 * evMarkBlocked -- mark the current thread as blocked on the given
 * semaphore.   This is only used for thread monitoring.  The pointer
 * will just be treated as an integer (i.e., it doesn't have to be
 * valid.) 
 */
void	evMarkBlocked( Semaphore * );

/* 
 * evMarkRunning -- mark the current thread as running.
 * This is only used for thread monitoring.  
 */
void	evMarkRunning( void );

#  endif

/* 
 * localEventMap -- contains all x-kernel event structures.  Event
 * pointer is both the key and the bound object
 */
extern Map	localEventMap;


#endif

#ifdef __STDC__

void	evInit( void );

#endif

#endif
