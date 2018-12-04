/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991 Carnegie Mellon University
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
 * This file is derivedi in part from the x-kernel distributed by the
 * University of Arizona. See the README file at the base of this
 * source subtree for details about distribution.
 *
 * The Mach 3 version of the x-kernel is substantially different from
 * the original UofA version. Please report bugs to mach@cs.cmu.edu,
 * and not directly to the x-kernel project.
 */
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/net_dir.cc,v $
 *
 * 
 * Purpose:  Construct/support the directory structure for the net server
 *		and its protocols.  One directory for the server and one
 *		for each "user accessable" protocol (eg: tcp, ucp).
 * 
 * HISTORY
 * $Log:	net_dir.cc,v $
 * Revision 2.3  94/07/13  18:06:04  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:09:30  jms
 * 	Fix net directories protections.
 * 	Upgrade to xkernel v3.2
 * 	Add TCP support
 * 	[94/01/10  11:25:13  jms]
 * 
 * Revision 2.4  92/07/05  23:33:34  dpj
 * 	Added explicit definition of remote_class_name()
 * 	under GXXBUG_VIRTUAL1.
 * 	[92/06/29  17:25:23  dpj]
 * 
 * Revision 2.3  92/03/05  15:09:58  jms
 * 	Upgrade to use no MACH_IPC_COMPAT features.  New IPC only!
 * 	[92/02/26  19:00:18  jms]
 * 
 * Revision 2.2  91/11/06  14:14:09  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:07:23  pjg]
 * 
 * Revision 2.3  91/05/05  19:30:41  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:04:55  dpj]
 * 
 * 	Removed net_proxy.
 * 	Added default protection.
 * 	[91/04/28  10:46:05  dpj]
 * 
 * 	Added usudp subdirectory.
 * 	[91/02/25  10:46:42  dpj]
 * 
 * Revision 2.2  90/10/29  18:11:43  dpj
 * 	Integration into the master source tree
 * 	[90/10/21  23:13:12  dpj]
 * 
 * 	First working version.
 * 	[90/10/03  22:00:22  dpj]
 * 
 *
 */

#include	<net_dir_ifc.h>
#include	<usudp_dir_ifc.h>
#include	<ustcp_dir_ifc.h>

extern "C" {
#include	<us_error.h>
#include	<net_types.h>

extern ns_authid_t fs_access_default_root_authid;

extern int	x_errno;
}

/*
 * Debugging definitions.
 */
struct debug_table {
	char	*name;
	int	*addr;
};

#ifdef TRACE_DEBUG
extern int		tracemach3;
extern int		traceudpp;
extern int		traceipp;
extern int		tracearpp;
extern int		traceether;
extern int		traceidle;
extern int		traceprocesscreation;
extern int		traceprocessswitch;
extern int		traceprotocol;
extern int		traceevent;
extern int		tracemsg;
static struct debug_table debug_table[] = {
			{ "mach3", &tracemach3},
			{ "udpp", &traceudpp},
			{ "ipp", &traceipp},
			{ "arpp", &tracearpp},
			{ "ether", &traceether},
			{ "idle", &traceidle},
			{ "processcreation", &traceprocesscreation},
			{ "processswitch", &traceprocessswitch},
			{ "protocol", &traceprotocol},
			{ "event", &traceevent},
			{ "msg", &tracemsg},
			{ 0, 0}
};
#else
    static struct debug_table debug_table[] = {{0,0}};
#endif TRACE_DEBUG

#define BASE dir
DEFINE_CLASS(net_dir)

void net_dir::init_class(usClass* class_obj)
{
	BASE::init_class(class_obj);
	BEGIN_SETUP_METHOD_WITH_ARGS(net_dir);
	SETUP_METHOD_WITH_ARGS(net_dir,ns_create);
	SETUP_METHOD_WITH_ARGS(net_dir,ns_list_types);
	END_SETUP_METHOD_WITH_ARGS;
}

net_dir::net_dir(ns_mgr_id_t mgr_id, access_table *acctab)
:
 dir(mgr_id, acctab),
 arp_prot(0)
{
	if (acctab) {
		(void) net_init();
	}
}

extern "C" {
void usudp_install_mgr(XObj, usTop*);
void ustcp_install_mgr(XObj, usTop*);
}

