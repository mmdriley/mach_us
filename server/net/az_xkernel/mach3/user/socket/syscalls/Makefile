#
# $RCSfile: Makefile,v $
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.3 $
# $Date: 1993/02/02 00:13:58 $
#

OBJS	= sc_accept.o sc_bind.o sc_close.o sc_connect.o sc_listen.o \
	  sc_socket.o \
	  sc_getpeernm.o sc_getsocknm.o sc_getsockopt.o sc_setsockopt.o \
	  sc_select.o sc_ioctl.o sc_fcntl.o \
	  sc_read.o sc_readv.o sc_write.o sc_writev.o \
	  sc_recv.o sc_recvfrom.o sc_recvmsg.o sc_send.o sc_sendto.o \
	  sc_sendmsg.o \
	  sc_dup.o sc_dup2.o sc_fork.o

% :: RCS/%,v

all: $(OBJS)

clean:
	rm -f $(OBJS)
