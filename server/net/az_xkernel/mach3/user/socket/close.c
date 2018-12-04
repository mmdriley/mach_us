/*
 * $RCSfile: close.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:17:04 $
 * $Author: menze $
 *
 * $Log: close.c,v $
 * Revision 1.2  1993/02/02  00:17:04  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include "xsi_main.h"

int
close(int s)
{
    xTrace1(xsi, TR_FUNCTIONAL_TRACE, "close(s=%d)", s);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	XSI(close, (xsi_server, xsi_cid, s, &stat));
	FD_CLR(s, &is_xsi_fd);
    } /* if */

    return sc_close(s);
} /* close */

			/*** end of close.c ***/
