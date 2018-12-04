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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/lib/mach3/mach3/mach3_abort.c,v $
 *
 * Purpose:
 *
 * HISTORY
 * $Log:	mach3_abort.c,v $
 * Revision 2.4  94/07/08  17:54:54  mrt
 * 	Updated copyrights
 * 
 * Revision 2.3  90/10/29  17:27:28  dpj
 * 	Merged-up to U25
 * 	[90/09/02  20:00:37  dpj]
 * 
 * 	First version.
 * 	[90/08/02  10:19:59  dpj]
 * 
 * Revision 2.2  90/08/22  18:09:12  roy
 * 	No change.
 * 	[90/08/14  17:03:58  roy]
 * 
 * Revision 2.1  90/08/02  10:19:40  dpj
 * Created.
 * 
 */

#ifndef lint
char * mach3_abort_rcsid = "$Header: mach3_abort.c,v 2.4 94/07/08 17:54:54 mrt Exp $";
#endif	lint

#if	MACH3_US || MACH3_SA

#include	<stdio.h>

abort()
{
	fprintf(stderr,"***** Aborting *****\n");
	exit(2001);
}

#endif	MACH3_US || MACH3_SA

