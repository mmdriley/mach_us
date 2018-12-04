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
 * This file is derived from the x-kernel distributed by the
 * University of Arizona. See the README file at the base of this
 * source subtree for details about distribution.
 *
 * The Mach 3 version of the x-kernel is substantially different from
 * the original UofA version. Please report bugs to mach@cs.cmu.edu,
 * and not directly to the x-kernel project.
 */
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/utils.c,v $
 *
 * 
 * Purpose:  Misc support stuff
 * 
 * HISTORY
 * $Log:	utils.c,v $
 * Revision 2.3  94/07/13  18:07:14  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:42  jms
 * 	Minor changes
 * 	[94/01/10  13:00:10  jms]
 * 
 * Revision 2.2  90/10/29  18:05:02  dpj
 * 	Integration into the master source tree
 * 	[90/10/21  23:13:49  dpj]
 * 
 * 	First working version.
 * 	[90/10/03  21:50:46  dpj]
 * 
 *
 */

#if	MACH3_MODS

#include	<base.h>
#include	<stdio.h>
#include	<mach_error.h>


int	traceabort;

Kabort(s)
char *s;
{
  fprintf(stderr, "%s\n", s);
  if (traceabort) {
	fprintf(stderr,
	"Kabort: xkernel suspended. Awaiting restart from debugger.\n");
	task_suspend(mach_task_self());
  }
  abort();
}


/*
 * Convert a xkernel error code to a mach_error_t.
 */
#define	XKERNEL_MODULE		(err_server|err_sub(15))
mach_error_t convert_xkernel_error(err)
	int		err;
{
	return(XKERNEL_MODULE | (err & code_emask));
}

#endif	MACH3_MODS
