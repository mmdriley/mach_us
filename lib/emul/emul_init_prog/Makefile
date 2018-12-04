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

# File:        Makefile
#
# Purpose:  Build file for program to start a "root" emulated process
#
# HISTORY
# $Log:	Makefile,v $
# Revision 2.11  94/06/29  15:01:06  mrt
# 	Updated for odemake. Removed ancient history.
# 	[94/02/24            mrt]
# 
# Revision 2.10  94/05/17  14:06:17  jms
# 	# Revision 2.8.2.1  94/02/18  11:21:47  modh
# 	# 	Needed to add -lc for 2.3.3 g++ compiler
# 	[94/04/28  18:15:27  jms]
# 
# Revision 2.8.2.1  94/02/18  11:21:47  modh
# 	Needed to add -lc for 2.3.3 g++ compiler
# 
# Revision 2.9  94/01/11  17:48:48  jms
# 	Add use of "proxies"
# 	[94/01/09  18:33:08  jms]
# 
# Revision 2.8  92/07/05  23:25:04  dpj
# 	Removed libmachobj++.
# 	[92/05/10  00:30:17  dpj]
# 
# Revision 2.7  91/11/13  16:39:15  dpj
# 	Removed all uses of libload, replaced by local loader.
# 	Removed special memory package.
# 	[91/11/08            dpj]
# 
# 	Use "memory" ofiles directly, no more "libmem"
# 	[91/05/07  11:23:35  jms]
# 
#
######################################################################

DEPENDENCIES		=
SAVE_D			=

CONFIGURATION		= MACH3_US

IDIR			= /us/bin/

PROGRAMS		= emul_init
ILIST			= emul_init

OFILES			= emul_init.o emul_user_misc.o emul_user_init.o \
			  emul_address_space.o loader.o loader_md.o \
			  emul_init_md.o

CFLAGS			=  -DFAKE_EVENTS=1 \
			  -DSTACK_SWITCH=1 -DSIGCHLD_WAIT=1 -DTaskMaster=1

CPlusPlusFLAGS		= ${CFLAGS}

LIBS			= -lproxies -lus++ ${LIBTHREADS} \
			  ${LIBMACH3} -lc -lcs ${LIBCPlusPlus}

VPATH			= :../${target_cpu}:..
INCFLAGS		= -I.. -I${UNIX_INCLUDE}

.include <${RULES_MK}>

emul_init.o: emul_config.h

emul_config.h:
	@echo "configuring for type all"
	@echo "#define emul_type_all 1" > emul_config.h
	@echo '#define EMUL_NAME "bsd_all"' >> emul_config.h


#.s.o: 
#	@echo ""
#	${_ASCPP_} -DASSEMBLER ${_CPPFLAGS_} ${.IMPSRC} > ${.TARGET:S/.o$/.as/}; \
#	${_AS_} ${_ASFLAGS_} -o ${.TARGET} ${.TARGET:S/.o$/.as/}
#	${RM} -f ${.TARGET:S/.o$/.as/}
