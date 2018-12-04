#
# socket/xk Makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.8 $
# $Date: 1993/02/02 00:08:05 $
#

DIRS = gen
FILES = msg_stream util xk_msg_server xksocket xsi_main xsi_notify \
	xsi_services xsi_bench

WARNINGS := 

INCLUDES := $(INCLUDES) -Igen

include $(MAKE_INCLUDE)/Makefile.common


#
# localdepend (and the other loop rules) are run with preorder
# traversal.  This rule forces generation of a header file in a
# subdirectory first.  Postorder would be nice ...
#

localdepend: gen/xsi_user.h

gen/xsi_user.h:	xsi.defs
	cd gen; make XRT:=../$(XRT) xsi_user.h
