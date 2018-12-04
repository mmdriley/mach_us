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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usudp_cots.cc,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: xkernel UDP endpoint for connection-oriented operation.
 *
 * HISTORY:
 * $Log:	usudp_cots.cc,v $
 * Revision 2.4  94/07/13  18:06:39  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:52  jms
 * 	Need dummy implementations of virtual methods in class usudp_cots
 * 		for 2.3.3 g++ -modh
 * 	[94/04/29  13:30:03  jms]
 * 
 * Revision 2.2  94/01/11  18:10:05  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	Initial Version
 * 	[94/01/10  11:53:28  jms]
 * 
 * Revision 2.3  92/07/05  23:33:47  dpj
 * 	tmp_cleanup_for_shutdown -> ns_tmp_cleanup_for_shutdown
 * 	[92/06/24  17:16:14  jms]
 * 
 * Revision 2.2  91/11/06  14:14:36  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:08:58  pjg]
 * 
 * Revision 2.2  91/05/05  19:30:57  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:24  dpj]
 * 
 * 	First working version.
 * 	[91/04/28  10:49:18  dpj]
 * 
 */

/*
 * XXX Instead of using a recordstream object to queue incoming messages
 * in standard record format, this code might queue native x-kernel
 * messages, and move them to records only when necessary.
 */

#include	<usudp_cots_ifc.h>
#include	<iobuf_user_ifc.h>

extern "C" {
#include	<base.h>

#include	<us_error.h>
#include	<net_error.h>
#include	<ns_types.h>
#include	<io_types.h>
#include	<io_types2.h>
#include	<net_types.h>
#include	<usx_internal.h>
#include	"upi.h"
#include	"process.h"
#include	"udp.h"
}

/*
 * Routine for building new clts endpoints.
 */
static mach_error_t new_cots(
			ns_mgr_id_t	mgr_id,
			access_table	*_acctab,
			vol_agency	*_connector,
			usx_iobuf_mgr	*_buf_mgr,
			usx_dir	*_protocol_dir,
			net_addr_t	*_localaddr,
			net_addr_t	*_peeraddr,
			XObj		sess,
			agency		**endpt,
			ns_type_t	*endpt_type);

usx_new_cots_fun_t usudp_new_cots = new_cots;

DEFINE_CLASS_MI(usudp_cots)
DEFINE_CASTDOWN2(usudp_cots, usx_endpt_base, usNetCOTS)

void usudp_cots::init_class(usClass* class_obj)
{
	usx_endpt_base::init_class(class_obj);

	BEGIN_SETUP_METHOD_WITH_ARGS(usudp_cots);
	SETUP_METHOD_WITH_ARGS(usudp_cots,net_get_localaddr);
	SETUP_METHOD_WITH_ARGS(usudp_cots,io_read1rec_seq);
	SETUP_METHOD_WITH_ARGS(usudp_cots,io_write1rec_seq);
	SETUP_METHOD_WITH_ARGS(usudp_cots,ns_get_attributes);
	SETUP_METHOD_WITH_ARGS(usudp_cots,net_snddis);
	SETUP_METHOD_WITH_ARGS(usudp_cots,net_rcvdis);
	END_SETUP_METHOD_WITH_ARGS;
}


/*
 * Initial (default) buffer size.
 */
int	usudp_cots_bufsize = 100;

usudp_cots::usudp_cots(ns_mgr_id_t	mgr_id, 
		       access_table	*_acctab,
		       vol_agency	*_connector,
		       usx_iobuf_mgr	*_buf_mgr,
		       usx_dir	*_protocol_dir,
		       net_addr_t	*_localaddr,
		       net_addr_t	*_peeraddr,
		       mach_error_t	*ret)

