/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989,1988 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they made and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
/*
 * File:  server_loop.c
 *
 * Purpose:
 *
 * HISTORY: 
 * $Log:	server_loop.c,v $
 * Revision 2.5  94/07/08  18:13:51  mrt
 * 	Updated copyrights
 * 
 * Revision 2.4  92/07/05  23:26:16  dpj
 * 	No further changes.
 * 	[92/06/29  22:49:08  dpj]
 * 
 * 	Fixed to use a big enough buffer to receive replies. This was causing 
 * 	diag_checkin to fail, so that we never saw any messages anymore...
 * 	[92/05/10  00:41:47  dpj]
 * 
 * Revision 2.3  92/03/05  15:02:49  jms
 * 	Upgrade to use no MACH_IPC_COMPAT features.  New IPC only!
 * 	[92/02/26  17:57:32  jms]
 * 
 * Revision 2.2  90/07/26  12:41:46  dpj
 * 	First version
 * 	[90/07/24  14:46:04  dpj]
 * 
 * Revision 1.9  89/05/17  16:35:44  dorr
 * 	include file cataclysm
 * 
 * Revision 1.8  89/05/04  17:48:42  mbj
 * 	Merge up to U3.
 * 	[89/04/17  15:24:20  mbj]
 * 
 * Revision 1.7.1.1  89/03/31  16:08:11  mbj
 * 	Create mbj_signal branch
 * 
 * Revision 1.7.1.1  89/03/30  17:02:49  mbj
 * 	Exit from server_loop if trying to receive on an invalid port.
 * 
 * Revision 1.7  88/10/30  16:55:25  dorr
 * move notify_loop to separate file.
 * 
 *
 */


#include <base.h>
#include <stdio.h>
#include <mach.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <mach_error.h>
#include <mig_errors.h>


#define SERVER_LOOP_INVALID_ARGUMENT (-1)	/* XXX assign an error # */
#define MSG_SIZE_MAX	8192

typedef struct {
	mach_msg_header_t	head;
	char		body[MSG_SIZE_MAX-sizeof(mach_msg_header_t)];
} msg_t;

typedef struct dispatch_proc_data {
	mach_error_fn_t	* proc;
	int		proc_cnt;
	mach_port_t		port;
	char		* server_name;
} * dispatch_proc_data_t;

static void		request_handler();

/*
 *	multi_server_loop:
 *
 *	this routine sets up an rpc server for a given port.  the server
 *	loops, taking in messages and handing them to a dispatch routine
 *	that calls the appropriate server side of the rpc interface.
 *	it also sets up a notification thread that waits for notification
 *	messages
 *
 *	parameters:
 *
 *  name:	in, char *,	
 *	the public name to be assigned to this server.
 *	if name is null, the server is not locatable via
 *	the naming server.  it can only be used by processes
 *	that have been explicitly passed its port
 *
 *  server_thread_cnt: in, int,  
 *	the number of concurrent listeners on the server port
 *	(XXX at some point we may want to extend it so that a count of zero
 *	creates as many threads as necessary up to some system max).
 *
 *  dispatch: in, mach_error_fn_t, 
 *	the user routine used to dispatch the server request.  This
 *	routine is usually automatically generated (by mig, for example).
 *	it takes an in message and an out message as parameters.
 *
 *  server_port: in, mach_port_t
 *	a port upon which server requests will be taken.  if the value of server_port
 *	is MACH_PORT_NULL, a port will be allocated for you.
 */
 
