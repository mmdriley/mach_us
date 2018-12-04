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
# Revision 2.2  94/06/29  15:02:09  mrt
# 	Created to export the .h and .defs
# 
# 	HISTORY
# 	[94/02/09            mrt]
# 

DEPENDENCIES	=

CONFIGURATION	= ${MACH_CONFIGURATION:UMACH_US}

MIG_HDRS	= mach_diag.h auth.h
DATAFILES	= auth_types.defs ${MIG_HDRS:S/.h/.defs/g}

INCLUDES	= ${MIG_HDRS} ${DATAFILES}

EXPDIR		= /us/include/servers/

.include <${RULES_MK}>
