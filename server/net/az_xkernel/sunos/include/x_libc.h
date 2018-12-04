/*     
 * $RCSfile: x_libc.h,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 23:53:33 $
 */

/*
 * C library prototypes
 */

#ifndef x_libc_h
#define x_libc_h

#include <string.h>

#ifdef __STDC__

extern void     bcopy( char *, char *, int );
extern int	bcmp( char *, char *, int );
extern void     bzero( char *, int );
extern int	qsort( char *, int, int, int(*)());

extern int	lwp_self();

#else

extern void     bcopy();
extern int	bcmp();
extern void     bzero();

#endif  __STDC__

#endif  ! x_libc_h
