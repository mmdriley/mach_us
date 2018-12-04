/*
 * process.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.22 $
 * $Date: 1993/02/01 23:53:44 $
 */

#include <lwp/lwp.h>
#include <lwp/stackdep.h>
#include <lwp/lwperror.h>

#include <varargs.h>

#include "platform.h"
#include "event.h"
#include "event_i.h"
#include "xk_debug.h"
#include "process.h"


int SignalsPossible = 0;


#ifdef __STDC__

static void	wake_sem( Event, VOID * );

#else

static void	wake_sem();

#endif



#define MAX_XK_THREADS 45
int xk_current_threads = 0;


int process_enter_monitor0();
int process_enter_monitor1();
int process_enter_monitor2();
int process_enter_monitor3();
int process_enter_monitor4();
int process_enter_monitor5();
int process_enter_monitor6();

Process *Active;


#define STACKCACHES 100

#define MAXARGS 6

mon_t master_monitor;
int total_processes = 0;

void LWP_Init()
{
  thread_t tid;

  (void) pod_setmaxpri(LWP_MAXPRIO);
  lwp_setstkcache(STACKSIZE, STACKCACHES);

  mon_create(&master_monitor);
}


#define COMMA

#define NUMARGS 0
#define ARGS	
#include "start_thread.c"
#undef NUMARGS
#undef ARGS

#undef COMMA
#define COMMA ,

#define NUMARGS 1
#define ARGS	a1
#include "start_thread.c"
#undef NUMARGS
#undef ARGS

#define NUMARGS 2
#define ARGS	a1, a2
#include "start_thread.c"
#undef NUMARGS
#undef ARGS

#define NUMARGS 3
#define ARGS	a1, a2, a3
#include "start_thread.c"
#undef NUMARGS
#undef ARGS

#define NUMARGS 4
#define ARGS	a1, a2, a3, a4
#include "start_thread.c"
#undef NUMARGS
#undef ARGS

#define NUMARGS 5
#define ARGS	a1, a2, a3, a4, a5
#include "start_thread.c"
#undef NUMARGS
#undef ARGS

#define NUMARGS 6
#define ARGS	a1, a2, a3, a4, a5, a6
#include "start_thread.c"
#undef NUMARGS
#undef ARGS



#undef CreateKernelProcess
#ifndef MUTS
/*VARARGS3*/
CreateKernelProcess(r, prio, nargs, a1, a2)
Pfi r;
short prio;
int nargs;
int a1, a2;
{

  CreateProcess2(r, (int) prio, a1, a2);
}
#else
/*VARARGS4*/
CreateKernelProcess(r, prio, nargs, a1, a2, a3)
Pfi r;
short prio;
int nargs;
int a1, a2, a3;
{

  CreateProcess3(r, (int) prio, a1, a2, a3);
}

#endif

Yield()
{
  Process *active = Active;

  mon_exit(master_monitor);

  if (lwp_yield(THREADNULL) < 0)
    printf("Error performing lwp_yeild in simul/process.c\n");

  mon_enter(master_monitor);
  Active = active;

}

int delay_state;
struct timeval delay_time;


static void
wake_sem( ev, arg )
    Event	ev;
    VOID 	*arg;
{
  semSignal((Semaphore *)arg);
}


void
Delay(n)
     int n;
{
  Semaphore s;

  semInit(&s, 0);
  /* event_register(wake_sem, &s , n, EV_ONCE); */
  evDetach( evSchedule(wake_sem, &s, n * 1000) );
  semWait(&s);
}


  

semInit(s, n)
register Semaphore *s;
unsigned n;
{
  s->count = n;
#ifdef ORIGINAL
  Q_INIT(s);
#else	/* -- Robbert van Renesse */
  if (cv_create(&s->cv, master_monitor) < 0) {
    lwp_perror("cv_create");
    Kabort("realP: cv_create failed");
  }
#endif
}


realP(s)
register Semaphore *s;
{
  thread_t tmp;
  Process *active = Active;

  xTrace2(processswitch, TR_MAJOR_EVENTS, "P on %#x", s, NULL);
  
  if (s->count < 0) {
    lwp_self(&tmp);
    xTrace2(processswitch, TR_GROSS_EVENTS, "Blocking p on %#x by %d", s, tmp.thread_id);

#ifdef XK_THREAD_TRACE
    evMarkBlocked(s);
#endif XK_THREAD_TRACE

#ifdef ORIGINAL
    Q_INSERTLAST(s, active);

    mon_exit(master_monitor);

    if (lwp_suspend(SELF) == -1)
      Kabort("lwp_suspend error in realP");

    mon_enter(master_monitor);
#else	/* -- Robbert van Renesse */
    if (cv_wait(s->cv) < 0) {
	lwp_perror("cv_wait");
	Kabort("realP: cv_wait failed");
    }
    xTrace2(processswitch, TR_GROSS_EVENTS, "Unblocked p on %#x by %d", s, tmp.thread_id);
#endif

#ifdef XK_THREAD_TRACE
    evMarkRunning();
#endif XK_THREAD_TRACE

    Active = active;

   }
}

