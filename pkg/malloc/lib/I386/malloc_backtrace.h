/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990 Carnegie Mellon University
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/pkg/malloc/lib/I386/malloc_backtrace.h,v $
 *
 * Purpose:
 *
 * HISTORY
 * $Log:	malloc_backtrace.h,v $
 * Revision 2.3  94/07/14  16:06:32  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  90/10/29  17:33:52  dpj
 * 	Created.
 * 	[90/10/27  18:01:56  dpj]
 * 
 * 	First working version.
 * 	[90/10/21  21:33:52  dpj]
 * 
 */

#ifndef	_malloc_backtrace_h
#define	_malloc_backtrace_h

struct	stackframe {
	struct stackframe	*link;
	char			*ret_addr;
};

typedef struct stackframe	*FRAMEPTR;

#define	MYFRAME(myfirstarg)	 ((FRAMEPTR) (((int *) &(myfirstarg)) - 2))

#define	NEXTFRAME(myframe)	((myframe)->link)
#define	RET_ADDR(myframe)	((myframe)->ret_addr)


#endif	_malloc_backtrace_h
