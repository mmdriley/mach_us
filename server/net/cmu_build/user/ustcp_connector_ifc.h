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
 * ObjectClass: ustcp_clts
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/ustcp_connector_ifc.h,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: xkernel TCP endpoint for connection-less operation.
 *
 * HISTORY:
 * $Log:	ustcp_connector_ifc.h,v $
 * Revision 2.4  94/07/13  18:06:15  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:34  jms
 * 	Change the order of multiple inheritance of to implementation class followed
 * 	by abstract class.  This change was needed because the other order does not
 * 	work with gcc2.3.3.
 * 
 * 	Need dummy implementations of virtual methods in class ustcp_connector
 * 		for 2.3.3 g++ -modh
 * 	[94/04/28  19:09:08  jms]
 * 
 * Revision 2.2  94/01/11  18:09:45  jms
 * 	Initial Version
 * 	[94/01/10  11:33:57  jms]
 * 
 * Revision 2.4  92/07/05  23:33:44  dpj
 * 	tmp_cleanup_for_shutdown -> ns_tmp_cleanup_for_shutdown
 * 	[92/06/24  17:15:49  jms]
 * 
 * Revision 2.3  91/11/06  14:14:32  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:08:39  pjg]
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

#ifndef	_ustcp_clts_ifc_h
#define	_ustcp_clts_ifc_h

#include	<usx_endpt_base_ifc.h>
#include	<us_net_clts_ifc.h>
#include	<usx_iobuf_mgr_ifc.h>
#include	<usx_dir_ifc.h>
#include	<dll.h>
#include        <bytestream_ifc.h>

extern "C" {
#include	<net_types.h>
#include	<io_types.h>
#define this _this
#include	<ip.h>
#include	<tcp.h>
#undef this
}

/*
 * Routine for building new clts endpoints.
 */
extern usx_new_clts_fun_t ustcp_new_connector;


/*
 * Pending connection description
 */
#define USTCP_CONN_PENDING 0
#define USTCP_CONN_ACCEPTED 1
#define USTCP_CONN_REJECTED 2

typedef struct ustcp_conninfo {
	/*
	 * Control information in "listen" state.
	 */
	dll_chain_t		chain;		/* link in list */
	int			state;		/* accepted/rejected */

	/* Connection description: from xOpenDone */
	XObj		lls;
	XObj		llp;
	XObj		hlpType;
	agency		*endpt;
} *ustcp_conninfo_t;

class ustcp_connector: public usx_endpt_base, public usNetConnector {
	struct mutex		pendconn_lock;
	struct condition	pendconn_listen_cond;
	int			pendconn_count;
	int			pendconn_max;
	dll_head_t		pendconn_queue;

	usx_dir			*protocol_dir;
	usx_iobuf_mgr		*buf_mgr;
	net_addr_t		localaddr;
	Part			participants[2];
	IPPaddr			ipp_local;
	IPPaddr			ipp_remote;
	io_recnum_t		outgoing_recnum;
	bytestream		*incoming_stream;
	net_options_t		null_options;
	int			readers_count;

      public:
	DECLARE_MEMBERS(ustcp_connector);
	ustcp_connector();
	ustcp_connector(ns_mgr_id_t mgr_id, access_table *_acctab,
		   usx_iobuf_mgr *_usx_iobuf_mgr, usx_dir *_protocol_dir,
		   net_addr_t *_localaddr, mach_error_t*);
	virtual ~ustcp_connector();
	virtual char* remote_class_name() const;

	virtual mach_error_t ns_tmp_cleanup_for_shutdown();
	virtual mach_error_t ns_register_agent(ns_access_t);
	virtual mach_error_t ns_unregister_agent(ns_access_t);

REMOTE	virtual mach_error_t net_get_localaddr(net_addr_t *);
REMOTE	virtual mach_error_t net_connect(net_addr_t*, net_options_t*, char*,
					 unsigned int, char*, unsigned int*,
					 ns_prot_t, unsigned int, ns_access_t,
					 usItem**, ns_type_t*);

REMOTE	virtual mach_error_t ns_get_attributes(ns_attr_t, int*);

	virtual xkern_return_t usx_pop_internal(XObj, Msg*);
	virtual xkern_return_t usx_opendone_internal(XObj, XObj, XObj);
	virtual xkern_return_t usx_closedone_internal(XObj);

/* from usNetConnector */
REMOTE	virtual mach_error_t net_listen(io_mode_t, net_addr_t*, 
					net_options_t*, char*,
					unsigned int*, int*);
REMOTE	virtual mach_error_t net_accept(int, net_options_t*, char*,
					unsigned int, ns_prot_t,
					unsigned int, ns_access_t,
					usItem**, ns_type_t*);
REMOTE	virtual mach_error_t net_reject(int, char*, unsigned int);
REMOTE	virtual mach_error_t net_get_connect_qinfo(int*, int*);
REMOTE	virtual mach_error_t net_set_connect_qmax(int);

};

#endif	_ustcp_connector_ifc_h