net_dir::net_dir(ns_mgr_id_t mgr_id, mach_error_t* ret)
:
 dir(mgr_id, ret),
 arp_prot(0)
{
	XObj			udp_protocol;
	usudp_dir		*usudp;

	XObj			tcp_protocol;
	ustcp_dir		*ustcp;
	ns_prot_t		subdir_prot = NULL;
	int			acl_len = 2;

	(void) net_init();

	/*
	 * Initialize protection for misc network subdirectories.
	 */

	subdir_prot = (ns_prot_t)malloc(sizeof(struct ns_prot_head) +
				(acl_len * sizeof(struct ns_acl_entry)));

	subdir_prot->head.version = NS_PROT_VERSION;
	subdir_prot->head.generation = 0;
	subdir_prot->head.acl_len = acl_len;

	subdir_prot->acl[0].authid = fs_access_default_root_authid;
	subdir_prot->acl[0].rights = NSR_ADMIN |
		NSR_REFERENCE | NSR_READ | NSR_GETATTR | NSR_LOOKUP;
	subdir_prot->acl[1].authid = NS_AUTHID_WILDCARD;
	subdir_prot->acl[1].rights =  
		NSR_REFERENCE | NSR_READ | NSR_GETATTR | NSR_LOOKUP;

	*ret = ns_set_protection(subdir_prot,
				     NS_PROT_SIZE(subdir_prot) / sizeof(int));
	if (*ret != ERR_SUCCESS) {
		us_internal_error("ns_set_protection(net_dir)",*ret);
	}

	/*
	 * Create the usudp protocol directory.
	 */
	udp_protocol = xGetProtlByName("usudp");
	if (udp_protocol == ERR_XOBJ) {
		us_internal_error("Cannot find usudp protocol",
							US_INTERNAL_ERROR);
	}

	usudp = new usudp_dir(mgr_id, access_tab, udp_protocol);

	subdir_prot->acl[1].rights |= NSR_INSERT;
	*ret = usudp->ns_set_protection(subdir_prot,
				     NS_PROT_SIZE(subdir_prot) / sizeof(int));
	if (*ret != ERR_SUCCESS) {
		us_internal_error("ns_set_protection(usudp)",*ret);
	}
	(void) usudp_install_mgr(udp_protocol,usudp);
	*ret = ns_insert_entry("udp",usudp);
	if (*ret != ERR_SUCCESS) {
		us_internal_error("ns_insert_entry(usudp)",*ret);
	}
	mach_object_dereference(usudp);

	/*
	 * Create the ustcp protocol directory.
	 */
	tcp_protocol = xGetProtlByName("ustcp");
	if (tcp_protocol == ERR_XOBJ) {
		us_internal_error("Cannot find ustcp protocol",
							US_INTERNAL_ERROR);
	}

	ustcp = new ustcp_dir(mgr_id, access_tab, tcp_protocol);
	*ret = ustcp->ns_set_protection(subdir_prot,
				     NS_PROT_SIZE(subdir_prot) / sizeof(int));
	if (*ret != ERR_SUCCESS) {
		us_internal_error("ns_set_protection(ustcp)",*ret);
	}
	(void) ustcp_install_mgr(tcp_protocol,ustcp);
	*ret = ns_insert_entry("tcp",ustcp);
	if (*ret != ERR_SUCCESS) {
		us_internal_error("ns_insert_entry(ustcp)",*ret);
	}
	free(subdir_prot);
	mach_object_dereference(ustcp);
}

#ifdef	GXXBUG_VIRTUAL1
char* net_dir::remote_class_name() const
	{ return dir::remote_class_name(); }
#endif	GXXBUG_VIRTUAL1


mach_error_t 
net_dir::ns_create(ns_name_t name, ns_type_t type, ns_prot_t prot,
		   int protlen, ns_access_t access, usItem **newobj)
{
	/*
	 * We do not support anything so far.
	 */
	return(US_UNSUPPORTED);
}


mach_error_t net_dir::ns_list_types(ns_type_t **types, int *count)
{
	mach_error_t		ret;
	vm_address_t		data;

	/*
	 * Get space for the reply.
	 */
	data = NULL;
	ret = vm_allocate(mach_task_self(),&data,1 * sizeof(ns_type_t),TRUE);
	if (ret != KERN_SUCCESS) {
		*count = 0;
		*types = NULL;
		return(ret);
	}

	/*
	 * Prepare the reply.
	 */
	((ns_type_t *)data)[0] = NST_INVALID;

	*types = (ns_type_t *)data;
	*count = 1;

	return(NS_SUCCESS);
}


mach_error_t net_dir::net_init()
{
	Local(arp_prot) = xGetProtlByName("arp");
	if (Local(arp_prot) == ERR_XOBJ) {
		ERROR((0,"Cannot find ARP protocol object"));
		return(US_UNKNOWN_ERROR);
	}

	return(ERR_SUCCESS);
}


mach_error_t net_dir::net_arp(ipaddr_t ipaddr, ethaddr_t *ethaddr)
{
	xkern_return_t		xret;
	union {
		ipaddr_t	ipaddr;
		ethaddr_t	ethaddr;
	} either;

	either.ipaddr = ipaddr;
	xret = (xkern_return_t)xControl(Local(arp_prot), RESOLVE, (char *)(&either), sizeof(either));

	if (xret == XK_FAILURE) {
		return(convert_xkernel_error(x_errno));
	}

	*ethaddr = either.ethaddr;

	return(ERR_SUCCESS);
}


mach_error_t net_dir::net_set_debug(char *name, int value)
{
	struct debug_table	*tp;

	for (tp = debug_table; tp->name != 0; tp++) {
		if (!strcmp(tp->name,name))
			break;
	}

	if (tp->name != NULL) {
		*(tp->addr) = value;
		return(ERR_SUCCESS);
	} else {
		return(US_OBJECT_NOT_FOUND);
	}
}


