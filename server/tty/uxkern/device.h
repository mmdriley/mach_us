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
 * $Log:	device.h,v $
 * Revision 2.3  94/07/14  12:10:04  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  90/10/02  11:34:30  mbj
 * 	Corrected #if MACH3_US -> #if MACH3 conditional.
 * 	[90/10/01  15:41:24  mbj]
 * 
 * Revision 2.1.1.1  90/09/10  18:03:19  mbj
 * 	Paul Neves' pure kernel tty_server changes
 * 
 * Revision 2.2  89/09/15  15:29:19  rwd
 * 	Created
 * 	[89/09/11            rwd]
 * 
 */
#ifdef	KERNEL
#define	KERNEL__
#undef	KERNEL
#endif	KERNEL

#include <device/device.h>

#ifdef	KERNEL__
#undef	KERNEL__
#define	KERNEL	1
#endif	KERNEL__

