#!/bin/csh -f
#
# $RCSfile: Makefile,v $
# 
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
#
# $Revision: 1.2 $
# $Date: 1993/02/01 23:56:55 $
#
# Map test makefile

XRT=../../..

CC=gcc
LIBPI = $(XRT)/pi/libPi_OPTIMIZEsunos
CFLAGS = -g $(INCLUDES) -DXK_DEBUG
INCLUDES = -I$(XRT)/include -I$(XRT)/include/prot -I$(XRT)/sunos/include -I$(XRT)/pi/include -I$(XRT)/etc

SRC = mapTest.c mapTestCor.c mapTestCor2.c idmap.c mapTestPerf.c

% :: RCS/%,v

mapTestCor:	mapTestCor.o $(LIBPI)
	$(CC) $(CFLAGS) -o mapTestCor $^

mapTestCor2:	idmap.o mapTestCor2.o $(LIBPI)
	$(CC) $(CFLAGS) -o mapTestCor2 $^

mapTestPerf:	mapTestPerf.o $(LIBPI)
	$(CC) $(CFLAGS) -o mapTestPerf $^


mapTest:	mapTest.o $(LIBPI)
	$(CC) $(CFLAGS) -o mapTest $^

depend:
	makedepend $(INCLUDES) $(SRC)

clean:
	rm -f mapTest2.o mapTest mapTest.o mapTest
# DO NOT DELETE THIS LINE -- make depend depends on it.

