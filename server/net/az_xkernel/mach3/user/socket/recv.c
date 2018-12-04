/*
 * $RCSfile: recv.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:18:33 $
 * $Author: menze $
 *
 * $Log: recv.c,v $
 * Revision 1.2  1993/02/02  00:18:33  menze
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
recv(int s, void *buf, int len, int flags)
{
    xTrace3(xsi, TR_FUNCTIONAL_TRACE, "recv(s=%d,len=%d,flags=%x)",
	    s, len, flags);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	char *rbuf = buf;

	if (XSI(recv, (xsi_server, xsi_cid, s, &rbuf, &len, len, flags,
		       &stat)) < 0)
	{
	    return -1;
	} /* if */
	if (rbuf != buf) {
	    kern_return_t kr;

	    bcopy(rbuf, buf, len);
	    kr = vm_deallocate(mach_task_self(), (vm_address_t) rbuf, len);
	    if (kr != KERN_SUCCESS) {
		quit(1, "recv: vm_deallocate(): %s", mach_error(kr));
	    } /* if */
	} /* if */
	return len;
    } else {
	return sc_recv(s, buf, len, flags);
    } /* if */
} /* recv */

			/*** end of recv.c ***/
