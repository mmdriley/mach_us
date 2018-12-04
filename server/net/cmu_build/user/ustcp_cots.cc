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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/ustcp_cots.cc,v $
 *
 * Author: J. Mark Stevenson
 *
 * Purpose: xkernel TCP endpoint for connection-oriented operation.
 *
 * HISTORY:
 * $Log:	ustcp_cots.cc,v $
 * Revision 2.4  94/07/13  18:06:18  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:38  jms
 * 	Need dummy implementations of virtual methods in class ustcp_cots
 * 		for 2.3.3 g++ -modh
 * 	[94/04/28  19:09:50  jms]
 * 
 * Revision 2.2  94/01/11  18:09:47  jms
 * 	Initial Version
 * 	[94/01/10  11:44:36  jms]
 * 
 */

#include	<ustcp_cots_ifc.h>
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
#include	"tcp.h"
}

boolean_t usx_debug_counts = TRUE;

/* This probably does not belong here, but this file is where it is used exclusively */
#define TCP_BUFFER_SPACE (4 * 1024)

/*
 * Routine for building new cots endpoints.
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

usx_new_cots_fun_t ustcp_new_cots = new_cots;

DEFINE_CLASS_MI(ustcp_cots)
DEFINE_CASTDOWN2(ustcp_cots, usx_endpt_base, usNetCOTS)

void ustcp_cots::init_class(usClass* class_obj)
{
	usx_endpt_base::init_class(class_obj);

	BEGIN_SETUP_METHOD_WITH_ARGS(ustcp_cots);
	SETUP_METHOD_WITH_ARGS(ustcp_cots,net_get_localaddr);
	SETUP_METHOD_WITH_ARGS(ustcp_cots,io_read_seq);
	SETUP_METHOD_WITH_ARGS(ustcp_cots,io_write_seq);
	SETUP_METHOD_WITH_ARGS(ustcp_cots,ns_get_attributes);
	SETUP_METHOD_WITH_ARGS(ustcp_cots,net_snddis);
	SETUP_METHOD_WITH_ARGS(ustcp_cots,net_rcvdis);
	END_SETUP_METHOD_WITH_ARGS;
}


/*
 * Initial (default) buffer size.
 */
int	ustcp_cots_bufsize = 10000;

ustcp_cots::ustcp_cots(ns_mgr_id_t	mgr_id, 
		       access_table	*_acctab,
		       vol_agency	*_connector,
		       usx_iobuf_mgr	*_buf_mgr,
		       usx_dir	*_protocol_dir,
		       net_addr_t	*_localaddr,
		       net_addr_t	*_peeraddr,
		       XObj		sess,
		       mach_error_t	*ret)

:
 usx_endpt_base(mgr_id, _acctab),
 connector(_connector), protocol_dir(_protocol_dir), buf_mgr(_buf_mgr),
 costate(NET_COSTATE_CONNECTED),
 tcp_session(sess)
{
	IPPaddr			ipp_local, ipp_remote;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	*ret = ERR_SUCCESS;
	DEBUG1(usx_debug, (0, "ustcp_cots::ustcp_cots: sessn 0x%x\n", Local(tcp_session)));

	(void) _connector->ns_register_stronglink();
	mach_object_reference(_protocol_dir);
	mach_object_reference(_buf_mgr);

	/*
	 * Cannot let the network code wait on the incoming stream.
	 *
	 * XXX Need a better strategy to drop old packets instead
	 * of new ones.
	 */
	incoming_stream = new bytestream(_buf_mgr, ustcp_cots_bufsize,
				IOS_ENABLED|IOS_WAIT_ALLOWED,
				IOS_ENABLED|IOS_WAIT_ALLOWED);

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

	if (NULL == Local(tcp_session)) {
		*ret = protocol_dir->usx_open_internal(this,TRUE,
					participants,&Local(tcp_session));
	}
	INT_TO_IO_OFF(0,&write_offset);
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

	*endpt = new ustcp_cots(mgr_id, _acctab, _connector, _buf_mgr,
			_protocol_dir, _localaddr, _peeraddr, sess, &ret);
	*endpt_type = NST_COTS_BYTES;
	return(ret);
}

ustcp_cots::ustcp_cots()
:
 connector(0), protocol_dir(0), buf_mgr(0), incoming_stream(0),
 tcp_session(ERR_XOBJ)
{}

