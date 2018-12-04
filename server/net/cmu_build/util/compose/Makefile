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
# Purpose:  Build the "compose" program (See xkernel manual)
# 
# HISTORY
# $Log:	Makefile,v $
# Revision 2.4  94/06/29  15:04:11  mrt
# 	Updated for odemake.
# 	[94/03/01            mrt]
# 
# Revision 2.3  94/05/17  14:10:03  jms
# 	Add "-lgcc"
# 	[94/04/29  13:39:43  jms]
# 
# Revision 2.2  94/01/11  18:10:50  jms
# 	Initial Version
# 	[94/01/10  13:16:04  jms]
# 
#
######################################################################

DEPENDENCIES		=
SAVE_D			=

CMUBTOP			?= ../..
MAKETOP			?= ${CMUBTOP}/../../../

CONFIGURATION		= MACH3_VUS

CTYPE			= HOST

.include "${CMUBTOP}/xk_make_defs.inc"

VPATH			= :${AZ_DIR}/util/compose

PROGRAMS		= compose
OFILES			= parse.o compose.o util.o error.o

# Overide CFLAGS and INCFLAGS set in xk_make_defs.inc
CFLAGS			= -DMACH3_MODS=1 -fwritable-strings -DARCH_INCLUDE=\"compose_cmu.h\"
INCFLAGS		= -I${AZ_DIR}/util/compose

.include  <${RULES_MK}>

