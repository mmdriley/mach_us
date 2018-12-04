/* 
 * Mach Operating System
 * Copyright (c) 1994,1993 Carnegie Mellon University
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/ustcp_dir_ifc.h,v $
 *
 * Author: J. Mark Stevenson
 *
 * Purpose: Manager for all TCP endpoints supported by the x-kernel.
 *
 * HISTORY
 * $Log:	ustcp_dir_ifc.h,v $
 * Revision 2.3  94/07/13  18:06:27  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:09:57  jms
 * 	Initial Version
 * 	[94/01/10  11:46:30  jms]
 * 
 */

#ifndef	_ustcp_dir_ifc_h
#define	_ustcp_dir_ifc_h


#include	<usx_dir_ifc.h>

extern "C" {
#include	<net_types.h>
#define this _this
#include	"upi.h"
#undef this
}

class ustcp_dir: public usx_dir {
      public:
	DECLARE_MEMBERS(ustcp_dir);
	ustcp_dir(ns_mgr_id_t, access_table *, XObj);
	ustcp_dir();

#ifdef	GXXBUG_VIRTUAL1
	virtual char* remote_class_name() const;
#endif	GXXBUG_VIRTUAL1

REMOTE	virtual mach_error_t ns_list_types(ns_type_t**, int*);

	virtual mach_error_t usx_open_internal(usx_endpt_base*, boolean_t,
					       Part*, XObj*);
	virtual mach_error_t usx_close_internal(usx_endpt_base*, boolean_t, XObj);
};

#endif	_ustcp_dir_ifc_h
