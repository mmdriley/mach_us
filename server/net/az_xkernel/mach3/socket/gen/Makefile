#
# $RCSfile: Makefile,v $
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.2 $
# $Date: 1993/02/02 00:12:57 $
#


DIRS =
FILES = xsi_server
INCLUDES := $(INCLUDES) -I..

xsi_user.h xsi_user.c xsi_server.c: ../xsi.defs
	mig -server xsi_server.c -user xsi_user.c -header xsi_user.h $^

include $(MAKE_INCLUDE)/Makefile.common
