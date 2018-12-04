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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/lib/mach3/mach3/mach3_exit.c,v $
 *
 * HISTORY
 * $Log:	mach3_exit.c,v $
 * Revision 2.5  94/07/08  17:54:58  mrt
 * 	Updated copyrights
 * 
 * Revision 2.4  90/11/10  00:38:11  dpj
 * 	Enabled definition of exit() for all architectures under
 * 	MACH3_US and MACH3_SA, to track changes in the pure kernel crt0.c.
 * 	[90/11/08  22:17:11  dpj]
 * 
 * Revision 2.3  90/10/29  17:27:35  dpj
 * 	Added _exit() routine.
 * 	[90/08/13  10:16:32  dpj]
 * 
 * Revision 2.2  90/07/26  12:37:13  dpj
 * 	First version
 * 	[90/07/24  14:28:16  dpj]
 * 
 *
 */

#ifndef lint
char * mach3_exit_rcsid = "$Header: mach3_exit.c,v 2.5 94/07/08 17:54:58 mrt Exp $";
#endif	lint

#if	MACH3_US || MACH3_SA

exit(code)
{
	(void) task_terminate(mach_task_self());
}

_exit(code)
{
	return(exit(code));
}

#endif	MACH3_US || MACH3_SA
