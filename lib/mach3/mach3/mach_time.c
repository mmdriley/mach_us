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
 * HISTORY
 * $Log:	mach_time.c,v $
 * Revision 2.4  94/07/08  17:55:24  mrt
 * 	Updated copyrights
 * 
 * Revision 2.3  90/08/22  18:11:22  roy
 * 	Added MACH3_UNIX impl.
 * 	[90/08/14  12:14:14  roy]
 * 
 * Revision 2.2  90/07/26  12:37:47  dpj
 * 	First version
 * 	[90/07/24  14:29:19  dpj]
 * 
 *
 */

#include	<mach.h>
#include	<mach/time_value.h>
#include 	<mach_error.h>


#if !defined(MACH3_UNIX)
/*
 * Simple time-of-day facility for standalone Mach programs.
 */

#include	<mach_privileged_ports.h>

kern_return_t	mach_get_time(time_value)
	time_value_t		*time_value;	/* OUT */
{
	return(host_get_time(mach_privileged_host_port(),time_value));
}


kern_return_t	mach_set_time(time_value)
	time_value_t		*time_value;
{
	return(host_set_time(mach_privileged_host_port(),*time_value));
}


#else !defined(MACH3_UNIX)
/*
 * Simple time-of-day facility for Unix programs
 */

#include <sys/time.h>

kern_return_t	mach_get_time(time_value)
	time_value_t		*time_value;	/* OUT */
{
	struct timeval time;
	extern int errno;

	errno = 0;
	if (gettimeofday(&time, 0) < 0)
		return(unix_err(errno));
	else {
		time_value->seconds = time.tv_sec;
		time_value->microseconds = time.tv_usec;
	}
	
	return(KERN_SUCCESS);
}


#endif !defined(MACH3_UNIX)

