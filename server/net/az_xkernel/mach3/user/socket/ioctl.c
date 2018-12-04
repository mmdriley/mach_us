/*
 * $RCSfile: ioctl.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:18:12 $
 * $Author: menze $
 *
 * $Log: ioctl.c,v $
 * Revision 1.3  1993/02/02  00:18:12  menze
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
#include <sys/ioctl.h>
#include "xsi_main.h"

int
ioctl(int s, int request, char *argp)
{
    char *inp;
    int inlen;
    char *outp;
    int outlen;

    xTrace3(xsi, TR_FUNCTIONAL_TRACE, "ioctl(s=%d,request=%d,argp=%08x)",
	    s, request, argp);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	if (request & IOC_IN) {
	    inp = argp;
	    inlen = IOCPARM_LEN(request);
	} /* if */
	if (request & IOC_OUT) {
	    outp = argp;
	    outlen = IOCPARM_LEN(request);
	} /* if */
	return XSI(ioctl, (xsi_server, xsi_cid, s, request,
			   inp, inlen, &outp, &outlen, &stat));
    } else {
	return sc_ioctl(s, request, argp);
    } /* if */
} /* ioctl */

			/*** end of ioctl.c ***/
