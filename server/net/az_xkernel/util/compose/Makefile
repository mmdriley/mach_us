#
# Compose makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.11.1.1 $
# $Date: 1993/02/03 06:45:14 $
#
# object for different architectures are kept in subdirectories of
# this directory.  Make variable $(ARCH) defines the architecture type.


ifeq ($(ARCH),) 

depend .default:
	@echo "Make variable ARCH must be defined"

endif

ifeq ($(RCS_VERSION),)

RCS_VERSION=nonexistant_version

endif


OBJS = 	parse.o compose.o util.o error.o
CC=gcc
CFLAGS=-g -fwritable-strings -DARCH_INCLUDE=\"compose_$(ARCH).h\"

MAKE_INCLUDE = ../../makefiles
RCS_CO_FLAGS = -r$(RCS_VERSION)

ARCH_PFX = $(ARCH)/
#
# We define "HOW" as "ARCH" so we can use the normal xkernel depend
# rule which rewrites the objects with a $(HOW) prefix
#
HOW = $(ARCH)

ifeq ($(ARCH),mips)

CFLAGS := $(CFLAGS) -I/usr/cs/include

endif

OBJS := $(addprefix $(ARCH_PFX), $(OBJS))
SRC = *.c

.PHONY : compose
compose: $(ARCH)/compose

setup:
	@-if [ ! -d $(ARCH) ] ; then umask 2; mkdir $(ARCH) ; fi

$(ARCH)/compose: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(ARCH)/%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	
include $(MAKE_INCLUDE)/Makefile.local

clean:  localclean
depend: localdepend

tags:
	etags -tw -f TAGS *.c *.h

include Makedep.$(HOW)

