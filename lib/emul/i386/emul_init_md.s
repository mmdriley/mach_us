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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/lib/emul/i386/emul_init_md.s,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Stack switching code for first emulation stack at
 *	initialization time.
 *
 * HISTORY
 * $Log:	emul_init_md.s,v $
 * Revision 2.3  94/07/08  16:12:43  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  91/11/13  17:15:42  dpj
 * 	First working version.
 * 	[91/11/12  17:49:41  dpj]
 * 
 */

	.align	2
	.globl	_emul_init_switch_stack
_emul_init_switch_stack:
	movl	4(%esp), %eax		/* argument: new stack */
	movl	%esp, %edx
	movl	%ebp, %ecx
	movl	%eax, %esp		/* switch stack */
	movl	%esp, %ebp		/* initial fp = sp ? */
	pushl	%edx			/* save old sp on new stack */
	pushl	%ecx			/* save old fp on new stack */
	call _call_emul_initialize
	popl	%ecx
	popl	%edx
	movl	%ecx, %ebp		/* restore old fp */
	movl	%edx, %esp		/* restore old sp */
	ret
