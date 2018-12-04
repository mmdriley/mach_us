#
# pxk Makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.17 $
# $Date: 1993/02/01 23:54:36 $
#

DIRS = 
FILES = event machine process init ocsum redefines time trace utils

default: $(HOW)/machine.o $(HOW)/process.o $(OBJ)

# files which must be compiled with cc instead of gcc
$(HOW)/machine.o $(HOW)/process.o : $(HOW)/%.o : %.c
	@if [ ! -d $(HOW) ] ; then mkdir $(HOW) ; fi ; \
	echo $(REALCC) -c $($(HOWTOCOMPILE)CFLAGS) $(CPPFLAGS) -o $@ $< ; \
	$(REALCC) -c $($(HOWTOCOMPILE)CFLAGS) $(CPPFLAGS) -o $@ $<

include $(MAKE_INCLUDE)/Makefile.common






