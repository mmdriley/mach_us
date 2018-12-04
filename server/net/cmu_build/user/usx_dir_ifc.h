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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_dir_ifc.h,v $
 *
 * Author: J. Mark Stevenson
 *
 * Purpose: Base manager for all endpoints supported by the x-kernel.
 *
 * HISTORY
 * $Log:	usx_dir_ifc.h,v $
 * Revision 2.3  94/07/13  18:06:55  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:23  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  12:48:17  jms]
 * 
 */

#ifndef	_usx_dir_ifc_h
#define	_usx_dir_ifc_h

#include	<net_dir_base_ifc.h>
#include	<us_net_connector_ifc.h>
#include	<usx_endpt_base_ifc.h>
#include	<usx_iobuf_mgr_ifc.h>

extern "C" {
#include	<net_types.h>
#define this _this
#include	"upi.h"
#undef this
}

class usx_dir;
typedef mach_error_t (*usx_new_clts_fun_t)(
			ns_mgr_id_t	mgr_id,
			access_table	*_acctab,
			usx_iobuf_mgr	*_buf_mgr,
			usx_dir	*_protocol_dir,
			net_addr_t	*_localaddr,
			agency		**endpt,
			ns_type_t	*endpt_type);

typedef mach_error_t (*usx_new_cots_fun_t)(
			ns_mgr_id_t	mgr_id,
			access_table	*_acctab,
			vol_agency	*_connector,
			usx_iobuf_mgr	*_buf_mgr,
			usx_dir	*_protocol_dir,
			net_addr_t	*_localaddr,
			net_addr_t	*_peeraddr,
			XObj		sess, /* Null for active connection */
			agency		**endpt,
			ns_type_t	*endpt_type);

class usx_dir: public net_dir_base {
	usx_iobuf_mgr	*buf_mgr;
	XObj		protocol;
	IPPaddr		default_ipaddr;
	unsigned int	next_free_port;

	usx_new_clts_fun_t usx_new_clts;
	usx_new_cots_fun_t usx_new_cots;

      protected:
	Map		openenable_map;		/* local addr to endpt */
	Map		demux_map;		/* session to endpt */

      public:
	DECLARE_MEMBERS(usx_dir);
	usx_dir(ns_mgr_id_t, access_table *, XObj,
		usx_new_clts_fun_t, usx_new_cots_fun_t);

	usx_dir();
	virtual ~usx_dir();

#ifdef	GXXBUG_VIRTUAL1
	virtual char* remote_class_name() const;
#endif	GXXBUG_VIRTUAL1

REMOTE	virtual mach_error_t ns_list_types(ns_type_t**, int*);
REMOTE	virtual mach_error_t net_create(net_addr_t*, int*, ns_prot_t, int,
					ns_access_t, usItem**, ns_type_t*,
					net_info_t*);
REMOTE	virtual mach_error_t net_lookup(net_addr_t*, ns_access_t, usItem**,
					ns_type_t*, net_info_t*);
REMOTE	virtual mach_error_t net_cots_lookup(net_addr_t*, net_addr_t*,
					     ns_access_t, usItem**,
					     ns_type_t*, net_info_t*);

	virtual mach_error_t usx_new_connection_endpt(vol_agency*,
					net_addr_t*, net_addr_t*,
					XObj, agency**);

	virtual mach_error_t usx_register_connection_endpt(agency*,
					net_addr_t*, net_addr_t*,
					ns_prot_t, int);

	virtual mach_error_t usx_get_connection_endpt(vol_agency*,
					net_addr_t*, net_addr_t*,
					ns_prot_t, int, XObj, agency**);

	virtual int usx_demux_internal(XObj, Msg*);
	virtual xkern_return_t usx_opendone_internal(XObj, XObj, XObj);
	virtual mach_error_t usx_open_internal(usx_endpt_base*, boolean_t,
					       Part*, XObj*);
	virtual xkern_return_t usx_closedone_internal(XObj);
	virtual mach_error_t usx_close_internal(usx_endpt_base*, boolean_t, XObj);
	virtual mach_error_t usx_openenable_internal(usx_endpt_base*, IPPaddr*, Part*);
	virtual mach_error_t usx_opendisable_internal(usx_endpt_base*, IPPaddr*, Part*);

};

#endif	_usx_dir_ifc_h
