/*
 * $RCSfile: xsi_services.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/02 00:09:05 $
 * $Author: menze $
 *
 * $Log: xsi_services.c,v $
 * Revision 1.8  1993/02/02  00:09:05  menze
 * copyright change
 *
 * Revision 1.7  1993/01/21  21:08:00  menze
 * Modified to use new participant interface
 *
 * Revision 1.6  1992/08/15  01:16:26  davidm
 * OOB message transmission is now implemented *and* tested.
 * Support for SIGURG was added.
 *
 * Revision 1.5  1992/08/01  18:42:25  davidm
 * This version now supports TCP flow-control, handles UDP connections
 * correctly, and also supports out-of-band message transmission (untested
 * yet).
 *
 * Revision 1.4  1992/07/24  04:15:03  davidm
 * minor changes
 *
 * Revision 1.3  1992/07/22  00:14:07  davidm
 * Various bug fixes and improvements.
 *
 * Revision 1.2  1992/07/03  08:47:48  davidm
 * Simplified because everything is now an x-thread instead of a C-thread.
 *
 * Revision 1.1  1992/06/30  21:27:25  davidm
 * Initial revision
 *
 */
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <mach/notify.h>
#include <fcntl.h>
#include "xsi.h"
#include "xsi_services.h"

#ifdef MAX
# undef MAX
#endif
#define MAX(a,b) ((a)>(b)?(a):(b))

#define KB 1024

#define XKSOCK_PROTL(sd)	(((sd)->so_type==SOCK_STREAM) ? \
					tcp_protl : udp_protl)
#define HASOOBDATA(sd)	((sd)->so_oobmark || ((sd)->so_state & SS_RCVATMARK))


static void
xsi_cond_wait(condition_t cond)
{
    extern mutex_t sledgehammer_concurrency_control;

    condition_wait(cond, sledgehammer_concurrency_control);
} /* xsi_cond_wait */


static void
timeout_handler(Event e, boolean_t *timer_expired)
{
    xTrace0(xksocketp, TR_FUNCTIONAL_TRACE,
	    "timeout_handler(): timer expired");

    /* make sure other guy sees timeout: */
    condition_broadcast(selectable_event);
    *timer_expired = TRUE;
} /* timeout_handler */


client_t*
so_client_lookup(int client_id, boolean_t inhibit_bind)
{
    client_t *cl;

    if (MAP_RESOLVE(client_map, &client_id, &cl) == XK_FAILURE) {
	if (inhibit_bind) {
	    return 0;
	} /* if */
	/* new client: allocate and initialized new socket descs: */
	xTrace1(xksocketp, TR_EVENTS,
		"so_client_lookup: adding client id %d to client_map",
		client_id);
	cl = (client_t*) xMalloc(sizeof(client_t));
	bzero((void*) cl, sizeof(client_t));
	semInit(&cl->clone_done, 0);
	if (mapBind(client_map, &client_id, cl) == ERR_BIND) {
	    xTrace0(xksocketp, TR_ERRORS,
		    "so_client_lookup: mapBind() failed");
	} /* if */
    } /* if */
    return cl;
} /* so_client_lookup */


