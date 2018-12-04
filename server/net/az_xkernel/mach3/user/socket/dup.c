/*
 * $RCSfile: dup.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:17:17 $
 * $Author: menze $
 *
 * $Log: dup.c,v $
 * Revision 1.2  1993/02/02  00:17:17  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include "xsi_main.h"

int
dup(int s)
{
    int ns;

    xTrace1(xsi, TR_FUNCTIONAL_TRACE, "dup(s=%d)", s);
    
    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    ns = sc_dup(s);

    if (FD_ISSET(s, &is_xsi_fd)) {
	if (XSI(dup2, (xsi_server, xsi_cid, s, ns, &stat)) < 0) {
	    close(ns);
	    return -1;
	} /* if */
	FD_SET(ns, &is_xsi_fd);
    } /* if */

    return ns;
} /* dup */

			/*** end of dup.c ***/
