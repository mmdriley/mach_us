/*
 * $RCSfile: xksocket.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/02 00:08:32 $
 * $Author: menze $
 *
 * $Log: xksocket.c,v $
 * Revision 1.4  1993/02/02  00:08:32  menze
 * copyright change
 *
 * Revision 1.3  1993/01/26  08:10:49  menze
 * Added include files and prototypes
 *
 * Revision 1.2  1992/12/01  22:15:08  menze
 * *** empty log message ***
 *
 * Revision 1.1  1992/06/30  21:17:30  davidm
 * Initial revision
 *
 */
#include <stdio.h>
#include <sys/signal.h>
#include "xksocket.h"
#include "xk_mach.h"
#include "xsi.h"
#include "xsi_services.h"

#ifdef __STDC__

int	kill( int, int );

#endif


int tracexksocketp = 0;
XObj xksock_protl = 0;
XObj udp_protl = 0;
XObj tcp_protl = 0;


static xkern_return_t
xksocket_demux(XObj self, XObj lls, Msg *msg)
{
    socket_t *sd;
    struct sockaddr sa;
    struct sockaddr_in *from = (struct sockaddr_in*) &sa;
    Part p[2];

    xTrace3(xksocketp, TR_FUNCTIONAL_TRACE,
	    "xksocket_demux(self=%08x,lls=%08x,msglen=%d)",
	    self, lls, msgLen(msg));

    /* enqueue message: */

    if (MAP_RESOLVE(active_map, &lls, &sd) == XK_FAILURE) {
	xTrace1(xksocketp, TR_ERRORS,
	 "xksocket_demux: couldn't find socket for session %08x, dropping msg",
		lls);
	return XK_SUCCESS;
    } /* if */

    if (!sd->so_session) {
	/* remove temporary session from active map: */
	mapUnbind(active_map, &lls);
    } /* if */

    if (sd->so_state & SS_CANTRCVMORE) {
	/* receiving disabled: */
	xTrace1(xksocketp, TR_ERRORS,
	   "xksocket_demux: socket %08x has receiving disabled, dropping msg",
		sd);
	return XK_FAILURE;
    } /* if */

    if (streamIsFlooded(&sd->so_rcv)) {
	xTrace1(xksocketp, TR_ERRORS,
	     "xksocket_demux: rcv queue of socket %08x is full, dropping msg",
		sd);
	return XK_FAILURE;
    } /* if */

    xControl(lls, GETPARTICIPANTS, (char*)p, 2*sizeof(Part));

    from->sin_family = AF_INET;
    from->sin_port = htons(*(int*) partPop(p[REMOTE_PART]));
    from->sin_addr.s_addr = *(int*) partPop(p[REMOTE_PART]);
    streamAppend(&sd->so_rcv, msg, &sa);

    xTrace2(xksocketp, TR_EVENTS,
 	    "xksocket_demux: delivering msg to socket %08x (msgLen=%d)",
	    sd, msgLen(msg));

    condition_broadcast(selectable_event);

    xTrace0(xksocketp, TR_FUNCTIONAL_TRACE,
	    "xksocket_demux: signalled to selectable_event");
    return XK_SUCCESS;
} /* xksocket_demux */


static xkern_return_t
xksocket_opendone(XObj self, XObj lls, XObj llp, XObj hlpType)
{
    socket_t *sd;
    socket_t *nsd;
    Part p[2];
    conid_t con;

    xTrace4(xksocketp, TR_FUNCTIONAL_TRACE,
	    "xksocket_opendone(self=%08x,lls=%08x,llp=%08x,hlpType=%08x)",
	    self, lls, llp, hlpType);

    /* find socket which listens on the given address: */
    xControl(lls, GETPARTICIPANTS, (char*)p, 2*sizeof(Part));
    con.c_protocol = lls->myprotl->id;
    con.c_port = *(u_int*) partPop(p[LOCAL_PART]);
    if (MAP_RESOLVE(passive_map, &con, &sd) == XK_FAILURE) {
	xTrace2(xksocketp, TR_EVENTS,
	  "xksocket_opendone: no socket listening for protocol %d port %d",
		con.c_protocol, con.c_port);
	return XK_FAILURE;
    } /* if */

    if (sd->so_type == SOCK_STREAM) {
	if (!(sd->so_options & SO_ACCEPTCONN)) {
	    xTrace1(xksocketp, TR_EVENTS,
		"xksocket_opendone: socket %08x does not accept connections",
		    sd);
	    return XK_FAILURE;
	} /* if */

	/* create a new socket for the new connection: */

	if (sd->so_qlen >= sd->so_qlimit) {
	    /* too many connections pending, return failure: */
	    xTrace1(xksocketp, TR_EVENTS,
	 "xksocket_opendone: %d connections pending, refusing new connection",
		    sd->so_qlen);
	    return XK_FAILURE;
	} /* if */

	/* create a socket descriptor for the new connection: */
	nsd = so_alloc_sock();

	nsd->so_state |= SS_NOFDREF | SS_ISCONNECTED;
	nsd->so_type = sd->so_type;
	nsd->so_options = sd->so_options & ~SO_ACCEPTCONN;
	nsd->so_session = lls;
	/* make reference to session permanent: */
	xDuplicate(nsd->so_session);

	so_new_session(nsd);

	/* insert new socket in queue of pending connections: */
	nsd->so_q = sd->so_q;
	sd->so_q = nsd;
	++sd->so_qlen;

	xTrace1(xksocketp, TR_FUNCTIONAL_TRACE,
		"xksocket_opendone: there are %d connections pending now",
		sd->so_qlen);
    } else {
	/* insert session into active map: */
	if (mapBind(active_map, &lls, sd) == ERR_BIND) {
	    xTrace0(xksocketp, TR_ERRORS, "so_new_session: mapBind() failed");
	} /* if */
    } /* if */

    /* wakeup sleepers: */
    condition_broadcast(selectable_event);

    return XK_SUCCESS;
} /* xksocket_opendone */


