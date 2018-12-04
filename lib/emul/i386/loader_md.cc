/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991 Carnegie Mellon University
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/lib/emul/i386/loader_md.cc,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Simple program loader -- machine dependent portion.
 *
 * Most of the code in this file was copied from the equivalent module
 * in POE, containing the following entries:
 *	Author:	Joseph S. Barrera III, Randall W. Dean
 *	Copyright (c) 1990 Joseph S. Barrera III, Randall W. Dean
 *
 * HISTORY
 * $Log:	loader_md.cc,v $
 * Revision 2.3  94/07/08  16:12:51  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  91/11/13  17:16:55  dpj
 * 	First working version.
 * 	[91/11/12  17:50:09  dpj]
 * 
 */

#ifndef lint
char * loader_md_rcsid = "$Header: loader_md.cc,v 2.3 94/07/08 16:12:51 mrt Exp $";
#endif	lint

#include <base.h>
#include <mach.h>
#include <mach/error.h>
#include <io_error.h>
#include <us_byteio_ifc.h>
#include <loader_info.h>

#include <errno.h>
#include <sys/exec.h>


/*
 *	Object:
 *		ex_get_header			EXPORTED function
 *
 *		Reads the exec header for the loader's benefit
 *
 */
mach_error_t loader_ex_get_header(
	usByteIO*		exec_obj,
	struct loader_info	*lp)
{
	mach_error_t		err;
	struct exec		x;
	io_offset_t		offset;
	unsigned int		len;

	UINT_TO_IO_OFF(0,&offset);
	len = sizeof(x);
	err = exec_obj->io_read(0,offset,(pointer_t)&x,&len);
	if (err) {
		if (err == IO_INVALID_SIZE)
			return(unix_err(ENOEXEC));
		else
			return (err);
	}

	switch ((int)x.a_magic) {

	    case 0407:
		lp->format = EX_READIN;
		lp->text_start  = 0;
		lp->text_size   = 0;
		lp->text_offset = 0;
		lp->data_start  = 0x10000;
		lp->data_size   = x.a_text + x.a_data;
		lp->data_offset = sizeof(struct exec);
		lp->bss_size    = x.a_bss;
		break;

	    case 0410:
		if (x.a_text == 0) {
			return(unix_err(ENOEXEC));
		}
		lp->format = EX_SHAREABLE;
		lp->text_start  = 0x10000;
		lp->text_size   = x.a_text;
		lp->text_offset = sizeof(struct exec);
		lp->data_start  = lp->text_start + lp->text_size;
		lp->data_size   = x.a_data;
		lp->data_offset = lp->text_offset + lp->text_size;
		lp->bss_size    = x.a_bss;
		break;

	    case 0413:
		if (x.a_text == 0) {
			return(unix_err(ENOEXEC));
		}
		lp->format = EX_PAGEABLE;
		lp->text_start  = 0x10000;
		lp->text_size   = sizeof(struct exec) + x.a_text;
		lp->text_offset = 0;
		lp->data_start  = lp->text_start + lp->text_size;
		lp->data_size   = x.a_data;
		lp->data_offset = lp->text_offset + lp->text_size;
		lp->bss_size    = x.a_bss;
		break;
	    default:
		return(unix_err(ENOEXEC));
	}
	lp->entry_1 = x.a_entry;
	lp->entry_2 = 0;

	return(ERR_SUCCESS);
}


/*
 * Find the entry point of an executable.
 */
mach_error_t loader_set_entry_address(
	struct loader_info *lp,
	int		*entry,		/* pointer to OUT array */
	unsigned int	*entry_count)	/* out */
{
	entry[0] = lp->entry_1;
	*entry_count = 1;

	return(ERR_SUCCESS);
}