:
 usx_endpt_base(mgr_id, _acctab),
 connector(_connector), protocol_dir(_protocol_dir), buf_mgr(_buf_mgr),
 costate(NET_COSTATE_CONNECTED)
{
	IPPaddr			ipp_local, ipp_remote;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	*ret = ERR_SUCCESS;
	(void) _connector->ns_register_stronglink();
	mach_object_reference(_protocol_dir);
	mach_object_reference(_buf_mgr);

	INT_TO_IO_RECNUM(0,&Local(outgoing_recnum));

//	add_delegate(Self,iobuf_user);
//	ret = setup_iobuf_user(Self,buf_mgr);
//	if (ret != ERR_SUCCESS) {
//		return(ret);
//	}

	/*
	 * Cannot let the network code wait on the incoming stream.
	 *
	 * XXX Need a better strategy to drop old packets instead
	 * of new ones.
	 */
//	new_object(Local(incoming_stream),recordstream);
//	ret = setup_recordstream(Local(incoming_stream),Local(buf_mgr),
//				usudp_cots_bufsize,
//				IOS_ENABLED|IOS_WAIT_ALLOWED,IOS_ENABLED);

	incoming_stream = new recordstream(_buf_mgr, usudp_cots_bufsize,
				IOS_ENABLED|IOS_WAIT_ALLOWED,IOS_ENABLED);

	USX_CVT_NETADDR(_localaddr,&ipp_local);
	USX_CVT_NETADDR(_peeraddr,&ipp_remote);
	partInit(Local(participants), 2);

	partPush(participants[LOCAL_PART], &(ipp_local.host), 
			sizeof(ipp_local.host));
	partPush(participants[LOCAL_PART], &(ipp_local.port),
			sizeof(ipp_local.port));
	partPush(participants[REMOTE_PART], &(ipp_remote.host),
			sizeof(ipp_remote.host));
	partPush(participants[REMOTE_PART], &(ipp_remote.port),
			sizeof(ipp_remote.port));

	Local(udp_session) = ERR_XOBJ;
	*ret = protocol_dir->usx_open_internal(this,TRUE,
					participants,&Local(udp_session));
}

static mach_error_t new_cots(
			ns_mgr_id_t	mgr_id,
			access_table	*_acctab,
			vol_agency	*_connector,
			usx_iobuf_mgr	*_buf_mgr,
			usx_dir	*_protocol_dir,
			net_addr_t	*_localaddr,
			net_addr_t	*_peeraddr,
			XObj		sess,
			agency		**endpt,
			ns_type_t	*endpt_type)
{
	mach_error_t	ret;

	*endpt = new usudp_cots(mgr_id, _acctab, _connector, _buf_mgr,
			_protocol_dir, _localaddr, _peeraddr, &ret);
	*endpt_type = NST_COTS_RECS;
	return(ret);
}

usudp_cots::usudp_cots()
:
 connector(0), protocol_dir(0), buf_mgr(0), incoming_stream(0)
{}

usudp_cots::~usudp_cots()
{
	if (connector) {
		(void) connector->ns_unregister_stronglink();
	}
	mach_object_dereference(Local(protocol_dir));
	mach_object_dereference(Local(buf_mgr));
	mach_object_dereference(Local(incoming_stream));
}

char* usudp_cots::remote_class_name() const
{
	return "usNetCOTS_recs_proxy";
}

mach_error_t usudp_cots::ns_tmp_cleanup_for_shutdown()
{
	mach_error_t		ret;

	if (Local(costate) == NET_COSTATE_CONNECTED) {
		(void) net_snddis(NULL,0);
	}

	ret = tmp_agency::ns_tmp_cleanup_for_shutdown();

	return(ret);
}


mach_error_t usudp_cots::net_get_localaddr(net_addr_t		*addr)	/* out */
{
	IPPaddr			ipp_addr;
	xkern_return_t		xret;

	XKERNEL_MASTER();
	xret = usx_get_IPPaddr(Local(udp_session), LOCAL_PART, &ipp_addr);
	XKERNEL_RELEASE();
	if (xret == XK_FAILURE) {
		us_internal_error("net_get_localaddr.usx_get_IPPaddr",
					convert_xkernel_error(x_errno));
 		return(US_INTERNAL_ERROR);
 	}

	net_addr_inet_init(addr,*(ipaddr_t*)&ipp_addr.host,htons((unsigned short)ipp_addr.port));

	return(ERR_SUCCESS);
 }