static inline int
so_connect(socket_t *sd, struct sockaddr_in *peer, XObj *session)
{
    Part p[2];
    u_int port;
    u_int lport;
    u_int addr;

    port = ntohs(peer->sin_port);

    partInit(p, ((sd->so_state & SS_ISNAMED) || sd->so_session) ? 2 : 1);
    partPush(p[REMOTE_PART], &peer->sin_addr.s_addr, sizeof(IPhost));
    partPush(p[REMOTE_PART], &port, sizeof(u_int));
    if (sd->so_state & SS_ISNAMED) {
	struct sockaddr_in *me = (struct sockaddr_in*) &sd->so_name;
	lport = ntohs(((struct sockaddr_in*)&sd->so_name)->sin_port);
	if (me->sin_addr.s_addr == INADDR_ANY) {
	    partPush(p[LOCAL_PART], ANY_HOST, 0);
	} else {
	    partPush(p[LOCAL_PART], &me->sin_addr.s_addr, sizeof(IPhost));
	} /* if */
	partPush(p[LOCAL_PART], &lport, sizeof(u_int));
    } else if (sd->so_session) {
	/*
	 * We want to open a session while connected to some other
	 * guy.  In this case, we like to use the socket's temporary
	 * port instead of another temporary port.
	 */
	xControl(sd->so_session, GETPARTICIPANTS, (char*)p, 2*sizeof(Part));
	lport = *(u_int*) partPop(p[LOCAL_PART]);
	addr = *(u_int*) partPop(p[LOCAL_PART]);
	partPush(p[LOCAL_PART], &addr, sizeof(IPhost));
	partPush(p[LOCAL_PART], &lport, sizeof(u_int));
    } /* if */

    /* create session: */
    xTrace2(xksocketp, TR_MAJOR_EVENTS,
	    "so_connect: opening session to %s port=%d",
	    inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
    *session = xOpen(xksock_protl, xksock_protl, XKSOCK_PROTL(sd), p);
    if (*session == ERR_XOBJ) {
	*session = 0;
	xTrace1(xksocketp, TR_ERRORS,
		"so_connect: xOpen(so_type=%d) failed", sd->so_type);
	return ECONNREFUSED;
    } /* if */
    return ESUCCESS;
} /* so_connect */


socket_t*
so_alloc_sock(void)
{
    socket_t *sd = (socket_t*) xMalloc(sizeof(socket_t));

    bzero((void*) sd, sizeof(socket_t));

    sd->so_rcnt = 1;	/* one reference */
    streamInit(&sd->so_rcv, 4*KB);

    return sd;
} /* so_alloc_sock */


static int
so_getsockname(socket_t *sd, struct sockaddr *name)
{
    Part p[2];
    struct sockaddr_in *me = (struct sockaddr_in*) name;

    if (sd->so_state & SS_ISNAMED) {
	bcopy((void*) &sd->so_name, (void*) name, sizeof(struct sockaddr));
    } else {
	if (!(sd->so_state & SS_ISCONNECTED)) {
	    xTrace0(xksocketp, TR_ERRORS,
		    "so_getsockname: socket is not connected");
	    return EBADF;
	} /* if */

	xControl(sd->so_session, GETPARTICIPANTS, (char*)p, 2*sizeof(Part));

	me->sin_family = AF_INET;
	me->sin_port = htons(*(int*) partPop(p[LOCAL_PART]));
	me->sin_addr.s_addr = *(int*) partPop(p[LOCAL_PART]);
    } /* if */

    xTrace2(xksocketp, TR_FUNCTIONAL_TRACE, "so_getsockname: addr=%s port=%d",
	    inet_ntoa(me->sin_addr), ntohs(me->sin_port));

    return ESUCCESS;
} /* so_getsockname */


void
so_new_session(socket_t *sd)
{
    if (sd->so_state & SS_NBIO) {
	int on = 1;

	xControl(sd->so_session, SETNONBLOCKINGIO, (char*)&on, sizeof(on));
    } /* if */
    if (sd->so_type == SOCK_STREAM) {
	/* tell TCP the size of the receive buffer: */
	u_short space;

	space = streamGetSize(&sd->so_rcv);
	xControl(sd->so_session, TCP_SETRCVBUFSIZE,
		 (char*)&space, sizeof(space));
	xControl(sd->so_session, TCP_SETRCVBUFSPACE,
		 (char*)&space, sizeof(space));
    } /* if */

    /* insert new connection in active map: */
    if (mapBind(active_map, &sd->so_session, sd) == ERR_BIND) {
	xTrace0(xksocketp, TR_ERRORS, "so_new_session: mapBind() failed");
    } /* if */
} /* so_new_session */


void
so_shutdown(socket_t *sd, int how)
{
#define CantRxTx (SS_CANTSENDMORE|SS_CANTRCVMORE)

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "so_shutdown(sd=%08x,how=%x): so_state=%x",
	    sd, how, sd->so_state);

    if (((sd->so_state | how) & CantRxTx) == CantRxTx) {
	int old_state = sd->so_state;
	conid_t con;
	struct sockaddr_in *me;
	u_int port;

	sd->so_state &= ~(SS_ISTMPPORT|SS_ISNAMED|SS_ISCONNECTED);

	me = (struct sockaddr_in*) &sd->so_name;

	port = ntohs(me->sin_port);
	if (old_state & SS_ISTMPPORT) {
	    /* undo xxx_GET_FREE_PROT_NUM: */
	    xTrace1(xksocketp, TR_MAJOR_EVENTS,
		    "so_shutdown: releasing temporary port number %d", 
		    port);
	    if (sd->so_type == SOCK_STREAM) {
		xControl(tcp_protl, TCP_RELEASEPROTNUM,
			 (char*) &port, sizeof(long));
	    } else {
		xControl(udp_protl, UDP_RELEASEPROTNUM,
			 (char*) &port, sizeof(long));
	    } /* if */
	} /* if */

	if ((sd->so_type == SOCK_DGRAM) && (old_state & SS_ISNAMED) ||
	    (sd->so_options & SO_ACCEPTCONN))
	{
	    /* disable passive opens: */
	    Part p[1];

	    port = ntohs(me->sin_port);

	    xTrace1(xksocketp, TR_MAJOR_EVENTS,
		    "so_shutdown: disabling passive open for port %d", port);

	    partInit(p, 1);
	    partPush(p[0], &port, sizeof(u_int));

	    xOpenDisable(xksock_protl, xksock_protl, XKSOCK_PROTL(sd), p);

	    /* unbind name from passive map: */

	    con.c_protocol = XKSOCK_PROTL(sd)->id;
	    con.c_port =
	      ntohs(((struct sockaddr_in*)&sd->so_name)->sin_port);
	    mapUnbind(passive_map, &con);
	} /* if */

	if (old_state & SS_ISCONNECTED) {
	    xTrace1(xksocketp, TR_MAJOR_EVENTS,
		    "so_shutdown: xClosing session %08x", sd->so_session);

	    mapUnbind(active_map, &sd->so_session);

	    xClose(sd->so_session);
	    sd->so_session = 0;
	} /* if */
    } /* if */
    sd->so_state |= how;
#undef CantRxTx
} /* so_shutdown */


static void
so_close(client_t *cl, int s)
{
    socket_t *sd = cl->sds[s];
    
    if (sd) {

	xTrace2(xksocketp, TR_MAJOR_EVENTS, "so_close(s=%d): rcnt=%d",
		s, sd->so_rcnt);

	--sd->so_rcnt;
	if (sd->so_rcnt == 0) {
	    so_shutdown(sd, SS_CANTSENDMORE | SS_CANTRCVMORE);
	    streamDestroy(&sd->so_rcv);
	    xFree((void*) sd);
	} /* if */
	cl->sds[s] = 0;
    } /* if */
} /* so_close */


static int
so_passive_open(socket_t *sd)
{
    conid_t con;
    struct sockaddr_in *me;
    Part p[1];
    int rc;

    me = (struct sockaddr_in*) &sd->so_name;

    if (!(sd->so_state & SS_ISNAMED)) {
	/* get system selected address: */
	so_getsockname(sd, (struct sockaddr*) me);
    } /* if */

    con.c_protocol = XKSOCK_PROTL(sd)->id;
    con.c_port = ntohs(me->sin_port);
    if (mapBind(passive_map, &con, sd) == ERR_BIND) {
	xTrace2(xksocketp, TR_ERRORS,
	    "so_passive_open: mapBind(prot=%d,port=%d) failed => EADDRINUSE",
		con.c_protocol, con.c_port);
	return EADDRINUSE;
    } /* if */

    xTrace2(xksocketp, TR_MAJOR_EVENTS,
	    "so_passive_open: enabling sessions on %s for port %d",
	    inet_ntoa(me->sin_addr), con.c_port);

    partInit(p, 1);
    partPush(p[0], &con.c_port, sizeof(u_int));

    rc = xOpenEnable(xksock_protl, xksock_protl, XKSOCK_PROTL(sd), p);
    if (rc != XK_SUCCESS) {
	xTrace0(xksocketp, TR_ERRORS, "so_passive_open: open enable failed");
	return EADDRINUSE;
    } /* if */
    
    return ESUCCESS;
} /* so_passive_open */


