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
# Purpose:  Build the Protocol Independent parts of the xkernel
# 
# HISTORY
# $Log:	Makefile,v $
# Revision 2.4  94/07/19  18:58:33  mrt
# 	Moved ptbl_static to local directory to remove
# 	printf statment.
# 
# Revision 2.3  94/06/29  15:04:02  mrt
# 	Updated for odemake.
# 	[94/03/01            mrt]
# 
# Revision 2.2  94/01/11  18:09:18  jms
# 	Initial Version
# 	[94/01/09  20:56:56  jms]
# 
#
######################################################################

CMUBTOP			?= ../..
MAKETOP			?= ${CMUBTOP}/../../../

DEPENDENCIES		=
SAVE_D			=

CONFIGURATION		= MACH3_US

LIBRARIES		= libxkpi.a

.include "${CMUBTOP}/xk_make_defs.inc"

IDMAP_DIR		= ${PI_DIR}/idmap

VPATH			= :${PI_DIR}:${IDMAP_DIR}

PI_OFILES		= msg.o hoststr.o list.o netmask.o part.o prottbl.o \
			  sessn_gc.o upi.o upi_defaults.o \
			  xk_debug.o event_monitor.o rwlock.o ptbl_static.o

IDMAP_OFILES		= idmap.o

OFILES			= ${PI_OFILES} ${GEN_OFILES} ${IDMAP_OFILES}

.include  <${RULES_MK}>



