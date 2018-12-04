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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/net_init.cc,v $
 *
 * 
 * Purpose:  "Main" module for the network server.
 * 
 * HISTORY
 * $Log:	net_init.cc,v $
 * Revision 2.3  94/07/13  18:06:09  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:09:37  jms
 * 	Minor changes
 * 	[94/01/10  11:28:25  jms]
 * 
 * Revision 2.4  92/07/05  23:33:39  dpj
 * 	Converted to new C++ RPC package.
 * 	[92/05/10  01:27:47  dpj]
 * 
 * Revision 2.3  92/03/05  15:10:02  jms
 * 	Upgrade to use no MACH_IPC_COMPAT features.  New IPC only!
 * 	[92/02/26  19:01:46  jms]
 * 
 * Revision 2.2  91/11/06  14:14:16  jms
 * 	Upgraded the calls that initialize the Diag system.
 * 	[91/10/03  15:18:44  pjg]
 * 
 * 	Initial C++ revision.
 * 	[91/09/27  16:07:42  pjg]
 * 
 * Revision 2.4  91/07/01  14:14:52  jms
 * 	Update call to mach_object_start_handler to new signature
 * 
 * Revision 2.3  91/05/05  19:30:46  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:10  dpj]
 * 
 * 	Create more than one MachObjects handler thread.
 * 	[91/04/28  10:46:39  dpj]
 * 
 * 	Fixed parsing of "-d" switch.
 * 	[91/02/25  10:47:34  dpj]
 * 
 * Revision 2.2  90/10/29  18:11:50  dpj
 * 	Integration into the master source tree
 * 	[90/10/21  23:13:17  dpj]
 * 
 * 	First working version.
 * 	[90/10/03  22:00:39  dpj]
 * 
 *
 */

#include <rpcmgr_ifc.h>
#include <net_dir_ifc.h>
#include <diag_ifc.h>

extern "C" {
#include	<ns_types.h>
#include	"net_types.h"
}

extern "C" {
user(int, char*[]);
}

/*
 * Global ID.
 */
ns_mgr_id_t			net_mgr_id = { 0, 0x01700001};

#define USAGE_MOREARGS	"[-diag <diag-name>] [-d<debug-level>] [<base-name>]"


/*
 * Global NET object.
 */
net_dir		*root;


user(int		argc,
     char		*argv[])
{
	int			new_debug_level = us_debug_level;
	char			*diag_name = NULL;
	char			*root_name;
	char			*program = argv[0];
	mach_error_t		ret;
	void      _init_user_proxies(void);
	void      _print_map(void);

	argc--;
	argv++;
	for (; argc > 0; argc--, argv++) {
		if (!strcmp(argv[0],"-diag")) {
			argc--;
			argv++;
			if (argc <= 0) xkernel_usage(program,USAGE_MOREARGS);
			diag_name = argv[0];
			continue;
		} 

		if (!strncmp(argv[0],"-d",2)) {
			if (sscanf(argv[0]+2,"%d",&new_debug_level) != 1) {
				xkernel_usage(program,USAGE_MOREARGS);
			}
			continue;
		} 

		break;
	}
	if ((argc > 1) || ((argc == 1) && (argv[0][1] == '-'))) {
		xkernel_usage(program,USAGE_MOREARGS);
	}
	if (argc == 1)  {
		root_name = argv[0];
	} else {
		root_name = "net_server";
	}

	__main();

//	cthread_init();
	intr_init();
	rpcmgr::GLOBAL->start_object_handler(2,30,30);
//	rpcmgr::GLOBAL->start_object_handler(3,5,5);

	(void) diag_startup(root_name);
	(void) set_diag_level(new_debug_level);

//	new_object(root,net_dir);
//	ret = invoke(root,mach_method_id(setup_net_dir_as_root),net_mgr_id);
	root = new net_dir(net_mgr_id, &ret);
	if (ret != ERR_SUCCESS) {
		mach_error("Cannot setup the root directory",ret);
		exit(1);
	}
	ret = root->ns_netname_export(root_name);
	if (ret != ERR_SUCCESS) {
		mach_error("Cannot export the initial agent",ret);
		exit(1);
	}
}

extern "C" {
void my_task_suspend(void);
}

void my_task_suspend(void)
{
//	task_suspend(mach_task_self());
}