static int
so_receive(client_t *cl, socket_t *sd, int nbytes, void **buf, int *len,
	   int flags, struct sockaddr *from, int *fromlen)
{
    if (flags & MSG_OOB) {
	/* check if any oob data available: */
	if ((sd->so_oobmark == 0 && (sd->so_state & SS_RCVATMARK) == 0) ||
	    (sd->so_options & SO_OOBINLINE) || (*len < sizeof(char)) ||
	    !(sd->so_state & SS_ISCONNECTED))
	{
	    return EINVAL;
	} /* if */

	/* tell TCP_GETOOBDATA whether we want to peek at data only: */
	*(char*)buf = flags & MSG_PEEK;

	/* get oob data: */
	if (xControl(sd->so_session, TCP_GETOOBDATA,
		     (char*)*buf, sizeof(char)) < 0) {
	    return EWOULDBLOCK;
	} /* if */

	*len = sizeof(char);
	return ESUCCESS;
    } /* if */

    /* wait for data to arrive: */
    while (!streamLen(&sd->so_rcv)) {
	if (cl->signal_pending) {
	    xTrace0(xksocketp, TR_MAJOR_EVENTS,
		    "so_receive: signal pending returning EINTR");
	    cl->signal_pending = FALSE;
	    *len = 0;
	    return EINTR;
	} /* if */

	if ((sd->so_type == SOCK_STREAM) && !(sd->so_state & SS_ISCONNECTED)) {
	    if (sd->so_state & SS_CANTRCVMORE) {
		xTrace0(xksocketp, TR_MAJOR_EVENTS,
			"so_receive: connection closed");
		*len = 0;
		return ESUCCESS;
	    } else {
		xTrace0(xksocketp, TR_MAJOR_EVENTS,
			"so_receive: bad file descriptor");
		*len = 0;
		return EBADF;
	    } /* if */
	} /* if */

	if (sd->so_state & SS_NBIO) {
	    xTrace0(xksocketp, TR_MAJOR_EVENTS,
		    "so_receive: returning EWOULDBLOCK");
	    *len = 0;
	    return EWOULDBLOCK;
	} /* if */

	/* block for a message to arrive: */
	xTrace1(xksocketp, TR_MAJOR_EVENTS,
		"so_receive: wait for selectable_event (socket 0x%08x)", sd);
	xsi_cond_wait(selectable_event);
	xTrace1(xksocketp, TR_MAJOR_EVENTS,
		"so_receive: selectable_event occurred (socket 0x%08x)", sd);
    } /* while */

    sd->so_state &= ~SS_RCVATMARK;

    /* limit read by oobmark: */
    if (sd->so_oobmark && (nbytes > sd->so_oobmark)) {
	nbytes = sd->so_oobmark;
    } /* if */

    /* limit read to length of queue: */
    if (nbytes > streamLen(&sd->so_rcv)) {
	nbytes = streamLen(&sd->so_rcv);
    } /* if */

    /* ensure buffer is of appropriate size: */
    if (*len < nbytes) {
	/* have to allocate a buffer: */
	kern_return_t kr;

	xTrace1(xksocketp, TR_EVENTS, "so_receive: vm_allocate(nbytes=%d)",
		nbytes);
	kr = vm_allocate(mach_task_self(), (void*)buf, nbytes, TRUE);
	if (kr != KERN_SUCCESS) {
	    xTrace2(xksocketp, TR_ERRORS,
		    "so_receive: vm_allocate(%d): %s", nbytes,
		    mach_error_string(kr));
	    *len = 0;
	    return ENOMEM;
	} /* if */
    } /* if */

    *len = streamRemove(&sd->so_rcv, *buf, nbytes,
			flags | ((sd->so_type == SOCK_DGRAM)?MSG_ATOMIC:0),
			from, fromlen);

    if ((sd->so_type == SOCK_STREAM) && sd->so_session) {
	/*
	 * Inform TCP of available buffer space:
	 */
	u_short space = streamGetSpace(&sd->so_rcv);
	xControl(sd->so_session, TCP_SETRCVBUFSPACE,
		 (char*)&space, sizeof(space));
    } /* if */

    /* adjust oobmark: */
    if (sd->so_oobmark) {
	sd->so_oobmark -= *len;
	if (sd->so_oobmark == 0) {
	    sd->so_state |= SS_RCVATMARK;
	} /* if */
    } /* if */

    xTrace1(xksocketp, TR_MAJOR_EVENTS,
	    "so_receive: returning %d bytes", *len);

    return ESUCCESS;
} /* so_receive */


static inline int
send_buf(XObj session, void *buf, int len, int flags)
{
    Msg msg;
    int rc = ESUCCESS;

    msgConstructBuffer(&msg, buf, len);
    if (flags & MSG_OOB) {
	if (xControl(session, TCP_OOBPUSH, (char*)&msg, sizeof(Msg*)) < 0) {
	    rc = EINVAL;
	} /* if */
    } else {
	switch (xPush(session, &msg)) {
	  case XMSG_ERR_HANDLE:
	    /* return some error: */
	    xTrace0(xksocketp, TR_ERRORS, "send_buf: xPush() failed");
	    rc = ENOBUFS;
	    break;

	  case XMSG_ERR_WOULDBLOCK:
	    xTrace0(xksocketp, TR_ERRORS,
		    "send_buf: xPush() would block => EWOULDBLOCK");
	    rc = EWOULDBLOCK;
	    break;
	} /* if */
    } /* if */
    msgDestroy(&msg);
    return rc;
} /* send_buf */


static int
wait_for_connection(client_t *cl, socket_t *sd)
{
    while (!sd->so_qlen) {
	if (cl->signal_pending) {
	    xTrace0(xksocketp, TR_MAJOR_EVENTS,
		    "wait_for_connection: signal pending returning EINTR");
	    cl->signal_pending = FALSE;
	    return EINTR;
	} /* if */

	if (sd->so_state & SS_NBIO) {
	    xTrace0(xksocketp, TR_MAJOR_EVENTS,
		    "wait_for_connection: returning EWOULDBLOCK");
	    return EWOULDBLOCK;
	} /* if */

	if (sd->so_state & SS_CANTRCVMORE) {
	    xTrace0(xksocketp, TR_MAJOR_EVENTS,
		    "wait_for_connection: can't receive more => EBADF");
	    return EBADF;
	} /* if */
	xTrace0(xksocketp, TR_MAJOR_EVENTS,
		"wait_for_connection: wait for selectable_event");
	xsi_cond_wait(selectable_event);
	xTrace0(xksocketp, TR_MAJOR_EVENTS,
		"wait_for_connection: selectable_event occurred");
    } /* while */
    return ESUCCESS;
} /* wait_for_connection */


void
bye_bye(mach_port_t port)
{
    int cid;
    client_t *cl;
    int s;

    xTrace1(xksocketp, TR_FUNCTIONAL_TRACE, "bye_bye(port=%d)", port);

    if (MAP_RESOLVE(port2cid_map, &port, &cid) == XK_FAILURE) {
	/* client unknown, nothing to be done: */
	xTrace1(xksocketp, TR_ERRORS, "bye_bye: no client with port %d", port);
	return;
    } /* if */
    mapUnbind(port2cid_map, &port);

    xTrace1(xksocketp, TR_FUNCTIONAL_TRACE, "bye_bye: client %d died", cid);

    cl = so_client_lookup(cid, TRUE);
    if (!cl) {
	/* client unknown, nothing to be done: */
	xTrace1(xksocketp, TR_ERRORS, "bye_bye: no client with id %d", cid);
	return;
    } /* if */
    mapUnbind(client_map, &cid);

    for (s = 0; s < FD_SETSIZE; s++) {
	so_close(cl, s);
    } /* for */
    xFree((void*) cl);
} /* bye_bye */


