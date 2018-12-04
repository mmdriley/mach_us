/*
/*
 * Mach Operating System
 * Copyright (c) 1994 Carnegie Mellon University
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
/* compose_cmu.h
 * Created from: compose_mips.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1991  Arizona Board of Regents
 *
 *
 * $Revision: 2.3 $
 * $Date: 94/07/13 17:56:02 $
 */

/*
 * Purpose: Where to find the "system" "h" files in a CMU build env.
 *
 * HISTORY:
 * $Log:	compose_cmu.h,v $
 * Revision 2.3  94/07/13  17:56:02  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:52  jms
 * 	Initial Version
 * 	[94/01/10  13:20:39  jms]
 * 
 *
 */
#include <string.h>
/*#include <unistd.h> */
#include <strings.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/dir.h>
#define DIRENT direct
#include <sys/stat.h>

int	unlink( char * );
int	access( char *, int );
