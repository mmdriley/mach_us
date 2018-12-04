/* 
 * time.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 23:54:06 $
 */

#include <sys/time.h>

#ifdef __STDC__

int	gettimeofday( struct timeval *, struct timezone * );

#endif

void
xGetTime(where)
    unsigned char where[8];
{
    gettimeofday((struct timeval *)where, 0);
}


