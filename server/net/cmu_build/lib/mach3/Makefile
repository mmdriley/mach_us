# 
# Mach Operating System
# Copyright (c) 1993-1988 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon 
# the rights to redistribute these changes.
#  
# 
# Purpose:  Build the Mach3 specific parts of the xkernel software
# 
# HISTORY
# $Log:	Makefile,v $
# Revision 2.3  94/06/29  15:03:59  mrt
# 	Updated for odemake.
# 	[94/03/01            mrt]
# 
# Revision 2.2  94/01/11  18:09:04  jms
# 	Initial Version
# 	[94/01/09  20:17:55  jms]
# 
######################################################################
DEPENDENCIES		=
SAVE_D			=

CMUBTOP			?= ../..
MAKETOP			?= ${CMUBTOP}/../../../

CONFIGURATION		= MACH3_US

LIBRARIES		= libxkmach3.a
#OTHERS			= userrt0.o

.include  "${CMUBTOP}/xk_make_defs.inc"

ETHDRV_DIR		= ${XMACH3_DIR}/drivers/eth

VPATH			= :${PXK_DIR}:${ETHDRV_DIR}

ETHDRV_PROT_OFILES	= ether.o

PXK_OFILES	= alloc.o input_process.o event.o init.o ocsum.o \
		  process.o time.o trace.o utils.o \
		  insque.o
# machine.o

OFILES			= ${ETHDRV_PROT_OFILES} ${PXK_OFILES}

CFLAGS			+= -DTEST_TYPE_FILTER
INCFLAGS		+= -I${PROTOCOL_DIR}/util -I${PROTOCOL_DIR}/eth

.include  <${RULES_MK}>



