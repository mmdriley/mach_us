/*
 * $RCSfile: getsockname.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:18:00 $
 * $Author: menze $
 *
 * $Log: getsockname.c,v $
 * Revision 1.2  1993/02/02  00:18:00  menze
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
getsockname(int s, struct sockaddr *name, int *namelen)
{
    xTrace1(xsi, TR_FUNCTIONAL_TRACE, "getsockname(s=%d)", s);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	if (*namelen < sizeof(struct sockaddr)) {
	    xTrace1(xsi, TR_ERRORS,
		    "getsockname: buffer too small (%d bytes)", *namelen);
	    return ENOBUFS;
	} /* if */
	return XSI(getsockname, (xsi_server, xsi_cid, s, name, namelen,
				 &stat));
    } else {
	return sc_getsockname(s, name, namelen);
    } /* if */
} /* getsockname */

			/*** end of getsockname.c ***/
