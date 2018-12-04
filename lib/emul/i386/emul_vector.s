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
 **********************************************************************
 * Author:  Alessandro Forin (af) at Carnegie-Mellon University
 *
 * HISTORY
 * $Log:	emul_vector.s,v $
 * Revision 2.10  94/07/08  16:12:48  mrt
 * 	Updated copyright
 * 
 * Revision 2.9  92/03/05  14:56:23  jms
 * 	Change name of emul_stack_list => emul_stack_free_list
 * 	Clear the free list lock in a newly forked child
 * 	[92/02/26  17:31:41  jms]
 * 
 * Revision 2.8  91/11/13  17:16:53  dpj
 * 	Removed call_emul_initialize(). The stack switch is now done
 * 	in emul_init.
 * 	[91/11/08            dpj]
 * 
 * Revision 2.7  91/10/06  22:27:09  jjc
 * 	Picked up from Dan Julin:
 * 	Replaced emulation vectors to avoid the "EE" array.
 * 	Added a routine to switch stacks before calling emul_initialize().
 * 	[91/10/01            jjc]
 * 
 * Revision 2.6  90/12/21  13:52:46  jms
 * 	Fixed child_fork routine to complete fork syscall return.
 * 	Return values were not being set properly.
 * 	[90/12/17  14:28:51  neves]
 * 	merge for roy/neves @osf
 * 	[90/12/20  13:16:27  jms]
 * 
 * Revision 2.5  90/11/27  18:19:06  jms
 * 	Add code caleed when starting a child to start it on the right way on the right
 * 	stack.
 * 	[90/11/20  13:06:48  jms]
 * 
 * 	Added child_fork to call the initial child routine after for forking.
 * 	[90/08/20  17:08:35  jms]
 * 
 * Revision 2.4  90/08/22  18:07:45  roy
 * 	Remove include of emul_vector.s.
 * 	[90/08/21  09:34:43  roy]
 * 
 * Revision 2.3  90/07/09  17:01:27  dorr
 * 	Add "initial emulator stack" logic.
 * 	[90/07/06  13:48:09  jms]
 * 
 * Revision 2.2  90/03/21  17:13:54  jms
 * 	First working version
 * 
 * Revision 2.2  89/10/06  13:45:52  mbj
 * 	Hacked BSD-server version to make multi-server version.
 * 	[89/10/05  16:34:11  mbj]
 * 
 * 	Version for the multi-server emulator.
 * 	[89/09/11  16:08:57  mbj]
 * 
 * Revision 2.2  89/08/09  14:35:23  rwd
 * 	Fixed register saving and child init carry clear.
 * 	[89/06/22            rwd]
 * 
 * 10-May-89  David Golub (dbg) at Carnegie-Mellon University
 *	Created SUN3 version from VAX version.
 *
 * 29-Apr-89  David Golub (dbg) at Carnegie-Mellon University
 *	Switch to emulator-mode stack to avoid problems with user code
 *	with small stacks (VICE) and signal return bugs (thread_abort).
 *	Move most of the work into C code.
 *
 * 13-Apr-89  David Golub (dbg) at Carnegie-Mellon University
 *	Added more individual entry points.
 *
 * 10-Apr-89  David Golub (dbg) at Carnegie-Mellon University
 *	getrusage(), sigreturn(), osigcleanup() now call individual
 *	routines.
 *
 *  7-Apr-89  David Golub (dbg) at Carnegie-Mellon University
 *	Added entry points for execv, execve.  Made fork and vfork call
 *	e_fork.
 *
 * DBG - mangled for temporary emulator.
 */

#include <sys/syscall.h>
#include <sys/errno.h>

/*
 * Emulator vector entry - allocate new stack
 */
	.data
	.globl	_emul_stack_lock
_emul_stack_lock:
	.long	0
	.text


/*
 * Generic call
 */
	.globl	_emul_common
_emul_common:
	pushl	%eax			/ save registers that C does not
	pushl	%ecx
	pushl	%edx
