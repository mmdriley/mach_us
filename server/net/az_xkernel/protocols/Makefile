#
# protocols Makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.31.1.1 $
# $Date: 1993/02/02 19:46:59 $
#

mach3_PROTDIRS = machnetipc

DIRS = $($(XPLATFORM)_PROTDIRS) arp bid bidctl blast chan eth icmp ip select \
	tcp test udp vcache vchan vdrop vmux vnet vsize

XRT=..
LIB=libProt_$(HOW)

OBJECTFILE = newObjectList

include $(MAKE_INCLUDE)/Makefile.lib
