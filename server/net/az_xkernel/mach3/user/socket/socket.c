/*
 * $RCSfile: socket.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:19:21 $
 * $Author: menze $
 *
 * $Log: socket.c,v $
 * Revision 1.2  1993/02/02  00:19:21  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "xsi_main.h"


int
socket(int domain, int type, int protocol)
{
    int fd;

    xTrace3(xsi, TR_FUNCTIONAL_TRACE,
	    "socket(dom=%d,typ=%d,prot=%d)", domain, type, protocol);

    /* get a (kernel) socket: */
    fd = sc_socket(domain, type, protocol);
    if (fd < 0) {
	return fd;
    } /* if */

    if (domain == PF_INET) {
	if (!xsi_server) {
	    xsi_user_init();
	} /* if */

	if (XSI(socket, (xsi_server, xsi_cid, domain, type, protocol, fd,
			 &stat)) < 0)
	{
	    close(fd);
	    return -1;
	} /* if */
	FD_SET(fd,&is_xsi_fd);
    } else {
	FD_CLR(fd,&is_xsi_fd);
    } /* if */
    return fd;
} /* socket */

			/*** end of socket.c ***/
