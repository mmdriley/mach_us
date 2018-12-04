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
 * $Log:	import_mach.h,v $
 * Revision 2.3  94/07/14  12:10:11  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  90/10/02  11:34:42  mbj
 * 	Removed incorrect mach/mach.h include.
 * 	[90/10/01  15:43:23  mbj]
 * 
 * Revision 2.1.1.1  90/09/10  18:04:27  mbj
 * 	Paul Neves' pure kernel tty_server changes
 * 
 */
/*
 * MACH interface definitions and data for UX out-of-kernel kernel.
 */

/*
 * <mach> must be included with 'KERNEL' off
 */
#ifdef	KERNEL
#define	KERNEL__
#undef	KERNEL
#endif	KERNEL

#include <mach.h>
#if	! MACH3
#include <mach/host.h>
#endif	! MACH3
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <cthreads.h>

#ifdef	KERNEL__
#undef	KERNEL__
#define	KERNEL	1
#endif	KERNEL__
