/*
 * $RCSfile: read.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:18:22 $
 * $Author: menze $
 *
 * $Log: read.c,v $
 * Revision 1.2  1993/02/02  00:18:22  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include "xsi_main.h"

int
read(int s, char *buf, int nbytes)
{
    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    if (FD_ISSET(s, &is_xsi_fd)) {
	char *rbuf = buf;

	xTrace2(xsi, TR_FUNCTIONAL_TRACE, "read(s=%d,nbytes=%d)", s, nbytes);

	if (XSI(read, (xsi_server, xsi_cid, s, &rbuf, &nbytes, nbytes, &stat))
	    < 0)
	{
	    return -1;
	} /* if */
	if (rbuf != buf) {
	    kern_return_t kr;

	    bcopy(rbuf, buf, nbytes);
	    kr = vm_deallocate(mach_task_self(),
			       (vm_address_t) rbuf, nbytes);
	    if (kr != KERN_SUCCESS) {
		quit(1, "read: vm_deallocate(): %s", mach_error(kr));
	    } /* if */
	} /* if */
	return nbytes;
    } else {
	return sc_read(s, buf, nbytes);
    } /* if */
} /* read */

			/*** end of read.c ***/
