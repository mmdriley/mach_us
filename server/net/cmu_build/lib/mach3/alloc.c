/* 
 * Mach Operating System
 * Copyright (c) 1994,1993 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they made and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
/*
 * alloc.c
 *
 * Purpose: replace xkernel malloc stuff with standard malloc stuff.
 *
 * HISTORY:
 * $Log:	alloc.c,v $
 * Revision 2.3  94/07/13  17:41:11  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:09:06  jms
 * 	Initial Version
 * 	[94/01/09  20:20:04  jms]
 * 
 */

extern void	free( char * );
extern char *	malloc( unsigned );

char *
xMalloc(num)
    unsigned int num;
{
    return(malloc(num));
}

int
xFree(p)
    char *p;
{
    free(p);
    return(0);
}

void
xMallocInit()
{
    0;
}