mach_error_t usudp_cots::net_get_peeraddr(net_addr_t		*addr)	/* out */
{
	IPPaddr			ipp_addr;
	xkern_return_t		xret;

	XKERNEL_MASTER();
	xret = usx_get_IPPaddr(Local(udp_session), REMOTE_PART, &ipp_addr);
	XKERNEL_RELEASE();
	if (xret == XK_FAILURE) {
		us_internal_error("net_get_peeraddr.usx_get_IPPaddr",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	net_addr_inet_init(addr,*(ipaddr_t*)&ipp_addr.host,htons((unsigned short)ipp_addr.port));

	return(ERR_SUCCESS);
}


mach_error_t 
usudp_cots::io_read1rec_seq(io_mode_t		mode,
			    char		*buf,
			    unsigned int	*len,		/* inout */
			    io_recnum_t		*recnum)	/* out */
{
	mach_error_t		ret;

	ret = Local(incoming_stream)->io_read1rec_seq(mode,buf,len,recnum);

	return(ret);
}


mach_error_t 
usudp_cots::io_write1rec_seq(io_mode_t		mode,
			     char		*buf,
			     unsigned int	len,
			     io_recnum_t	*recnum)	/* out */
{
	mach_error_t		ret;
	unsigned int		count = 1;
	io_record_t		rec;

	if ((mode & IOM_PROBE) == 0) {
		/*
		 * XXX Should construct an x-kernel message directly out of the
		 * user memory when possible, with assistance from the RPC
		 * system (i.e. just keep IPC receive buffers around...).
		 */

		ret = Local(buf_mgr)->io_alloc_record(len,&rec);
		if (ret != ERR_SUCCESS) {
			return(ret);
		}

		bcopy(buf,ioblk_start(rec->first_block),len);
		rec->first_block->end_offset += len;
	} else {
		rec = 0;
	}

	ret = io_putrec_seq(mode,&rec,&count,recnum);

	return(ret);
}


mach_error_t 
usudp_cots::io_getrec_seq(io_mode_t		mode,
			  io_record_t		*rec,		/* inout */
			  unsigned int		*count,		/* inout */
			  io_recnum_t		*recnum)	/* out */
{
	mach_error_t		ret;

	ret = Local(incoming_stream)->io_getrec_seq(mode,rec,count,recnum);

	return(ret);
}


mach_error_t 
usudp_cots::io_putrec_seq(io_mode_t		mode,
			  io_record_t		*rec,		/* inout */
			  unsigned int		*count,		/* inout */
			  io_recnum_t		*recnum)	/* out */
{
	Msg			xmsg;
	IPPaddr			ipp_addr;
	mach_error_t		ret;
	xkern_return_t		xret;

	if (*count != 1) {
		us_internal_error("io_putrec_seq() with count > 1",
							US_NOT_IMPLEMENTED);
		return(US_NOT_IMPLEMENTED);
	}

	if (mode & ~(IOM_WAIT | IOM_PROBE | IOM_TRUNCATE)) {
		return(IO_INVALID_MODE);
	}

#ifdef	NOTDEF
	if (! (Local(write_strategy) & IOS_ENABLED)) {
		return(IO_REJECTED);
	}
#endif	NOTDEF

	if (Local(costate) != NET_COSTATE_CONNECTED) {
		return(NET_NOT_CONNECTED);
	}

	if (mode & IOM_PROBE) {
		XKERNEL_MASTER();
		*recnum = Local(outgoing_recnum);
		XKERNEL_RELEASE();
		return(ERR_SUCCESS);
	}

	/*
	 * Prepare x-kernel message.
	 */
	ret = Local(buf_mgr)->usx_convert_record_to_xmsg(*rec,&xmsg);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	XKERNEL_MASTER();

	/*
	 * Send the message.
	 */
	xret = xPush(Local(udp_session),&xmsg);
	if (xret == XK_FAILURE) {
		us_internal_error("x_push",convert_xkernel_error(x_errno));
		XKERNEL_RELEASE();
		msgDestroy(&xmsg);
		return(US_INTERNAL_ERROR);
	}

	/*
	 * Update record number.
	 */
	*recnum = Local(outgoing_recnum);
	INCREMENT_IO_RECNUM(&Local(outgoing_recnum));

	XKERNEL_RELEASE();

	msgDestroy(&xmsg);
	return(ERR_SUCCESS);
}


mach_error_t 
usudp_cots::ns_get_attributes(ns_attr_t		attr,
			      int		*attrlen)
{
	mach_error_t		ret;

//	ret = invoke_super(Super,mach_method_id(ns_get_attributes),attr,attrlen);
	ret = tmp_agency::ns_get_attributes(attr,attrlen);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	attr->type = NST_COTS_RECS;

	/*
	 * XXX Should return the number of available records.
	 */

	return(ERR_SUCCESS);
}


mach_error_t 
usudp_cots::net_snddis(char			*udata,
		       unsigned int		udatalen)
{
	if (Local(costate) != NET_COSTATE_CONNECTED) {
		return(NET_NOT_CONNECTED);
	}

	XKERNEL_MASTER();

	Local(costate) = NET_COSTATE_DISCONNECTED;

	/*
	 * Make sure no one can read/write data here; wakeup
	 * threads blocked in pending I/O operations.
	 */
	(void) Local(incoming_stream)->io_set_read_strategy(0);
	(void) Local(incoming_stream)->io_set_write_strategy(0);
	(void) Local(incoming_stream)->io_flush_stream();

	if (Local(udp_session) != ERR_XOBJ) {
		(void) Local(protocol_dir)->usx_close_internal(this,
						FALSE,Local(udp_session));
		Local(udp_session) = ERR_XOBJ;
	}

	XKERNEL_RELEASE();

	return(ERR_SUCCESS);
}


mach_error_t 
usudp_cots::net_rcvdis(char		*udata,			/* OUT */
		       unsigned int	*udatalen)		/* INOUT */
{
	if (Local(costate) != NET_COSTATE_DISCONNECTED) {
		*udatalen = 0;
		return(NET_IS_CONNECTED);
	}

	*udatalen = 0;
	return(ERR_SUCCESS);
}


xkern_return_t usudp_cots::usx_pop_internal(XObj		s,
				 Msg			*xmsg)
{
	IPPaddr			ipp_addr;
	xkern_return_t		xret;
	io_record_t		rec;
	unsigned int		count = 1;
	io_recnum_t		recnum;
	mach_error_t		ret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	if (s != Local(udp_session)) {
		us_internal_error("usx_pop_internal: session mismatch",
							US_INTERNAL_ERROR);
//		msgDestroy(xmsg);
		return(XK_FAILURE);
	}

	ret = buf_mgr->usx_convert_xmsg_to_record(xmsg,&rec);
	if (ret != ERR_SUCCESS) {
//		msgDestroy(xmsg);
		return(XK_FAILURE);
	}

	ret = Local(incoming_stream)->io_putrec_seq(0,&rec,&count,&recnum);
	if (ret != ERR_SUCCESS) {
		(void) Local(buf_mgr)->io_free_record(rec);
		return(ret);
	}

	return(XK_SUCCESS);
}

xkern_return_t
usudp_cots::usx_opendone_internal(XObj llp, XObj lls, XObj llm)
{
  return XK_FAILURE;
}

xkern_return_t
usudp_cots::usx_closedone_internal(XObj llp)
{
  return XK_FAILURE;
}
