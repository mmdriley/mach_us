/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	strings.h,v $
 * Revision 2.2  94/05/17  14:09:03  jms
 * 	Fix types of strlen for gcc2.3.3
 * 	[94/04/28  19:04:05  jms]
 * 
 * Revision 2.1.1.1  94/02/18  11:37:05  modh
 * 	Fix include problems with strings.h for 2.3.3 g++
 * 
 * Revision 2.3  91/05/14  17:55:24  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/02/14  14:18:57  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:45:12  mrt]
 * 
 */

/*
 * External function definitions
 * for routines described in string(3).
 */
char	*strcat();
char	*strncat();
int	strcmp();
int	strncmp();
char	*strcpy();
char	*strncpy();
long unsigned int	strlen();
char	*index();
char	*rindex();