kern_return_t
do_hello(mach_port_t server, int cid, mach_port_t alive_port,
	 fd_set *is_xsi_fd, int *errno)
{
    mach_port_t previous;
    kern_return_t kr;
    client_t *cl;
    socket_t **sds;
    int s;

    xTrace1(xksocketp, TR_FUNCTIONAL_TRACE, "do_hello(cid=%d)", cid);

    *errno = ESUCCESS;

    if (mapBind(port2cid_map, &alive_port, cid) == ERR_BIND) {
	xTrace0(xksocketp, TR_ERRORS, "do_hello: mapBind() failed");
    } /* if */

    kr = mach_port_request_notification(mach_task_self(),
					alive_port, MACH_NOTIFY_DEAD_NAME,
					TRUE, notify,
					MACH_MSG_TYPE_MAKE_SEND_ONCE,
					&previous);
    if ((kr != KERN_SUCCESS) || (previous != MACH_PORT_NULL)) {
	xTrace1(xksocketp, TR_ERRORS,
		"do_hello: mach_port_request_notification: %s",
		mach_error_string(kr));
    } /* if */

    cl = so_client_lookup(cid, FALSE);
    sds = cl->sds;

    FD_ZERO(is_xsi_fd);
    for (s = 0; s < FD_SETSIZE; s++) {
	if (sds[s]) {
	    FD_SET(s, is_xsi_fd);
	} /* if */
    } /* for */

    return KERN_SUCCESS;
} /* do_hello */


kern_return_t
do_clone(mach_port_t server, int cid, int pid, mach_port_t alive_port,
	 int *errno)
{
    mach_port_t previous;
    kern_return_t kr;
    client_t *cl;
    socket_t **sds;
    client_t *pcl;
    socket_t **psds;
    int s;

    xTrace2(xksocketp, TR_FUNCTIONAL_TRACE, "do_clone(cid=%d,parent=%d)",
	    cid, pid);

    *errno = ESUCCESS;

    if (mapBind(port2cid_map, &alive_port, cid) == ERR_BIND) {
	xTrace0(xksocketp, TR_ERRORS, "do_clone: mapBind() failed");
    } /* if */

    kr = mach_port_request_notification(mach_task_self(),
					alive_port, MACH_NOTIFY_DEAD_NAME,
					TRUE, notify,
					MACH_MSG_TYPE_MAKE_SEND_ONCE,
					&previous);
    if ((kr != KERN_SUCCESS) || (previous != MACH_PORT_NULL)) {
	xTrace1(xksocketp, TR_ERRORS,
		"do_clone: mach_port_request_notification: %s",
		mach_error_string(kr));
    } /* if */

    cl = so_client_lookup(cid, FALSE);
    sds = cl->sds;
    pcl = so_client_lookup(pid, TRUE);
    if (!pcl) {
	/* parent is not a client of ours, so just return: */
	xTrace1(xksocketp, TR_MAJOR_EVENTS,
		"do_clone: parent %d is not an xsi client", pid);
	return KERN_SUCCESS;
    } /* if */
    psds = pcl->sds;

    for (s = 0; s < FD_SETSIZE; s++) {
	sds[s] = psds[s];
	if (sds[s]) {
	    /* adjust reference count: */
	    ++sds[s]->so_rcnt;
	} /* if */
    } /* for */

    semSignal(&pcl->clone_done);

    return KERN_SUCCESS;
} /* do_clone */


kern_return_t
do_await_clone_done(mach_port_t server, int cid, int *errno)
{
    client_t *cl;

    xTrace1(xksocketp, TR_FUNCTIONAL_TRACE, "do_await_clone_done(cid=%d)",
	    cid);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);

    semWait(&cl->clone_done);

    return KERN_SUCCESS;
} /* do_await_clone_done */


kern_return_t
do_close(mach_port_t server, int cid, int s, int *errno)
{
    client_t *cl;

    xTrace2(xksocketp, TR_FUNCTIONAL_TRACE, "do_close(cid=%d,s=%d)", cid, s);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, TRUE);
    if (cl) {
	so_close(cl, s);
    } /* if */

    return KERN_SUCCESS;
} /* do_close */


/*
 * Create and initialize a new socket structure.
 */
kern_return_t
do_socket(mach_port_t server, int cid,
	  int domain, int type, int protocol, int s, int *errno)
{
    socket_t **sds;
    socket_t *sd;

    xTrace5(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_socket(cid=%d,dom=%d,typ=%d,protl=%d,s=%d)",
	    cid, domain, type, protocol, s);

    *errno = ESUCCESS;

    sds = so_client_lookup(cid, FALSE)->sds;

    if ((type != SOCK_STREAM) && (type != SOCK_DGRAM)) {
	/* unsupported type: */
	xTrace1(xksocketp, TR_ERRORS,
		"do_socket: socket type %d unsupported", type);
	*errno = ESOCKTNOSUPPORT;
	return KERN_SUCCESS;
    } /* if */

    sd = sds[s] = so_alloc_sock();

    sd->so_type = type;

    return KERN_SUCCESS;
} /* do_socket */


