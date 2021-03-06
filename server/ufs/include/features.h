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
 *
 */
/*
 **********************************************************************
 * HISTORY
 * $Log:	features.h,v $
 * Revision 1.4  94/07/21  17:49:58  mrt
 * 	Updated copyright
 * 
 **********************************************************************
 *
 *  This file is used to pick up the various configuration options that affect
 *  the use of kernel include files by special user mode system applications.
 *  The machine dependent configuration files that ultimately end up being
 *  included below are generated by the configuration script.  Only those
 *  configurations which differ in some significant way or are used external to
 *  the kernel need be defined here.  When the default configuration isn't
 *  appropriate, an alternate configuration can be selected by defining the
 *  appropriate pre-processor symbol.
 *
 *  The entire file (and hence all configuration symbols which it indirectly
 *  defines) is enclosed in the KERNEL_FEATURES conditional to prevent
 *  accidental interference with normal user applications.  Only special system
 *  applications need to know the precise layout of internal kernel structures
 *  and they will explicitly set this flag to obtain the proper environment.
 */
#ifdef	KERNEL_FEATURES


#ifndef	CONFIG_COMPAT
/*
 *  Use the default configuration if none is explicitly specified.
 */
#define	CONFIG_DEFAULT
#endif

#ifdef	CONFIG_DEFAULT
#endif

#ifdef	CONFIG_COMPAT
#include <machine/COMPAT.h>
#endif


/*
 * PAY CLOSE ATTENTION NOW.  Why these are in a machine specific
 * directory, I don't exactly know.
 */
#undef	CS_RFS
#define	CS_RFS	0

#undef	VICE
#define	VICE	0


#endif	KERNEL_FEATURES
