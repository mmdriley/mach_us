# 
# TCP Makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.8 $
# $Date: 1993/02/01 22:25:40 $

	
FILES = in_hacks sb tcp_debug tcp_subr tcp_timer tcp_usrreq tcp_x	\
	tcp_output tcp_input tcp_hdr tcp_port
DIRS =

ifeq ($(CC),gcc)

#
# tcp_debug does some string expansion within macros that requires the
# -traditional flag, so it needs its own rule.
#

default: $(HOW)/tcp_debug.o $(OBJ)

$(HOW)/tcp_debug.o : tcp_debug.c
	@if [ ! -d $(HOW) ] ; then mkdir $(HOW) ; fi ; \
	echo $(CC) -c $(CFLAGS) -traditional -o $@ $< ; \
	$(CC) -c $(CFLAGS) -traditional -o $@ $<


endif


INCLUDES := $(INCLUDES) -I../util

include $(MAKE_INCLUDE)/Makefile.common
