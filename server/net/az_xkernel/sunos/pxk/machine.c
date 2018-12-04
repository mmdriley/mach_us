/* 
 * machine.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.17 $
 * $Date: 1993/02/01 23:54:01 $
 */

#include "xtype.h"
#include "xk_debug.h"
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>

#include "machine.h"

/* sparc changes */
#ifdef sparc
#include <lwp/lwp.h>
#include <lwp/stackdep.h>
#include "process.h"
#include "assert.h"
void signal_handler();
extern mon_t master_monitor;
#endif sparc



int tracesystem=0;
extern int SignalsPossible;

struct	int_vector {
    Pfi		handler;
    VOID	*arg;
/* device is now given by the index
   int	device;
*/
};


int inInterrupt;
int	dispatch();

/* don't know how many sockets we should allow for */
/* update this in [udp,tcp].c too */
#define NUMSOCKETSICANUSE 30
struct	int_vector  ivec[NUMSOCKETSICANUSE+1];
int                 ivec_in_use_mask;
extern	int	errno;

void
cancelSignalHandler(sock)
    int sock;
{
  ivec_in_use_mask &= ~(1 << sock);
  xTrace2(system, TR_MAJOR_EVENTS, "Cancelling handler for %d mask = %x", sock, ivec_in_use_mask);
}


void
installSignalHandler( sock, f, arg )
    int 	sock;
    Pfi 	f;
    VOID	*arg;
{
    ivec[sock].handler = f;
    ivec[sock].arg = arg;
    ivec_in_use_mask |= (1 << sock);
    SignalsPossible = 1;
    xTrace3(system, TR_MAJOR_EVENTS,
	    "Setting handler for %d, arg = %x, mask = %x",
	    sock, arg, ivec_in_use_mask);
}

static unsigned int heldmask = 0;


xholdsignals(sock)
{
  xTrace1(system, TR_EVENTS, "Holding file descriptor %d", sock);
  heldmask |= (1 << sock);
}

xreleasesignals(sock)
int sock;
{
  xTrace1(system, TR_EVENTS, "Releasing file descriptor %d", sock);
  heldmask &= ~(1 << sock);
}

/*
 *
 *  Interrupt Handler
 *
 *  The handler is now invoked with a single integer argument
 *  identifying the socket to service, so that more than one
 *  socket may use the same handler.
 */
findsocket()
{
  int	mask, i, n;
  static struct timeval zero = {0, 0};
  for (i = 0; i < 32; i++) {
    mask = (1 << i);
    n = select(32, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &zero);
    if (n == 1) {
      return (i);
    }
  }
  return(33);
}

fixFileMask()
{
  int	mask, i;
  static struct timeval zero = {0, 0};
  for (i = 0; i < 32; i++) {
    if (ivec_in_use_mask & (1 << i)) {
      mask = (1 << i);
      if (select(32, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &zero) < 0 && errno == EBADF) {
	xTrace1(system, TR_MAJOR_EVENTS, "Turning off socket #%d", i);
	ivec_in_use_mask &= ~(1 << i);
      }
    }
  }
}

/*ARGSUSED*/
dispatch(interruptNo)
int interruptNo;
{
  register int i;
  int	mask, n, foundany=0;
  static struct timeval zero;

  inInterrupt++;
  while (1) {
    mask = ivec_in_use_mask & ~heldmask;
    if((n = select(NUMSOCKETSICANUSE, (fd_set *)&mask, (fd_set *)0, (fd_set *)0, &zero)) == -1) {
      if (errno == EBADF) {
	xTrace0(system, TR_MAJOR_EVENTS, "EBADF on select in dispatch");
	fixFileMask();
	continue;
      } else {
	perror("select");
	inInterrupt--;
	return;
      }
    } else if(!n) {
      xTrace0(system, TR_MAJOR_EVENTS, "dispatch: can't find any input");
      break;
    } else if(!mask) {
      printf("select returns a zeroed mask! ivecmask %#x, n is %d\n",
	     ivec_in_use_mask, n);
      break;
    } else {
      for(i=0;i<NUMSOCKETSICANUSE;i++){
	if(mask & (1 << i)) {
	  if (! ivec[i].handler) {
	    printf("Null handler for sock %d\n", i);
	  } else {
	    xTrace1(system, TR_MAJOR_EVENTS, "Calling handler for sock %d", i);
	    (*ivec[i].handler)(ivec[i].arg);
	    foundany ++;
	  }
	}
      }
    }
  }
  if(!foundany){
    xTrace4(system, TR_MAJOR_EVENTS,
      "unknown interrupt %d (mask %#x ivec %#x real %d) in dispatch\n", 
      interruptNo, mask, ivec_in_use_mask, findsocket());
  }
  inInterrupt--;
}

