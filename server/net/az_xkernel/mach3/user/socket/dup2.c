/*
 * $RCSfile: dup2.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:17:23 $
 * $Author: menze $
 *
 * $Log: dup2.c,v $
 * Revision 1.2  1993/02/02  00:17:23  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include "xsi_main.h"

int
dup2(int from, int to)
{
    xTrace2(xsi, TR_FUNCTIONAL_TRACE, "dup2(from=%d,to=%d)", from, to);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (sc_dup2(from, to) < 0) {
	xTrace0(xsi, TR_ERRORS, "dup2 failed");
	return -1;
    } /* if */

    if (FD_ISSET(from, &is_xsi_fd)) {
	if (XSI(dup2, (xsi_server, xsi_cid, from, to, &stat)) < 0) {
	    close(to);
	    return -1;
	} /* if */
	FD_SET(to, &is_xsi_fd);
    } /* if */

    return 0;
} /* dup2 */

			/*** end of dup2.c ***/
