/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989,1988,1987 Carnegie Mellon University
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
 * HISTORY
 * $Log:	mach_exception.h,v $
 * Revision 2.3  94/07/08  17:55:17  mrt
 * 	Updated copyrights
 * 
 * Revision 2.2  93/01/20  17:37:17  jms
 * 	Grab from another world.
 * 	[93/01/18  16:14:03  jms]
 * 
 * Revision 1.1  90/02/18  17:48:08  bww
 * 	Mach Release 2.5
 * 	[90/02/18  17:47:49  bww]
 * 
 * Revision 1.4  89/10/25  00:02:41  mrt
 * 	Changed the include exclusion variable to _USR_MACH_EXCEPTION so
 * 	that it would not conflict with include/mach/exception.h.
 * 	[89/10/09            mrt]
 * 
 * Revision 1.3  89/05/05  18:45:18  mrt
 * 	Cleanup for Mach 2.5
 * 
 * 25-Apr-88  Karl Hauth (hauth) at Carnegie-Mellon University
 *	Created.
 *
 */ 

#ifndef	_USR_MACH_EXCEPTION_H_
#define	_USR_MACH_EXCEPTION_H_	1

#include <mach/kern_return.h>

char		*mach_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception
#endif	c_plusplus
				);


void		mach_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception
#endif	c_plusplus
				);


char		*mach_sun3_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);


void		mach_sun3_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

char		*mach_romp_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

void		mach_romp_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

char		*mach_vax_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

void		mach_vax_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

#endif	_USR_MACH_EXCEPTION_H_