ustcp_cots::~ustcp_cots()
{
	DEBUG1(usx_debug, (0, "ustcp_cots::~ustcp_cots: sessn 0x%x\n", Local(tcp_session)));

	if (connector) {
		(void) connector->ns_unregister_stronglink();
	}
	mach_object_dereference(Local(protocol_dir));
	mach_object_dereference(Local(buf_mgr));
	mach_object_dereference(Local(incoming_stream));

	/* Does this deallocation belong here? Do we have an order problem with closedone? */
	if ((Local(tcp_session) != ERR_XOBJ) &&
	    (Local(tcp_session) != NULL)) {
#if DESTROY_TCP_SESS_IN_COTS_DESTRUCTOR
		xDestroy(Local(tcp_session));
#endif DESTROY_TCP_SESS_IN_COTS_DESTRUCTOR
		Local(tcp_session) = ERR_XOBJ;
	}
}

char* ustcp_cots::remote_class_name() const
{
	return "usNetCOTS_bytes_proxy";
}

mach_error_t ustcp_cots::ns_tmp_cleanup_for_shutdown()
{
	mach_error_t		ret;

	if (Local(costate) == NET_COSTATE_CONNECTED) {
		(void) net_snddis(NULL,0);
	}

	ret = tmp_agency::ns_tmp_cleanup_for_shutdown();

	return(ret);
}


mach_error_t ustcp_cots::net_get_localaddr(net_addr_t		*addr)	/* out */
{
	IPPaddr			ipp_addr;
	xkern_return_t		xret;

	XKERNEL_MASTER();
	xret = usx_get_IPPaddr(Local(tcp_session), LOCAL_PART, &ipp_addr);
	XKERNEL_RELEASE();
	if (xret == XK_FAILURE) {
		us_internal_error("net_get_localaddr.usx_get_IPPaddr",
					convert_xkernel_error(x_errno));
 		return(US_INTERNAL_ERROR);
 	}

	net_addr_inet_init(addr,*(ipaddr_t*)&ipp_addr.host,htons((unsigned short)ipp_addr.port));

	return(ERR_SUCCESS);
 }


mach_error_t ustcp_cots::net_get_peeraddr(net_addr_t		*addr)	/* out */
{
	IPPaddr			ipp_addr;
	xkern_return_t		xret;

	XKERNEL_MASTER();
	xret = usx_get_IPPaddr(Local(tcp_session), REMOTE_PART, &ipp_addr);
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
ustcp_cots::io_read_seq(io_mode_t mode, char *buf, io_count_t *count,
				io_offset_t *offset)
{
	mach_error_t		ret;

	DEBUG1(usx_debug, (0, "ustcp_cots::io_read_seq:mode=0x%x, this 0x%x\n", mode, this));

	ret = Local(incoming_stream)->io_read_seq(mode, buf, count, offset);
	DEBUG1(usx_debug, (0, "ustcp_cots::io_read_seq:count=%d\n",*count));
	return(ret);
}

mach_error_t 
ustcp_cots::io_write_seq(io_mode_t mode, char *buf, io_count_t *count,
				 io_offset_t *offset)
{
	mach_error_t		ret;
	io_block_t		blk;

	DEBUG1(usx_debug, (0, "ustcp_cots::io_write_seq: count=%d, this = 0x%x\n",*count, this));

	if ((mode & IOM_PROBE) == 0) {
		/*
		 * XXX Should construct an x-kernel message directly out of the
		 * user memory when possible, with assistance from the RPC
		 * system (i.e. just keep IPC receive buffers around...).
		 */


		ret = Local(buf_mgr)->io_alloc_block(*count,&blk);
		if (ret != ERR_SUCCESS) {
			return(ret);
		}

		bcopy(buf,ioblk_start(blk),*count);
		blk->end_offset += *count;

	} else {
		blk = NULL;
	}

	ret = io_putbytes_seq(mode,&blk,count,offset);

	if (blk) {
		(void) Local(buf_mgr)->io_free_block(blk);
	}
	return(ret);
}


mach_error_t 
ustcp_cots::io_getbytes_seq(io_mode_t mode, io_block_t *blk,
				    io_count_t *count, io_offset_t *offset)
{
	mach_error_t		ret;

	DEBUG1(usx_debug, (0, "ustcp_cots::io_getbytes_seq: sessn 0x%x\n", Local(tcp_session)));

	ret = Local(incoming_stream)->io_getbytes_seq(mode,blk,count,offset);

	DEBUG1(usx_debug, (0, "ustcp_cots::io_getbytes_seq: count=%d, this 0x%x\n", *count, this));
	return(ret);
}


mach_error_t 
ustcp_cots::io_putbytes_seq(io_mode_t mode, io_block_t *blk,
				    io_count_t *count, io_offset_t *offset)
{
	Msg			xmsg;
	IPPaddr			ipp_addr;
	mach_error_t		ret;
	xkern_return_t		xret;

	DEBUG1(usx_debug, (0, "ustcp_cots::io_putbytes_seq:count=%d, this 0x%x\n",*count, this));

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
		return(ERR_SUCCESS);
	}

	/*
	 * Prepare x-kernel message.
	 */
	ret = Local(buf_mgr)->usx_convert_block_to_xmsg(*blk,&xmsg);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	XKERNEL_MASTER();

	/*
	 * Send the message.
	 */
	xret = xPush(Local(tcp_session),&xmsg);
	if (xret == XK_FAILURE) {
		us_internal_error("x_push",convert_xkernel_error(x_errno));
		XKERNEL_RELEASE();
		msgDestroy(&xmsg);
		return(US_INTERNAL_ERROR);
	}
	XKERNEL_RELEASE();

	msgDestroy(&xmsg);

	*offset = Local(write_offset);
	ADD_LONG_TO_DLONG(&Local(write_offset),*count);
	return(ERR_SUCCESS);
}


