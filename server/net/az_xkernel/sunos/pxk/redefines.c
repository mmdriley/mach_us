/*
 * redefines.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 23:54:11 $
 */

/*
 * file of functions which don't work with sun4 gcc unless
 * recompiled.
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <varargs.h>

char *inet_ntoa(in)
	struct in_addr in;
{
	static char b[18];
	register char *p;

	p = (char *)&in;
#define	UC(b)	(((int)b)&0xff)
	sprintf(b, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
	return (b);
}