0:	movl	$1,%eax
	xchg	%eax,_emul_stack_lock	/ lock emul_stack lock
	orl	%eax,%eax
	jne	0b
	movl	_emul_stack_free_list,%eax	/ grab an emulator stack
	orl	%eax,%eax
	jne	3f			/ if none:
	movl	$0,_emul_stack_lock	/   release the lock
	call	_emul_stack_alloc	/   allocate a stack
	jmp	4f			/   and use it
3:	movl	(%eax),%ecx		/   else
	movl	%ecx,_emul_stack_free_list	/   remove the stack from list
	movl	$0,_emul_stack_lock	/   and unlock the lock
4:	movl	%esp,(%eax)		/ Save user's SP at top of new stack
	movl	%eax,%esp		/ switch to emulator's stack
	pushl	%ebx			/ save the remaining registers
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushl	%esp			/ push address of registers
	call	_emul_syscall		/ call C code
	addl	$4,%esp			/ pop parameter
/*
 * Return
 */
	.globl	emul_exit
emul_exit:
	popl	%ebp			/ restore registers
	popl	%edi
	popl	%esi
	popl	%ebx
	movl	%esp,%ecx		/ save emulator stack address
	movl	(%esp),%esp		/ return to user's stack

0:	movl	$1,%eax
	xchg	%eax,_emul_stack_lock	/ lock emul_stack lock
	orl	%eax,%eax
	jne	0b
	movl	_emul_stack_free_list,%eax
	movl	%eax,(%ecx)		/ chain emulator stack on list
	movl	%ecx,_emul_stack_free_list
	movl	$0,_emul_stack_lock	/ release the lock
	movb	$0,_sig_unsafe		/ signals ok now

	popl	%edx			/ restore regs that C does not save
	popl	%ecx
	popl	%eax
	popf				/ restore flags
	ret				/ return to user

/*
 * Child_fork:  Completes emul_common so that the "emul_child_init" routine
 * was called as if it were just another syscall emulation.
 * Child_fork is the routine started (on the users stack) after
 * a "fork".
 */
	.globl	_child_fork
_child_fork:
#if REGS_NOT_PUSHED_BY_EMUL_COMMON
	pushl	%eax
	pushl	%ecx
	pushl	%edx			/ save regs c does not
#endif REGS_NOT_PUSHED_BY_EMUL_COMMON
	movl	$0,_emul_stack_lock	/ clear the lock
	movl	_emul_first_cthread_stack, %eax
					/ Get "initial" emulator stack
	movl	%esp,(%eax)		/ Save user's SP at top of new stack
	movl	%eax,%esp		/ switch to emulator's stack
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushl	%esp			/ push address of registers

	call	_child_init_md		/ call C code
	addl	$4,%esp			/ pop parameter
	jmp	emul_exit	

/*
 * Get segment registers
 */
	.globl	_get_seg_regs
_get_seg_regs:
	movl	4(%esp),%edx	/ point to register buffer
	xorl	%eax,%eax	/ clear high 16 bits
	mov	%cs,%ax
	movl	%eax,0(%edx)	/ cs to regs[0]
	mov	%ss,%ax
	movl	%eax,4(%edx)	/ ss to regs[1]
	mov	%ds,%ax
	movl	%eax,8(%edx)	/ ds to regs[2]
	mov	%es,%ax
	movl	%eax,12(%edx)	/ es to regs[3]
	mov	%fs,%ax
	movl	%eax,16(%edx)	/ fs to regs[4]
	mov	%gs,%ax
	movl	%eax,20(%edx)	/ gs to regs[5]
	ret

#if	XXX
/*
 * Calls that do not fit in the table:
 */
	.globl	_emul_task_by_pid
_emul_task_by_pid:
	pea	_emul_vector_base+(8+(8*(-33)))
	jmp	_emul_common

	.globl	_emul_htg_syscall
_emul_htg_syscall:
	pea	_emul_vector_base+(8+(8*(-59)))
	jmp	_emul_common
#endif	XXX
