#
# $RCSfile: Makefile,v $
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.6 $
# $Date: 1993/02/04 07:38:32 $
#

ifeq ($(HOWTOCOMPILE),) 

default:
	@echo Make variable $$\(HOWTOCOMPILE\) must be defined

else

CC	= gcc
XPLATFORM = mach3
#HOWTOCOMPILE = DEBUG
#HOWTOCOMPILE = OPTIMIZE
HOW = $(HOWTOCOMPILE)$(XPLATFORM)
CFLAGS	= -g -O
XSILIBS	= -L.. -L/usr/mach/lib -lxksocket_$(HOW) -lnetname -lthreads
SYSLIBS	= -lmach

PGMS = xk-rtrip xk-truput mach-rtrip mach-truput

% :: RCS/%,v

default: $(addprefix $(HOW)/, $(PGMS))


$(HOW)/mach-%: %.c
	@-if [ ! -d $(HOW) ] ; then mkdir $(HOW); fi ;
	$(CC) $(CFLAGS) -o $@ $^ $(SYSLIBS)

$(HOW)/xk-%: %.c
	@-if [ ! -d $(HOW) ] ; then mkdir $(HOW); fi ; 
	$(CC) $(CFLAGS) -o $@ $^ $(XSILIBS) $(SYSLIBS)

