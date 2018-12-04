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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/lib/mach3/mach3/mach3_setjmp.c,v $
 *
 * HISTORY
 * $Log:	mach3_setjmp.c,v $
 * Revision 2.3  94/07/08  17:55:09  mrt
 * 	Updated copyrights
 * 
 * Revision 2.2  90/07/26  12:37:33  dpj
 * 	First version
 * 	[90/07/24  14:28:39  dpj]
 * 
 *
 */

#ifndef lint
char * mach3_setjmp_rcsid = "$Header: mach3_setjmp.c,v 2.3 94/07/08 17:55:09 mrt Exp $";
#endif	lint

#if	MACH3_US || MACH3_SA

#include	"setjmp.h"

int setjmp(env)
	jmp_buf		env;
{
	return(_setjmp(env));
}

void longjmp(env,val)
	jmp_buf		env;
	int		val;
{
	_longjmp(env,val);
	return;
}

#endif	MACH3_US || MACH3_SA
