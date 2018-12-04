/*
 * $RCSfile: connect.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:17:10 $
 * $Author: menze $
 *
 * $Log: connect.c,v $
 * Revision 1.2  1993/02/02  00:17:10  menze
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
connect(int s, struct sockaddr *name, int namelen)
{
    xTrace2(xsi, TR_FUNCTIONAL_TRACE, "connect(s=%d,namelen=%d)", s, namelen);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	return XSI(connect, (xsi_server, xsi_cid, s, name, namelen, &stat));
    } else {
	return sc_connect(s, name, namelen);
    } /* if */
} /* connect */

			/*** end of connect.c ***/
