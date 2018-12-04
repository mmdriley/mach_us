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
 * ObjectClass: usudp_cots
 *
 * SuperClass: tmp_agency
 *
 * Delegated Objects: std_prot
 *
 * ClassMethods:
 *
 * Notes:
 */ 
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usudp_cots_ifc.h,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: xkernel UDP endpoint for connection-oriented operation.
 *
 * HISTORY:
 * $Log:	usudp_cots_ifc.h,v $
 * Revision 2.4  94/07/13  18:06:42  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:55  jms
 * 	Change the order of multiple inheritance of to implementation class followed
 * 	by abstract class.  This change was needed because the other order does not
 * 	work with gcc2.3.3.
 * 
 * 	Need dummy implementations of virtual methods in class usudp_cots
 * 		for 2.3.3 g++ -modh
 * 	[94/04/29  13:30:28  jms]
 * 
 * Revision 2.2  94/01/11  18:10:09  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  11:54:08  jms]
 * 
 * Revision 2.4  92/07/05  23:33:49  dpj
 * 	tmp_cleanup_for_shutdown -> ns_tmp_cleanup_for_shutdown
 * 	[92/06/24  17:16:50  jms]
 * 
 * Revision 2.3  91/11/06  14:14:39  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:09:04  pjg]
 * 
 * Revision 2.2  91/05/05  19:30:59  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:28  dpj]
 * 
 * 	First working version.
 * 	[91/04/28  10:49:33  dpj]
 * 
 */

#ifndef	_usudp_cots_ifc_h
#define	_usudp_cots_ifc_h

#include	<usx_endpt_base_ifc.h>
#include	<usx_dir_ifc.h>
#include	<us_net_cots_ifc.h>
#include	<us_net_connector_ifc.h>
#include	<usx_iobuf_mgr_ifc.h>
#include	<recordstream_ifc.h>

extern "C" {
#include	<ns_types.h>
#include	<net_types.h>
#include	<io_types.h>
#define this _this
#include	<ip.h>
#include	<udp.h>
#undef this
}

/*
 * Routine for building new cots endpoints.
 */
extern usx_new_cots_fun_t usudp_new_cots;

class usudp_cots: public usx_endpt_base, public usNetCOTS {
	net_costate_t		costate;
	vol_agency		*connector;
	IPPaddr			ipp_local;
	IPPaddr			ipp_remote;
	Part			participants[2];
	XObj			udp_session;
	usx_dir		*protocol_dir;
	usx_iobuf_mgr		*buf_mgr;
	io_recnum_t		outgoing_recnum;
	recordstream		*incoming_stream;
      public:
	DECLARE_MEMBERS(usudp_cots);
	usudp_cots(ns_mgr_id_t, access_table*, vol_agency*, usx_iobuf_mgr*,
		   usx_dir*, net_addr_t*, net_addr_t*, mach_error_t*);
	usudp_cots();
	virtual ~usudp_cots();
	virtual char* remote_class_name() const;

	virtual mach_error_t ns_tmp_cleanup_for_shutdown();
REMOTE	virtual mach_error_t net_get_localaddr(net_addr_t*);
	virtual mach_error_t net_get_peeraddr(net_addr_t*);
REMOTE	virtual mach_error_t io_read1rec_seq(io_mode_t, char*, unsigned int*,
					     io_recnum_t*);
REMOTE	virtual mach_error_t io_write1rec_seq(io_mode_t, char*, unsigned int,
					      io_recnum_t*);
	virtual mach_error_t io_getrec_seq(io_mode_t, io_record_t*,
					   unsigned int*, io_recnum_t*);
	virtual mach_error_t io_putrec_seq(io_mode_t, io_record_t*,
					   unsigned int*, io_recnum_t*);
REMOTE	virtual mach_error_t ns_get_attributes(ns_attr_t, int*);
REMOTE	virtual mach_error_t net_snddis(char*, unsigned int);
REMOTE	virtual mach_error_t net_rcvdis(char*, unsigned int*);


	virtual xkern_return_t usx_pop_internal(XObj, Msg*);
	virtual xkern_return_t usx_opendone_internal(XObj, XObj, XObj);
	virtual xkern_return_t usx_closedone_internal(XObj);

};

#endif	_usudp_cots_ifc_h

