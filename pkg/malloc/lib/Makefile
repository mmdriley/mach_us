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
# HISTORY
# $Log:	Makefile,v $
# Revision 2.3  94/06/29  15:03:17  mrt
# 	Updated for odemake. Condensed History.
# 	[94/03/06            mrt]
# 
# Revision 2.2  90/10/29  17:33:57  dpj
# 	Created.
# 	[90/10/27  18:02:11  dpj]
# 
# 	First working version.
# 	[90/10/21  21:34:16  dpj]
# 
#
######################################################################

DEPENDENCIES		=

IDIR			= /us/lib/
ILIST			= libmalloc.a

LIBRARIES		= libmalloc.a
EXPLIB_TARGETS		= export_libmalloc.a

OFILES			= malloc.o trace.o

INCFLAGS		= -I${TARGET_MACHINE}

CFLAGS			= -DTRACE -DDEBUG -DTRACE_DEPTH=8

.include <${RULES_MK}>

