/*
 * $RCSfile: sc_getsockopt.s,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:15:09 $
 */
	.text
	.globl	sc_getsockopt

	.ent	sc_getsockopt

#include <sys/syscall.h>

sc_getsockopt:
	li	$2,SYS_getsockopt
	syscall
	beq	$7,$0,$L1
	nop
	j	_cerror
$L1:	j	$31

	.end	sc_getsockopt
