/*
 * $RCSfile: getsockopt.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:18:06 $
 * $Author: menze $
 *
 * $Log: getsockopt.c,v $
 * Revision 1.2  1993/02/02  00:18:06  menze
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
getsockopt(int s, int level, int optname, void *optval, int *optlen)
{
    xTrace3(xsi, TR_FUNCTIONAL_TRACE, "getsockopt(s=%d,level=%d,opt=%d)",
	    s, level, optname);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	/* we assume inline transmission... */
	return XSI(getsockopt, (xsi_server, xsi_cid, s, level, optname,
				(xsi_varbuf_t*)&optval,
				(mach_msg_type_number_t*) &optlen, &stat));
    } else {
	return sc_getsockopt(s, level, optname, optval, optlen);
    } /* if */
} /* setsockopt */

			/*** end of getsockopt.c ***/
