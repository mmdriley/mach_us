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
 * ObjectClass: usx_iobuf_mgr
 * Manager for a set of I/O buffers objects
 *		-- extended for network service under the x-kernel.
 * 
 * SuperClass: default_iobuf_mgr
 * 	
 * Delegated Objects:
 * 
 * ClassMethods:
 *
 * Notes:
 *
 * Bugs:
 * 
 * Features:
 * 
 * Transgressions:
 *
 */
 
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_iobuf_mgr_ifc.h,v $
 *
 * 
 * Purpose:  Interface to manipulate/translate between MachUS IO data 
 *		structures, and those of the xkernel.
 * 
 * HISTORY:
 * $Log:	usx_iobuf_mgr_ifc.h,v $
 * Revision 2.3  94/07/13  18:07:12  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:40  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  12:57:02  jms]
 * 
 * Revision 2.3  91/11/06  14:17:39  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:10:22  pjg]
 * 
 * Revision 2.2  91/05/05  19:31:19  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:06:09  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:49:52  dpj]
 * 
 * Revision 2.2  89/10/30  16:35:44  dpj
 * 	First version.
 * 	[89/10/27  19:16:08  dpj]
 * 
 *
 */

#ifndef	_usx_iobuf_mgr_ifc_h
#define	_usx_iobuf_mgr_ifc_h

#include	<default_iobuf_mgr_ifc.h>
#include	<usx_internal.h>

class usx_iobuf_mgr: public default_iobuf_mgr {
      public:
	DECLARE_LOCAL_MEMBERS(usx_iobuf_mgr);
	virtual mach_error_t io_alloc_block(unsigned int, io_block_t*);
	virtual mach_error_t io_free_block(io_block_t);

	virtual mach_error_t usx_convert_block_to_xmsg(io_block_t, Msg*);
	virtual mach_error_t usx_convert_xmsg_to_block_lst(Msg*, io_block_t*);

	virtual mach_error_t usx_convert_xmsg_to_record(Msg*, io_record_t*);
	virtual mach_error_t usx_convert_record_to_xmsg(io_record_t, Msg*);
};

#endif	_usx_iobuf_mgr_ifc_h