realV(s)
register Semaphore *s;
{
  register Process *p;
  xTrace2(processswitch, TR_MAJOR_EVENTS, "V on %#x by %s", s, NULL);

  if (s->count <= 0) {
#ifdef ORIGINAL
    Q_REMOVEFIRST(s, p);
    if (p) {
      lwp_resume(p->lwp);
      xTrace2(processswitch, TR_GROSS_EVENTS, "Unblocking %d by V on %#x", p->lwp.thread_id, s);
    }
#else	/* -- Robbert van Renesse */
    if (cv_notify(s->cv) < 0) {
      lwp_perror("cv_notify");
      Kabort("realV: cv_notify failed");
    }
#endif
  }
}

VAll(s)
register Semaphore *s;
{
  register Process *p;
  xTrace2(processswitch, TR_MAJOR_EVENTS, "VAll on %#x by %s", s, NULL);

#ifdef ORIGINAL
  while (s->count < 0) {
    Q_REMOVEFIRST(s, p);
    if (p) {
      lwp_resume(p->lwp);
    }
    s->count ++;
  }
#else	/* -- Robbert van Renesse */
  if (cv_broadcast(s->cv) < 0) {
    lwp_perror("cv_broadcast");
    Kabort("VAll: cv_broadcast failed");
  }
  s->count = 0;
#endif
}

#ifndef ORIGINAL	/* -- Robbert van Renesse */

/* InitSema may, and in this case, does allocate resources.  There
 * has to be a routine to release semaphores.  This is it.
 * -- Robbert van Renesse
 */
FreeSema(s)
register Semaphore *s;
{
  if (cv_destroy(s->cv) < 0) {
    lwp_perror("cv_destroy");
    Kabort("realP: cv_destroy failed");
  }
}
#endif

extern mon_t master_monitor;



/********************************
 debug code
 ********************************/

struct tname {
  int tid;
  char *name;
} tnames[20000];

int tname_idx = 0;  

char *find_thread_name(tid)
     int tid;
{
  int i;
  
  for (i=0; i<20000; i++)
    if (tnames[i].tid == tid)
      return tnames[i].name;
  return NULL;
}


show_lwp(fullp)
     int fullp;
{
  int maxsize = 50, num, i;
  thread_t vec[50];
  statvec_t state;
  extern thread_t event_thread;

  num = lwp_enumerate(vec, maxsize);

  printf("Active Threads: %d\n", num);
  if (fullp) {
    for (i=0; i<num; i++) {
      if (/* vec[i].thread_id == event_thread.thread_id */ 1) {
	printf("==>");
	printf("%d: thread_id: %d  key: %d", i, 
	       (int) vec[i].thread_id, (int) vec[i].thread_key);

	if (lwp_ping(vec[i]) == -1)
	  printf("   DEAD\n");
	else printf("\n");
	if (lwp_getstate(vec[i], &state) == -1)
	  printf("ERROR getting state on process\n");
	printf("Prio: %d, blocked on ", state.stat_prio);
	switch (state.stat_blocked.any_kind) {
	case NO_TYPE:
	  printf("No TYPE\n");
	  break;
	case CV_TYPE:
	  printf("CV TYPE %d\n", state.stat_blocked.any_object.any_cv);
	  break;
	case MON_TYPE:
	  printf("MON TYPE %d\n",state.stat_blocked.any_object.any_mon);
	  break;
	case LWP_TYPE:
	  printf("LWP TYPE %d\n",state.stat_blocked.any_object.any_thread);
	  break;
	default:
	  printf("NO state for client process\n");
	}
      }
    }
    printf("\n");
  }

}

show_mon(label)
     char *label;
{
  thread_t owner;
  thread_t vec[20];
  int n, i;

  /*
  printf("Master Monitor (%s)\n", label);
  n = mon_waiters(master_monitor, &owner, vec, 20);

  printf("N waiting on monitor: %d\n", n);
  printf("Owner: %d\n", owner.thread_id);

  printf("Blocked are: \n");
  for (i=0; i<n; i++)
    printf("%d\n", vec[i].thread_id);
    */
}

  

check_event_thread()
{
  int maxsize = 50, num, i;
  thread_t vec[50];
  statvec_t state;
  extern thread_t event_thread;

  if (lwp_ping(event_thread) == -1)
    printf("Event thread is DEAD\n");
  
  if (lwp_getstate(event_thread, &state) == -1)
    printf("ERROR getting state on process\n");

  switch (state.stat_blocked.any_kind) {
  case NO_TYPE:
    break;
  case CV_TYPE:
    printf("Blocked on CV TYPE %d\n",
	   state.stat_blocked.any_object.any_cv);
    break;
  case MON_TYPE:
    printf("Blocked on MON TYPE %d\n",
	   state.stat_blocked.any_object.any_mon);
    break;
  case LWP_TYPE:
    printf("Blocked on LWP TYPE %d\n",
	   state.stat_blocked.any_object.any_thread);
    break;
  default:
    printf("NO state for client process\n");
  }
}
