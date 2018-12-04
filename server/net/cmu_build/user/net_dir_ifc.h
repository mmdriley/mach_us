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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/net_dir_ifc.h,v $
 *
 * 
 * Purpose:  Interface for network dir support (see net_dir.cc)
 * 
 * HISTORY
 * $Log:	net_dir_ifc.h,v $
 * Revision 2.3  94/07/13  18:06:07  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:09:36  jms
 * 	Update to xkernel v3.2
 * 	[94/01/10  11:25:57  jms]
 * 
 * Revision 2.5  92/07/05  23:33:37  dpj
 * 	Added explicit definition of remote_class_name()
 * 	under GXXBUG_VIRTUAL1.
 * 	[92/06/29  17:25:38  dpj]
 * 
 * Revision 2.4  91/11/06  14:14:13  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:07:29  pjg]
 * 
 * Revision 2.3  91/05/05  19:30:43  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:02  dpj]
 * 
 * 	Include upi.h instead of userupi.h.
 * 	[91/02/25  10:47:15  dpj]
 * 
 * Revision 2.2  90/10/29  18:11:47  dpj
 * 	Integration into the master source tree
 * 	[90/10/21  23:13:22  dpj]
 * 
 * 	First working version.
 * 	[90/10/03  22:00:32  dpj]
 * 
 *
 */

#ifndef	_net_dir_ifc_h
#define	_net_dir_ifc_h

#include	<dir_ifc.h>

extern "C" {
#include	<net_types.h>
/*
 * X-kernel definitions.
 */
#define this _this
#include	<upi.h>
#include	<process.h>
#include	<arp.h>
#undef this
}


class net_dir: public dir {
	XObj		arp_prot;
      public:
	DECLARE_MEMBERS(net_dir);
	net_dir(ns_mgr_id_t =null_mgr_id, access_table * =0);
	net_dir(ns_mgr_id_t, mach_error_t*);

#ifdef	GXXBUG_VIRTUAL1
	virtual char* remote_class_name() const;
#endif	GXXBUG_VIRTUAL1

REMOTE	virtual mach_error_t ns_create(ns_name_t, ns_type_t, ns_prot_t,
				       int, ns_access_t, usItem **);
REMOTE	virtual mach_error_t ns_list_types(ns_type_t **, int *);

	virtual mach_error_t net_arp(ipaddr_t, ethaddr_t *);
	virtual mach_error_t net_set_debug(char *, int);
	mach_error_t net_init();
};

#endif	_net_dir_ifc_h
