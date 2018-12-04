/*
 * input_thread.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/02 00:06:12 $
 */

/*
 * Mach3
 *
 *  This file is based on process.c as of Sun Jul  5 15:20:14 MST 1992
 *
 */

#include <cthreads.h>
#include <mach.h>

#include <mach/message.h>
/* xkernel.h includes xk_debug.h */
#include "xkernel.h"
#include "assert.h"
/* platform includes process.h */
#include "platform.h"
#include "eth.h"
#include "list.h"
#include "process_msg.h"
#include "event_i.h"

#define XK_THREAD_STATS	XK_DEBUG

#ifdef XK_THREAD_STATS
static int 		activeThreads;   
static int		activeHighWater;  /* keeps track of max concurrent threads used */
#endif
ProtectedQueue	xkFreeQueue;
ProtectedQueue	xkInQueue;

#define MAX_XK_THREADS 16
#define BUFFER_POOL_SIZE 32


static InputBuffer	xkBuffers[BUFFER_POOL_SIZE];
static int		numThreads;

static void
protectedQueueInit( pq )
    ProtectedQueue	*pq;
{
    pq->lock = mutex_alloc();
    pq->notEmpty = condition_alloc();
    pq->list = (list_t)xMalloc(sizeof(struct list_head));
    list_init(pq->list);
}


/*
 * xkBufferPoolInit()
 *
 *    Initial input handling structures
 *
 */
void
xkBufferPoolInit()
{
    int i;
    
    /*
     * create a pool of buffers for incoming data
     */
    for ( i = 0; i < BUFFER_POOL_SIZE; i++ ) {
	msgConstructAllocate
	  (&xkBuffers[i].upmsg, 
	   ROUND4(MAX_ETH_DATA_SZ + ROUND4(sizeof(struct mach_hdrs))),
	   &xkBuffers[i].data);
	xkBuffers[i].q.next = 0;
	enlist( xkFreeQueue.list, &xkBuffers[i].q);
    }
}


/*
 * Entry point for a pool thread
 *
 * A pool thread is activated by the driver signalling on the input queue's
 * condition variable; it processes packets from the queue until the
 * input queue is empty.
 *
 */
static void
xkInputPool( evSelf, arg )
    Event	evSelf;
    VOID	*arg;
{
    InputBuffer	*buffer;
    Msg         *msg;
    int		threadId;
    
    threadId = numThreads++;
    /* 
     * As an event we were started under the master lock.
     */
#ifdef XK_THREAD_TRACE
    evMarkBlocked(0);
#endif
    xk_master_unlock();
    while( TRUE ) {
	/* 
	 * Pull a buffer off the input queue
	 */
	pqRemove( xkInQueue, buffer );
#ifdef XK_THREAD_STATS
	if ( ++activeThreads > activeHighWater ) {
	    activeHighWater = activeThreads;
	}
#endif
	xTrace1(processcreation, TR_EVENTS,
		"%d shepherd threads active", activeThreads);
	/* 
	 * Shepherd the message up
	 */
	msg = &buffer->upmsg;
	xTrace1(processcreation, TR_EVENTS,
		"input_process: thread %d acquires master lock", threadId);
	xk_master_lock();
#ifdef XK_THREAD_TRACE
	evMarkRunning();
#endif
	xTrace2(processcreation, TR_EVENTS,
		"input_process: thread %d demux with buffer %d",
		threadId, ((int)buffer - (int)xkBuffers)/sizeof(InputBuffer));
	xDemux(buffer->driverProtl, msg);
	/*
	 * refresh the buffer -- this must run within the lock
	 */
	msgConstructAllocate(msg, ROUND4( MAX_ETH_DATA_SZ +
					  ROUND4(sizeof(struct mach_hdrs))),
			     &buffer->data);
#ifdef XK_THREAD_TRACE
	evMarkBlocked(0);
#endif
	xk_master_unlock();
	/* 
	 * Return the input buffer to the free queue
	 */
	pqAppend( xkFreeQueue, buffer );
#ifdef XK_THREAD_STATS
	activeThreads--;
#endif
    }
}



/*
 * xkFillThreadPool -- create initial threads 
 */
void
xkFillThreadPool()
{
    register int i = MAX_XK_THREADS+1;
	
    /*
     * Initialize queues for synchronizing between the driver threads
     * and the shepherd threads.
     */
    protectedQueueInit( &xkInQueue );      
    protectedQueueInit( &xkFreeQueue );      

    for ( i=0; i < MAX_XK_THREADS; i++ ) {
	evDetach(evSchedule(xkInputPool, 0, 0));
    }
    xTrace1(processcreation, TR_GROSS_EVENTS,
	    "initialized %d threads for incoming packets", MAX_XK_THREADS);
}


void
xkThreadDumpStats()
{
    xTrace0(processcreation, TR_ALWAYS, "xkernel input thread statistics:");
    xTrace3(processcreation, TR_ALWAYS,
	    "\tthreads: %d\tactive: %d\thigh-water: %d",
	    numThreads, activeThreads, activeHighWater);
}
