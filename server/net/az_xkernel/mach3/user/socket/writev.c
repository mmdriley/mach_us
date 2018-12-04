/*
 * $RCSfile: writev.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:19:43 $
 * $Author: menze $
 *
 * $Log: writev.c,v $
 * Revision 1.2  1993/02/02  00:19:43  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include <sys/types.h>
#include <sys/uio.h>
#include "xsi_main.h"

int
writev(int s, struct iovec *iov, int iovcnt)
{
    xTrace2(xsi, TR_FUNCTIONAL_TRACE, "writev(s=%d,iovcnt=%d)", s, iovcnt);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    return sc_writev(s, iov, iovcnt);
} /* writev */

			/*** end of writev.c ***/
