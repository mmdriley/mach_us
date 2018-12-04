/*
 * process.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.40 $
 * $Date: 1993/02/02 00:05:18 $
 */

/*
 * Mach3
 *
 *  This file is based on process.c from the non-kernel version as
 *  of Thu Jan 23 18:08:19 1992, AND the in-kernel version from
 *  August 1991
 *
 */


#ifdef XKMACHKERNEL
#include <kern/lock.h>
#include <kern/kalloc.h>
#include <kern/syscall_subr.h>
#include <sys/varargs.h>
#else /*  XKMACHKERNEL */
#include <varargs.h>
#include <cthreads.h>
#include <sys/time.h>
#include <mach.h>
#endif XKMACHKERNEL

/* xkernel.h includes xk_debug.h */
#include "xkernel.h"
#include "assert.h"
/* platform includes process.h */
#include "platform.h"
#include "event_i.h"
#ifdef XKMACHKERNEL
#include "kern_process_msg.h"
#else
#include "process_msg.h"
#endif
#include "xk_mach.h"

/* externs */
void bufferPoolInit();
void xkFillThreadPool();

/* forwards */

static void enter_master_sledgehammer_monitor(

#if defined(__STDC__) && ! defined(XKMACHKERNEL)
					      int *
#endif
					      );


#define MAXARGS		6

#ifdef XKMACHKERNEL
#define MAX_XK_THREADS 16
#define XK_MAX_RUNNING_THREADS 16  /* try to keep running threads less than this */
#else
#define MAX_KERNEL_THREADS_FOR_CTHREADS 5
int xk_thread_limit = MAX_KERNEL_THREADS_FOR_CTHREADS;
#define MAX_XK_THREADS 32
#define XK_MAX_RUNNING_THREADS 24  /* try to keep running threads less than this */
#endif XKMACHKERNEL

int total_processes = 0;	/* unused ? */
static int current_xk_threads = 0;
int max_xk_threads_inuse = 0;

#ifdef XKMACHKERNEL
/*
 * The InfoQ is used by the xkernel master monitor
 * The Master Lock allows only one thread at a time to run
 */
queue_head_t			xkInfoQ;
decl_simple_lock_data(,	xkInfo_lock)
decl_simple_lock_data(,	xkThread_lock)
decl_simple_lock_data(,	xkMaster_lock)

#define XTHREAD_SELF() current_thread()
#define XTHREAD_TYPE	thread_t

#else /* XKMACHKERNEL */

mutex_t sledgehammer_concurrency_control;
#define XTHREAD_SELF()	cthread_self()
#define XTHREAD_TYPE	cthread_t
Process *Active;

#endif XKMACHKERNEL

#ifdef XKLOCKDEBUG
int xklockdepthreq = 0;
int xklockdepth = 0;
int tracexklock = 0;
#endif XKLOCKDEBUG

/* the master concurrency locks */
/* there's another copy in process.h -- rcs */
/* #ifdef XKMACHKERNEL
 * #define	MASTER_LOCK   simple_lock( &xkMaster_lock )
 * #define	MASTER_UNLOCK simple_unlock( &xkMaster_lock )
 * #else
 * #define MASTER_LOCK   mutex_lock(sledgehammer_concurrency_control)
 * #define MASTER_UNLOCK mutex_unlock(sledgehammer_concurrency_control)
 * #endif XKMACHKERNEL
 */

typedef struct {
#ifdef XKMACHKERNEL
	queue_head_t	q;	/* used by queue routines */
#endif XKMACHKERNEL
	int	*args;	/* pointer to argument block */
} argBlock;

/*
 * xk_master_lock, xk_master_unlock
 *
 * user callable master concurrency control
 *
 */
void
xk_master_lock()
{
  MASTER_LOCK;
}

void
xk_master_unlock()
{
  MASTER_UNLOCK;
}

#define XK_TASK_PRIORITY	25


/*
 *
 * threadInit
 *
 */
