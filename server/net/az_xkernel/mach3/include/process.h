/* 
 * process.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.18 $
 * $Date: 1993/02/02 00:02:50 $
 */

#ifndef process_h
#define process_h

#ifndef XKMACHKERNEL
#include <mach.h>
#include <cthreads.h>
#else
#include <kern/thread.h>
#endif XKMACHKERNEL
#include <mach/message.h>

#include "xtype.h"

/* define standard priority; higher numbers have less priority */
#define STD_PRIO	5
#define THREAD_MAXPRIO 31

#ifndef NULL
#define NULL 0
#endif

#if (defined(USE_GC))
#define malloc(X) gc_malloc(X)
#define free(X) gc_free(X)
#endif

#define CreateProcess0 CreateProcess
#define CreateProcess1 CreateProcess
#define CreateProcess2 CreateProcess
#define CreateProcess3 CreateProcess
#define CreateProcess4 CreateProcess
#define CreateProcess5 CreateProcess
#define CreateProcess6 CreateProcess
#define xCreateProcess CreateProcess


#define XKPROCESSDEF

#ifdef XKPROCESSDEF
typedef struct {
#ifdef mips
  int x; /* not a $%#%@ idea, hope it isn't used... */
#endif
} jmp_buff; /* Had to rename because of Mach C compiler [mats] */

typedef struct _Process {
  jmp_buff		jb;
  long 			*stack, *stacklimit;
  short			prio;
  struct _Process *	link;
  int			index;
#ifdef MUTS
  void *muts;
#endif
#ifdef XKMACHKERNEL
  thread_t              thread;	/* mats */
#else
  cthread_t             thread;	/* mats */
#endif XKMACHKERNEL
} Process;
#endif XKPROCESSDEF

#define STACKSIZE (32*1024)

#ifdef XKMACHKERNEL
typedef struct sQueue {
	struct sQueue 	*head, **tail, *link;
	thread_t	thread;
} WaitingQueue;

typedef struct sSemaphore {
	int			count;
	WaitingQueue		waitQueue;
} Semaphore;

#define Q_INIT(Q) { \
  (Q)->head = NULL; \
  (Q)->tail = &(Q)->head; \
}

#define Q_EMPTY(Q) ((Q)->tail == &(Q)->head)

#define Q_INSERTLAST(Q, E) { \
  *((Q)->tail) = (E); \
  (E)->link = NULL; \
  (Q)->tail = &(E)->link; \
}

#define Q_INSERTFIRST(Q, E) { \
  (E)->link = (Q)->head; \
  (Q)->head = (E); \
  if (Q_EMPTY(Q)) (Q)->tail = &(E)->link; \
}

#define Q_REMOVEFIRST(Q, E) { \
  if ((E) = (Q)->head) { \
    if (! ((Q)->head = (E)->link)) (Q)->tail = &(Q)->head; \
  } \
}

#else

typedef struct sSemaphore {
	struct mutex		lock;
	struct condition	cond;
	int			count;
	int			sleepers;
} Semaphore;
#endif XKMACHKERNEL

#define semWait(S) { if (--(S)->count < 0) realP(S); }
#define semSignal(S) { if (++(S)->count <= 0) realV(S); }

#ifdef __STDC__
extern void semInit( Semaphore *, unsigned int );
extern void realP( Semaphore * );
extern void realV( Semaphore * );
#else
extern void semInit( );
extern void realP( );
extern void realV( );
#endif

extern Process *Active;
extern int	SignalsPossible;
#define kSwitch() Yield()

/* The -DXKLOCKDEBUG switch provides for counting the locking depth, and
   complaining if it isn't 1.  It has a bug, in that the increment & decrement
   instructions aren't atomic in RISC architectures.  Presumably, the occasions
   where this fails are rare.  */

#ifdef XKLOCKDEBUG
extern int xklockdepthreq;
extern int xklockdepth;
extern int tracexklock;
#endif

/* the master concurrency locks */

#ifndef XKLOCKDEBUG

#ifdef XKMACHKERNEL
#define	MASTER_LOCK	simple_lock( &xkMaster_lock )
#define	MASTER_UNLOCK	simple_unlock( &xkMaster_lock )
#else  /* ! XKMACHKERNEL */
#define MASTER_LOCK	mutex_lock(sledgehammer_concurrency_control)
#define MASTER_UNLOCK	mutex_unlock(sledgehammer_concurrency_control)
#endif XKMACHKERNEL

#else /* XKLOCKDEBUG */

#ifdef XKMACHKERNEL
#define	MASTER_LOCK							      \
{xklockdepthreq++;							      \
 xTrace1(xklock,TR_EVENTS,"requesting xklock, depthreq %d",xklockdepthreq);   \
 simple_lock( &xkMaster_lock );						      \
 xklockdepth++;								      \
 if (xklockdepth!=1)							      \
 {xTrace1(xklock,TR_ERRORS,"got xklock, wrong depth %d",xklockdepth); };      \
 xTrace2(xklock,TR_EVENTS,"got xklock, depth %d, depthreq %d",		      \
	 xklockdepth,xklockdepthreq); }
#define	MASTER_UNLOCK							      \
{if (xklockdepth!=1)							      \
 {xTrace1(xklock,TR_ERRORS,"giving up xklock, wrong depth %d",xklockdepth); };\
 xTrace2(xklock,TR_EVENTS,"giving up xklock, depth %d, depthreq %d",	      \
	 xklockdepth,xklockdepthreq);					      \
 xklockdepth--;								      \
 simple_unlock( &xkMaster_lock );					      \
 xTrace1(xklock,TR_EVENTS,"gave up xklock, depthreq %d",xklockdepthreq);      \
 xklockdepthreq--; }
#else  /* ! XKMACHKERNEL */
#define	MASTER_LOCK							      \
{xklockdepthreq++;							      \
 xTrace1(xklock,TR_EVENTS,"requesting xklock, depthreq %d",xklockdepthreq);   \
 mutex_lock(sledgehammer_concurrency_control);				      \
 xklockdepth++;								      \
 if (xklockdepth!=1)							      \
 { xTrace1(xklock,TR_ERRORS,"got xklock, wrong depth %d",xklockdepth); };     \
 xTrace2(xklock,TR_EVENTS,"got xklock, depth %d, depthreq %d",		      \
	 xklockdepth,xklockdepthreq); }
#define	MASTER_UNLOCK							      \
{if (xklockdepth!=1)							      \
 {xTrace1(xklock,TR_ERRORS,"giving up xklock, wrong depth %d",xklockdepth); };\
 xTrace2(xklock,TR_EVENTS,"giving up xklock, depth %d, depthreq %d",	      \
	 xklockdepth,xklockdepthreq);					      \
 xklockdepth--;								      \
 mutex_unlock(sledgehammer_concurrency_control);			      \
 xTrace1(xklock,TR_EVENTS,"gave up xklock, depthreq %d",xklockdepthreq);      \
 xklockdepthreq--; }
#endif XKMACHKERNEL

#endif XKLOCKDEBUG


#ifdef __STDC__

void	CreateKernelProcess( Pfi, int, int, int, int );
bool	CreateProcess();
void	Yield( void );
void	threadInit( void );
void	inputThreadInit( void );
void	xk_master_lock( void );
void	xk_master_unlock( void );
void	xkThreadDumpStats( void );

#  ifndef XKMACHKERNEL
void	VAll( Semaphore * );
void	max_cthread_priority( void );
#  endif

#endif


#endif
