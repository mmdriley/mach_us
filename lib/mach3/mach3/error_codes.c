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
 * File:	errr_codes.c
 * Purpose:
 *	Generic error code interface
 *
 * HISTORY:
 * $Log:	error_codes.c,v $
 * Revision 2.3  94/07/08  17:54:49  mrt
 * 	Updated copyrights
 * 
 * Revision 2.2  90/07/26  12:37:07  dpj
 * 	First version
 * 	[90/07/24  14:28:08  dpj]
 * 
 * Revision 1.4  90/03/21  17:21:55  jms
 * 	Comment changes
 * 	[90/03/16  16:51:31  jms]
 * 
 * 	Mods for new taskmaster interface
 * 	[89/12/19  16:15:50  jms]
 * 
 *
 * Revision 1.3  90/01/02  22:03:45  dorr
 * 	created.
 * 
 * Revision 1.2.1.1  89/12/19  17:06:16  dorr
 * 	09-Mar-88	Douglas Orr (dorr) at Carnegie-Mellon University
 * 
 *	created.
 */

#include <mach/error.h>
#include "errorlib.h"
#include "err_kern.sub"
#include "err_us.sub"
#include "err_server.sub"
#include "err_ipc.sub"


struct error_system errors[err_max_system+1] = {
	/* 0; err_kern */
	{
		errlib_count(err_os_sub),
		"(operating system/?) unknown subsystem error",
		err_os_sub,
	},
	/* 1; err_us */
	{
		errlib_count(err_us_sub),
		"(user space/?) unknown subsystem error",
		err_us_sub,
	},
	/* 2; err_server */
	{
		errlib_count(err_server_sub),
		"(server/?) unknown subsystem error",
		err_server_sub,
	},
	/* 3 (& 3f); err_ipc */
	{
		errlib_count(err_ipc_sub),
		"(ipc/?) unknown subsystem error",
		err_ipc_sub,
	},
};


int error_system_count = errlib_count(errors);


