/* 
 * process.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.10 $
 * $Date: 1993/02/01 23:53:22 $
 */

#ifndef process_h
#define process_h

#include "xtype.h"

/* the "normal" priority for process creation */
#define STD_PRIO		5

#include <lwp/lwp.h>
#define LWP_MAXPRIO 32

#ifndef NULL
#define NULL 0
#endif


#if (defined(USE_GC) && !defined(sparc))        /* not for sparc */
#define malloc(X) gc_malloc(X)
#define free(X) gc_free(X)
#endif

#if 0
typedef struct {
#ifdef sun
  int pc, smask, onstack;
  int d2, d3, d4, d5, d6, d7;
  int a2, a3, a4, a5, a6;
  long *sp;
#endif
#ifdef vax
  long *pc, *sp, *fp, *ap;
  int r6, r7, r8, r9, r10, r11;
#endif
} jmp_buf;
#endif

typedef struct _Process {
#if 0
  jmp_buf		jb;
#endif
  long 			*stack, *stacklimit;
  short			prio;
  struct _Process *	link;
  int			extrastuff;
  int			index;
#ifdef MUTS
  void *muts;
#endif
#ifdef sparc
  thread_t              lwp;	/* cjt */
#endif
} Process;
#define STACKSIZE (32*1024)


typedef struct sSemaphore {
#ifdef sparc	/* -- Robbert van Renesse:  a condition variable for sync */
  cv_t				 cv;
#else
  struct _Process		*head;
  struct _Process	       **tail;
#endif
  short				 unused;
  short				 count;
} Semaphore;


#ifdef __STDC__

extern void LWP_Init( void );

extern semInit( Semaphore *, unsigned );
extern realP( Semaphore * );
extern realV( Semaphore * );

extern mon_t	master_monitor;
#define xk_master_lock() 	mon_enter( master_monitor )
#define xk_master_unlock()	mon_exit( master_monitor )

int	CreateProcess0( Pfi, short );
int	CreateProcess1( Pfi, short, int );
int	CreateProcess2( Pfi, short, int, int );
int	CreateProcess3( Pfi, short, int, int, int );
int	CreateProcess4( Pfi, short, int, int, int, int );
int	CreateProcess5( Pfi, short, int, int, int, int, int );
int	CreateProcess6( Pfi, short, int, int, int, int, int, int );

#endif __STDC__

#define semWait(S) { if (--(S)->count < 0) realP(S); }
#define semSignal(S) { if (++(S)->count <= 0) realV(S); }


extern Process *Active;
extern int	SignalsPossible;
#define kSwitch() Yield()

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

#endif
