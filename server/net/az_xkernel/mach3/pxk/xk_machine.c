/*
 * machine.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:05:55 $
 */

/*
 * Mach3
 */

#ifndef XKMACHKERNEL
#include <sys/time.h>
#include <mach.h>
#include <cthreads.h>
#else
#include <kern/host.h>
#include <mach/time_value.h>
#endif  XKMACHKERNEL

#include <mach/message.h>

#include "xk_debug.h"
#include "upi.h"
#include "platform.h"

extern char *sprintf();

extern int SignalsPossible;

struct	int_vector {
  Pfi	handler;
/* device is now given by the index
  int	device;
*/
};


int inInterrupt;


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
}

void
installSignalHandler(sock, f)
int sock;
Pfi f;
{
}

void
xholdsignals(sock)
{
}

void
xreleasesignals(sock)
int sock;
{
}

void
findsocket()
{
}

void
fixFileMask()
{
}

void
dispatch(interruptNo)
int interruptNo;
{
}

#ifdef XKMACHKERNEL

x_gettime(where)
unsigned char where[8];
{
	/* gettimeofday(where, 0); */

	host_get_time( (host_t)1, (time_value_t *)where );
}
#else
struct itimerval i_value, i_zero;
#endif XKMACHKERNEL

#define handlerresulttype void
typedef handlerresulttype (*handlertype)();

void
onfault(h)
handlertype h;
{
}
  
void sig_int_handler();
void event_handler();


void
sig_int_handler()
{

}


#ifdef READ_CLOCK
int
read_clock(msec)	/* returns the number of msec */
long	*msec;     	/* since sometime in late 1986 */
{
  struct  timeval   time;
  struct  timezone  zone;

  gettimeofday(&time, &zone);
  *msec = (time.tv_sec - 500000000)*1000 + (time.tv_usec / 1000);
  return(0);
}
#endif READ_CLOCK


void
definehandler(sig, handler)
int sig;
handlertype handler;
{
}

void signal_handler(sigid, handler)
int sigid;
handlertype handler;
{
}


