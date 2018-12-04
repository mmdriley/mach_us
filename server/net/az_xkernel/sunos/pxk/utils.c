/* 
 * utils.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 23:54:16 $
 */

#include "x_stdio.h"
#include "x_libc.h"
#include "x_util.h"

extern char *	malloc();
extern int	abort();

void
Kabort(s)
    char *s;
{
    fprintf(stderr, "xkernel abort: %s\n", s);
    abort();
}


char *
xMalloc( unsigned s )
{
    char *p;

    if ( p = malloc(s) ) {
	return p;
    }
    Kabort("malloc failed");
    return 0;
}
