/*
 * $RCSfile: sc_fork.s,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:14:42 $
 */
	.text
	.globl	sc_fork

	.ent	sc_fork

#include <sys/syscall.h>

sc_fork:
	li	$2,SYS_fork
	syscall
	beq	$7,$0,$L1
	nop
	j	_cerror
$L1:	beq	$3,$0,$L2
	move	$2,$0
$L2:	j	$31

	.end	sc_fork
