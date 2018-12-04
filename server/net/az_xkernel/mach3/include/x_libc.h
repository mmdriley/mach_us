/*     
 * $RCSfile: x_libc.h,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/02 00:03:01 $
 */

/*
 * C library prototypes
 */

#ifndef x_libc_h
#define x_libc_h

#ifndef XKMACHKERNEL

#include <string.h>
#include <stdio.h>

extern void     bcopy( char *, char *, int );
extern int	bcmp( char *, char *, int );
extern void     bzero( char *, int );
extern int	qsort( char *, int, int, int(*)());

#endif ! XKMACHKERNEL

#endif ! x_libc_h
