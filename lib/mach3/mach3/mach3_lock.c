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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/lib/mach3/mach3/mach3_lock.c,v $
 *
 * Purpose: simple spin locks
 *
 * HISTORY
 * $Log:	mach3_lock.c,v $
 * Revision 2.3  94/07/08  17:55:08  mrt
 * 	Updated copyrights
 * 
 * Revision 2.2  90/10/29  17:27:52  dpj
 * 	Created.
 * 	[90/10/27  17:49:02  dpj]
 * 
 * 	Created to replace lock.c, to avoid conflicts with the cthreads
 * 	version of spin locks.
 * 	[90/10/21  21:20:52  dpj]
 * 
 *
 */

#ifndef lint
char * skel_rcsid = "$Header: mach3_lock.c,v 2.3 94/07/08 17:55:08 mrt Exp $";
#endif	lint


/*
 * Note: most of the functions are machine-dependent, and found in
 * ${MACHINE}/lock.s.
 */


void
mach3_spin_lock(p)
	register int *p;
{
	while (*p != 0 || !mach3_spin_try_lock(p))
		;	/* spin */
}



