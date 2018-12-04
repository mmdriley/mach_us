/*
 * $RCSfile: write.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:19:35 $
 * $Author: menze $
 *
 * $Log: write.c,v $
 * Revision 1.3  1993/02/02  00:19:35  menze
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
write(int s, char *buf, int nbytes)
{
    if (!nbytes) {
	return 0;
    } /* if */

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	xTrace2(xsi, TR_FUNCTIONAL_TRACE, "write(s=%d,nbytes=%d)", s, nbytes);
	
	if (XSI(write, (xsi_server, xsi_cid, s, (void*) buf, nbytes, &nbytes,
			&stat)) < 0)
	{
	    return -1;
	} /* if */
	return nbytes;
    } else {
	return sc_write(s, buf, nbytes);
    } /* if */
} /* write */

			/*** end of write.c ***/
