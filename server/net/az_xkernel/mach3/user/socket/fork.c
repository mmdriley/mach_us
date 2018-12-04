/*
 * $RCSfile: fork.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:17:34 $
 * $Author: menze $
 *
 * $Log: fork.c,v $
 * Revision 1.3  1993/02/02  00:17:34  menze
 * copyright change
 *
 * Revision 1.2  1992/08/15  01:19:59  davidm
 * select() totally revised---didn't work when user passed fd_sets which
 * were smaller than sizeof(fd_set); it is common practice (?) to use
 * just an "int" instead of fd_set
 *
 * Support for signal handling added.  If a server call returns EINTR,
 * the library checks the BSD server for pending signals by executing
 * a NOP system call (sigblock(sigmask(SIGKILL))).
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include "xsi_main.h"

int
fork()
{
    int pid;

    xTrace0(xsi, TR_FUNCTIONAL_TRACE, "fork()");

    if ((pid = sc_fork()) > 0) {
	/* this is the parent: */
	XSI(await_clone_done, (xsi_server, xsi_cid, &stat));
    } else {
	/* this is the child: */
	int parent = xsi_cid;

	mach_init();
	xsi_setup();
	XSI(clone, (xsi_server, xsi_cid, parent, alive_port, &stat));
    } /* if */
    return pid;
} /* fork */

			/*** end of fork.c ***/
