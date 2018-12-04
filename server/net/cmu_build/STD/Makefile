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
# Purpose:  Actually construct the netserver executable.
# 
# HISTORY
# $Log:	Makefile,v $
# Revision 2.3  94/06/29  15:03:53  mrt
# 	Updated for odemake.
# 	[94/03/01            mrt]
# 
# Revision 2.2  94/01/11  18:08:54  jms
# 	Initial Version
# 	[94/01/09  20:15:07  jms]
# 
#
######################################################################
DEPENDENCIES		=
SAVE_D			=

CMUBTOP			?= ..
MAKETOP			?= ${CMUBTOP}/../../../

CONFIGURATION		= MACH3_US

IDIR			= /us/bin/

PROGRAMS		= net_server
ILIST			= net_server

VPATH			= :${CMUBTOP}/user
COMPOSE			= ${CMUBTOP}/util/compose/compose
INCFLAGS		= -I${CMUBTOP}/user

SERVER_OFILES		= xk_main.o net_init.o net_dir.o utils.o

US_PROTL_OFILES		= usx_dir.o usx_endpt_base.o \
			  usudp_dir.o usudp_clts.o usudp_cots.o \
			  usudp_protocol.o \
			  ustcp_dir.o ustcp_connector.o ustcp_cots.o \
			  ustcp_protocol.o \
			  usx_iobuf_mgr.o usx_internal.o

PRIV_OFILES		= protocols.o protTbl.o traceLevels.o

OFILES			= ${PRIV_OFILES} \
			  ${US_PROTL_OFILES} \
			  ${SERVER_OFILES}

LIBS			= -lus++ \
			  ${CMUBTOP}/lib/protocols/libxkprotl.a \
			  ${CMUBTOP}/lib/mach3/libxkmach3.a \
			  ${CMUBTOP}/lib/pi/libxkpi.a \
			  ${LIBTHREADS} ${LIBMACH3} -lc++


.ifdef DEBUG
  CFLAGS	= -DDEBUG=1
.else
  CFLAGS	= -DNDEBUG -DMSGOPT
.endif

CPlusPlusFLAGS		= ${CFLAGS}

.include "${CMUBTOP}/xk_make_defs.inc"

# Should be the same as from osf.us.mk except without "-nostdinc"
DEF_CPlusPlusFLAGS	= -MD -DMI=1 ${DEBUGFLAGS} ${_MACH3_DEFINES_}


xk_main.o traceLevels.o: protocols.h


.include  <${RULES_MK}>

protocols.c protocols.h protTbl.c traceLevels.c : comp.stamp
comp.stamp: graph.comp
	${COMPOSE} -m /dev/null < ${graph.comp:P}
