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
 * ObjectClass: usx_endpt_base
 *
 *
 * ClassMethods:
 *
 * Notes:
 */ 
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_endpt_base_ifc.h,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Base class for xkernel UDP endpoint for connection-less operation.
 *
 * HISTORY:
 * $Log:	usx_endpt_base_ifc.h,v $
 * Revision 2.3  94/07/13  18:07:03  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:27  jms
 * 	Revised with the introduction of common "usx_" logic and xkernel v3.2
 * 	[94/01/10  12:50:55  jms]
 * 
 * Revision 2.2  91/11/06  14:14:23  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:08:13  pjg]
 * 
 * Revision 2.2  91/05/05  19:30:54  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:20  dpj]
 * 
 * 	First really working version.
 * 	[91/04/28  10:49:00  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:48:08  dpj]
 * 
 */

#ifndef	_usx_endpt_base_ifc_h
#define	_usx_endpt_base_ifc_h

#include	<net_endpt_base_ifc.h>
#include	<usx_internal.h>

extern "C" {
#include	<net_types.h>
#include	<io_types.h>
#define this _this
#include	<ip.h>
#include	<udp.h>
#undef this
}

class usx_endpt_base: public net_endpt_base {
      public:
	DECLARE_MEMBERS_ABSTRACT_CLASS(usx_endpt_base);
	usx_endpt_base(ns_mgr_id_t =null_mgr_id, access_table * =0);

	virtual xkern_return_t usx_pop_internal(XObj, Msg*) =0;
	virtual xkern_return_t usx_opendone_internal(XObj, XObj, XObj) =0;
	virtual xkern_return_t usx_closedone_internal(XObj) =0;
};

#endif	_usx_endpt_base_ifc_h

