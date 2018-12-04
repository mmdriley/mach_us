/*
 * $RCSfile: sc_connect.s,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:14:31 $
 */
	.text
	.globl	sc_connect

	.ent	sc_connect

#include <sys/syscall.h>

sc_connect:
	li	$2,SYS_connect
	syscall
	beq	$7,$0,$L1
	nop
	j	_cerror
$L1:	j	$31

	.end	sc_connect