kern_return_t
do_ioctl(mach_port_t server, int cid, int s, int request,
	 char *inp, int inlen, char **outp, int *outlen, int *errno)
{
    client_t *cl;
    socket_t *sd;
    char *errreq;

    xTrace5(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_ioctl(cid=%d,s=%d,request=%d,inlen=%d,outlen=%d)",
	    cid, s, request, inlen, *outlen);

    *errno = ESUCCESS;
    *outlen = 0;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    if (!sd) {
	*errno = EBADF;
	return KERN_SUCCESS;
    } /* if */

    errreq = 0;
    switch (request) {
      case FIONREAD:
	*(int*)*outp = streamLen(&sd->so_rcv);
	*outlen = sizeof(int);
	break;

      case FIONBIO:
	if (sd->so_session) {
	    int on = 1;
	    xControl(sd->so_session, SETNONBLOCKINGIO, (char*)&on, sizeof(on));
	} /* if */
	if (*(int*)inp) {
	    sd->so_state |= SS_NBIO;
	} else {
	    sd->so_state &= ~SS_NBIO;
	} /* if */
	break;

      case FIOASYNC:	errreq = "FIOASYNC"; break;
      case FIOSETOWN:	errreq = "FIOSETOWN"; break;
      case FIOGETOWN:	errreq = "FIOGETOWN"; break;
      case SIOCSHIWAT:	errreq = "SIOCSHIWAT"; break;
      case SIOCGHIWAT:	errreq = "SIOCGHIWAT"; break;
      case SIOCSLOWAT:	errreq = "SIOCSLOWAT"; break;
      case SIOCGLOWAT:	errreq = "SIOCGLOWAT"; break;

      case SIOCATMARK:
	*(int*)*outp = sd->so_state & SS_RCVATMARK;
	*outlen = sizeof(int);
	break;

      case SIOCSPGRP:
	sd->so_pgrp = *(int*)inp;
	sd->so_cid = cid;
	break;

      case SIOCGPGRP:
	*(int*)*outp = sd->so_pgrp;
	*outlen = sizeof(int);
	break;
    } /* switch */

    if (errreq) {
	xTrace1(xksocketp, TR_ERRORS, "do_ioctl: request %s unsupported",
		errreq);
	*errno = EINVAL;
    } /* if */

    return KERN_SUCCESS;
} /* do_ioctl */


kern_return_t
do_fcntl(mach_port_t server, int cid, int s, int cmd, int *arg, int *errno)
{
    client_t *cl;
    socket_t *sd;
    char *errreq;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_fcntl(cid=%d,s=%d,cmd=%d,arg=%d)", cid, s, cmd, *arg);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    if (!sd) {
	*errno = EBADF;
	return KERN_SUCCESS;
    } /* if */

    errreq = 0;
    switch (cmd) {
      case F_DUPFD: errreq = "F_DUPFD"; *arg = 0; break;
      case F_GETFD: errreq = "F_GETFD"; *arg = 0; break;
      case F_SETFD: errreq = "F_SETFD"; *arg = 0; break;
      case F_GETFL: errreq = "F_GETFL"; *arg = 0; break;
      case F_SETFL: errreq = "F_SETFL"; *arg = 0; break;

      case F_GETOWN:
	*arg = sd->so_pgrp;
	break;

      case F_SETOWN:
	sd->so_pgrp = *arg;
	sd->so_cid = cid;
	*arg = 0; 
	break;
    } /* switch */

    if (errreq) {
	xTrace1(xksocketp, TR_ERRORS, "do_fcntl: request %s unsupported",
		errreq);
	*errno = EINVAL;
    } /* if */

    return KERN_SUCCESS;
} /* do_fcntl */


kern_return_t
do_connect(mach_port_t server, int cid, int s,
	   xsi_sockaddr_t name, int namelen, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_connect(cid=%d,s=%d,namelen=%d)", cid, s, namelen);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    /* check if address is of right family: */
    if ((name->sa_family != AF_INET) && (name->sa_family != AF_UNSPEC)) {
	xTrace1(xksocketp, TR_ERRORS,
		"do_connect: address family %d unsupported", name->sa_family);
	*errno = EAFNOSUPPORT;
	return KERN_SUCCESS;
    } /* if */

    /*
     * If socket is of type SOCK_DGRAM and there is already a connection,
     * disassociate old connection:
     */
    if (sd->so_state & SS_ISCONNECTED) {
	if (sd->so_type == SOCK_STREAM) {
	    xTrace0(xksocketp, TR_ERRORS,
		    "do_connect: socket is already connected");
	    *errno = EISCONN;
	    return KERN_SUCCESS;
	} else {
	    so_shutdown(sd, SS_CANTSENDMORE|SS_CANTRCVMORE);
	} /* if */
    } /* if */

    /* establish new connection: */
    *errno = so_connect(sd, (struct sockaddr_in*) name, &sd->so_session);
    if (*errno != ESUCCESS) {
	return KERN_SUCCESS;
    } /* if */

    sd->so_state |= SS_ISCONNECTED;

    so_new_session(sd);

    xTrace1(xksocketp, TR_MAJOR_EVENTS,
	    "do_connect: session %08x successfully created", sd->so_session);

    return KERN_SUCCESS;
} /* do_connect */


kern_return_t
do_send(mach_port_t server, int cid, int s,
	void *msg, int len, int *nsent, int flags, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_send(cid=%d,s=%d,len=%d,flags=0x%x)",
	    cid, s, len, flags);

    *errno = ESUCCESS;
    *nsent = 0;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    if (flags & MSG_DONTROUTE) {
	xTrace0(xksocketp, TR_ERRORS, "do_send: MSG_DONTROUTE ignored");
    } /* if */

    if (!(sd->so_state & SS_ISCONNECTED)) {
	/* must be in connected state */
	xTrace0(xksocketp, TR_ERRORS, "do_send: socket is not connected");
	*errno = EBADF;
	return KERN_SUCCESS;
    } /* if */

    *errno = send_buf(sd->so_session, msg, len, flags);
    if (*errno == ESUCCESS) {
	xTrace1(xksocketp, TR_MAJOR_EVENTS, "do_send: %d bytes sent", len);
	*nsent = len;
    } /* if */
    return KERN_SUCCESS;
} /* do_send */


kern_return_t
do_sendto(mach_port_t server, int cid, int s,
	  void *msg, int len, int *nsent, int flags,
	  struct sockaddr *toaddr, int tolen, int *errno)
{
    client_t *cl;
    socket_t *sd;
    XObj session;

    xTrace5(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_sendto(cid=%d,s=%d,len=%d,flags=0x%x,tolen=%d)",
	    cid, s, len, flags, tolen);

    *errno = ESUCCESS;
    *nsent = 0;

    if (flags & MSG_DONTROUTE) {
	xTrace0(xksocketp, TR_ERRORS, "do_sendto: MSG_DONTROUTE ignored");
    } /* if */

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    *errno = so_connect(sd, (struct sockaddr_in*) toaddr, &session);
    if (*errno != ESUCCESS) {
	return KERN_SUCCESS;
    } /* if */

    /* send message: */
    *errno = send_buf(session, msg, len, flags);
    if (*errno == ESUCCESS) {
	xTrace1(xksocketp, TR_MAJOR_EVENTS, "do_sendto: %d bytes sent", len);
	*nsent = len;
    } /* if */

    /* close session: */
    xClose(session);

    return KERN_SUCCESS;
} /* do_sendto */


