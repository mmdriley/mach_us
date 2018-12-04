/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 */
/*
 * HISTORY
 * $Log: xk_msg_server.c,v $
 * Revision 1.4  1993/02/02  00:08:26  menze
 * copyright change
 *
 * Revision 1.3  1993/01/26  08:10:06  menze
 * Added include files and prototypes
 *
 * Revision 1.2  1992/07/22  00:14:07  davidm
 * Various bug fixes and improvements.
 *
 * Revision 1.1  1992/07/03  08:47:48  davidm
 * Initial revision
 *
 * Revision 2.7  92/04/01  19:38:21  rpd
 * 	Added MIG_DESTROY_REQUEST.
 * 	[92/03/09            rpd]
 * 
 * Revision 2.6  92/01/27  16:43:28  rpd
 * 	Added MACH_SEND_TIMED_OUT case.
 * 	[92/01/26            rpd]
 * 
 * Revision 2.5  92/01/23  15:22:28  rpd
 * 	Fixed to handle MACH_RCV_TOO_LARGE from receives.
 * 	Fixed to supply MACH_SEND_TIMEOUT when necessary.
 * 	[92/01/20            rpd]
 * 
 * Revision 2.4  91/05/14  17:53:22  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/14  14:17:47  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:44:20  mrt]
 * 
 * Revision 2.2  90/08/06  17:23:58  rpd
 * 	Created.
 * 
 */

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <cthreads.h>
#include "xkernel.h"
#include "xk_mach.h"
#include "xsi.h"

/*
 *	Routine:	xk_msg_server
 *	Purpose:
 *		A simple generic server function.
 */

extern mutex_t sledgehammer_concurrency_control;
#define MASTER_LOCK	mutex_lock(sledgehammer_concurrency_control)
#define MASTER_UNLOCK	mutex_unlock(sledgehammer_concurrency_control)

extern	char *malloc( unsigned );
extern  void free( void * );

mach_msg_return_t
xk_msg_server(boolean_t (*demux)(), mach_msg_size_t max_size,
	      mach_port_t rcv_name)
{
    register mig_reply_header_t *bufRequest, *bufReply, *bufTemp;
    register mach_msg_return_t mr;

    bufRequest = (mig_reply_header_t *) malloc(max_size);
    if (bufRequest == 0) {
	return KERN_RESOURCE_SHORTAGE;
    } /* if */
    bufReply = (mig_reply_header_t *) malloc(max_size);
    if (bufReply == 0) {
	return KERN_RESOURCE_SHORTAGE;
    } /* if */

    for (;;) {
      get_request:

	/* need to release master lock such that upcalls can be executed: */
	MASTER_UNLOCK;
	mr = mach_msg(&bufRequest->Head, MACH_RCV_MSG,
		      0, max_size, rcv_name,
		      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	MASTER_LOCK;

	while (mr == MACH_MSG_SUCCESS) {
	    /* we have a request message */

	    (void) (*demux)(&bufRequest->Head, &bufReply->Head);

	    if (bufReply->RetCode != KERN_SUCCESS) {
		if (bufReply->RetCode == MIG_NO_REPLY)
		    goto get_request;

		if (bufReply->RetCode == MIG_DESTROY_REQUEST) {
		    /* destroy request without sending a reply */

		    mach_msg_destroy(&bufRequest->Head);
		    goto get_request;
		}

		/* don't destroy the reply port right,
		   so we can send an error message */
		bufRequest->Head.msgh_remote_port = MACH_PORT_NULL;
		mach_msg_destroy(&bufRequest->Head);
	    }

	    if (bufReply->Head.msgh_remote_port == MACH_PORT_NULL) {
		/* no reply port, so destroy the reply */
		if (bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)
		    mach_msg_destroy(&bufReply->Head);

		goto get_request;
	    }

	    /* send reply and get next request */

	    bufTemp = bufRequest;
	    bufRequest = bufReply;
	    bufReply = bufTemp;

	    /* need to rel. master lock such that upcalls can be executed: */
	    MASTER_UNLOCK;
	    /*
	     *	We don't want to block indefinitely because the client
	     *	isn't receiving messages from the reply port.
	     *	If we have a send-once right for the reply port, then
	     *	this isn't a concern because the send won't block.
	     *	If we have a send right, we need to use MACH_SEND_TIMEOUT.
	     *	To avoid falling off the kernel's fast RPC path unnecessarily,
	     *	we only supply MACH_SEND_TIMEOUT when absolutely necessary.
	     */
	    mr = mach_msg(&bufRequest->Head,
			  (MACH_MSGH_BITS_REMOTE(bufRequest->Head.msgh_bits) ==
			   MACH_MSG_TYPE_MOVE_SEND_ONCE) ?
			  MACH_SEND_MSG|MACH_RCV_MSG :
			  MACH_SEND_MSG|MACH_SEND_TIMEOUT|MACH_RCV_MSG,
			  bufRequest->Head.msgh_size, max_size, rcv_name,
			  0, MACH_PORT_NULL);
	    MASTER_LOCK;
	} /* while */

	/* a message error occurred */

	switch (mr) {
	  case MACH_SEND_INVALID_DEST:
	  case MACH_SEND_TIMED_OUT:
	    /* the reply can't be delivered, so destroy it */
	    mach_msg_destroy(&bufRequest->Head);
	    break;

	  case MACH_RCV_TOO_LARGE:
	    /* the kernel destroyed the request */
	    break;

	  default:
	    /* should only happen if the server is buggy */
	    free((char *) bufRequest);
	    free((char *) bufReply);
	    return mr;
	} /* switch */
    } /* for */
} /* xk_msg_server */

			/*** end of xk_msg_server.c ***/
