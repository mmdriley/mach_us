/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990 Carnegie Mellon University
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
 * $Log:	syscall.s,v $
 * Revision 2.3  94/07/08  17:58:00  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  90/07/26  12:36:32  dpj
 * 	First version
 * 	[90/07/24  14:23:47  dpj]
 * 
 *
 */

/*
 * Invoke an indirect UNIX trap, to access any potential UNIX emulator.
 */

	.globl	_mach3_syscall
	.align	2
_mach3_syscall:
	popl	%edx		/ ret addr
	popl	%eax		/ syscall number
	pushl	%edx
	.byte 0x9a; .long 0; .word 0x7
	movl	(%esp),%edx	/ add one element to stack so
	pushl	%edx		/ caller "pop" will work
	jb	1f
	ret

1:
	movl	$-1,%eax
	ret
