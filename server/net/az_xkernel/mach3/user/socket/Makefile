#
# $RCSfile: Makefile,v $
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.9 $
# $Date: 1993/02/04 03:21:22 $
# $Author: menze $
#
# $Log: Makefile,v $
# Revision 1.9  1993/02/04  03:21:22  menze
# Modified the default rule to create the object directory if it doesn't
# exist.
#
# Revision 1.8  1993/02/02  00:16:45  menze
# copyright change
#
# Revision 1.7  1993/01/26  08:14:09  menze
# Minor Makedep tweak
#
# Changed source tree location
#
# Revision 1.6  1992/12/07  22:00:57  menze
# Now builds the test directory
#
# Revision 1.5  1992/12/04  22:10:22  menze
# Modified to not only rebuild library if necessary
#
# Revision 1.4  1992/12/01  23:23:46  menze
# *** empty log message ***
#
# Revision 1.3  1992/08/15  01:19:59  davidm
# select() totally revised---didn't work when user passed fd_sets which
# were smaller than sizeof(fd_set); it is common practice (?) to use
# just an "int" instead of fd_set
#
# Support for signal handling added.  If a server call returns EINTR,
# the library checks the BSD server for pending signals by executing
# a NOP system call (sigblock(sigmask(SIGKILL))).
#
# Revision 1.2  1992/07/22  18:41:30  davidm
# removed xsi_user.c from SRCS
#
# Revision 1.1  1992/07/22  18:39:58  davidm
# Initial revision
#
#

CC = gcc

XSIDIR	= ../../socket
INCLUDE	= -I$(XSIDIR)/gen -I$(XSIDIR)
PWD	= $(shell pwd)
override LIB	= libxksocket_$(HOW).a

XPLATFORM = mach3
#HOWTOCOMPILE = DEBUG
#HOWTOCOMPILE = OPTIMIZE
ifeq ($(HOWTOCOMPILE),) 

default:
	@echo Make variable $$\(HOWTOCOMPILE\) must be defined

else

MACHINE = mips

OPTIMIZECPPFLAGS = $(AUXFLAGS) -D_PATH_SERVICES='"/etc/services"'
#DEBUGCPPFLAGS = -DXK_DEBUG $(AUXFLAGS) -D_PATH_SERVICES='"${PWD}/services"'
DEBUGCPPFLAGS = -DXK_DEBUG $(AUXFLAGS) -D_PATH_SERVICES='"/etc/services"'

OPTIMIZECFLAGS = -g -O
DEBUGCFLAGS = -g -O

####################################################################

CPPFLAGS = $(INCLUDE) $($(HOWTOCOMPILE)CPPFLAGS) -Dpmax

HOW = $(HOWTOCOMPILE)$(XPLATFORM)

ifeq ($(CC),gcc)

DEBUGWARNINGS = -Wreturn-type -Wunused
OPTIMIZEWARNINGS = $(DEBUGWARNINGS)
CC_SPECIFIC_FLAGS = -fwritable-strings

else

endif

CFLAGS = $($(HOWTOCOMPILE)CFLAGS) $(CPPFLAGS) $($(HOWTOCOMPILE)WARNINGS) $(CC_SPECIFIC_FLAGS)

./$(HOW)/%.o : ./%.c
	@if [ ! -d $(dir $@) ] ; then mkdir $(shell echo $(dir $@) | sed 's+/$$++'); fi ; \
	echo $(CC) -c $(CFLAGS) -o $@ $< ; \
	$(CC) -c $(CFLAGS) -o $@ $<

####################################################################

OBJS	= ./$(HOW)/accept.o ./$(HOW)/bind.o ./$(HOW)/close.o \
	  ./$(HOW)/connect.o ./$(HOW)/listen.o ./$(HOW)/socket.o \
	  ./$(HOW)/getpeername.o ./$(HOW)/getsockname.o \
	  ./$(HOW)/getsockopt.o ./$(HOW)/setsockopt.o ./$(HOW)/select.o \
	  ./$(HOW)/ioctl.o ./$(HOW)/fcntl.o ./$(HOW)/dup.o ./$(HOW)/dup2.o \
	  ./$(HOW)/fork.o ./$(HOW)/vfork.o ./$(HOW)/read.o ./$(HOW)/readv.o \
	  ./$(HOW)/write.o ./$(HOW)/writev.o ./$(HOW)/recv.o \
	  ./$(HOW)/recvfrom.o ./$(HOW)/recvmsg.o ./$(HOW)/send.o \
	  ./$(HOW)/sendto.o ./$(HOW)/sendmsg.o ./$(HOW)/xsi_main.o \
	  ./$(HOW)/xsi_user.o ./$(HOW)/getservbyname.o \
	  ./$(HOW)/getservbyport.o ./$(HOW)/getservent.o

SRCS	= accept.c bind.c close.c connect.c listen.c socket.c \
	  getpeername.c getsockname.c getsockopt.c setsockopt.c select.c \
	  ioctl.c fcntl.c dup.c dup2.c fork.c vfork.c read.c readv.c \
	  write.c writev.c recv.c recvfrom.c recvmsg.c send.c \
	  sendto.c sendmsg.c xsi_main.c getservbyname.c \
	  getservbyport.c getservent.c

HDRS	= xsi_main.h

RCSFILES = Makefile $(SRCS) $(HDRS)

all: 
	cd syscalls ; make	
	make $(LIB)
	cd tst ; make

% :: RCS/%,v

./$(HOW)/xsi_user.o: $(XSIDIR)/gen/xsi_user.c
	$(CC) $(CFLAGS) -o $@ -c $^


$(LIB): $(OBJS) syscalls/sc_*.o
	ar ruv $@ $(OBJS) syscalls/sc_*.o
	ranlib $@

rcsupdate: TFM
	ci -l $(RCSFILES); true

clean:	TFM
	rm -f $(OBJS)

realclean: clean
	true

depend:	TFM
	@$(CC) -MM $(INCLUDE) *.c | sed "s|^|$(HOW)/|" >Makedep.$(HOW)

TFM:

.PHONY:	TFM

Makedep.$(HOW):
	touch $@

include Makedep.$(HOW)

endif
