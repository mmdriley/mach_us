/*
 * $RCSfile: input_process.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/02 00:07:32 $
 */

/*
 * Mach3
 *
 *  This file is based on process.c as of Sun Jul  5 15:20:14 MST 1992
 *
 */

#include <kern/syscall_subr.h>
#include <sys/varargs.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>

#include <mach/message.h>
#include <device/net_status.h>
/* xkernel.h includes xk_debug.h */
#include "xkernel.h"
#include "assert.h"
/* platform includes process.h */
#include "platform.h"
#include "eth.h"
#include "list.h"
#include "kern_process_msg.h"
#include "eth_i.h"

int 	max_thread_pool_used;	/* keeps track of max concurrent threads used */
int	high_water_thread_pool_used;  /* really keeps max */

#define MAX_XK_THREADS 8
#define BUFFER_POOL_SIZE 32

#ifdef TIME_XKINPUTPOOL
extern XTime push_begin, push_end, push_deltat; extern long push_end_ct;
#endif

/* 
 * These queues are shared between the shepherd threads and the
 * interrupt handler.
 *
 * 'idleThreads' contains 'struct threadIBlock's for shepherd threads
 * which the driver can assign to input buffers.  
 *
 * 'freeBufs' contains 'InputBuffer's which the driver can grab to
 * place incoming packets.  
 *
 * 'xkIncomingData' contains 'InputBuffers' which were not immediately
 * assigned to threads.  The driver appends to this queue and shepherd
 * threads remove from it.  Its lock is set when a shepherd is going
 * to remove the last item in the queue and prevents the driver from
 * writing to the queue.
 */
struct list_head	xkThreadQ;
struct list_head	xkBufferPool;
struct list_head	xkIncomingData;
int			xkIncomingData_lock;

static InputBuffer	xkBuffers[BUFFER_POOL_SIZE];
static int		threadCount = 0;


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
    
    xTrace0(init, TR_MORE_EVENTS, "xkBufferPoolInit");
    list_init(&xkBufferPool);
    list_init(&xkIncomingData);
    xkIncomingData_lock = 0;
    /*
     * create a pool of buffers for incoming data
     */
    for ( i = 0; i < BUFFER_POOL_SIZE; i++ ) {
	msgConstructAllocate (&xkBuffers[i].upmsg, ROUND4(MAX_ETH_DATA_SZ),
			      &xkBuffers[i].data);
	xkBuffers[i].driverProtl = 0;
	enlist_tail(&xkBufferPool, (list_entry_t)&xkBuffers[i]);
    }
    xTrace1(init, TR_MORE_EVENTS, "xkBufferPoolInit initialized %d bufs", i);
}


/*
 * Entry point for a pool thread
 *
 *  A pool thread is activated by the driver; it processes the packet
 *   assigned by the driver and then dequeues from the incoming
 *   packet queue until it is empty.  The packet queue is for handling
 *   traffic bursts.
 *
 */
