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
 * HISTORY
 * $Log:	device_utils.h,v $
 * Revision 2.4  94/07/14  12:10:09  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  92/03/05  15:15:35  jms
 * 	Upgrade to use no MACH_IPC_COMPAT features.  New IPC only!
 * 	[92/02/26  19:48:21  jms]
 * 
 * Revision 2.2  90/10/02  11:34:39  mbj
 * 	Paul Neves' pure kernel tty_server changes
 * 	[90/09/10  18:04:00  mbj]
 * 
 * Revision 2.2  89/09/15  15:29:30  rwd
 * 	Undefine KERNEL while including device_types.h
 * 	[89/09/11            rwd]
 * 
 */
/*
 * Support routines for device interface in out-of-kernel kernel.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <uxkern/import_mach.h>

#ifdef	KERNEL
#define	KERNEL__
#undef	KERNEL
#endif	KERNEL
#include <device/device_types.h>
#ifdef	KERNEL__
#undef	KERNEL__
#define	KERNEL	1
#endif	KERNEL__

mach_port_t	device_server_port;

void	dev_utils_init();

void	dev_number_hash_enter();	/* dev_t, char * */
void	dev_number_hash_remove();	/* dev_t */
char *	dev_number_hash_lookup();	/* dev_t */

int	dev_error_to_errno();		/* int */