kern_return_t
do_select(mach_port_t server, int cid, int seqno, int *nfds, int specified,
	  fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	  struct timeval *timeout, int *errno)
{
    Event timer = 0;
    int nreadyfds = 0;
    fd_set ready_rfds;
    fd_set ready_wfds;
    fd_set ready_efds;
    client_t *cl;
    socket_t **sds;
    boolean_t timer_expired = FALSE;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_select(cid=%d,seqno=%d,nfds=%d,timeout=%08x)",
	    cid, seqno, *nfds, timeout);

    *errno = ESUCCESS;
    cl = so_client_lookup(cid, FALSE);
    sds = cl->sds;

    FD_ZERO(&ready_rfds);
    FD_ZERO(&ready_wfds);
    FD_ZERO(&ready_efds);

    if (specified & SELECT_TIMEOUT) {
	/* setup time-out: */
	if ((timeout->tv_sec == 0) && (timeout->tv_usec == 0)) {
	    /* force a poll: */
	    xTrace0(xksocketp, TR_MAJOR_EVENTS,
		    "do_select: doing a poll without timeout");
	    timer_expired = TRUE;
	} else {
	    xTrace1(xksocketp, TR_MAJOR_EVENTS,
		    "do_select: setting timer to %d usecs",
		    1000000 * timeout->tv_sec + timeout->tv_usec);
	    timer = evSchedule((void (*)())timeout_handler, &timer_expired,
			       1000000 * timeout->tv_sec + timeout->tv_usec);
	} /* if */
    } /* if */

    while (TRUE) {
	int s;

	for (s = 0; s < *nfds; s++) {
	    socket_t *sd = sds[s];

	    if ((specified & SELECT_READFDS) && FD_ISSET(s, readfds)) {
		if (!sd) {
		    *errno = EBADF;
		    break;
		} /* if */

		if (streamLen(&sd->so_rcv) || sd->so_qlen) {
		    FD_SET(s, &ready_rfds);
		    ++nreadyfds;
		} else if ((sd->so_type == SOCK_STREAM) &&
			   !((sd->so_state & SS_ISCONNECTED) ||
			     (sd->so_options & SO_ACCEPTCONN)))
		{
		    if (sd->so_state & SS_CANTRCVMORE) {
#if 0
			kill(cid, SIGPIPE);
			*errno = EINTR;
			break;
#else
			FD_SET(s, &ready_rfds);
			++nreadyfds;
#endif
		    } else {
			*errno = EBADF;
			break;
		    } /* if */
		} /* if */
	    } /* if */

	    if ((specified & SELECT_WRITEFDS) && FD_ISSET(s, writefds)) {
		if (!sd) {
		    *errno = EBADF;
		    break;
		} /* if */

		if (sd->so_type == SOCK_STREAM) {
		    u_short space;

		    if (!(sd->so_state & SS_ISCONNECTED)) {
			kill(cid, SIGPIPE);
			*errno = EINTR;
			break;
		    } /* if */

		    /* see if TCP has buffer space available: */
		    if (xControl(sd->so_session, TCP_GETSNDBUFSPACE,
				 (char*)&space, sizeof(space)) == 0) {
			if (space > 0) {
			    FD_SET(s, &ready_wfds);
			    ++nreadyfds;
			} /* if */
		    } /* if */
		} else {
		    /* UDP is always writable: */
		    FD_SET(s, &ready_wfds);
		    ++nreadyfds;
		} /* if */
	    } /* if */

	    if ((specified & SELECT_EXCEPTFDS) && FD_ISSET(s, exceptfds)) {
		if (!sd) {
		    *errno = EBADF;
		    break;
		} /* if */

		if (HASOOBDATA(sd)) {
		    FD_SET(s, &ready_efds);
		    ++nreadyfds;
		} /* if */
	    } /* if */
	} /* for */

	if (nreadyfds || timer_expired || (cl->select_cancel_seqno == seqno) ||
	    (cl->signal_pending))
	{
	    break;
	} /* if */

	xTrace1(xksocketp, TR_MAJOR_EVENTS,
		"do_select(cid=%d): wait for selectable_event", cid);
	xsi_cond_wait(selectable_event);
	xTrace1(xksocketp, TR_MAJOR_EVENTS,
		"do_select(cid=%d): selectable_event occurred", cid);
    } /* while */

    if (timer) {
	/* cancel alarm: */
	evCancel(timer);
    } /* if */

    if (*errno != ESUCCESS) {
	*nfds = 0;
	return KERN_SUCCESS;
    } /* if */

    *nfds = nreadyfds;
    *readfds = ready_rfds;
    *writefds = ready_wfds;
    *exceptfds = ready_efds;
    xTrace1(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_select: %d descriptors are ready for i/o", nreadyfds);
    if (!nreadyfds) {
	if (cl->signal_pending) {
	    xTrace0(xksocketp, TR_MAJOR_EVENTS,
		    "do_select: signal pending");
	    cl->signal_pending = FALSE;
	    *errno = EINTR;
	} else if (timer_expired) {
	    /* timer expired, return an otherwise unused errno: */
	    *errno = EXDEV;
	} else {
	    /* select was cancelled, return an otherwise unused errno: */
	    *errno = ESRCH;
	} /* if */
    } /* if */
    return KERN_SUCCESS;
} /* do_select */


kern_return_t
do_select_cancel(mach_port_t server, int cid, int seqno, int *errno)
{
    client_t *cl;

    xTrace2(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_select_cancel(cid=%d,seqno=%d)", cid, seqno);

    *errno = ESUCCESS;
    cl = so_client_lookup(cid, FALSE);

    cl->select_cancel_seqno = seqno;
    condition_broadcast(selectable_event);

    return KERN_SUCCESS;
} /* do_select_cancel */


kern_return_t
do_recv(mach_port_t server, int cid, int s,
	void **buf, int *len, int maxlen, int flags, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_recv(cid=%d,s=%d,maxlen=%d,flags=0x%x)",
	    cid, s, maxlen, flags);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    *errno = so_receive(cl, sd, maxlen, buf, len, flags, 0, 0);

    return KERN_SUCCESS;
} /* do_recv */


kern_return_t
do_recvfrom(mach_port_t server, int cid, int s,
	    void **buf, int *len, int maxlen, int flags,
	    struct sockaddr *from, int *fromlen, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_recvfrom(cid=%d,s=%d,maxlen=%d,flags=0x%x)",
	    cid, s, maxlen, flags);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    *errno = so_receive(cl, sd, maxlen, buf, len, flags, from, fromlen);

    return KERN_SUCCESS;
} /* do_recvfrom */