/********************************************
 *
 *  Clock Device
 *
 *********************************************/

struct itimerval i_value, i_zero;

#define handlerresulttype void
typedef handlerresulttype (*handlertype)();

onfault(h)
handlertype h;
{
  struct sigvec vec;

  vec.sv_mask         = 0;
  vec.sv_onstack      = 1;
  vec.sv_handler = h;
  sigvec(SIGINT, &vec, (struct sigvec *)0);
  sigvec(SIGSEGV, &vec, (struct sigvec *)0);
  sigvec(SIGBUS, &vec, (struct sigvec *)0);
}
  
int sig_int_handler();
void event_handler();

void
init_clock(ih,interval)
Pfv	ih;
long	interval;
{
  thread_t tid;
  extern int exit();

  errno = 0;

  definehandler(SIGPIPE, SIG_IGN);
  definehandler(SIGINT, (handlertype) sig_int_handler);
  definehandler(SIGIO,  (handlertype) dispatch);
  definehandler(SIGURG,  (handlertype) dispatch);

  /* start event processor */
  CreateProcess1(event_handler, LWP_MAXPRIO-1, interval);

  if (errno)
    perror("init_clock:");
  return;
}


sig_int_handler()
{
  pod_exit(0);
}


read_clock(msec)	/* returns the number of msec */
long	*msec;     	/* since sometime in late 1986 */
{
  struct  timeval   time;
  struct  timezone  zone;

  gettimeofday(&time, &zone);
  *msec = (time.tv_sec - 500000000)*1000 + (time.tv_usec / 1000);
  return(0);
}


definehandler(sig, handler)
int sig;
handlertype handler;
{
  struct sigvec lvec;
  thread_t tid;
  if (handler == SIG_IGN) {
    lvec.sv_mask = -1;
    lvec.sv_onstack = 1;
    lvec.sv_handler = SIG_IGN;
    sigvec(sig, &lvec, (struct sigvec *)NULL);
  } else {
    lwp_create(&tid, signal_handler, LWP_MAXPRIO, 0, lwp_newstk(),
	       2, sig, handler);
  }

}

void signal_handler(sigid, handler)
int sigid;
handlertype handler;
{
  eventinfo_t sigmem;
  char *arg;
  int asz;
  thread_t sender;

  agt_create(&sender, sigid, (char *)&sigmem);
  
  for (;;) {
    (void) msg_recv(&sender, &arg, &asz, 0, 0, INFINITY);

    mon_enter(master_monitor);
    (void) msg_reply(sender);

    /* printf("."); fflush(stdout);*/

    (handler)(sigid);

    /* printf("-"); fflush(stdout);*/

    mon_exit(master_monitor);
  }
}


int state;
int state_int;
thread_t event_thread;

mon_t saved_monitor;

void event_handler(interval)
     int interval;		/* number of milliseconds */
{

  struct timeval sleep_time;
  int ti = 0;

  xTrace1(event, TR_EVENTS, "event_handler called with interval %d", interval);
  sleep_time.tv_sec = interval / 1000;
  sleep_time.tv_usec = (interval % 1000)*1000;
  xTrace2(event, TR_MAJOR_EVENTS, "event handler sleep time: %d secs, %d usecs",
	  sleep_time.tv_sec, sleep_time.tv_usec);
  lwp_self(&event_thread);
  /* printf("Event Handler is thread %d\n", event_thread.thread_id);*/

  saved_monitor = master_monitor;

  for (;;) {
    clock_ih();
    xIfTrace(event, TR_ERRORS) {
      if (saved_monitor.monit_id != master_monitor.monit_id ||
	  saved_monitor.monit_key != master_monitor.monit_key)
	xTrace2(event, TR_ALWAYS, "event_handler: We're going down in flames %x %x", saved_monitor.monit_id, master_monitor.monit_id);
    }
    mon_exit(master_monitor);
    if (lwp_sleep(&sleep_time) < 0)
      printf("lwp_sleep error for quantum %d.\n", interval);
    mon_enter(master_monitor);
  }
}









