/*
 * $RCSfile: sc_readv.s,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:15:30 $
 */
	.text
	.globl	sc_readv

	.ent	sc_readv

#include <sys/syscall.h>

sc_readv:
	li	$2,SYS_readv
	syscall
	beq	$7,$0,$L1
	nop
	j	_cerror
$L1:	j	$31

	.end	sc_readv