void threadInit()
{
  kern_return_t	kr;

  xTrace0(processcreation, TR_FULL_TRACE, "Enter event function threadInit");

#ifndef XKMACHKERNEL
  cthread_init();
  sledgehammer_concurrency_control = mutex_alloc();

  kr = task_priority(mach_task_self(), XK_TASK_PRIORITY, FALSE);
  if ( kr != XK_SUCCESS ) {
      sprintf(errBuf, "threadInit could not set task priority: %s",
	      mach_error_string(kr));
      xError(errBuf);
  }

  /* 
   * Since we don't have any real concurrency under the xkernel master
   * lock, we can run with just a single mach thread.
   */
  cthread_set_kernel_limit(xk_thread_limit);


#else /* XKMACHKERNEL */

  simple_lock_init( &xkMaster_lock );
  queue_init( &xkInfoQ );
  simple_lock_init( &xkInfo_lock );
#endif ! XKMACHKERNEL
}


/* 
 * Events must be initialized before inputThreadInit can be called
 */
void
inputThreadInit()
{
    /* defined in input_process.c */
    xkFillThreadPool();
    xkBufferPoolInit();
}


/*
 *  xkernel_activate_thread
 *
 *     startup a thread, given the arguments
 *
 */
static void xkernel_activate_thread(args, priority)
     int *args;
     int priority;
{
#ifdef XKMACHKERNEL
  argBlock *info = (argBlock *)xMalloc(sizeof(argBlock));
#endif XKMACHKERNEL

  XTHREAD_TYPE child;

  xTrace0(processcreation, TR_FULL_TRACE, "xkernel_activate_thread enter");
#ifdef XKMACHKERNEL
  /* 
    in the kernel we can't pass args to a thread, so we have to
    enqueue the arg block.  The thread will dequeue the args when
    it starts up in the master monitor
   */
  info->args = args;
  simple_lock( &xkInfo_lock );
  enqueue_tail( &xkInfoQ, &info->q );
  simple_unlock( &xkInfo_lock );
  xTrace1(processcreation, TR_EVENTS, "processcreate: create arg block at %x", args);

  child = kernel_thread( kernel_task, enter_master_sledgehammer_monitor );

#else /* XKMACHKERNEL */
  /* here we can start the thread with a pointer to arguments directly */
  if (current_xk_threads+4 >= xk_thread_limit) {
    xk_thread_limit += 2;
    cthread_set_kernel_limit(xk_thread_limit);
    xTrace1(processcreation, TR_ERRORS, "Cthread kernel limit now %d", xk_thread_limit);
  }
  child = cthread_fork((any_t)enter_master_sledgehammer_monitor, (void *)args);
  cthread_set_name(child,"xkernel thread");

  cthread_detach(child);
#endif XKMACHKERNEL
  xTrace1(processcreation, TR_MAJOR_EVENTS, "Created thread_id: %d", child);
  xTrace1(processswitch, TR_GROSS_EVENTS, "Created thread_id: %d", child);
}


/*
 *
 * CreateProcess(function_ptr, priority, argcount, arg1, arg2, ...)
 *
 */
bool
CreateProcess(va_alist)
     va_dcl
{
  va_list ap;
  Pfi r;
  short prio;
  int nargs, i;
  int *arg_vec;
  int *argp;

  xTrace0(processcreation, TR_EVENTS, "Enter CreateProcess");

  va_start(ap);
  r = va_arg(ap, Pfi);
  
/* xkernel and Mach threads have the same priorities */
  prio = va_arg(ap, short);
  nargs = va_arg(ap, int);

  arg_vec = (int *)xMalloc((MAXARGS+2)*sizeof(int));
  argp = arg_vec;
  *argp++ = (int)r;
  *argp++ = nargs;
  for (i=0; i<nargs; i++)   *argp++ = va_arg(ap, int);

  xTrace2(processcreation, TR_MAJOR_EVENTS, "Mach CreateProcess: func_addr %x nargs %d",
	 r, nargs);
  xkernel_activate_thread(arg_vec, prio);
  return(TRUE);
}

/*
 *  CreateKernelProcess
 *
 *
 */
