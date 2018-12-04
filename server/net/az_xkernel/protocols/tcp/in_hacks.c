/*
 * in_hacks.c
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:23:22 $
 */

#include "xkernel.h"
#include "tcp_internal.h"

#define CKSUM_TRACE 8


int
in_pcballoc(so, head)
	XObj so;
	struct inpcb *head;
{
	register struct inpcb *inp;

	inp = (struct inpcb *)xMalloc(sizeof *inp);
	inp->inp_head = head;
	inp->inp_session = so;
	insque(inp, head);
	sotoinpcb(so) = inp;
	return (0);
}


/*ARGSUSED*/
void
in_pcbdisconnect(inp)
    struct inpcb *inp;
{
    Kabort("in_pcbdisconnect");
}


void
in_pcbdetach(inp)
    struct inpcb *inp;
{
    remque(inp);
    xFree((char *)inp);
}


bool
in_broadcast(addr)
    struct in_addr addr;
{
    return (ntohl(addr.s_addr) & 0xff) == 0 ||
      	   (ntohl(addr.s_addr) & 0xff) == 0xff;
}


