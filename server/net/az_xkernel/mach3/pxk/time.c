/*
 * time.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.10 $
 * $Date: 1993/02/02 00:05:23 $
 */

#ifdef XKMACHKERNEL

#include <mach/time_value.h>
extern time_value_t time;

#else

#include <sys/time.h>

#endif XKMACHKERNEL

#include "xtime.h"

void
xGetTime(where)
XTime *where;
{
#ifdef XKMACHKERNEL
  where->sec = time.seconds;
  where->usec = time.microseconds;
#else
  gettimeofday((struct timeval *)where, 0);
#endif XKMACHKERNEL
}


