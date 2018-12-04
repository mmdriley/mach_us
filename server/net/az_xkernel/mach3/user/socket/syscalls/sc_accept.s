/*
 * $RCSfile: sc_accept.s,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:14:14 $
 */
	.text
	.globl	sc_accept

	.ent	sc_accept

#include <sys/syscall.h>

sc_accept:
	li	$2,SYS_accept
	syscall
	beq	$7,$0,$L1
	nop
	j	_cerror
$L1:	j	$31

	.end	sc_accept