mach_error_t
multi_server_loop( 
	    server_name,
	    server_thread_cnt,
	    dispatch_cnt,
	    dispatch, 
	    server_port
	    )
	char			* server_name;
	int			server_thread_cnt;
	int			dispatch_cnt;
	mach_error_fn_t		* dispatch;
	mach_port_t			server_port;
{
	int i;
	mach_error_t		err;
	dispatch_proc_data_t	dp = New(struct dispatch_proc_data);


	if ((server_thread_cnt < 1)
	||  (server_port == MACH_PORT_NULL && server_name == NULL) )
		return (SERVER_LOOP_INVALID_ARGUMENT);


	/* get a port for yourself */
	if (server_port == MACH_PORT_NULL) {
		if (err = mach_port_allocate( mach_task_self(), 
				MACH_PORT_RIGHT_RECEIVE, &server_port))
			return (err);
	}

	/* save the parameters */
	dp->proc = NewArray( mach_error_fn_t, dispatch_cnt);
	for( i=0; i<dispatch_cnt; i++ ) 
		dp->proc[i] = dispatch[i];
	dp->proc_cnt = dispatch_cnt;
	dp->port = server_port;
	if (server_name == NULL)
		dp->server_name = NewStr( "anonymous");
	else
		dp->server_name = NewStr( server_name);


#if (! ONLY_ONE_SERVER_LOOP)
	/* fire off a few receivers */
	/* fork n times */
	for (i=0; i<server_thread_cnt; i++)
		cthread_detach( cthread_fork( request_handler, dp ));
#endif (! ONLY_ONE_SERVER_LOOP)

	/* publicize the name, if any */
	if (server_name != NULL) {
		if (err = netname_check_in(name_server_port,
					server_name, MACH_PORT_NULL, server_port))
			return (err);
	}

#if ONLY_ONE_SERVER_LOOP
	request_handler(dp);
#endif ONLY_ONE_SERVER_LOOP

	return (ERR_SUCCESS);
}

/*
 *	server_loop:  single dispatch interface
 */
mach_error_t
server_loop( 
	    server_name,
	    server_thread_cnt,
	    dispatch, 
	    notify,
	    server_port
	    )
	char			* server_name;
	int			server_thread_cnt;
	mach_error_fn_t		dispatch;
	mach_error_fn_t		notify;
	mach_port_t		server_port;
{
	mach_error_t		err;

	err = multi_server_loop(server_name, server_thread_cnt, 
			 1, &dispatch,
			 server_port);

	return (err);
}


	
/*
 *	request_handler: take incoming requests and dispatch them through
 *	the user-provided dispatch routine
 */
static void
request_handler(dp)
	dispatch_proc_data_t	dp;
{
	mach_error_t		err;
	boolean_t		replying;
	msg_t			msg1, msg2;
	msg_t			*in_msg, *tmp_msg;
	mig_reply_header_t 	*out_msg;
	int			i;

	/* Setup for first request */
	in_msg = &msg1;
	out_msg = ((mig_reply_header_t *)(&msg2));
	bzero(&msg1, sizeof(msg_t));
	bzero(&msg2, sizeof(msg_t));
	replying = FALSE;

	for (;;) {
		if (replying) {
			out_msg->Head.msgh_local_port = MACH_PORT_NULL;
			err = mach_msg(out_msg, 
					MACH_RCV_MSG|MACH_SEND_MSG,
					out_msg->Head.msgh_size,
					sizeof(msg_t),
					dp->port,
					MACH_MSG_TIMEOUT_NONE,
					MACH_PORT_NULL);

			tmp_msg = (msg_t *)out_msg;
			out_msg = (mig_reply_header_t *)in_msg;
			in_msg = tmp_msg;
		} else {
			in_msg->head.msgh_local_port = dp->port;
			in_msg->head.msgh_size = sizeof(msg_t);
			in_msg->head.msgh_bits = MACH_MSGH_BITS_ZERO;
			err = mach_msg_receive(in_msg);
		}

		replying = FALSE;
		out_msg->Head.msgh_bits = in_msg->head.msgh_bits;
		out_msg->Head.msgh_size = sizeof(mig_reply_header_t);

		if (err) {
		    fprintf(stderr,
			"%s: server_loop.request_handler: error %#x %s\n",
			dp->server_name, err, mach_error_string(err));
		    if (err == MACH_RCV_INVALID_NAME) {
			fprintf(stderr, "Invalid port was %d\n", dp->port);
			return;	 /* Thread isn't useful in this state! */
		    }
		    continue;
		}

		/* XXX request_handler: hash multiple interfaces */
		for (i=0; i<dp->proc_cnt; i++ ) {
			if (dp->proc[i]( in_msg, out_msg)) {
				if ((out_msg->Head.msgh_remote_port != MACH_PORT_NULL)
				    && (out_msg->RetCode != MIG_NO_REPLY)) 
					replying = TRUE;
				break;
			}
		}
	}
}
