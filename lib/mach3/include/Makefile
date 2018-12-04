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
# Revision 2.4  94/06/29  15:01:19  mrt
# 	Updated for odemake. Added all the .h and .defs files that
# 	should be exported from the lib/mach3 directories
# 	[94/03/04            mrt]
# 
# Revision 2.3  93/01/20  17:36:58  jms
# 	Add files mach_exception.h exception_all.h for installation
# 	[93/01/18  16:04:50  jms]
# 
# Revision 2.2  91/12/20  17:43:33  jms
# 	Used to install include files before building the rest of mach3
# 	[91/12/20  15:25:38  jms]
# 
#
######################################################################

DEPENDENCIES		=
SAVE_D			=

# us/include/mach for notify.defs
VPATH			= ../mach3:${EXPORTBASE}/us/include/mach
# for auth_types.defs, adds it to VPATH as well
INCFLAGS		= -I../us

IDIR			= /us/include/servers/

MIG_HDRS		= netname.h service.h notify.h \
			  mach_diag.h auth.h

DATAFILES		= ${MIG_HDRS:S/.h$/.defs/g} auth_types.defs

INCLUDES		= ${MIG_HDRS} ${DATAFILES} netname_defs.h \
			   errorlib.h mach_exception.h exception_all.h

errorlib.h_IDIR		= /us/include/
mach_exception.h_IDIR	= /us/include/
exception.all.h_IDIR	= /us/include/

.include <${RULES_MK}>
