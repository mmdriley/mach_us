#
# prottbldump makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.9 $
# $Date: 1993/02/02 00:00:19 $
#
# object for different architectures are kept in subdirectories of
# this directory.  Make variable $(ARCH) defines the architecture type.

CC = gcc
CFLAGS = -g -fwritable-strings -W -Wreturn-type -Wimplicit -Wunused -Wswitch $(INCLUDES)

SRC = ptbldump.c
OBJS = ptbldump.o

ifeq ($(ARCH),) 
depend .default:
	@echo "Make variable ARCH must be defined"
endif

ifeq ($(RCS_VERSION),)

RCS_VERSION=nonexistant_version

endif

#
# We define "HOW" as "ARCH" so we can use the normal xkernel depend
# rule which rewrites the objects with a $(HOW) prefix
#
HOW = $(ARCH)

ifeq ($(ARCH),sparc)
XPLATFORM=sunos
endif
ifeq ($(ARCH),mips)
XPLATFORM=mach3
endif

INCLUDES = -I../../include -I../../pi -I../../pi/include -I../../include/prot -I../../$(XPLATFORM)/include

LIB = ../../pi/libPi_DEBUG$(XPLATFORM)

OBJS := $(addprefix $(ARCH)/, $(OBJS))

ptbldump:	$(ARCH)/ptbldump 

setup:
	@-if [ ! -d $(ARCH) ] ; then umask 2; mkdir $(ARCH) ; fi

$(ARCH)/ptbldump:	$(OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(HOW)/%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<

include ../../makefiles/Makefile.local

$(LIB):
	@echo
	@echo ptbldump requires the library $(LIB) 
	@echo To make this library, create a standard kernel build area
	@echo and run 'make libPi' in that area
	@echo
	@false

ptblData.c: ../../etc/prottbl
	ptbldump ../../etc/prottbl > ptblData.c

clean:  localclean
depend: localdepend

include Makedep.$(HOW)

