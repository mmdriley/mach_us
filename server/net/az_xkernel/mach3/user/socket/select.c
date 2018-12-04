/*
 * $RCSfile: select.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/02 00:18:51 $
 * $Author: menze $
 *
 * $Log: select.c,v $
 * Revision 1.4  1993/02/02  00:18:51  menze
 * copyright change
 *
 * Revision 1.3  1993/01/26  08:15:04  menze
 * removed unused variable
 *
 * Revision 1.2  1992/08/15  01:19:59  davidm
 * select() totally revised---didn't work when user passed fd_sets which
 * were smaller than sizeof(fd_set); it is common practice (?) to use
 * just an "int" instead of fd_set
 *
 * Support for signal handling added.  If a server call returns EINTR,
 * the library checks the BSD server for pending signals by executing
 * a NOP system call (sigblock(sigmask(SIGKILL))).
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include <sys/types.h>
#include <sys/time.h>
#include <cthreads.h>
#include "xsi_main.h"

static int seqno = 0;
static int pfd[2];
static int ux_errno;
static int ux_nfds;
static fd_set ux_readfds;
static fd_set ux_writefds;
static fd_set ux_exceptfds;
static struct timeval *ux_timeout;
static fd_set xk_readfds;
static fd_set xk_writefds;
static fd_set xk_exceptfds;


static void
unix_select(void)
{
    xTrace0(xsi, TR_MAJOR_EVENTS, "select: invoking unix select()");

    /* also wait for input on pipe such that parent thread can stop us... */
    FD_SET(pfd[0], &ux_readfds);
    if (pfd[0] >= ux_nfds) {
	ux_nfds = pfd[0] + 1;
    } /* if */

    ux_nfds = sc_select(ux_nfds, &ux_readfds, &ux_writefds, &ux_exceptfds,
			ux_timeout);
    ux_errno = errno;

    xTrace2(xsi, TR_MAJOR_EVENTS,
	    "select: unix select() returned %d, errno=%d", ux_nfds, errno);

    if ((ux_nfds > 0) && FD_ISSET(pfd[0], &ux_readfds)) {
	/* socket select was faster... */
	FD_CLR(pfd[0], &ux_readfds);
	--ux_nfds;
    } else {
	/* wake-up socket select: */
	XSI(select_cancel, (xsi_server, xsi_cid, seqno, &stat));
    } /* if */
} /* unix_select */