void
CreateKernelProcess(r, prio, arg1, arg2, arg3)
    Pfi	r;
    int	prio, arg1, arg2, arg3;
{
    CreateProcess(r, prio, 3, arg1, arg2, arg3);
}


/*
 *
 * Yield
 *
 */
void
Yield()
{
#ifdef XKMACHKERNEL

  xTrace0(processswitch, TR_FULL_TRACE, "Enter event function Yield");
  MASTER_UNLOCK;
  swtch();
  MASTER_LOCK;

#else /* XKMACHKERNEL */
  Process *active = Active;

  xTrace0(processswitch, TR_FULL_TRACE, "Enter event function Yield");
  MASTER_UNLOCK;
  cthread_yield();
  MASTER_LOCK;
  Active = active;
#endif XKMACHKERNEL
}

/*
 *
 * enter_master_sledgehammer_monitor
 *
 * Concurrency control is enforced in the xkernel by means of a monitor,
 *  in which all execution takes place. Here it is!
 * 
 * A new thread begins here.
 * Inside the kernel:
 *	Dequeue a newThreadInfo block and do what it says
 * Outside the kernel:
 *	We are passed the argBlock.
 *
 * All the process block stuff looks totally defunct
 *
 */
static void
enter_master_sledgehammer_monitor
#ifdef XKMACHKERNEL
  ()
#else
  (args)
  int *args;
