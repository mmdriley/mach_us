/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989,1988 Carnegie Mellon University
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
 * devnum.h
 *
 * Machine specific device number indices for the I386.
 */
/*
 **********************************************************************
 * HISTORY
 * $Log:	devnum.h,v $
 * Revision 2.3  94/07/14  13:12:50  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/06/01  18:20:02  mrt
 * 	Moved from i386_mach to i386 directory for odemake.
 * 	[94/03/02            mrt]
 * 
 * Revision 2.3  90/08/14  22:03:31  mbj
 * 	Changed the i386 device numbers to more closely match reality.
 * 	We're still making up numbers for CPU ptys since the mainline
 * 	doesn't yet have them.
 * 	[90/08/14  14:49:47  mbj]
 * 
 * Revision 2.2  90/03/21  17:33:57  jms
 * 	First working version for i386.  Some entries facked because
 * 	we didnn't have a good answer. (sic 99)
 * 
 *
 **********************************************************************
 */

#define DEVNUM_CONSOLE	1
#define DEVNUM_SERIAL	0
#define DEVNUM_TTY	2
#define DEVNUM_PTS	9
#define DEVNUM_PTC	10
#define DEVNUM_CMUPTY	98 /* XXX */
#define DEVNUM_CMUPTYC	99 /* XXX */
