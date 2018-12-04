/*
 * $RCSfile: xsi.h,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/02 00:08:44 $
 * $Author: menze $
 *
 * $Log: xsi.h,v $
 * Revision 1.5  1993/02/02  00:08:44  menze
 * copyright change
 *
 * Revision 1.4  1993/01/26  08:11:18  menze
 * Added prototypes
 *
 * Revision 1.3  1992/08/15  01:16:26  davidm
 * OOB message transmission is now implemented *and* tested.
 * Support for SIGURG was added.
 *
 * Revision 1.2  1992/08/01  18:42:25  davidm
 * This version now supports TCP flow-control, handles UDP connections
 * correctly, and also supports out-of-band message transmission (untested
 * yet).
 *
 * Revision 1.1  1992/07/22  00:14:07  davidm
 * Initial revision
 *
 */
#ifndef xsi_h
#define xsi_h

#include <mach.h>
#include <errno.h>
#include "xsi_user.h"
#include "xksocket.h"

/*
 * Connection id specifies a socket uniquely:
 */
typedef struct conid_t {
    int		c_protocol;		/* protocol id */
    u_int	c_port;			/* port number */
} conid_t;

/*
 * For every active client we maintain an array of pointers to socket
 * structures.  A pointer is 0 if the corresponding socket has not
 * been created.  Note that "dup()" may result in a socket structure
 * being referenced more than once.
 */
typedef struct client {
    boolean_t	  signal_pending;	/* was a signal posted? */
    boolean_t	  select_cancel_seqno;	/* seqno of last cancelled select */
    Semaphore	  clone_done;		/* clone done by child? */
    socket_t	  *sds[FD_SETSIZE];	/* socket descriptors */
} client_t;

extern Map active_map;			/* map of active sessions */
extern Map passive_map;			/* map of passive sessions */
extern Map client_map;		 	/* map of active clients */
extern Map port2cid_map;		/* map of notification ports */

extern condition_t selectable_event;

extern mach_port_t service;
extern mach_port_t notify;

extern void xsi_init(void);
extern boolean_t xsi_server(mach_msg_header_t *request,
			    mach_msg_header_t *reply);

extern boolean_t xsi_server(mach_msg_header_t *request,
			    mach_msg_header_t *reply);

extern mach_msg_return_t	xk_msg_server(boolean_t (*demux)(),
					      mach_msg_size_t max_size,
					      mach_port_t rcv_name);


#endif /* xsi_h */