#endif XKMACHKERNEL
{
  Pfi user_fn;
  int nargs;
  int threadcount;
#ifdef XKMACHKERNEL
  argBlock	*info;
  int		*args;
#else
  Process	*pd;
  int *args_ptr = args;
#endif XKMACHKERNEL

  xTrace0(processcreation, TR_MAJOR_EVENTS, "enter_master_monitor: entering\n");
  MASTER_LOCK;

  xTrace0(processcreation, TR_MAJOR_EVENTS, "enter_master_monitor: locked\n");


#ifdef XKMACHKERNEL
  simple_lock( &xkInfo_lock );
  info = (argBlock *)dequeue_head(&xkInfoQ);
  simple_unlock( &xkInfo_lock );
  if (info) {
    args = info->args;
  }
  else {
    xTrace0(processcreation, TR_EVENTS, "processcreate: no args");
  }
#else
  pd = (Process *)xMalloc(sizeof(Process));
  pd->link = NULL;  pd->thread = XTHREAD_SELF();
#endif XKMACHKERNEL

  user_fn = (Pfi)*(args++);
  nargs = *(args++);
  threadcount = current_xk_threads++;
  if (threadcount > max_xk_threads_inuse) {
    max_xk_threads_inuse = threadcount;
    xTrace1(processcreation, TR_EVENTS, "processcreate: new thread high water mark %d", threadcount);
  }

  if (nargs > MAXARGS) 
    Kabort("master_sledgehammer_monitor: cannot handle more than 6 args.");
  {
    int argv[MAXARGS];
    int i;
    for (i=0;i<nargs;i++)
      argv[i] = *(args++);
#ifndef XKMACHKERNEL
    Active = pd;
    xFree((char *)args_ptr);
#else
    xFree((char *)(info->args));
    xFree((char *)info);
#endif ! XKMACHKERNEL
    xTrace1(processcreation,TR_EVENTS,"enter_master_monitor: starting thread %d",
	    XTHREAD_SELF());
    switch(nargs) {
    case 0: (user_fn)();
            break;
    case 1: (user_fn)(argv[0]);
            break;
    case 2: (user_fn)(argv[0],argv[1]);
            break;
    case 3: (user_fn)(argv[0],argv[1],argv[2]);
            break;
    case 4: (user_fn)(argv[0],argv[1],argv[2],argv[3]);
            break;
    case 5: (user_fn)(argv[0],argv[1],argv[2],argv[3],argv[4]);
            break;
    case 6: (user_fn)(argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
            break;
    }
#ifndef XKMACHKERNEL
    Active = NULL;
#endif ! XKMACHKERNEL
  }

#ifndef XKMACHKERNEL
  xFree((char *)pd);
#endif XKMACHKERNEL

  current_xk_threads--;
  MASTER_UNLOCK;
  xTrace1(processcreation, TR_MAJOR_EVENTS, "master_monitor: thread %d unlocks",
	   XTHREAD_SELF() );

#ifdef XKMACHKERNEL
  thread_terminate( current_thread() );
  thread_halt_self();
  panic( "The zombie enter_master_sledgehammer_monitor strolls" );
#endif XKMACHKERNEL
}


int delay_state;

#ifndef XKMACHKERNEL
/* where is this used ? */
struct timeval delay_time;
#endif ! XKMACHKERNEL

/*
 *
 * wake_sem
 *
 */
static void
wake_sem(ev,sp)
    Event	ev;
    Semaphore	*sp;
{
    xTrace0(processswitch, TR_FULL_TRACE, "Enter event function semSignal");
    semSignal(sp);
}

/*
 *
 * Delay
 *
 */
void
Delay(n) /* n in ms */
     int n;
{
  Semaphore s;

  xTrace0(processswitch, TR_FULL_TRACE, "Enter event function Delay");
  semInit(&s, 0);
  evDetach( evSchedule(wake_sem, (VOID *)&s , n*1000) ); /* 3rd arg is in us */
  semWait(&s);
}


/*
 *
 * semInit
 *
 */
void
semInit(s, n)
	Semaphore		*s;
	unsigned int		n;
{
        xTrace0(processswitch, TR_FULL_TRACE, "Enter event function semInit");
#ifdef XKMACHKERNEL
	Q_INIT( &s->waitQueue );
#else
	mutex_init(&s->lock);
	condition_init(&s->cond);
	s->sleepers = 0;
#endif XKMACHKERNEL
	s->count = n;
}

/*
 *
 * realP
 *
 */
void
realP(s)
	Semaphore		*s;
{
#ifdef XKMACHKERNEL
	WaitingQueue	*w;
#endif

	xTrace2(processswitch, TR_MAJOR_EVENTS, "P on %#x by 0x%x", s, XTHREAD_SELF());
	if (s->count < 0) {
		xTrace2(processswitch, TR_GROSS_EVENTS, "Blocking P on %#x by 0x%x",
							s, XTHREAD_SELF());
#ifdef XK_THREAD_TRACE
		evMarkBlocked(s);
#endif
#ifndef	XKMACHKERNEL
		MASTER_UNLOCK;   /* let the realV signaller get a chance */
		mutex_lock(&s->lock);	/* lock the semaphore while modifying*/
		s->sleepers--;   /* put ourself into the pool */
		while (s->count <= s->sleepers)  /* check for someone
						    leaving the pool */
		  {
		    /* wait for wake-up signal AND lock acquistion,
		       else release lock */
		    condition_wait(&s->cond,&s->lock);
		    }
		s->sleepers++;  /* remove ourself */
		mutex_unlock(&s->lock);  /* let another into condition wait */
		/* changed suggested by Travostino, OSF */
		MASTER_LOCK; /* get back into the xkernel swing of things */
#else

		/*
		 * I own the master lock so no other xk process
		 * can change these structures while I am (running?)
		 *
		 * Add my thread_t to the list of waiters
		 * and note how many are waiting
		 */

		w = (WaitingQueue *)xMalloc( sizeof(WaitingQueue));
		assert( w != NULL );
		w->thread = current_thread();
		Q_INSERTLAST( &s->waitQueue, w );

		/* Release the master lock so that others can run */
		MASTER_UNLOCK;
		/*
		 * Suspend my thread waiting for realV to wake me up
		 * NOTE: suspend/resume uses a counter, there is
		 *  no deadlock window here.
		 */
		xTrace2(processswitch, TR_GROSS_EVENTS,
			"Suspending P on %#x by 0x%x",
			s, current_thread());

		thread_suspend( w->thread );
		thread_block((Pfv) 0);
		xTrace2(processswitch, TR_GROSS_EVENTS,
			"Resuming P on %#x by 0x%x", s, current_thread());
		/*
		 * Resumed; get the master lock back so that I can do work
		 */
		MASTER_LOCK;
#endif ! XKMACHKERNEL
#ifdef XK_THREAD_TRACE
		evMarkRunning();
#endif
		xTrace2(processswitch, TR_MAJOR_EVENTS,
			"Waking-up from P on %#x by 0x%x", s, XTHREAD_SELF());
	      }
      }

/*
 *
 * realV
 *
 */
void
realV(s)
	Semaphore		*s;
{
#ifdef XKMACHKERNEL
	WaitingQueue	*w;
#endif

	xTrace2(processswitch, TR_MAJOR_EVENTS, "V on %#x by 0x%x", s, XTHREAD_SELF());
#ifndef XKMACHKERNEL
	mutex_lock(&s->lock);  /* get the lock so we can check the
				  semaphore state and then signal the waiters */
	if (s->count <= 0) {
		xTrace2(processswitch, TR_GROSS_EVENTS, "Unblocking V on %#x by 0x%x", 
							s, XTHREAD_SELF());
		condition_signal(&s->cond);  /* let a waiter go;
						he tries for the lock now */
	}
	mutex_unlock(&s->lock);  /* let sleepers have a chance */
#else

	/*
	 * I own the master lock so no other xk process
	 * can change these structures while I am
	 */

	/*
	 * Are there any threads waiting on this semaphore?
	 */
	if( s->count <= 0 ) {

		/*
		 * Pull the first waiter from the queue
		 */
		
		Q_REMOVEFIRST( &s->waitQueue, w );

		/*
		 * Resume that thread
		 * It will go right to sleep waiting on the master lock
		 */

		xTrace2(processswitch, TR_GROSS_EVENTS, "Resuming via V: %#x thread: 0x%x",
				  s, w->thread );

		thread_resume( w->thread );
		xFree( w );
	}
#endif ! XKMACHKERNEL
}

#ifndef XKMACHKERNEL
/*
 *
 * VAll
 *
 *  This operation is not supported; it cannot be easily implemented under
 *  the Mach cthreads package.  However, a simpler form that uses
 *  an alternative user call in place of semWait could be implemented.
 *  If that call is "waitP", then this routine will do "wake all threads
 *  in waitP".
 *
 */
void
VAll(s)
	Semaphore		*s;
{
	xTrace2(processswitch, TR_MAJOR_EVENTS, "VAll on %#x by 0x%x", s, XTHREAD_SELF());
	s->count = 0;
	MASTER_UNLOCK;
	mutex_lock(&s->lock);
	mutex_unlock(&s->lock);
	MASTER_LOCK;
	condition_broadcast(&s->cond);
}


/* 
 * max_cthread_priority:
 *
 * Sets the priority of the current cthread to its maximal value.
 * NOTE: The cthread should be wired down before this call.
 */
void
max_cthread_priority()
{
    kern_return_t	kr;
    int			count = THREAD_SCHED_INFO_COUNT;
    int			infoBuf[THREAD_SCHED_INFO_COUNT];
    struct thread_sched_info	*info = (struct thread_sched_info *)infoBuf;
    
    kr = thread_info(mach_thread_self(), THREAD_SCHED_INFO, infoBuf, &count);
    if ( kr != KERN_SUCCESS ) {
	sprintf(errBuf, "xkernel cthread priority error: could not get thread info (%s)",
		mach_error_string(kr));
	xError(errBuf);
    } else {
	xTrace2(processcreation, TR_MAJOR_EVENTS,
		"xkernel thread priorities:  cur %d, max %d",
		info->base_priority, info->max_priority);
    }
    
    kr = thread_priority(mach_thread_self(), info->max_priority, TRUE);
    if ( kr != KERN_SUCCESS ) {
	sprintf(errBuf, "xkernel cthread priority error: could not set priority (%s)",
		mach_error_string(kr));
	xError(errBuf);
    } else {
	xTrace1(processcreation, TR_EVENTS,
		"cthread priority set to %d", info->max_priority);
    }
}

#endif ! XKMACHKERNEL
