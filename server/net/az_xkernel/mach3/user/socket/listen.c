/*
 * $RCSfile: listen.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:18:17 $
 * $Author: menze $
 *
 * $Log: listen.c,v $
 * Revision 1.2  1993/02/02  00:18:17  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include "xsi_main.h"

int
listen(int s, int backlog)
{
    xTrace2(xsi, TR_FUNCTIONAL_TRACE, "listen(s=%d,backlog=%d)", s, backlog);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	return XSI(listen, (xsi_server, xsi_cid, s, backlog, &stat));
    } else {
	return sc_listen(s, backlog);
    } /* if */
} /* listen */

			/*** end of listen.c ***/