mapTest.o: /cs/x32/merge/include/xkernel.h
mapTest.o: /cs/x32/merge/sunos/include/platform.h
mapTest.o: /cs/x32/merge/include/xk_debug.h
mapTest.o: /cs/x32/merge/pi/include/x_stdio.h /usr/include/stdio.h
mapTest.o: /cs/x32/merge/include/trace.h /usr/include/sys/types.h
mapTest.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
mapTest.o: /usr/include/netinet/in.h /cs/x32/merge/sunos/include/process.h
mapTest.o: xtype.h /usr/include/lwp/lwp.h /usr/include/sys/time.h
mapTest.o: /usr/include/sys/time.h /cs/x32/merge/include/msg_s.h
mapTest.o: /cs/x32/merge/include/upi.h /cs/x32/merge/include/idmap.h
mapTest.o: /cs/x32/merge/include/part.h /cs/x32/merge/include/msg.h
mapTest.o: /cs/x32/merge/include/prot/ip_host.h
mapTest.o: /cs/x32/merge/include/prot/eth_host.h
mapTest.o: /cs/x32/merge/include/event.h /cs/x32/merge/sunos/include/assert.h
mapTest.o: /cs/x32/merge/etc/site.h /cs/x32/merge/include/xtime.h
mapTest.o: /cs/x32/merge/include/prottbl.h /cs/x32/merge/include/netmask.h
mapTest.o: /cs/x32/merge/sunos/include/x_libc.h /usr/include/string.h
mapTest.o: /cs/x32/merge/include/x_util.h /cs/x32/merge/include/upi_inline.h
mapTestCor.o: idmap.c /cs/x32/merge/include/upi.h xtype.h
mapTestCor.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
mapTestCor.o: /usr/include/sys/sysmacros.h /cs/x32/merge/include/idmap.h
mapTestCor.o: /cs/x32/merge/include/msg_s.h /cs/x32/merge/include/part.h
mapTestCor.o: /cs/x32/merge/include/msg.h
mapTestCor.o: /cs/x32/merge/sunos/include/platform.h
mapTestCor.o: /cs/x32/merge/include/xk_debug.h
mapTestCor.o: /cs/x32/merge/pi/include/x_stdio.h /usr/include/stdio.h
mapTestCor.o: /cs/x32/merge/include/trace.h /usr/include/netinet/in.h
mapTestCor.o: /cs/x32/merge/sunos/include/process.h /usr/include/lwp/lwp.h
mapTestCor.o: /usr/include/sys/time.h /usr/include/sys/time.h
mapTestCor.o: /cs/x32/merge/include/prot/ip_host.h
mapTestCor.o: /cs/x32/merge/include/prot/eth_host.h idmap_internal.h
mapTestCor.o: /cs/x32/merge/sunos/include/assert.h
mapTestCor.o: /cs/x32/merge/sunos/include/x_libc.h /usr/include/string.h
mapTestCor.o: idmap_templ.c idmap_init.c
mapTestCor2.o: /cs/x32/merge/include/xkernel.h
mapTestCor2.o: /cs/x32/merge/sunos/include/platform.h
mapTestCor2.o: /cs/x32/merge/include/xk_debug.h
mapTestCor2.o: /cs/x32/merge/pi/include/x_stdio.h /usr/include/stdio.h
mapTestCor2.o: /cs/x32/merge/include/trace.h /usr/include/sys/types.h
mapTestCor2.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
mapTestCor2.o: /usr/include/netinet/in.h
mapTestCor2.o: /cs/x32/merge/sunos/include/process.h xtype.h
mapTestCor2.o: /usr/include/lwp/lwp.h /usr/include/sys/time.h
mapTestCor2.o: /usr/include/sys/time.h /cs/x32/merge/include/msg_s.h
mapTestCor2.o: /cs/x32/merge/include/upi.h /cs/x32/merge/include/idmap.h
mapTestCor2.o: /cs/x32/merge/include/part.h /cs/x32/merge/include/msg.h
mapTestCor2.o: /cs/x32/merge/include/prot/ip_host.h
mapTestCor2.o: /cs/x32/merge/include/prot/eth_host.h
mapTestCor2.o: /cs/x32/merge/include/event.h
mapTestCor2.o: /cs/x32/merge/sunos/include/assert.h /cs/x32/merge/etc/site.h
mapTestCor2.o: /cs/x32/merge/include/xtime.h /cs/x32/merge/include/prottbl.h
mapTestCor2.o: /cs/x32/merge/include/netmask.h
mapTestCor2.o: /cs/x32/merge/sunos/include/x_libc.h /usr/include/string.h
mapTestCor2.o: /cs/x32/merge/include/x_util.h
mapTestCor2.o: /cs/x32/merge/include/upi_inline.h
idmap.o: /cs/x32/merge/include/upi.h xtype.h /usr/include/sys/types.h
idmap.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
idmap.o: /cs/x32/merge/include/idmap.h /cs/x32/merge/include/msg_s.h
idmap.o: /cs/x32/merge/include/part.h /cs/x32/merge/include/msg.h
idmap.o: /cs/x32/merge/sunos/include/platform.h
idmap.o: /cs/x32/merge/include/xk_debug.h /cs/x32/merge/pi/include/x_stdio.h
idmap.o: /usr/include/stdio.h /cs/x32/merge/include/trace.h
idmap.o: /usr/include/netinet/in.h /cs/x32/merge/sunos/include/process.h
idmap.o: /usr/include/lwp/lwp.h /usr/include/sys/time.h
idmap.o: /usr/include/sys/time.h /cs/x32/merge/include/prot/ip_host.h
idmap.o: /cs/x32/merge/include/prot/eth_host.h idmap_internal.h
idmap.o: /cs/x32/merge/sunos/include/assert.h
idmap.o: /cs/x32/merge/sunos/include/x_libc.h /usr/include/string.h
idmap.o: idmap_templ.c idmap_init.c
mapTestPerf.o: /cs/x32/merge/include/xkernel.h
mapTestPerf.o: /cs/x32/merge/sunos/include/platform.h
mapTestPerf.o: /cs/x32/merge/include/xk_debug.h
mapTestPerf.o: /cs/x32/merge/pi/include/x_stdio.h /usr/include/stdio.h
mapTestPerf.o: /cs/x32/merge/include/trace.h /usr/include/sys/types.h
mapTestPerf.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
mapTestPerf.o: /usr/include/netinet/in.h
mapTestPerf.o: /cs/x32/merge/sunos/include/process.h xtype.h
mapTestPerf.o: /usr/include/lwp/lwp.h /usr/include/sys/time.h
mapTestPerf.o: /usr/include/sys/time.h /cs/x32/merge/include/msg_s.h
mapTestPerf.o: /cs/x32/merge/include/upi.h /cs/x32/merge/include/idmap.h
mapTestPerf.o: /cs/x32/merge/include/part.h /cs/x32/merge/include/msg.h
mapTestPerf.o: /cs/x32/merge/include/prot/ip_host.h
mapTestPerf.o: /cs/x32/merge/include/prot/eth_host.h
mapTestPerf.o: /cs/x32/merge/include/event.h
mapTestPerf.o: /cs/x32/merge/sunos/include/assert.h /cs/x32/merge/etc/site.h
mapTestPerf.o: /cs/x32/merge/include/xtime.h /cs/x32/merge/include/prottbl.h
mapTestPerf.o: /cs/x32/merge/include/netmask.h
mapTestPerf.o: /cs/x32/merge/sunos/include/x_libc.h /usr/include/string.h
mapTestPerf.o: /cs/x32/merge/include/x_util.h
mapTestPerf.o: /cs/x32/merge/include/upi_inline.h