kern_return_t
do_getpeername(mach_port_t server, int cid, int s,
	       struct sockaddr *name, int *namelen, int *errno)
{
    client_t *cl;
    socket_t *sd;
    Part p[2];
    struct sockaddr_in *peer = (struct sockaddr_in*) name;

    xTrace2(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_getpeername(cid=%d,s=%d)", cid, s);

    *errno = ESUCCESS;
    *namelen = 0;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    if (!(sd->so_state & SS_ISCONNECTED)) {
	xTrace1(xksocketp, TR_ERRORS,
		"do_getpeername: socket %d is not connected", s);
	*errno = EBADF;
	return KERN_SUCCESS;
    } /* if */

    xControl(sd->so_session, GETPARTICIPANTS, (char*)p, 2*sizeof(Part));

    peer->sin_family = AF_INET;
    peer->sin_port = htons(*(int*) partPop(p[REMOTE_PART]));
    peer->sin_addr.s_addr = *(int*) partPop(p[REMOTE_PART]);
    *namelen = sizeof(struct sockaddr_in);

    xTrace2(xksocketp, TR_FUNCTIONAL_TRACE, "do_getpeername: addr=%s port=%d",
	    inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));

    return KERN_SUCCESS;
} /* do_getpeername */


kern_return_t
do_getsockname(mach_port_t server, int cid, int s,
	       struct sockaddr *name, int *namelen, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace2(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_getsockname(cid=%d,s=%d)", cid, s);

    *namelen = 0;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    *errno = so_getsockname(sd, name);
    if (*errno == ESUCCESS) {
	*namelen = sizeof(struct sockaddr);
    } /* if */

    return ESUCCESS;
} /* do_getsockname */


kern_return_t
do_read(mach_port_t server, int cid, int s,
	void **buf, int *len, int nbytes, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_read(cid=%d,s=%d,nbytes=%d)", cid, s, nbytes);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    *errno = so_receive(cl, sd, nbytes, buf, len, 0, 0, 0);

    return KERN_SUCCESS;
} /* do_read */


kern_return_t
do_write(mach_port_t server, int cid, int s,
	void *buf, int len, int *nbytes, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_write(cid=%d,s=%d,nbytes=%d)", cid, s, len);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    if (!(sd->so_state & SS_ISCONNECTED)) {
	/* must be in connected state */
	xTrace0(xksocketp, TR_ERRORS, "do_write: socket is not connected");
	if (sd->so_state & SS_CANTSENDMORE) {
	    /* send SIGPIPE to sender: */
	    kill(cid, SIGPIPE);
	    *errno = EINTR;
	} else {
	    *errno = EBADF;
	} /* if */
	return KERN_SUCCESS;
    } /* if */

    *errno = send_buf(sd->so_session, buf, len, 0);
    if (*errno == ESUCCESS) {
	xTrace1(xksocketp, TR_MAJOR_EVENTS, "do_write: %d bytes sent", len);
	*nbytes = len;
    } /* if */

    return KERN_SUCCESS;
} /* do_write */


kern_return_t
do_bind(mach_port_t server, int cid, int s,
	xsi_sockaddr_t name, int namelen, int *errno)
{
    client_t *cl;
    socket_t *sd;
    u_int port;

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_bind(cid=%d,s=%d,namelen=%d)", cid, s, namelen);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    /* check if address is of right family: */
    if ((name->sa_family != AF_INET) && (name->sa_family != AF_UNSPEC)) {
	xTrace1(xksocketp, TR_ERRORS,
		"do_bind: address family %d unsupported", name->sa_family);
	*errno = EAFNOSUPPORT;
	return KERN_SUCCESS;
    } /* if */

    port = ntohs(((struct sockaddr_in*)name)->sin_port);
    if (!port) {
	/* pick a free port: */
	if (sd->so_type == SOCK_STREAM) {
	    xControl(tcp_protl, TCP_GETFREEPROTNUM,
		     (char*)&port, sizeof(port));
	} else {
	    xControl(udp_protl, UDP_GETFREEPROTNUM,
		     (char*)&port, sizeof(port));
	} /* if */

	((struct sockaddr_in*)name)->sin_port = htons(port);
	sd->so_state |= SS_ISTMPPORT;

	xTrace1(xksocketp, TR_MAJOR_EVENTS,
		"do_bind: got temporary port number %d", port);
    } /* if */

    sd->so_state |= SS_ISNAMED;
    bcopy((void*) name, (void*) &sd->so_name, sizeof(sd->so_name));

    if (sd->so_type == SOCK_DGRAM) {
	/* do a passive open to enable reception of incoming packets: */
	*errno = so_passive_open(sd);
    } /* if */

    return KERN_SUCCESS;
} /* do_bind */


kern_return_t
do_listen(mach_port_t server, int cid, int s, int backlog, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_listen(cid=%d,s=%d,backlog=%d)", cid, s, backlog);

    *errno = ESUCCESS;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    if ((sd->so_type != SOCK_STREAM) && (sd->so_type != SOCK_SEQPACKET)) {
	xTrace1(xksocketp, TR_ERRORS,
		"do_listen: socket type %d is not appropriate", sd->so_type);
	*errno = EOPNOTSUPP;
	return KERN_SUCCESS;
    } /* if */

    if (!(sd->so_state & SS_ISNAMED)) {
	xTrace0(xksocketp, TR_ERRORS,
		"do_listen: socket has no name bound");
	*errno = EOPNOTSUPP;
	return KERN_SUCCESS;
    } /* if */

    *errno = so_passive_open(sd);
    if (*errno != ESUCCESS) {
	return KERN_SUCCESS;
    } /* if */

    sd->so_options |= SO_ACCEPTCONN;
    sd->so_qlimit = MAX(1, backlog);
    sd->so_qlen = 0;
    sd->so_q = 0;

    return KERN_SUCCESS;
} /* do_listen */


kern_return_t
do_accept(mach_port_t server, int cid, int s,
	  struct sockaddr *addr, int *addrlen, int ns, int *errno)
{
    client_t *cl;
    socket_t *sd;

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_accept(cid=%d,s=%d,ns=%d)", cid, s, ns);

    *errno = ESUCCESS;
    *addrlen = 0;

    cl = so_client_lookup(cid, FALSE);
    sd = cl->sds[s];

    if (!(sd->so_options & SO_ACCEPTCONN)) {
	xTrace0(xksocketp, TR_ERRORS,
		"do_accept: socket does not accept connections");
	*errno = EOPNOTSUPP;
	return KERN_SUCCESS;
    } /* if */

    *errno = wait_for_connection(cl, sd);
    if (*errno != ESUCCESS) {
	return KERN_SUCCESS;
    } /* if */

