/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990 Carnegie Mellon University
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
 * HISTORY
 * $Log:	mach_privileged_ports.c,v $
 * Revision 2.5  94/07/08  17:55:22  mrt
 * 	Updated copyrights
 * 
 * Revision 2.4  91/10/06  22:27:26  jjc
 * 	Replaced occurrences of msgh_kind with msgh_seqno.
 * 	[91/10/04            jjc]
 * 
 * Revision 2.3  90/10/29  17:28:05  dpj
 * 	Added set_mach_privileged_host_port(), set_mach_device_server_port().
 * 	[90/08/15  14:24:24  dpj]
 * 
 * Revision 2.2  90/07/26  12:37:43  dpj
 * 	First version
 * 	[90/07/24  14:29:15  dpj]
 * 
 *
 */

/*
 * Management of the special privileged ports exported by the Mach kernel.
 *
 * Much of this code is copied from bsd/mach_init.c in the 
 * single-server UNIX sources.
 */

#include	<mach_privileged_ports.h>
#include	<mach/message.h>

#ifdef	DEBUG_NO_SA
#include	<stdio.h>
#endif	DEBUG_NO_SA


static mach_port_t	mach_privileged_host_port_internal = MACH_PORT_NULL;
static mach_port_t	mach_device_server_port_internal = MACH_PORT_NULL;
static int		tried_bootstrap = 0;


kern_return_t	mach_privileged_ports_bootstrap()
{
	mach_port_t		bootstrap_port;
	mach_port_t		reply_port;
	kern_return_t		result;

	struct imsg {
	    mach_msg_header_t	hdr;
	    mach_msg_type_t	port_desc_1;
	    mach_port_t		port_1;
	    mach_msg_type_t	port_desc_2;
	    mach_port_t		port_2;
	} imsg;

	/*
	 * Get our bootstrap port
	 */
	result = task_get_bootstrap_port(mach_task_self(), &bootstrap_port);
#ifdef	DEBUG_NO_SA
	fprintf(stderr,"mach_privileged_ports_bootstrap.task_get_bootstrap(): 0x%x\n",result);
#endif	DEBUG_NO_SA
	if (result != KERN_SUCCESS)
		return(result);

	/*
	 * Allocate a reply port
	 */
	reply_port = mach_reply_port();
#ifdef	DEBUG_NO_SA
	fprintf(stderr,"mach_privileged_ports_bootstrap.mach_reply_port(): 0x%x\n",result);
#endif	DEBUG_NO_SA
	if (reply_port == MACH_PORT_NULL)
		return(result);

	/*
	 * Send a message to it, asking for the host and device ports
	 */
	imsg.hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
					    MACH_MSG_TYPE_MAKE_SEND_ONCE);
	imsg.hdr.msgh_size = 0;
	imsg.hdr.msgh_remote_port = bootstrap_port;
	imsg.hdr.msgh_local_port = reply_port;
	imsg.hdr.msgh_seqno = 0;
	imsg.hdr.msgh_id = 999999;

#ifdef	DEBUG_NO_SA
	fprintf(stderr,
		"mach_privileged_ports_bootstrap(): about to send a msg\n");
#endif	DEBUG_NO_SA
	result = mach_msg(&imsg.hdr,
			  MACH_SEND_MSG|MACH_RCV_MSG|MACH_RCV_TIMEOUT,
			  sizeof imsg.hdr, sizeof imsg, reply_port,
			  500, MACH_PORT_NULL);
#ifdef	DEBUG_NO_SA
	fprintf(stderr,
	"mach_privileged_ports_bootstrap(): kr=0x%x, id=%d, size=%d\n",
			result,imsg.hdr.msgh_id,imsg.hdr.msgh_size);
	fprintf(stderr,"  name=%d num=%d inline=%d longform=%d data=0x%x\n",
		imsg.port_desc_1.msgt_name,
		imsg.port_desc_1.msgt_number,
		imsg.port_desc_1.msgt_inline,
		imsg.port_desc_1.msgt_longform,
		imsg.port_1);
	fprintf(stderr,"  name=%d num=%d inline=%d longform=%d data=0x%x\n",
		imsg.port_desc_2.msgt_name,
		imsg.port_desc_2.msgt_number,
		imsg.port_desc_2.msgt_inline,
		imsg.port_desc_2.msgt_longform,
		imsg.port_2);
#endif	DEBUG_NO_SA
	if ((result == MACH_MSG_SUCCESS) &&
		(imsg.hdr.msgh_size == sizeof imsg) &&
		(imsg.port_desc_1.msgt_name == MACH_MSG_TYPE_PORT_SEND) &&
		(imsg.port_desc_2.msgt_name == MACH_MSG_TYPE_PORT_SEND)) {
#ifdef	DEBUG_NO_SA
#else	DEBUG_NO_SA
		mach_privileged_host_port_internal = imsg.port_1;
		mach_device_server_port_internal = imsg.port_2;
#endif	DEBUG_NO_SA
		return(KERN_SUCCESS);
	}

	return(result);
}


mach_port_t	mach_privileged_host_port()
{
	static int			tried_syscall = 0;
	int				result;

#ifdef	DEBUG_NO_SA
	fprintf(stderr,"mach_privileged_host_port: old port=0x%x\n",mach_privileged_host_port_internal);
#endif	DEBUG_NO_SA
	if ((mach_privileged_host_port_internal == MACH_PORT_NULL) &&
						(tried_bootstrap == 0)) {
		(void) mach_privileged_ports_bootstrap();
		tried_bootstrap = 1;
	}

	if ((mach_privileged_host_port_internal == MACH_PORT_NULL) &&
						(tried_syscall == 0)) {
		tried_syscall = 1;
		result = mach3_syscall(-33,-1);
		if (result >= 0) {
			mach_privileged_host_port_internal = result;
		}
	}

	return(mach_privileged_host_port_internal);
}

mach_port_t	mach_device_server_port()
{
	static int			tried_syscall = 0;
	int				result;

#ifdef	DEBUG_NO_SA
	fprintf(stderr,"mach_device_server_port: old port=0x%x\n",mach_device_server_port_internal);
#endif	DEBUG_NO_SA

	if ((mach_device_server_port_internal == MACH_PORT_NULL) &&
						(tried_bootstrap == 0)) {
		(void) mach_privileged_ports_bootstrap();
		tried_bootstrap = 1;
	}

	if ((mach_device_server_port_internal == MACH_PORT_NULL) &&
						(tried_syscall == 0)) {
		tried_syscall = 1;
		result = mach3_syscall(-33,-2);
#ifdef	DEBUG_NO_SA
	fprintf(stderr,"mach_device_server_port.syscall: 0x%x\n",result);
#endif	DEBUG_NO_SA
		if (result >= 0) {
			mach_device_server_port_internal = result;
		}
	}

	return(mach_device_server_port_internal);
}


void set_mach_privileged_host_port(port)
	mach_port_t		port;
{
	mach_privileged_host_port_internal = port;
}


void set_mach_device_server_port(port)
	mach_port_t		port;
{
	mach_device_server_port_internal = port;
}


