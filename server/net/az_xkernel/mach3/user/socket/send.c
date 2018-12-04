/*
 * $RCSfile: send.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:18:56 $
 * $Author: menze $
 *
 * $Log: send.c,v $
 * Revision 1.3  1993/02/02  00:18:56  menze
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
#include <sys/types.h>
#include <sys/socket.h>
#include "xsi_main.h"

int
send(int s, void *msg, int len, int flags)
{
    xTrace3(xsi, TR_FUNCTIONAL_TRACE, "send(s=%d,len=%d,flags=%x)",
	    s, len, flags);

    if (!len) {
	return 0;
    } /* if */

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	if (XSI(send, (xsi_server, xsi_cid, s, msg, len, &len, flags, &stat))
	    < 0)
	{
	    return -1;
	} /* if */
	return len;
    } else {
	return sc_send(s, msg, len, flags);
    } /* if */
} /* send */

			/*** end of send.c ***/
