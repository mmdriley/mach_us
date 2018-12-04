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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/ustcp_cots_ifc.h,v $
 *
 * Author: J. Mark Stevenson
 *
 * Purpose: xkernel TCP endpoint for connection-oriented operation.
 *
 * HISTORY:
 * $Log:	ustcp_cots_ifc.h,v $
 * Revision 2.4  94/07/13  18:06:21  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:40  jms
 * 	Need dummy implementations of virtual methods in class ustcp_cots
 * 		for 2.3.3 g++ -modh
 * 	[94/04/28  19:10:01  jms]
 * 
 * Revision 2.2  94/01/11  18:09:54  jms
 * 	Initial Version
 * 	[94/01/10  11:45:23  jms]
 * 
 */

#ifndef	_ustcp_cots_ifc_h
#define	_ustcp_cots_ifc_h

#include	<usx_endpt_base_ifc.h>
#include	<usx_dir_ifc.h>
#include	<us_net_cots_ifc.h>
#include	<us_net_connector_ifc.h>
#include	<usx_iobuf_mgr_ifc.h>
#include	<us_byteio_ifc.h>
#include	<bytestream_ifc.h>

extern "C" {
#include	<ns_types.h>
#include	<net_types.h>
#include	<io_types.h>
#define this _this
#include	<ip.h>
#include	<tcp.h>
#undef this
}

/*
 * Routine for building new cots endpoints.
 */
extern usx_new_cots_fun_t ustcp_new_cots;

class ustcp_cots: public usx_endpt_base, public usNetCOTS {
	net_costate_t		costate;
	vol_agency		*connector;
	IPPaddr			ipp_local;
	IPPaddr			ipp_remote;
	Part			participants[2];
	XObj			tcp_session;
	usx_dir			*protocol_dir;
	usx_iobuf_mgr		*buf_mgr;
	bytestream		*incoming_stream;
	io_offset_t		write_offset;	/* offset for write */
      public:
	DECLARE_MEMBERS(ustcp_cots);
	ustcp_cots(ns_mgr_id_t, access_table*, vol_agency*, usx_iobuf_mgr*,
		   usx_dir*, net_addr_t*, net_addr_t*, XObj, mach_error_t*);
	ustcp_cots();
	virtual ~ustcp_cots();
	virtual char* remote_class_name() const;

	virtual mach_error_t ns_tmp_cleanup_for_shutdown();
REMOTE	virtual mach_error_t net_get_localaddr(net_addr_t*);
REMOTE	virtual mach_error_t ns_get_attributes(ns_attr_t, int*);

	virtual mach_error_t net_get_peeraddr(net_addr_t*);
REMOTE	virtual mach_error_t net_snddis(char*, unsigned int);
REMOTE	virtual mach_error_t net_rcvdis(char*, unsigned int*);


REMOTE	virtual mach_error_t io_read_seq(io_mode_t, char *, io_count_t *,
					 io_offset_t *);
REMOTE	virtual mach_error_t io_write_seq(io_mode_t, char *, io_count_t *,
					  io_offset_t *);
	virtual mach_error_t io_getbytes_seq(io_mode_t, io_block_t *,
					     io_count_t *, io_offset_t *);
	virtual mach_error_t io_putbytes_seq(io_mode_t, io_block_t *,
					     io_count_t *, io_offset_t *);

	virtual xkern_return_t usx_pop_internal(XObj, Msg*);
	virtual xkern_return_t usx_opendone_internal(XObj, XObj, XObj);
	virtual xkern_return_t usx_closedone_internal(XObj);

	/*
	 * From usByteIO but not implemented
	 */
	virtual mach_error_t io_read(io_mode_t, io_offset_t, pointer_t, 
				     unsigned int*);
	virtual mach_error_t io_write(io_mode_t, io_offset_t, pointer_t, 
				      unsigned int*);
	virtual mach_error_t io_append(io_mode_t, pointer_t, unsigned int*);
	virtual mach_error_t io_set_size(io_size_t);
	virtual mach_error_t io_get_size(io_size_t *);
	virtual mach_error_t io_map(task_t, vm_address_t*, vm_size_t,
				    vm_offset_t, boolean_t, vm_offset_t,
				    boolean_t, vm_prot_t, vm_prot_t,
				    vm_inherit_t);
       
};

#endif	_ustcp_cots_ifc_h
