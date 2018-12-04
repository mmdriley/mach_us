/*
 * $RCSfile: sc_send.s,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:15:45 $
 */
	.text
	.globl	sc_send

	.ent	sc_send

#include <sys/syscall.h>

sc_send:
	li	$2,SYS_send
	syscall
	beq	$7,$0,$L1
	nop
	j	_cerror
$L1:	j	$31

	.end	sc_send
