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
 * This file is derived from the x-kernel distributed by the
 * University of Arizona. See the README file at the base of this
 * source subtree for details about distribution.
 *
 * The Mach 3 version of the x-kernel is substantially different from
 * the original UofA version. Please report bugs to mach@cs.cmu.edu,
 * and not directly to the x-kernel project.
 */
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/xk_main.cc,v $
 *
 * 
 * Purpose:  startup for the xkernel portion of the network server
 * 
 * HISTORY
 * $Log:	xk_main.cc,v $
 * Revision 2.3  94/07/13  18:07:18  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:44  jms
 * 	Create fake "ROM" entry for the ip_address and gateway address.
 * 	Upgrade for xkernel v3.2
 * 	Derived created from the file ".../mach3/init.c"
 * 	[94/01/10  13:11:45  jms]
 * 
 * Revision 2.3  91/05/05  19:30:05  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:03:53  dpj]
 * 
 * 	Run the initialization code under protection of the XKERNEL_MASTER
 * 	lock. Without it, service threads might run too soon and get in
 * 	trouble... 
 * 	[91/04/28  10:42:10  dpj]
 * 
 * Revision 2.2  90/10/29  18:04:47  dpj
 * 	Integration into the master source tree
 * 	[90/10/21  23:13:58  dpj]
 * 
 * 	First working version.
 * 	[90/10/03  21:50:04  dpj]
 * 
 *
 */

#include	<base.h>

extern "C" {
#include	<stdio.h>
#define x_stdio_h

#define this _this
#include	"process.h"
#include	"upi.h"
#include	"debug.h"
#include	"protocols.h"

#include	"ip.h"
#include	"platform.h"
#include	"xk_debug.h"
#undef this

#include "usx_internal.h"

extern void		xkInit();

extern IPhost		inet_addr();
extern int	 	user();
extern int		tracemach3;
extern void		clock_ih();

extern int		mach3_output_console();
}

#define  INTERVAL	100

extern int rom_next;

char			*xkinit_interface_devname = NULL;
char			*xkinit_ip_address_str = NULL;
char			*xkinit_ip_default_gateway_str = NULL;

int			x_errno;


extern "C" xkernel_usage(char *program, char *moreargs)
{
	fprintf(stderr,"Usage: %s \n",program);
	fprintf(stderr,
    "        -I <interface name> -i <IP address> [-g <IP default gateway>]\n");
	fprintf(stderr,"        [-console|-unix] [-stop]\n");
	fprintf(stderr,"        %s\n",moreargs);
	exit(1);
}


main(int argc, char *argv[])
{
	char		*program = argv[0];
	int		have_ip_address = 0;
	int		have_interface_name = 0;
	IPhost		addr_buf;
	int		stop = 0;
	Semaphore	startup_sem;

	/*
	 * This program may be needed to access the Diag server over
	 * the network. Always start with output on the console to
	 * avoid deadlocks.
	 */
	mach3_output_console();

//	localInit();

	/*
	 * Parse basic arguments.
	 */
	argc--;
	argv++;
	for (; argc > 0; argc--, argv++) {
		if (!strcmp(argv[0],"-I")) {
			argc--;
			argv++;
			if (argc <= 0) xkernel_usage(program,"[<user args>]");
			xkinit_interface_devname = argv[0];
			have_interface_name = 1;
			xTrace1(mach3,1,"mach3: interface is %s",
							argv[0]);
			continue;
		}

		if (!strcmp(argv[0],"-i")) {
			argc--;
			argv++;
			if (argc <= 0) xkernel_usage(program,"[<user args>]");
			addr_buf = inet_addr(argv[0]);
			xkinit_ip_address_str = argv[0];
			have_ip_address = 1;
			xTrace4(mach3,1,"mach3: my IP address is %d.%d.%d.%d",
					addr_buf.a,addr_buf.b,
					addr_buf.c,addr_buf.d);
			continue;
		}

		if (!strcmp(argv[0],"-g")) {
			argc--;
			argv++;
			if (argc <= 0) xkernel_usage(program,"[<user args>]");
			addr_buf = inet_addr(argv[0]);
			xkinit_ip_default_gateway_str = argv[0];
			xTrace4(mach3,3,"mach3: my IP gateway is %d.%d.%d.%d",
			addr_buf.a,addr_buf.b,
			addr_buf.c,addr_buf.d);

			/* Fake up a "rom" entry for the gateway */
			rom[rom_next][0] = "ip";
			rom[rom_next][1] = "gateway";
			rom[rom_next][2] = xkinit_ip_default_gateway_str;
			rom_next++;
			rom[rom_next][0] = NULL;
			continue;
		}

		if (!strcmp(argv[0],"-console")) {
			mach3_output_console();
			continue;
		}

#if	MACH3_UNIX || MACH3_VUS
		if (!strcmp(argv[0],"-unix")) {
			mach3_output_unix();
			continue;
		}
#endif	MACH3_UNIX || MACH3_VUS

		if (!strcmp(argv[0],"-stop")) {
			stop = 1;
			continue;
		}

		/*
		 * Unknown argument. Must be for the user program.
		 *
		 * Fix argument block to look like new.
		 */
		argc++;
		argv--;
		argv[0] = program;
		break;
	}

	if ((! have_ip_address) || (!(have_interface_name))) {
		xkernel_usage(program,"[<user args>]");
	}

	/*
	 * Pause to allow to attach a debugger.
	 */
	if (stop) {
		fprintf(stderr,
		"Init: xkernel suspended. Awaiting restart from debugger.\n");
		task_suspend(mach_task_self());
	}

	/*
	 * Initialize and start x-kernel internals.
	 */
	xkInit();

	/* Get real */

	user(argc,argv);
}