static void
xkInputPool( evSelf, arg )
    Event	evSelf;
    VOID	*arg;
{
    struct threadIBlock	ti;
    struct input_buffer *buffer;
    Msg                 *msg  = 0;

    ti.thread = current_thread();
    ti.buf = 0;
#ifdef XK_DEBUG
    ti.xkThreadId = threadCount++;
#endif    
    xTrace1(processcreation, TR_EVENTS, "xkernel shepherd thread %d starts",
	    ti.xkThreadId);
    /* 
     * We were started as an event, under the master lock
     */
    while (1) {
	if ( ti.buf == 0 ) {
	    do {
		/*
		 * Place ourselves on the idle thread queue and wait for
		 * the driver to kick us in the future. 
		 *
		 * The assert_wait indicates our intention to block.  If the
		 * driver wakes us up (with a clear_wait) after the
		 * assert_wait but before the thread_block, the block will
		 * have no effect.
		 */
		xTrace1(processcreation, TR_EVENTS,
			"xkernel shepherd thread %d preparing to sleep",
			ti.xkThreadId);
		assert_wait(0, FALSE);		
		xAssert(ti.thread);
		enlist_tail( &xkThreadQ, (list_entry_t)&ti );
#ifdef XK_THREAD_TRACE
		evMarkBlocked(0);
#endif
		xk_master_unlock();
		thread_block((void (*)()) 0);	/* block ourselves */
		xTrace1(processcreation, TR_EVENTS, "xkernel shepherd thread %d awakes",
			ti.xkThreadId);
	    } while ( ti.buf == 0 );
	} else {
	    xTrace1(processcreation, TR_EVENTS,
		    "xkernel shepherd thread %d picks up buffer from incomingQueue",
		    ti.xkThreadId);
	    xk_master_unlock();
	}
	buffer = ti.buf;
	xTrace2(processcreation, TR_MORE_EVENTS,
		"input_process: thread %d demux with buffer %d",
		ti.xkThreadId, ((int)buffer - (int)xkBuffers)/sizeof(InputBuffer));
	msg = &ti.buf->upmsg;
	max_thread_pool_used++;
	if (high_water_thread_pool_used < max_thread_pool_used)
	  { high_water_thread_pool_used = max_thread_pool_used;
	    xTrace1(processcreation, TR_EVENTS,
		    "new high_water_thread_pool_used: %d",
		    high_water_thread_pool_used); };
	xTrace4(processcreation, TR_DETAILED,
	  "input_process: &ti %x  buf %x  buffer %x  msg %x",
		&ti, ti.buf, buffer, msg);
	xTrace4(processcreation, TR_DETAILED,
		"input_process: used %d  high %d  xkBfs %x  szIB %x",
		max_thread_pool_used, high_water_thread_pool_used,
		xkBuffers, sizeof(InputBuffer));
	xTrace3(processcreation, TR_DETAILED,
		"input_process: intbuffer %x  intxkBfs %x  - %d",
		(int)buffer, (int)xkBuffers, ((int)buffer - (int)xkBuffers));

	xk_master_lock();
#ifdef XK_THREAD_TRACE
	evMarkRunning();
#endif
	xTrace1(processcreation, TR_DETAILED,
		"xkernel shepherd thread %d acquires master lock",
		ti.xkThreadId);
	xAssert(xIsProtocol(ti.buf->driverProtl));
	xTrace2(processcreation, TR_DETAILED,
		"input_process: demuxing msg %x to protocol %x",
		msg, ti.buf->driverProtl);
#ifdef TIME_XKINPUTPOOL
	wait_for_tick(); xGetTime(&push_begin);
#endif
	xDemux(ti.buf->driverProtl, msg);
#ifdef TIME_XKINPUTPOOL
	push_end_ct = wait_for_tick(); xGetTime(&push_end);
#endif
	xAssert(ti.thread);
	max_thread_pool_used--;
	/*
	 * refresh the buffer
	 */
	msgConstructAllocate(msg, ROUND4(MAX_ETH_DATA_SZ), &buffer->data);
	enlist_tail( &xkBufferPool, (list_entry_t)ti.buf );
	/*
	 * only one thread at a time can shorten the list; therefore we
	 * only need to lock if the list in the critical
	 * shortness area (i.e., if we will remove the only element)
	 */
	if ( xkIncomingData.head ) {
	    if ( xkIncomingData.head == xkIncomingData.tail ) {
		/* 
		 * This lock prevents the driver from adding anything
		 * to the queue 
		 */
		xkIncomingData_lock = 1;
		ti.buf = (InputBuffer *)delist_head_strong(&xkIncomingData);
		xkIncomingData_lock = 0;
	    } else {
		ti.buf = (InputBuffer *)delist_head(&xkIncomingData);
	    }
	} else {
	    ti.buf = 0;
	}
    }
}



/*
 *
 * xkFillThreadPool -- create initial threads and hang them in xkThreadQ
 *
 * this limits the number of threads that can be simultaneously
 * used for processing incoming packets.  The threads put their
 * argument structure on the xkThreadQ.
 *
 */
void
xkFillThreadPool()
{
    register int i = MAX_XK_THREADS + 1;
    
    xTrace0(init, TR_FUNCTIONAL_TRACE, "xkFillThreadPool");
    list_init(&xkThreadQ);
    while( i-- > 0 ) {
	evDetach( evSchedule(xkInputPool, 0, 0) ); 
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
	    MAX_XK_THREADS, max_thread_pool_used, high_water_thread_pool_used);
}
