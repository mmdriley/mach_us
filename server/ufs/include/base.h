
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
 * File:        uck_base.h
 * Purpose:
 *	base set of declarations for unix kernel server
 *	
 *
 * HISTORY:
 * $Log:	base.h,v $
 * Revision 1.19  94/07/21  17:49:30  mrt
 * 	Updated copyright
 * 
 * Revision 1.18  92/07/05  23:36:49  dpj
 * 	private -> PRIVATE to avoid C++ problem.
 * 	[92/06/24  17:44:42  dpj]
 * 
 * Revision 1.17  92/03/05  15:15:52  jms
 * 	Upgrade to use no MACH_IPC_COMPAT features.  New IPC only!
 * 	[92/02/26  19:49:40  jms]
 * 
 * Revision 1.16  91/07/01  14:16:06  jms
 * 	Merge to new branch.
 * 	[91/05/29  10:50:07  roy]
 * 
 * Revision 1.15.2.1  90/11/05  18:47:11  roy
 * 	No change.
 * 
 * 
 * Revision 1.15.1.1  90/09/24  11:55:05  roy
 * 	Added defn of NULL.
 * 
 * 
 * Revision 1.15  89/07/09  14:22:26  dpj
 * 	Added include of mach_object.h, needed by the debug system.
 * 	This file should really be named differently, to avoid conflicts
 * 	with include/base.h.
 * 	[89/07/08  13:21:38  dpj]
 * 
 * Revision 1.14  89/05/18  10:35:32  dorr
 * 	include file cataclysm
 * 
 * Revision 1.13  89/03/17  13:06:42  sanzi
 * 	include debug.h.  use its declaration of MACH_ERROR.
 * 	[89/03/12  20:26:11  dorr]
 * 
 *
 *   28-April-1988  Douglas Orr (dorr) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_UFS_BASE_H_
#define	_UFS_BASE_H_

#include <debug.h>
#include <features.h>
#include <mach/port.h>
#include "time.h"

#define	ALLOW_WRITE	1

/* macro defines */
#define	uprintf			printf

#define	PRIVATE			static

/* macro "functions" */

/*
 * This will never work multi-threaded.
 * This definition is nonsense, but it is to reduce the
 * number of warning messages about the s variable that
 * smart compilers generate.
 */
#define	splx(_s_)	(_s_ = (_s_ == 4 ? _s_ : _s_ - 1))

#define	spl0()	0
#define	spl1() 	0
#define	spl2()	0
#define	spl3()	0
#define	spl4()	0
#define	spl5()	0
#define	spl6()	0
#define	spl7()	0
#define	splzs()	0
#define	splie()	0
#define	splnet() 0
#define	splimp() 0
#define	splclock() 0
#define	splbio() 0

#define	ovbcopy(s,d,l)		bcopy(s,d,l)	/* this probably won't work */
#define	blkclr(b,s)		bzero(b,s)
#define	free(ip,bno,size)	free_blk_frag(ip,bno,size)  /* give free a new name so it 
							       doesn't conflict with malloc/free */

#define	New(type)		(type *)malloc( sizeof(type) )
#define NewArray(type,cnt)	(type *)malloc( sizeof(type) * (cnt) )

#define	min(x,y)		((x)<(y)?(x):(y))
#define	max(x,y)		((x)>(y)?(x):(y))

#ifndef NULL
#define NULL	0
#endif  NULL

/* typedefs */

/* globals */
extern 	mach_port_t	io_port;
extern	struct 	timeval time;
/* functions */

#endif _UFS_BASE_H_
