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
 **********************************************************************
 * HISTORY
 * $Log:	ttyloc.h,v $
 * Revision 1.3  94/07/21  16:40:36  mrt
 * 	Updated copyright
 * 
 * 15-Aug-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed TLC_MYHOST to expand to the host ID.
 *
 * 15-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Made conditional on _TTYLOC_ symbol to permit recursive
 *	includes.
 *	[V1(1)]
 *
 * 09-Sep-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added TLC_TACTTY and TLC_RANDOMTTY definitions.
 *
 * 28-Mar-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created (V3.06h).
 *
 **********************************************************************
 */
 
#ifndef	_TTYLOC_
#define	_TTYLOC_
struct ttyloc
{
    long tlc_hostid;		/* host identifier of location (on Internet) */
    long tlc_ttyid;		/* terminal identifier of location (on host) */
};

/*
 *  Pseudo host location of Front Ends
 */
#define	TLC_FEHOST	((128<<24)|(2<<16)|(254<<8)|255)
#define	TLC_SPECHOST	((0<<24)|(0<<16)|(0<<8)|0)

/*
 *  Pseudo terminal index of console
 */
#define	TLC_CONSOLE	((unsigned short)0177777)
#define	TLC_DISPLAY	((unsigned short)0177776)

/*
 *  Constants
 */
#define	TLC_UNKHOST	((long)(0))
#define	TLC_UNKTTY	((long)(-1))
#define	TLC_DETACH	((long)(-2))
#define	TLC_TACTTY	((long)(-3))		/* unused by kernel */
#define	TLC_RANDOMTTY	((long)(-4))		/* unused by kernel */

#if	US
/*
 *  IP address of local host (as determined by the network module)
 */
extern long hostid;	/* should really include kernel.h */
#define	TLC_MYHOST	(hostid)

#endif	US
#endif	_TTYLOC_
