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

# HISTORY
# $Log:	Makefile,v $
# Revision 2.6  94/06/29  15:01:22  mrt
# 	Updated for odemake.
# 	[94/02/09            mrt]
# 
# Revision 2.5  92/03/05  14:56:40  jms
# 	Must build mach_sa subdir first
# 	[92/02/26  17:43:25  jms]
# 
# 
#
######################################################################
DEPENDENCIES	=
SAVE_D		=

CONFIGURATION	= MACH3_US

IDIR		= /us/lib/
ILIST		= libmach3_us.a

TARGET_NAME	= libmach3_us.a

EXPLIB_TARGETS	= export_libmach3_us.a

VPATH		= ../mach_sa:../mach3/${TARGET_MACHINE}:../unix:../unix/${TARGET_MACHINE}
# adds them to VPATH as well
INCFLAGS	= -I../mach3 -I../us

.include "../mach_sa/Makefile-include"
.include "../mach3/Makefile-include"
.include "../unix/Makefile-include"
.include "../unix/${TARGET_MACHINE}/Makefile-machdep"
.include "../us/Makefile-include"

OBJECTS		= ${MACH_SA_OFILES} ${MACH3_OFILES} ${UNIX_OFILES} ${US_OFILES}

.include <${RULES_MK}>

${SHORTNAMES}: $${${.TARGET}_LONGNAME}
	${RM} -f ${.TARGET}
	${CP} ${${${.TARGET}_LONGNAME}:P} ${.TARGET}

${TARGET_NAME} : ${TARGET_NAME}_1 ${OBJECTS}
	${RM} -f ${.TARGET}
	${CP} ${TARGET_NAME}_1 ${.TARGET}
	${_AR_} ${DEF_ARFLAGS} ${.TARGET} ${.ALLSRC:M*.o}
	${_RANLIB_} ${.TARGET}
	@echo "${_RANLIB_} ${.TARGET} done"






