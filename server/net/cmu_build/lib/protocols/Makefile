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
# Purpose:  Makefile to build the xkernel protocols used by the netserver
# 
# HISTORY
# $Log:	Makefile,v $
# Revision 2.3  94/06/29  15:04:05  mrt
# 	Updated for odemake.
# 	[94/03/01            mrt]
# 
# Revision 2.2  94/01/11  18:09:24  jms
# 	Initial Version
# 	[94/01/09  21:05:01  jms]
# 
#
######################################################################
DEPENDENCIES		=
SAVE_D			=

CMUBTOP			?= ../..
MAKETOP			?= ${CMUBTOP}/../../../

CONFIGURATION		= MACH3_US

LIBRARIES		= libxkprotl.a

.include "${CMUBTOP}/xk_make_defs.inc"

ETH_DIR			= ${PROTOCOL_DIR}/eth
ARP_DIR			= ${PROTOCOL_DIR}/arp
VNET_DIR		= ${PROTOCOL_DIR}/vnet
IP_DIR			= ${PROTOCOL_DIR}/ip
ICMP_DIR		= ${PROTOCOL_DIR}/icmp
UDP_DIR			= ${PROTOCOL_DIR}/udp
TCP_DIR			= ${PROTOCOL_DIR}/tcp
UTIL_DIR		= ${PROTOCOL_DIR}/util

VPATH			= :tcp:${ARP_DIR}:${VNET_DIR}:${ICMP_DIR}

ETH_PROT_OFILES		= eth.o
ARP_PROT_OFILES		= arp.o arp_table.o arp_mach3.o
VNET_PROT_OFILES	= vnet.o
IP_PROT_OFILES		= ip.o iproute.o ip_gc.o ip_control.o ip_util.o \
			  ip_hdr.o ip_frag.o ip_rom.o ip_input.o
ICMP_PROT_OFILES	= icmp.o icmp_reqrep.o
UDP_PROT_OFILES		= udp.o udp_port.o
TCP_PROT_OFILES		= in_hacks.o sb.o tcp_debug.o tcp_subr.o tcp_timer.o \
			  tcp_usrreq.o tcp_x.o tcp_output.o tcp_input.o \
			  tcp_hdr.o tcp_port.o

OFILES		= ${ETH_PROT_OFILES} ${ARP_PROT_OFILES} \
		  ${VNET_PROT_OFILES} ${IP_PROT_OFILES} ${ICMP_PROT_OFILES} \
		  ${UDP_PROT_OFILES} ${TCP_PROT_OFILES}

#CFLAGS		= -E ${CFLAGS}

INCFLAGS	+= -I${UTIL_DIR} -I${ETH_DIR} -I${IP_DIR} \
		   -I${UDP_DIR} -I${TCP_DIR}

.include  <${RULES_MK}>