mach_error_t 
ustcp_cots::ns_get_attributes(ns_attr_t		attr,
			      int		*attrlen)
{
	mach_error_t		ret;

	DEBUG1(usx_debug, (0, "ustcp_cots::ns_get_attributes: sessn 0x%x\n", Local(tcp_session)));

//	ret = invoke_super(Super,mach_method_id(ns_get_attributes),attr,attrlen);
	ret = tmp_agency::ns_get_attributes(attr,attrlen);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	attr->type = NST_COTS_BYTES;

	/*
	 * XXX Should return the number of available records.
	 */

	return(ERR_SUCCESS);
}


mach_error_t 
ustcp_cots::net_snddis(char			*udata,
		       unsigned int		udatalen)
{
	XKERNEL_MASTER();

	DEBUG1(usx_debug, (0, "ustcp_cots::net_snddis: sessn 0x%x\n", Local(tcp_session)));


	if (Local(costate) != NET_COSTATE_CONNECTED) {
		XKERNEL_RELEASE();
		return(NET_NOT_CONNECTED);
	}

	Local(costate) = NET_COSTATE_DISCONNECTED;

	/*
	 * Make sure no one can read/write data here; wakeup
	 * threads blocked in pending I/O operations.
	 */
	(void) Local(incoming_stream)->io_set_read_strategy(0);
	(void) Local(incoming_stream)->io_set_write_strategy(0);
	(void) Local(incoming_stream)->io_flush_stream();

	if (Local(tcp_session) != ERR_XOBJ) {
		(void) Local(protocol_dir)->usx_close_internal(this,
						FALSE,Local(tcp_session));
		Local(tcp_session) = ERR_XOBJ;
	}

	XKERNEL_RELEASE();

	return(ERR_SUCCESS);
}


mach_error_t 
ustcp_cots::net_rcvdis(char		*udata,			/* OUT */
		       unsigned int	*udatalen)		/* INOUT */
{
	DEBUG1(usx_debug, (0, "ustcp_cots::net_rcvdis\n: sessn 0x%x\n", Local(tcp_session)));

	if (Local(costate) != NET_COSTATE_DISCONNECTED) {
		*udatalen = 0;
		return(NET_IS_CONNECTED);
	}

	*udatalen = 0;
	return(ERR_SUCCESS);
}


