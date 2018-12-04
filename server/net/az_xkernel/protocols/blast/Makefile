# 
# BLAST Makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.8 $
# $Date: 1993/02/01 22:19:47 $
#
	
FILES = blast blast_control blast_debug blast_input blast_output \
        blast_util blast_hdr blast_stack
DIRS =

#
# Configure BLAST to send nacks when last fragment doesn't complete
# the datagram on all platforms except Mach
#
ifneq ($(XPLATFORM), mach3)

CPPFLAGS := $(CPPFLAGS) -DBLAST_LAST_FRAG_NACKS

endif

include $(MAKE_INCLUDE)/Makefile.common
