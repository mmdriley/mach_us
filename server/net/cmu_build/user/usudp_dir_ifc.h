/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992 Carnegie Mellon University
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usudp_dir_ifc.h,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Manager for all UDP endpoints supported by the x-kernel.
 *
 * HISTORY
 * $Log:	usudp_dir_ifc.h,v $
 * Revision 2.3  94/07/13  18:06:47  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:15  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  11:55:50  jms]
 * 
 * Revision 2.4  92/07/05  23:33:57  dpj
 * 	Added explicit definition of remote_class_name()
 * 	under GXXBUG_VIRTUAL1.
 * 	[92/06/29  17:26:16  dpj]
 * 
 * Revision 2.3  91/11/06  14:14:46  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:09:27  pjg]
 * 
 * Revision 2.2  91/05/05  19:31:07  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:42  dpj]
 * 
 * 	First really working version.
 * 	[91/04/28  10:50:29  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:48:37  dpj]
 * 
 */

/*
 * Conditional for alternate mode of operation. See the .c file for details.
 */
#define	USE_SECONDARY_DEMUX_MAP		0

#if	USE_SECONDARY_DEMUX_MAP
#define	SECONDARY_DEMUX_MAP_DECL	Map	secondary_demux_map
#else	USE_SECONDARY_DEMUX_MAP
#define	SECONDARY_DEMUX_MAP_DECL	int	dummy_secondary_demux_map
#endif	USE_SECONDARY_DEMUX_MAP

#ifndef	_usudp_dir_ifc_h
#define	_usudp_dir_ifc_h

#include	<usx_dir_ifc.h>

extern "C" {
#include	<net_types.h>
#define this _this
#include	"upi.h"
#undef this
}

class usudp_dir: public usx_dir {
	SECONDARY_DEMUX_MAP_DECL;

      public:
	DECLARE_MEMBERS(usudp_dir);
	usudp_dir(ns_mgr_id_t, access_table *, XObj);
	usudp_dir();

#ifdef	GXXBUG_VIRTUAL1
	virtual char* remote_class_name() const;
#endif	GXXBUG_VIRTUAL1

REMOTE	virtual mach_error_t ns_list_types(ns_type_t**, int*);

	virtual int usx_demux_internal(XObj, Msg*);

	virtual mach_error_t usx_open_internal(usx_endpt_base*, boolean_t,
					       Part*, XObj*);
	virtual mach_error_t usx_close_internal(usx_endpt_base*, boolean_t, XObj);

};
#endif	_usudp_dir_ifc_h