xkern_return_t ustcp_cots::usx_pop_internal(XObj		s,
				 Msg			*xmsg)
{
	IPPaddr			ipp_addr;
	xkern_return_t		xret;
	io_block_t		blk_lst;
	io_block_t		blk, next_blk;
	unsigned int		count = 1;
	io_offset_t		offset;
	mach_error_t		ret;
	ushort			space;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	DEBUG1(usx_debug, (0, "ustcp_cots::usx_pop_internal: sess 0x%x\n", s));
	if (s != Local(tcp_session)) {
		us_internal_error("usx_pop_internal: session mismatch",
							US_INTERNAL_ERROR);
//		msgDestroy(xmsg);
		return(XK_FAILURE);
	}

	ret = buf_mgr->usx_convert_xmsg_to_block_lst(xmsg,&blk_lst);
	if (ret != ERR_SUCCESS) {
//		msgDestroy(xmsg);
		return(XK_FAILURE);
	}

	blk = blk_lst;
	while (blk != NULL) {
		count = ioblk_cursize(blk);
		next_blk = blk->next;
		DEBUG0(usx_debug_counts, (0, "ustcp_cots::usx_pop_internal: count %d, next 0x%x, sess 0x%x\n", count, next_blk, s));
		ret = Local(incoming_stream)->io_putbytes_seq(
				IOM_WAIT, &blk, &count, &offset);

		if (ret != ERR_SUCCESS) {
			(void) Local(buf_mgr)->io_free_block(blk);
			return(ret);
		}
		blk = next_blk;
	}

	/*
	 * Inform TCP of available buffer space:
	 * This is where "flow control" happens.
	 */
	/* u_short space = streamGetSpace(&sd->so_rcv); */
	space = TCP_BUFFER_SPACE;	/* XXX value come from stream land ? */
	xret = xControl(s, TCP_SETRCVBUFSPACE,
		 (char*)&space, sizeof(space));
	if (xret == XK_FAILURE) {
		us_internal_error("ustcp_cots::usx_pop_internal.xControl",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	DEBUG1(usx_debug, (0, "ustcp_cots::usx_pop_internal: count %d, sess 0x%x\n", count, s));
	return(XK_SUCCESS);
}

xkern_return_t
ustcp_cots::usx_opendone_internal(XObj lls, XObj llp, XObj llm)
{
  return XK_FAILURE;
}

xkern_return_t
ustcp_cots::usx_closedone_internal(XObj sess)
{
	xkern_return_t		xret;

	DEBUG1(usx_debug, (0, "ustcp_cots::usx_closedone_internal: sess 0x%x\n", sess));

	if (Local(costate) != NET_COSTATE_CONNECTED) {
		return(XK_SUCCESS);
	}

	Local(costate) = NET_COSTATE_DISCONNECTED;

	/*
	 * Disable writing from the other side, just in case.
	 * This should not be necessary.
	 */
	(void) Local(incoming_stream)->io_set_write_strategy(0);

	/*
	 * Let the client drain the data in the buffer,
	 * but do not let him/her wait for more (~IOS_WAIT_ALLOWED).
	 *
	 * XXX This may be UNIX-specific.
	 *
	 * XXX Should notify the client with a signal/callback...
	 */
	(void) Local(incoming_stream)->io_set_read_strategy(IOS_ENABLED);

	/*
	 * Release references? XXX
	 */
	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	/*
	 * First close (release one reference) the session.
	 *
	 * We must do this even if the entry is not found in the
	 * demux map, to match the reference acquired earlier from
	 * usx_open_internal().
	 */
	xret = xClose(sess);
	if (xret == XK_FAILURE) {
		us_internal_error("ustcp_cots::usx_closedone_internal.xClose",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	return(XK_SUCCESS);
}

mach_error_t 
ustcp_cots::io_read(io_mode_t, io_offset_t, pointer_t, 
				     unsigned int*)
{
	DEBUG1(usx_debug, (0, "ustcp_cots::io_read: sessn 0x%x\n", Local(tcp_session)));

	return _notdef();
}
mach_error_t 
ustcp_cots::io_write(io_mode_t, io_offset_t, pointer_t, unsigned int*)
{
	DEBUG1(usx_debug, (0, "ustcp_cots::io_write: sessn 0x%x\n", Local(tcp_session)));

	return _notdef();
}
mach_error_t 
ustcp_cots::io_append(io_mode_t, pointer_t, unsigned int*)
{
	DEBUG1(usx_debug, (0, "ustcp_cots::io_append: sessn 0x%x\n", Local(tcp_session)));

	return _notdef();
}
mach_error_t 
ustcp_cots::io_set_size(io_size_t)
{
	DEBUG1(usx_debug, (0, "ustcp_cots::io_set_size: sessn 0x%x\n", Local(tcp_session)));

	return _notdef();
}
mach_error_t 
ustcp_cots::io_get_size(io_size_t *)
{
	DEBUG1(usx_debug, (0, "ustcp_cots::io_get_size: sessn 0x%x\n", Local(tcp_session)));

	return _notdef();
}
mach_error_t 
ustcp_cots::io_map(task_t, vm_address_t*, vm_size_t,
			   vm_offset_t, boolean_t, vm_offset_t,
			   boolean_t, vm_prot_t, vm_prot_t,
			   vm_inherit_t)
{
	DEBUG1(usx_debug, (0, "ustcp_cots::io_map: sessn 0x%x\n", Local(tcp_session)));

	return _notdef();
}
       

