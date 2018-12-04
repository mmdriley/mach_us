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
 * File:	errrolib.h
 * Purpose:
 *	Generic error code interface
 *
 * HISTORY:
 * $Log:	errorlib.h,v $
 * Revision 2.4  94/07/08  17:54:51  mrt
 * 	Updated copyrights
 * 
 * Revision 2.3  91/12/20  17:43:46  jms
 * 	Add multiple inclusion guards.
 * 	[91/12/20  15:29:49  jms]
 * 
 * Revision 2.2  90/08/13  15:44:25  jjc
 * 	Picked up from Mark Stevenson for including in error_codes.c.
 * 	[90/08/07            jjc]
 * 
 * Revision 2.2  90/07/26  12:44:36  dpj
 * 	First version.
 * 	[90/07/24  16:40:33  dpj]
 * 
 * Revision 1.4  90/03/21  17:22:02  jms
 * 	Comment Changes
 * 	[90/03/16  16:52:20  jms]
 * 
 * 	[89/12/19  16:16:56  jms]
 * 
 *	new taskmaster changes
 *
 * Revision 1.3  90/01/02  22:04:20  dorr
 * 	no change.
 * 
 * Revision 1.2.1.1  89/12/19  17:06:35  dorr
 * 	16-May-88	Mary R. Thompson (mrt) at Carnegie Mellon
 *	Corrected the definitions of IPC_RCV_MOD and IPC_SEND_MOD
 *
 * 09-Mar-88	Douglas Orr (dorr) at Carnegie-Mellon University
 *	created.
 */

#ifndef	_ERRORLIB_H_
#define	_ERRORLIB_H_

#include <mach/error.h>

#define	IPC_SEND_MOD		(err_ipc|err_sub(0))
#define	IPC_RCV_MOD		(err_ipc|err_sub(1))
#define	IPC_MIG_MOD		(err_ipc|err_sub(2))

#define	SERV_NETNAME_MOD	(err_server|err_sub(0))
#define	SERV_ENV_MOD		(err_server|err_sub(1))
#define	SERV_EXECD_MOD		(err_server|err_sub(2))


#define	NO_SUCH_ERROR		"unknown error code"

struct error_subsystem {
	char			* subsys_name;
	int			max_code;
	char			* * codes;
};

struct error_system {
	int			max_sub;
	char			* bad_sub;
	struct error_subsystem	* subsystem;
};

extern	struct error_system 	errors[err_max_system+1];

#define	errlib_count(s)		(sizeof(s)/sizeof(s[0]))

#endif	_ERRORLIB_H_