static xkern_return_t
xksocket_closedone(XObj s)
{
    socket_t *sd;

    xTrace1(xksocketp, TR_FUNCTIONAL_TRACE, "xksocket_closedone(s=%08x)", s);

    if (MAP_RESOLVE(active_map, &s, &sd) == XK_FAILURE) {
	xTrace1(xksocketp, TR_ERRORS,
		"xksocket_closedone: couldn't find socket for session %08x",
		s);
	return XK_SUCCESS;
    } /* if */

    xTrace1(xksocketp, TR_EVENTS, "xksocket_closedone: closing socket %08x",
	    sd);

    /* shutdown connection: */
    so_shutdown(sd, SS_CANTSENDMORE | SS_CANTRCVMORE);

    /* let processes waiting for event on this socket know: */
    condition_broadcast(selectable_event);

    xTrace0(xksocketp, TR_FUNCTIONAL_TRACE,
	    "xksocket_closedone: returns successfully");

    return XK_SUCCESS;
} /* xksocket_closedone */


static int
xksocket_control_protl(XObj self, int opcode, char *buf, int len)
{
    XObj lls;
    socket_t *sd;
    u_int mark;

    switch (opcode) {
      case TCP_OOBMODE:
	checkLen(len, 2*sizeof(void*));

	lls = (XObj) ((void**)buf)[0];

	if (MAP_RESOLVE(active_map, &lls, &sd) == XK_FAILURE) {
	    xTrace1(xksocketp, TR_ERRORS,
	      "xksocket_control_protl: couldn't find socket for session %08x",
		    lls);
	} /* if */

	mark = (u_int) ((void**)buf)[1];
	sd->so_oobmark = mark;
	if (!sd->so_oobmark) {
	    sd->so_state |= SS_RCVATMARK;
	} /* if */

	/* send SIGURG to client (doesn't work reliably with BSD emulator */
	if (sd->so_pgrp) {
	    client_t *cl;

	    xTrace1(xksocketp, TR_MAJOR_EVENTS,
		    "xksocket_control_protl: delivering SIGURG to pgrp %d",
		    sd->so_pgrp);

	    kill(sd->so_pgrp, SIGURG);

	    cl = so_client_lookup(sd->so_cid, TRUE);
	    if (cl) {
		cl->signal_pending = TRUE;
	    } /* if */
	    condition_broadcast(selectable_event);
	} /* if */

	xTrace2(xksocketp, TR_MAJOR_EVENTS,
		"xksocket_control_protl: oobmark of socket %08x set to %d",
		sd, sd->so_oobmark);
	return 0;

      default:
	return xControl(xGetDown(self, 0), opcode, buf, len);
    } /* switch */
} /* xksocket_control_protl */


void
xksocket_init(XObj self)
{
    int i;
    XObj llp;

    xTrace1(xksocketp, TR_FUNCTIONAL_TRACE, "xksocket_init(self=%08x)",
	    self);

    xAssert(xIsProtocol(self));

    xksock_protl = self;

    /* initialize RPC server: */
    xsi_init();

    xksock_protl->demux	    = xksocket_demux;
    xksock_protl->opendone  = xksocket_opendone;
    xksock_protl->closedone = xksocket_closedone;
    xksock_protl->control   = xksocket_control_protl;

    for (i = 0; (llp = xGetDown(xksock_protl, i)) != ERR_XOBJ; i++) {

	xAssert(xIsProtocol(llp));

	if (strcmp(llp->name, "tcp") == 0) {
	    tcp_protl = llp;
	} else if (strcmp(llp->name, "udp") == 0) {
	    udp_protl = llp;
	} else {
	    xTrace1(xksocketp, TR_ERRORS,
		    "xksocket_init: lower-level protocol `%s' unexpected",
		    llp->name);
	} /* if */
    } /* for */
    if (!tcp_protl) {
	quit(1, "xksocket_init: missing tcp as a lower-level protocol\n");
    } /* if */
    if (!udp_protl) {
	quit(1, "xksocket_init: missing udp as a lower-level protocol\n");
    } /* if */

    xTrace0(xksocketp, TR_FUNCTIONAL_TRACE, "xksocket_init: done");
} /* xksocket_init */

			/*** end of xksocket.c ***/
