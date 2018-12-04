/*
 * $RCSfile: recvmsg.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:18:45 $
 * $Author: menze $
 *
 * $Log: recvmsg.c,v $
 * Revision 1.2  1993/02/02  00:18:45  menze
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
recvmsg(int s, struct msghdr *msg, int flags)
{
    xTrace2(xsi, TR_FUNCTIONAL_TRACE, "recvmsg(s=%d,flags=%x)", s, flags);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    return recvmsg(s, msg, flags);
} /* recvmsg */

			/*** end of recvmsg.c ***/
