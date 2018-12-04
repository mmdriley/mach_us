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
 * $Log:	console.c,v $
 * Revision 2.5  94/07/08  17:50:47  mrt
 * 	Updated copyrights
 * 
 * Revision 2.4  91/07/01  14:10:16  jms
 * 	Re-initialized the probing code when setting the console port
 * 	explicitly. Set the console_port to MACH_PORT_NULL to cause
 * 	a re-initialization.
 * 	[91/06/16  21:03:52  dpj]
 * 
 * Revision 2.3  90/10/29  17:27:14  dpj
 * 	Added mach_console_port(), set_mach_console_port().
 * 	[90/08/15  14:23:10  dpj]
 * 
 * Revision 2.2  90/07/26  12:36:49  dpj
 * 	First version
 * 	[90/07/24  14:26:09  dpj]
 * 
 *
 */

/*
 * Simple console output facility for standalone Mach programs.
 */

#include	<mach.h>
#include	<mach_privileged_ports.h>
#include	<device/device_types.h>

#ifdef	DEBUG && (MACH3_UNIX || MACH3_VUS || MACH3_US)
#include	<stdio.h>
#endif	DEBUG && (MACH3_UNIX || MACH3_VUS || MACH3_US)


static mach_port_t	mach_console_port_internal = MACH_PORT_NULL;
static int		tried_open = 0;


mach_port_t mach_console_port()
{
	kern_return_t		result;

	if (mach_console_port_internal != MACH_PORT_NULL) {
		return(mach_console_port_internal);
	}

	if (tried_open == 1) {
		return(MACH_PORT_NULL);
	}

	tried_open = 1;
	result = device_open(mach_device_server_port(),
					D_READ|D_WRITE,
					"console",
					&mach_console_port_internal);
#ifdef	DEBUG && (MACH3_UNIX || MACH3_VUS || MACH3_US)
		fprintf(stderr,"mach_console_port.device_open(): 0x%x\n",result);
#endif	DEBUG && (MACH3_UNIX || MACH3_VUS || MACH3_US)
	if (result != KERN_SUCCESS) {
		mach_console_port_internal = MACH_PORT_NULL;
	}

	return(mach_console_port_internal);
}

void set_mach_console_port(port)
	mach_port_t		port;
{
	mach_console_port_internal = port;

	tried_open = 0;
}


kern_return_t	console_write(str,len)
	char		*str;
	int		len;
{
	kern_return_t		result;
	int			count;

	if (mach_console_port_internal == MACH_PORT_NULL) {
		/*
		 * Force initialization.
		 */
		(void) mach_console_port();
	}

	if (len <= 0) return(KERN_SUCCESS);

	result = device_write_inband(mach_console_port_internal,0,0,
							str,len,&count);
#ifdef	DEBUG && (MACH3_UNIX || MACH3_VUS || MACH3_US)
	fprintf(stderr,"console_write.device_write_inband(): 0x%x %d\n",
							result,count);
#endif	DEBUG && (MACH3_UNIX || MACH3_VUS || MACH3_US)

	if (str[len - 1] == '\n') {
		(void) device_write_inband(mach_console_port_internal,0,0,
							"\r",1,&count);
	}

	return(result);
}


