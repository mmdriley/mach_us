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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/lib/emul/i386/emul_address_space.cc,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Machine-dependent manipulation of the process address space:
 *	user space, emulation space, user stack.
 *
 * This file must be separate from emul_machdep, because it is also
 * needed for emul_init.
 *
 * Portions of the code in this file was copied from the equivalent module
 * in POE, containing the following entries:
 *	Author:	Joseph S. Barrera III, Randall W. Dean
 *	Copyright (c) 1990 Joseph S. Barrera III, Randall W. Dean
 *
 * HISTORY
 * $Log:	emul_address_space.cc,v $
 * Revision 2.3  94/07/08  16:12:41  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  91/11/13  16:45:28  dpj
 * 	First working version.
 * 	[91/11/12  17:49:27  dpj]
 * 
 */

#ifndef lint
char * emul_address_space_rcsid = "$Header: emul_address_space.cc,v 2.3 94/07/08 16:12:41 mrt Exp $";
#endif	lint

#include <base.h>

extern "C" {
#include <mach.h>
#include <mach/machine/vm_param.h>
#include <machine/vmparam.h>
}
#include <machine/machine_address_space.h>
#include <loader_info.h>

#define	NBPW		sizeof(int)	/* XXX */


/*
 * Parameters for UNIX stack to be created upon exec().
 *
 * May be changed at runtime.
 */
vm_offset_t		user_stack_base_actual;
vm_offset_t		user_stack_top_actual;
vm_size_t		user_stack_size_actual;
vm_offset_t		user_stack_base_new;
vm_offset_t		user_stack_top_new;
vm_size_t		user_stack_size_new;


/*
 * Find initial user stack and initialize user_stack_* global variables.
 */
mach_error_t find_initial_user_stack()
{
	/*
	 * We could probe, but why bother, since we are about to
	 * blow-away that stack anyway.
	 */

	user_stack_base_actual = round_page(USRSTACK - DFLSSIZ);
	user_stack_top_actual = USRSTACK;
	user_stack_size_actual = DFLSSIZ;

	user_stack_base_new = user_stack_base_actual;
	user_stack_top_new = user_stack_top_actual;
	user_stack_size_new = user_stack_size_actual;

	return(ERR_SUCCESS);
}


/*
 * Reset the user stack for exec().
 */
mach_error_t reset_user_stack()
{
	mach_error_t		err;

	(void) vm_deallocate(mach_task_self(),user_stack_base_actual,
					user_stack_size_actual);

	err = vm_allocate(mach_task_self(),&user_stack_base_new,
					user_stack_size_new,FALSE);
	if (err) return(err);
	user_stack_base_actual = user_stack_base_new;
	user_stack_top_actual = user_stack_top_new;
	user_stack_size_actual = user_stack_size_new;

	return(ERR_SUCCESS);
}


/*
 * Clean emulation space for initialization.
 */
mach_error_t clean_for_init()
{
	kern_return_t		kr;

	if (EMULATION_BASE_TEXT_ADDRESS < EMULATION_BASE_DATA_ADDRESS) {
		kr = vm_deallocate(mach_task_self(),
			EMULATION_BASE_TEXT_ADDRESS,
			user_stack_base_actual - EMULATION_BASE_TEXT_ADDRESS);
	} else {
		kr = vm_deallocate(mach_task_self(),
			EMULATION_BASE_DATA_ADDRESS,
			user_stack_base_actual - EMULATION_BASE_DATA_ADDRESS);
	}

	return(kr);
}


/*
 * Clean user address space before exec().
 */
mach_error_t clean_for_exec()
{
	kern_return_t		kr;

	if (EMULATION_BASE_TEXT_ADDRESS < EMULATION_BASE_DATA_ADDRESS) {
		kr = vm_deallocate(mach_task_self(),0,
						EMULATION_BASE_TEXT_ADDRESS);
	} else {
		kr = vm_deallocate(mach_task_self(),0,
						EMULATION_BASE_DATA_ADDRESS);
	}

	return(kr);
}


/*
 * Find the start of the argument block on the user stack.
 */
vm_offset_t set_arg_addr(vm_size_t arg_size)
{
	/*
	 * Round arg size to fullwords
	 */
	arg_size = (arg_size + NBPW-1) & ~(NBPW - 1);

	/*
	 * Put argument list at top of stack.
	 */
	return (user_stack_top_actual - arg_size);
}


/*
 * Decide if a given address is inside the emulation space or not.
 */
extern "C" boolean_t is_in_emulation_space(int addr)
{
	if ((addr >= EMULATION_BASE_TEXT_ADDRESS) &&
			(addr < user_stack_base_actual)) 
		return(TRUE);
	else
		return(FALSE);
}


/*
 * Adjust loader information for loading the emulation library
 */
mach_error_t adjust_loadinfo_for_emulator(
	struct loader_info	*li)
{
	li->data_start += EMULATION_BASE_TEXT_ADDRESS - li->text_start;
	li->text_start = EMULATION_BASE_TEXT_ADDRESS;

	return(ERR_SUCCESS);
}