    /* dequeue pending connection: */
    cl->sds[ns] = sd->so_q;
    sd->so_q = sd->so_q->so_q;
    --sd->so_qlen;

    /* socket has now a reference: */
    sd->so_state &= ~SS_NOFDREF;

    return KERN_SUCCESS;
} /* do_accept */


kern_return_t
do_dup2(mach_port_t server, int cid, int from, int to, int *errno)
{
    client_t *cl;

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_dup2(cid=%d,from=%d,to=%d)", cid, from, to);

    *errno = ESUCCESS;

    if (from == to) {
	return KERN_SUCCESS;
    } /* if */

    cl = so_client_lookup(cid, FALSE);

    if (cl->sds[to]) {
	/* close descriptor first: */
	so_close(cl, to);
    } /* if */

    /* copy reference to descriptor: */
    cl->sds[to] = cl->sds[from];

    if (cl->sds[to]) {
	/* adjust reference count: */
	++cl->sds[to]->so_rcnt;
    } /* if */

    return KERN_SUCCESS;
} /* do_dup2 */


kern_return_t
do_getsockopt(mach_port_t server, int cid, int s,
	      int level, int optname, void **buf, int *len, int *errno)
{
    socket_t *sd;
    char *errreq;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_getsockopt(cid=%d,s=%d,level=%d,name=%d)",
	    cid, s, level, optname);

    *errno = ESUCCESS;
    *len = sizeof(int);

    if (level != SOL_SOCKET) {
	*errno = ENOPROTOOPT;
	*len = 0;
	return KERN_SUCCESS;
    } /* if */

    sd = so_client_lookup(cid, FALSE)->sds[s];

    errreq = 0;
    switch (optname) {
      case SO_DEBUG:	 errreq = "SO_DEBUG"; break;
      case SO_KEEPALIVE: errreq = "SO_KEEPALIVE"; break;
      case SO_DONTROUTE: errreq = "SO_DONTROUTE"; break;
      case SO_LINGER:	 errreq = "SO_LINGER"; break;
      case SO_BROADCAST: errreq = "SO_BROADCAST"; break;

      case SO_OOBINLINE:
	if (sd->so_type == SOCK_STREAM) {
	    *(int*)*buf = sd->so_options & SO_OOBINLINE;
	} else {
	    *errno = ENOPROTOOPT;
	} /* if */
	break;

      case SO_SNDBUF:
	if (sd->so_type == SOCK_STREAM) {
	    if (sd->so_session) {
		*(int*)*buf = sd->so_snd_size;
	    } else {
		*errno = EBADF;
	    } /* if */
	} else {
	    *errno = ENOPROTOOPT;
	} /* if */
	break;

      case SO_RCVBUF:
	if (sd->so_session) {
	    *(int*)*buf = streamGetSize(&sd->so_rcv);
	} else {
	    *errno = EBADF;
	} /* if */
	break;

      case SO_REUSEADDR:
	*(int*)*buf = sd->so_options & SO_REUSEADDR;
	break;

      case SO_TYPE:
	*(int*)*buf = sd->so_type;
	break;

      case SO_ERROR:
	*(int*)*buf = sd->so_error;
	sd->so_error = ESUCCESS;
	break;

      default:
	errreq = "???"; break;
    } /* switch */
    if (errreq) {
	xTrace1(xksocketp, TR_ERRORS, "do_getsockopt: request %s ignored",
		errreq);
	*len = 0;
    } /* if */
    return KERN_SUCCESS;
} /* do_getsockopt */


kern_return_t
do_setsockopt(mach_port_t server, int cid, int s,
	      int level, int optname, void *buf, int len, int *errno)
{
    socket_t *sd;
    char *errreq;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "do_setsockopt(cid=%d,s=%d,level=%d,name=%d)",
	    cid, s, level, optname);

    *errno = ESUCCESS;

    if (level != SOL_SOCKET) {
	*errno = ENOPROTOOPT;
	return KERN_SUCCESS;
    } /* if */

    sd = so_client_lookup(cid, FALSE)->sds[s];

    errreq = 0;
    switch (optname) {
      case SO_DEBUG:	 errreq = "SO_DEBUG"; break;
      case SO_KEEPALIVE: errreq = "SO_KEEPALIVE"; break;
      case SO_DONTROUTE: errreq = "SO_DONTROUTE"; break;
      case SO_LINGER:	 errreq = "SO_LINGER"; break;
      case SO_BROADCAST: errreq = "SO_BROADCAST"; break;

      case SO_OOBINLINE:
	if (sd->so_type == SOCK_STREAM) {
	    if (*(int*)buf) {
		sd->so_options |= SO_OOBINLINE;
	    } else {
		sd->so_options &= ~SO_OOBINLINE;
	    } /* if */
	    if (sd->so_session) {
		xControl(sd->so_session, TCP_SETOOBINLINE,
			 (char*)buf, sizeof(int));
	    } /* if */
	} else {
	    *errno = ENOPROTOOPT;
	} /* if */
	break;

      case SO_SNDBUF:
	if (sd->so_type == SOCK_STREAM) {
	    if (sd->so_session) {
		sd->so_snd_size = *(int*)buf;
		xControl(sd->so_session, TCP_SETSNDBUFSIZE,
			 (char*)&sd->so_snd_size, sizeof(u_short));
	    } else {
		*errno = EBADF;
	    } /* if */
	} else {
	    *errno = ENOPROTOOPT;
	} /* if */
	break;

      case SO_RCVBUF:
	if (sd->so_session) {
	    streamSetSize(&sd->so_rcv, *(int*)buf);
	} else {
	    *errno = EBADF;
	} /* if */
	break;

      case SO_TYPE:	 errreq = "SO_TYPE"; *errno = ENOPROTOOPT; break;
      case SO_ERROR:	 errreq = "SO_ERROR"; *errno = ENOPROTOOPT; break;
      case SO_REUSEADDR:
	if (len != sizeof(int)) {
	    *errno = EFAULT;
	    return KERN_SUCCESS;
	} /* if */
	if (*(int*)buf) {
	    sd->so_options |= SO_REUSEADDR;
	} else {
	    sd->so_options &= ~SO_REUSEADDR;
	} /* if */
	break;

      default:
	errreq = "???"; break;
    } /* switch */
    if (errreq) {
	xTrace1(xksocketp, TR_ERRORS, "do_setsockopt: request %s ignored",
		errreq);
    } /* if */
    return KERN_SUCCESS;
} /* do_setsockopt */

			/*** end of xsi_services.c ***/