int
select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout)
{
    int s;
    boolean_t any_unix_fds = FALSE;
    boolean_t any_xsi_fds  = FALSE;

    xTrace1(xsi, TR_FUNCTIONAL_TRACE, "select(nfds=%d)", nfds);

    if (!xsi_server) {
	xsi_user_init();
    } /* if */

    /* determine if we have both, unix and socket descriptors: */
    for (s = 0; s < nfds; s++) {
	if (readfds && FD_ISSET(s,readfds) ||
	    writefds && FD_ISSET(s,writefds) ||
	    exceptfds && FD_ISSET(s,exceptfds))
	{
	    if (FD_ISSET(s, &is_xsi_fd)) {
		any_xsi_fds = TRUE;
		if (any_unix_fds) {
		    break;
		} /* if */
	    } else {
		any_unix_fds = TRUE;
		if (any_xsi_fds) {
		    break;
		} /* if */
	    } /* if */
	} /* if */
    } /* for */

    if (any_xsi_fds) {
	int mlen;
	int i;
	int specified;
	struct timeval dummytimeout;

	++seqno;
	mlen = howmany(nfds, NFDBITS);

	/* make sure none of the sets is a NULL pointer: */
	specified = 0;
	if (readfds) {
	    specified |= SELECT_READFDS;
	    bcopy(readfds, &xk_readfds, mlen*sizeof(fd_mask));
	    bzero(readfds, mlen*sizeof(fd_mask));
	} /* if */
	if (writefds) {
	    specified |= SELECT_WRITEFDS;
	    bcopy(writefds, &xk_writefds, mlen*sizeof(fd_mask));
	    bzero(writefds, mlen*sizeof(fd_mask));
	} /* if */
	if (exceptfds) {
	    specified |= SELECT_EXCEPTFDS;
	    bcopy(exceptfds, &xk_exceptfds, mlen*sizeof(fd_mask));
	    bzero(exceptfds, mlen*sizeof(fd_mask));
	} /* if */
	if (timeout) {
	    specified |= SELECT_TIMEOUT;
	} else {
	    timeout = &dummytimeout;
	} /* if */

	if (any_unix_fds) {
	    /*
	     * Complex case: need to do a select on both, Unix file
	     * descriptors and socket file descriptors.
	     */
	    int rc;

	    /* split sets into unix and socket sets: */
	    ux_nfds = nfds;
	    if (specified & SELECT_TIMEOUT) {
		ux_timeout = timeout;
	    } else {
		ux_timeout = 0;
	    } /* if */
	    FD_ZERO(&ux_readfds);
	    FD_ZERO(&ux_writefds);
	    FD_ZERO(&ux_exceptfds);
	    for (s = 0; s < nfds; s++) {
		if (FD_ISSET(s, &xk_readfds) && !FD_ISSET(s, &is_xsi_fd)) {
		    FD_SET(s, &ux_readfds);
		    FD_CLR(s, &xk_readfds);
		} /* if */
		if (FD_ISSET(s, &xk_writefds) && !FD_ISSET(s, &is_xsi_fd)) {
		    FD_SET(s, &ux_writefds);
		    FD_CLR(s, &xk_writefds);
		} /* if */
		if (FD_ISSET(s, &xk_exceptfds) && !FD_ISSET(s, &is_xsi_fd)) {
		    FD_SET(s, &ux_exceptfds);
		    FD_CLR(s, &xk_exceptfds);
		} /* if */
	    } /* for */

	    if ((specified & SELECT_TIMEOUT) &&
		!(timeout->tv_sec | timeout->tv_usec))
	    {
		/* just a poll: do unix and socket select() sequentially: */

		xTrace0(xsi, TR_MAJOR_EVENTS, "select: doing polls");

		ux_nfds = sc_select(ux_nfds, &ux_readfds, &ux_writefds,
				    &ux_exceptfds, ux_timeout);
		ux_errno = errno;
		rc = XSI(select, (xsi_server, xsi_cid, seqno, &nfds,
				  specified,
				  &xk_readfds, &xk_writefds, &xk_exceptfds,
				  timeout, &stat));
	    } else {
		cthread_t child;
		  
		if (pipe(pfd) < 0) {
		    perror("select: pipe()");
		    exit(1);
		} /* if */
		child = cthread_fork((any_t (*)())unix_select, (any_t)0);

		xTrace0(xsi, TR_MAJOR_EVENTS,
			"select: invoking xsi_select()");
		rc = XSI(select, (xsi_server, xsi_cid, seqno, &nfds,
				  specified,
				  &xk_readfds, &xk_writefds, &xk_exceptfds,
				  timeout, &stat));
		xTrace3(xsi, TR_MAJOR_EVENTS,
		     "select: xsi_select() returned rc=%d, nfds=%d, errno=%d",
			rc, nfds, errno);

		/* wake-up cthread from unix select: */
		sc_write(pfd[1], "!", 1);

		cthread_join(child);

		sc_close(pfd[0]);
		sc_close(pfd[1]);
	    } /* if */

	    if (rc < 0) {
		if (ux_nfds > 0) {
		    /* unix select has ready file descriptors copy fd sets: */
		    errno = ESUCCESS;
		    rc = ux_nfds;
		    if (readfds) {
			bcopy(&ux_readfds, readfds, mlen*sizeof(fd_mask));
		    } /* if */
		    if (writefds) {
			bcopy(&ux_writefds, writefds, mlen*sizeof(fd_mask));
		    } /* if */
		    if (exceptfds) {
			bcopy(&ux_exceptfds, exceptfds, mlen*sizeof(fd_mask));
		    } /* if */
		} else if (errno == EXDEV) {
		    /* timer expired: */
		    errno = ESUCCESS;
		    rc = 0;
		} else if (errno == ESRCH) {
		    /* socket select cancelled by unix select: */
		    errno = ux_errno;
		    rc = ux_nfds;
		} else {
		    rc = -1;
		} /* if */
	    } else if (ux_nfds > 0) {
		/*
		 * Both unix and socket fds have ready descriptors,
		 * build union of both sets:
		 */
		for (i = 0; i < mlen; i++) {
		    if (readfds) {
			readfds->fds_bits[i]   =
			  xk_readfds.fds_bits[i] | ux_readfds.fds_bits[i];
		    } /* if */
		    if (writefds) {
			writefds->fds_bits[i]  =
			  xk_writefds.fds_bits[i] | ux_writefds.fds_bits[i];
		    } /* if */
		    if (exceptfds) {
			exceptfds->fds_bits[i] =
			  xk_exceptfds.fds_bits[i] | ux_exceptfds.fds_bits[i];
		    } /* if */
		} /* if */
		rc = nfds + ux_nfds;
	    } else {
		/* only socket descriptors are ready: */
		if (readfds) {
		    bcopy(&xk_readfds, readfds, mlen*sizeof(fd_mask));
		} /* if */
		if (writefds) {
		    bcopy(&xk_writefds, writefds, mlen*sizeof(fd_mask));
		} /* if */
		if (exceptfds) {
		    bcopy(&xk_exceptfds, exceptfds, mlen*sizeof(fd_mask));
		} /* if */
		rc = nfds;
	    } /* if */
	    return rc;
	} else {
	    if (XSI(select, (xsi_server, xsi_cid, seqno, &nfds, specified,
			     &xk_readfds, &xk_writefds, &xk_exceptfds,
			     timeout, &stat)) < 0)
	    {
		if (errno == EXDEV) {
		    /* timer expired: */
		    errno = ESUCCESS;
		    return 0;
		} else {
		    return -1;
		} /* if */
	    } else {
		if (readfds) {
		    bcopy(&xk_readfds, readfds, mlen*sizeof(fd_mask));
		} /* if */
		if (writefds) {
		    bcopy(&xk_writefds, writefds, mlen*sizeof(fd_mask));
		} /* if */
		if (exceptfds) {
		    bcopy(&xk_exceptfds, exceptfds, mlen*sizeof(fd_mask));
		} /* if */
		return nfds;
	    } /* if */
	} /* if */
    } else {
	return sc_select(nfds, readfds, writefds, exceptfds, timeout);
    } /* if */
} /* select */

			/*** end of select.c ***/
