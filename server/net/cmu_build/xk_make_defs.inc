
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
#
# 
# Purpose:  Make include file to define all of the magic switch and
#	build variables needed to build the xkernel parts of the
#	net server
# 
# HISTORY
# $Log:	xk_make_defs.inc,v $
# Revision 2.3  94/06/29  15:04:15  mrt
# 	Updated for odemake.
# 	[94/03/01            mrt]
# 
# Revision 2.2  94/01/11  18:11:01  jms
# 	Initial Version
# 	[94/01/10  13:23:53  jms]
# 
#
######################################################################
AZ_DIR			?= ${CMUBTOP}/../az_xkernel

PROTOCOL_DIR		= ${AZ_DIR}/protocols
PI_DIR			= ${AZ_DIR}/pi
XMACH3_DIR		= ${AZ_DIR}/mach3
PXK_DIR			= ${XMACH3_DIR}/pxk

CFLAGS			+= -DMACH=1 -Di386 \
			  -DLITTLE=1234 -DBIG=4321 -DENDIAN=LITTLE \
			  -DMACH3_MODS=1 -DMACH3_MULTI=1

# Put _MACH3_INCLUDES_ first to get the US cthreads stuff
INCFLAGS		+= -I${MACH3_INCLUDES} \
			  -I${CMUBTOP}/etc \
			  -I${XMACH3_DIR}/include -I${AZ_DIR}/include \
			  -I${AZ_DIR}/include/prot -I${PI_DIR}/include
