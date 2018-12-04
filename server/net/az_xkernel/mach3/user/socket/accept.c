/*
 * $RCSfile: accept.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:16:52 $
 * $Author: menze $
 *
 * $Log: accept.c,v $
 * Revision 1.2  1993/02/02  00:16:52  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include "xsi_main.h"

int
accept(int s, struct sockaddr *addr, int *addrlen)
{
    xTrace1(xsi, TR_FUNCTIONAL_TRACE, "accept(s=%d)", s);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	struct sockaddr dummy_addr;
	int dummy_addrlen;
	int ns;

	/* get a unique number for the new socket: */
	if ((ns = sc_dup(s)) < 0) {
	    return -1;
	} /* if */
	
	if (addr == 0) {
	    addr = &dummy_addr;
	    dummy_addrlen = sizeof(struct sockaddr);
	    addrlen = &dummy_addrlen;
	} /* if */

	if (*addrlen < sizeof(struct sockaddr)) {
	    xTrace1(xsi, TR_ERRORS,
		    "accept: address buffer too small (%d bytes)", *addrlen);
	    close(ns);
	    return ENOBUFS;
	} /* if */
	if (XSI(accept, (xsi_server, xsi_cid, s, addr, addrlen, ns, &stat))
	    < 0)
	{
	    close(ns);
	    return -1;
	} else {
	    FD_SET(ns, &is_xsi_fd);
	    return ns;
	} /* if */
    } else {
	return sc_accept(s, addr, addrlen);
    } /* if */
} /* accept */

			/*** end of accept.c ***/
