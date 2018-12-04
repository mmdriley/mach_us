/*
 * $RCSfile: xksocket.h,v $
 *
 * Copyright (c) 1992  Arizona Board of Regents
 *
 * $Revision: 1.4 $
 * $Date: 1992/08/15 01:16:26 $
 * $Author: davidm $
 *
 * $Log: xksocket.h,v $
 * Revision 1.4  1992/08/15  01:16:26  davidm
 * OOB message transmission is now implemented *and* tested.
 * Support for SIGURG was added.
 *
 * Revision 1.3  1992/08/01  18:42:25  davidm
 * This version now supports TCP flow-control, handles UDP connections
 * correctly, and also supports out-of-band message transmission (untested
 * yet).
 *
 * Revision 1.2  1992/07/24  04:15:03  davidm
 * minor changes
 *
 * Revision 1.1  1992/07/22  00:14:07  davidm
 * Initial revision
 *
 */
#ifndef xksocket_h
#define xksocket_h

#include <sys/socket.h>
#include <netinet/in.h>
#include "xkernel.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"
#include "msg_stream.h"

/*
 * Socket state bits:
 */
#define SS_NOFDREF		0x0001	/* no file table reference */
#define	SS_ISCONNECTED		0x0002	/* socket has a session */
#define SS_ISNAMED		0x0004	/* socket has a name bound */
#define SS_CANTSENDMORE		0x0010	/* can't send more data to peer */
#define SS_CANTRCVMORE		0x0020	/* can't receive more data from peer */
#define SS_RCVATMARK		0x0040	/* at oob mark on input */
#define SS_NBIO			0x0100	/* non-blocking ops */
#define SS_ISTMPPORT		0x0200	/* local port was picked by system */

typedef struct socket {
    short		so_rcnt;	/* # of refs to this struct */
    short		so_state;	/* state of socket (SS_* above) */
    short		so_type;	/* generic type (SOCK_* in socket.h) */
    u_short		so_error;	/* asynchronous errors */
    u_short		so_oobmark;	/* chars to oob mark */
    short		so_options;	/* options (SO_* in socket.h) */
    short		so_linger;	/* time to linger while closing */
    short		so_pgrp;	/* pgrp for signals */
    int			so_cid;		/* client who set the pgrp */
    struct sockaddr	so_name;	/* own name after bind() */
    XObj		so_session;	/* x-session for this socket */

    /* variables to accept connections: */
    short		so_qlimit;	/* max length of so_q */
    short		so_qlen;	/* number of conn. on so_q */
    struct socket	*so_q;		/* queue of pending connections */

    /* send/receive buffers: */
    msg_stream		so_rcv;		/* queue of received messages */
    u_short		so_snd_size;	/* send buffer size */
} socket_t;

extern int  tracexksocketp;		/* trace level */
extern XObj xksock_protl;		/* pointer to xksocket protocol */
extern XObj tcp_protl;			/* pointer to tcp protocol */
extern XObj udp_protl;			/* pointer to udp protocol */

extern void xksocket_init(XObj self);
extern xkern_return_t xksocket_pop(XObj s, XObj lls, Msg *msg);

#if defined(OLD_RESOLVE)
#  define MAP_RESOLVE(map,key,val)	\
		((int)((*val)=(typeof (*val))mapResolve((map),(key))))
#else
#  define MAP_RESOLVE(map,key,val)	\
		mapResolve((map),(u_int)(key),(u_int)(val))
#endif

#endif /* xksocket_h */
