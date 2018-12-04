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
 * File: us/lib/mach3/mach3/mach3_findiop.c
 *
 * Purpose: Statically defines _iob
 *
 *
 * HISTORY
 * $Log:	mach3_findiop.c,v $
 * Revision 2.5  94/07/08  17:55:03  mrt
 * 	Updated copyrights
 * 
 * Revision 2.4  90/10/29  17:27:45  dpj
 * 	Merged-up to U25
 * 	[90/09/02  20:00:48  dpj]
 * 
 * Revision 2.3  90/08/22  18:10:12  roy
 * 	Allocate static bufs for stdout, stderr.
 * 	[90/08/14  12:31:01  roy]
 * 
 * Revision 2.2  90/07/26  12:37:20  dpj
 * 	First version
 * 	[90/07/24  14:28:26  dpj]
 * 
 */

#if	MACH3_US || MACH3_SA

#include <stdio.h>

#define NSTATIC	20	/* stdin + stdout + stderr + the usual */

char	stdout_buf[BUFSIZ];
char	stderr_buf[BUFSIZ];

FILE _iob[NSTATIC] = {
	{ 0, NULL,       NULL,       0,      _IOREAD,       0 }, /* stdin  */
	{ 0, stdout_buf, stdout_buf, BUFSIZ, _IOWRT|_IOLBF, 1 }, /* stdout */
	{ 0, stderr_buf, stderr_buf, BUFSIZ, _IOWRT|_IOLBF, 2 }, /* stderr */
};

#endif	MACH3_US || MACH3_SA
