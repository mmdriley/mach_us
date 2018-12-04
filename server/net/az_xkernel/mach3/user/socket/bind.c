/*
 * $RCSfile: bind.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:16:58 $
 * $Author: menze $
 *
 * $Log: bind.c,v $
 * Revision 1.2  1993/02/02  00:16:58  menze
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
bind(int s, struct sockaddr *name, int namelen)
{
    xTrace2(xsi, TR_FUNCTIONAL_TRACE, "bind(s=%d,namelen=%d)", s, namelen);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	return XSI(bind, (xsi_server, xsi_cid, s, name, namelen, &stat));
    } else {
	return sc_bind(s, name, namelen);
    } /* if */
} /* bind */

			/*** end of bind.c ***/
