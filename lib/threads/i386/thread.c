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
 * $Log:	thread.c,v $
 * Revision 2.7  94/07/08  13:32:30  mrt
 * 	Updated copyright
 * 
 * Revision 2.6  92/03/05  15:04:52  jms
 * 	No Changes
 * 	[92/02/26  18:22:43  jms]
 * 
 * Revision 2.5  91/07/01  14:11:05  jms
 * 	No Further Changes
 * 	[91/06/24  17:11:11  jms]
 * 
 * 	Fix cthread_do_handler bugs and modify to work with new 
 *	interrupt mechinism
 * 	[91/04/15  18:02:39  jms]
 * 
 * 	Fix the discription of fields in a jmp_buf and their uses.
 * 	[91/02/21  14:06:39  jms]
 * 
 * Revision 2.4  90/10/29  17:31:37  dpj
 * 	Correct MACH_CALL macro invocation to avoid embedded newlines.
 * 	[90/10/27  17:58:35  dpj]
 * 
 * 	Correct MACH_CALL macro invocation to avoid embedded newlines.
 * 	[90/10/21  21:27:04  dpj]
 * 
 * Revision 2.3  90/07/09  17:05:14  dorr
 * 	cthread_thread-stack and cthread_do_handler added
 * 	[90/07/06  16:30:11  jms]
 * 
 * Revision 2.2  90/03/14  17:29:24  orr
 * 	initial checkin.
 * 	[90/02/04  16:51:21  orr]
 * 
 */
/*
 * i386/thread.c
 *
 * Cproc startup for AT386 MTHREAD implementation.
 */

#ifndef	lint
static char rcs_id[] = "$Header: thread.c,v 2.7 94/07/08 13:32:30 mrt Exp $";
#endif	not lint



#include <cthreads.h>
#include "cthread_internals.h"
#include <interrupt.h>

#if	MTHREAD

#include <mach.h>
#include <mach_error.h>

/*
 * C library imports:
 */
extern bzero();

/*
 * Set up the initial state of a MACH thread
 * so that it will invoke cthread_body(child)
 * when it is resumed.
 */
void
cproc_setup(child)
	register cproc_t child;
{
	register int *top = (int *) (child->stack_base + child->stack_size);
	struct i386_thread_state state;
	register struct i386_thread_state *ts = &state;
	kern_return_t r;
	extern void cthread_body();

	/*
	 * Set up I386 call frame and registers.
	 * See I386 Architecture Handbook.
	 */
	bzero((char *) ts, sizeof(struct i386_thread_state));
	/*
	 * Inner cast needed since too many C compilers choke on the type void (*)().
	 */
	ts->eip = (int) (int (*)()) cthread_body;
	*--top = (int) child;	/* argument to function */
	*--top = 0;
	ts->uesp = (int) top;

	MACH_CALL(thread_set_state(child->id,i386_THREAD_STATE,(thread_state_t) &state,i386_THREAD_STATE_COUNT),r);
}


vm_offset_t
cthread_thread_stack(thread)
	register thread_t	thread;
{
	struct i386_thread_state state;
	int count = i386_THREAD_STATE_COUNT;
	int r;

	if( thread_get_state(thread, \
				   i386_THREAD_STATE, \
				   (thread_state_t) &state, \
				   &count) ) {
		return 0;
	}

	return state.uesp & cthread_stack_mask;

}


#define	JMP_BUF_EBX	0
#define	JMP_BUF_ESI	1
#define JMP_BUF_EDI	2
#define JMP_BUF_EBP	3
#define JMP_BUF_UESP	4
#define JMP_BUF_EIP	5
#define JMP_BUF_PC	JMP_BUF_EIP
/*
 *	force a thread to execute a longjmp handler
 */
cthread_do_handler(thread, h, val)
	thread_t		thread;
	intr_handler_t		h;
	mach_error_t		val;
{
	struct i386_thread_state	regs;
	int		count;
	mach_error_t	err;

	/*
	 *  make sure that the thread isn't blocked in some
	 *  nasty old mach call so that it'll run our handler
	 *  when it wakes up
	 */
	(void)thread_abort(thread);

	/* Find out where we are */
	count = i386_THREAD_STATE_COUNT;
	err = thread_get_state(thread, i386_THREAD_STATE, &regs, 
			       &count);
	if (err) goto finish;

	/* Save abort return.  Might contain important msg interrupt info */
	h->abort_return = regs.eax;

	/*
	 * make the target thread call a longjmp handler
	 */


	regs.eip = h->jmpbuf[JMP_BUF_PC];

	regs.eax = (int)val;

	regs.edi = h->jmpbuf[JMP_BUF_EDI];
	regs.esi = h->jmpbuf[JMP_BUF_ESI];
	regs.ebp = h->jmpbuf[JMP_BUF_EBP];
	regs.ebx = h->jmpbuf[JMP_BUF_EBX];
	regs.uesp = h->jmpbuf[JMP_BUF_UESP];
 

	err = thread_set_state(thread, i386_THREAD_STATE, &regs, 
			       i386_THREAD_STATE_COUNT);

    finish:
	return err;

}
#endif	MTHREAD
