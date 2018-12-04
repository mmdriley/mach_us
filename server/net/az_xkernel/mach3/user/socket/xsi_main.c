/*
 * $RCSfile: xsi_main.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/02 00:19:49 $
 * $Author: menze $
 *
 * $Log: xsi_main.c,v $
 * Revision 1.4  1993/02/02  00:19:49  menze
 * copyright change
 *
 * Revision 1.3  1992/12/04  01:12:50  menze
 * debugging flag changed from ! NDEBUG to XK_DEBUG
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include "xsi_main.h"

mach_port_t xsi_server = 0;
mach_port_t alive_port;
int xsi_cid;
fd_set is_xsi_fd;
#ifdef XK_DEBUG
  int tracexsi = 0/*TR_FULL_TRACE*/;
#endif /* XK_DEBUG*/


/*
 * This is a hack to make sure the right (i.e., our) functions get linked
 * into the program instead of the standard library functions.
 */
static void
ld_hack(void)
{
    accept(0, (struct sockaddr *) 0, (int *) 0);
    bind(0, (struct sockaddr *) 0, (int) 0);
    close(0);
    connect(0, (struct sockaddr *) 0, (int) 0);
    dup(0);
    dup2(0, 0);
    fork();
    listen(0, 0);
    socket(0, 0, 0);
    getpeername(0, (struct sockaddr *) 0, (int *) 0);
    getsockname(0, (struct sockaddr *) 0, (int *) 0);
    getsockopt(0, 0, 0, (void*) 0, (int*) 0);
    setsockopt(0, 0, 0, (void*) 0, 0);
    select(0, (fd_set*)0, (fd_set*)0, (fd_set*)0, (struct timeval*) 0);
    fcntl(0, 0, 0);
    read(0, (char*) 0, 0);
    readv(0, (struct iovec *) 0, 0);
    write(0, (char*) 0, 0);
    writev(0, (struct iovec *) 0, 0);
    recv(0, (void*) 0, 0, 0);
    recvfrom(0, (void*)0, 0, 0, (struct sockaddr*) 0, (int*) 0);
    recvmsg(0, (struct msghdr*) 0, 0);
    send(0, (void*) 0, 0, 0);
    sendto(0, (void*) 0, 0, 0, (struct sockaddr*) 0, 0);
    sendmsg(0, (struct msghdr*) 0, 0);
    vfork();
} /* ld_hack */


void
xsi_setup()
{
    kern_return_t kr;

    kr = netname_look_up(name_server_port, "", XSI_SERVER_NAME, &xsi_server);
    if (kr != KERN_SUCCESS) {
	quit(1, "xsi_user_init: netname_lookup(): %s\n",
	     mach_error_string(kr));
    } /* if */

    /* allocate signal server port: */
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
			    &alive_port);
    if (kr != KERN_SUCCESS) {
	quit(1, "xsi_user_init: mach_port_allocate(): %s\n",
	     mach_error_string(kr));
    } /* if */

    xsi_cid = getpid();
} /* xsi_setup */


void
xsi_user_init(void)
{
    static boolean_t inited = FALSE;

    if (inited) {
	return;
    } /* if */
    inited = TRUE;

    xTrace0(xsi, TR_FUNCTIONAL_TRACE, "xsi_user_init()");

    xsi_setup();

    /* determine inherited sockets: */
    XSI(hello, (xsi_server, xsi_cid, alive_port, &is_xsi_fd, &stat));
} /* xsi_user_init */

			/*** end of xsi_main.c ***/
