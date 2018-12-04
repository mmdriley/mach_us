/*
 * $RCSfile: sendmsg.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:19:01 $
 * $Author: menze $
 *
 * $Log: sendmsg.c,v $
 * Revision 1.2  1993/02/02  00:19:01  menze
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
sendmsg(int s, struct msghdr *msg, int flags)
{
    xTrace2(xsi, TR_FUNCTIONAL_TRACE, "sendmsg(s=%d,flags=%x)", s, flags);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    return sc_sendmsg(s, msg, flags);
} /* sendmsg */

			/*** end of sendmsg.c ***/
