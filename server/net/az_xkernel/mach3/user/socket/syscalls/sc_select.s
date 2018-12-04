/*
 * $RCSfile: sc_select.s,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:15:40 $
 */
	.text
	.globl	sc_select

	.ent	sc_select

#include <sys/syscall.h>

sc_select:
	li	$2,SYS_select
	syscall
	beq	$7,$0,$L1
	nop
	j	_cerror
$L1:	j	$31

	.end	sc_select
