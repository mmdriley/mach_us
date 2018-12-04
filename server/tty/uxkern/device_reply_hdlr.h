/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989 Carnegie Mellon University
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
 * HISTORY:
 * $Log:	device_reply_hdlr.h,v $
 * Revision 2.3  94/07/14  12:10:06  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  90/10/02  11:34:33  mbj
 * 	Paul Neves' pure kernel tty_server changes
 * 	[90/09/10  18:03:50  mbj]
 * 
 */
/*
 * Handler for device read and write replies.
 */
#ifndef	DEVICE_REPLY_HDLR_H
#define	DEVICE_REPLY_HDLR_H

#include <mach/kern_return.h>

typedef kern_return_t	(*kr_fn_t)();

void	reply_hash_enter();	/* port_t, char *, kr_fn_t, kr_fn_t */
void	reply_hash_remove();	/* port_t */

void	device_reply_hdlr();

#endif	DEVICE_REPLY_HDLR_H

