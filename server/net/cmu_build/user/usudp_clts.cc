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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usudp_clts.cc,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: xkernel UDP endpoint for connection-less operation.
 *
 * HISTORY:
 * $Log:	usudp_clts.cc,v $
 * Revision 2.4  94/07/13  18:06:34  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:47  jms
 * 	Need dummy implementations of virtual methods in class usudb_clts
 * 		for 2.3.3 g++ -modh
 * 	[94/04/29  13:26:21  jms]
 * 
 * Revision 2.2  94/01/11  18:10:01  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  11:51:17  jms]
 * 
 * Revision 2.3  92/07/05  23:33:41  dpj
 * 	tmp_cleanup_for_shutdown -> ns_tmp_cleanup_for_shutdown
 * 	[92/06/24  17:09:32  jms]
 * 	Converted to new C++ RPC package.
 * 	[92/05/10  01:27:59  dpj]
 * 
 * Revision 2.2  91/11/06  14:14:27  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:08:29  pjg]
 * 
 * Revision 2.2  91/05/05  19:30:48  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:14  dpj]
 * 
 * 	First really working version.
 * 	[91/04/28  10:48:20  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:47:53  dpj]
 * 
 */

/*
 * XXX Instead of using a recordstream object to queue incoming messages
 * in standard record format, this code might queue native x-kernel
 * messages, and move them to records only when necessary.
 */

#include	<usudp_clts_ifc.h>
#include	<iobuf_user_ifc.h>
#include	<agent_ifc.h>

extern "C" {
#include	<base.h>
#include	<macro_help.h>

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
static mach_error_t new_clts(ns_mgr_id_t mgr_id, access_table *_acctab,
			usx_iobuf_mgr *_buf_mgr, usx_dir *_protocol_dir,
			net_addr_t *_localaddr, 
			agency **endpt, ns_type_t *endpt_type);

usx_new_clts_fun_t usudp_new_clts = new_clts;

/*
 * Initial (default) buffer size.
 */
int	usudp_clts_bufsize = 100;


/*
 * Manipulation of the session pool.
 */

/*
 * Advance index in the session pool array.
 */
#define	SESSPOOL_NEXT(_i)	((++(_i) == SESSION_POOL_SIZE) ? 0 : (_i))

/*
 * Clean-up an entry in the session pool.
 */
#define	SESSPOOL_CLEAN(_i)						\
    MACRO_BEGIN								\
	if (Local(session_pool)[(_i)].s != ERR_XOBJ) {			\
		(void) Local(protocol_dir)->usx_close_internal(this,	\
				FALSE,Local(session_pool)[(_i)].s);	\
		Local(session_pool)[(_i)].s = ERR_XOBJ;		\
	}								\
	net_addr_destroy(&Local(session_pool)[(_i)].peeraddr);		\
    MACRO_END

/*
 * Allocate a new entry in the session pool, by recycling an old one.
 */
#define	SESSPOOL_FIND_EMPTY(_ip)					\
    MACRO_BEGIN								\
	if (Local(reuse_session) == Local(lru_session))			\
		Local(reuse_session) =					\
				SESSPOOL_NEXT(Local(reuse_session));	\
	Local(reuse_session) = SESSPOOL_NEXT(Local(reuse_session));	\
	*(_ip) = Local(reuse_session);					\
	SESSPOOL_CLEAN(*(_ip));						\
	Local(lru_session) = *(_ip);					\
    MACRO_END

/*
 * Find an entry in the session pool with the specified peer address.
 */
#define	SESSPOOL_FIND_ADDR(_addr,_ip)					\
    MACRO_BEGIN								\
	boolean_t		found = FALSE;				\
	*(_ip) = Local(lru_session);					\
	do {								\
		if ((Local(session_pool)[*(_ip)].s != ERR_XOBJ) &&	\
				net_addr_equal((_addr),			\
				&Local(session_pool)[*(_ip)].peeraddr)) {\
			found = TRUE;					\
			break;						\
		}							\
		*(_ip) = SESSPOOL_NEXT(*(_ip));				\
	} while (*(_ip) != Local(lru_session));				\
	if (found)							\
		Local(lru_session) = *(_ip);				\
	else								\
		*(_ip) = -1;						\
    MACRO_END

/*
 * Find an entry in the session pool with the specified session.
 */
#define	SESSPOOL_FIND_SESSION(_s,_ip)					\
    MACRO_BEGIN								\
	boolean_t		found = FALSE;				\
	*(_ip) = Local(lru_session);					\
	do {								\
		if ((_s) == Local(session_pool)[*(_ip)].s) {		\
			found = TRUE;					\
			break;						\
		}							\
		*(_ip) = SESSPOOL_NEXT(*(_ip));				\
	} while (*(_ip) != Local(lru_session));				\
	if (found)							\
		Local(lru_session) = *(_ip);				\
	else								\
		*(_ip) = -1;						\
    MACRO_END


DEFINE_CLASS_MI(usudp_clts)
DEFINE_CASTDOWN2(usudp_clts, usx_endpt_base, usNetCLTS)

void usudp_clts::init_class(usClass* class_obj)
{
	usNetCLTS::init_class(class_obj);
	net_endpt_base::init_class(class_obj);

	BEGIN_SETUP_METHOD_WITH_ARGS(usudp_clts);
	SETUP_METHOD_WITH_ARGS(usudp_clts,net_get_localaddr);
	SETUP_METHOD_WITH_ARGS(usudp_clts,net_connect);
	SETUP_METHOD_WITH_ARGS(usudp_clts,net_clts_read1rec);
	SETUP_METHOD_WITH_ARGS(usudp_clts,net_clts_write1rec);
	SETUP_METHOD_WITH_ARGS(usudp_clts,ns_get_attributes);
	END_SETUP_METHOD_WITH_ARGS;
}

usudp_clts::usudp_clts(ns_mgr_id_t mgr_id, access_table *_acctab,
		       usx_iobuf_mgr *_buf_mgr, usx_dir *_protocol_dir,
		       net_addr_t *_localaddr, mach_error_t* ret)
:
 usx_endpt_base(mgr_id, _acctab),
 protocol_dir(_protocol_dir), buf_mgr(_buf_mgr),
 lru_session(SESSION_POOL_SIZE - 1), reuse_session(0),
 readers_count(0)
{
	int			i;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	*ret = ERR_SUCCESS;
	mach_object_reference(_protocol_dir);
	mach_object_reference(_buf_mgr);

	for (i = 0; i < SESSION_POOL_SIZE; i++) {
		Local(session_pool)[i].s = ERR_XOBJ;
		net_addr_inet_init_default(&Local(session_pool)[i].peeraddr);
	}

	INT_TO_IO_RECNUM(0,&Local(outgoing_recnum));

	net_addr_null_init(&Local(localaddr));
	net_addr_copy(_localaddr,&Local(localaddr));

	USX_CVT_NETADDR(_localaddr,&Local(ipp_local));
	Local(ipp_remote).host = (IPhost){0xff,0xff,0xff,0xff};	/* ANY_HOST??? */
	Local(ipp_remote).port = (unsigned int)ANY_PORT;

	partInit(participants, 2);
	partPush(participants[LOCAL_PART], &(ipp_local.host),
			sizeof(ipp_local.host));
	partPush(participants[LOCAL_PART], &(ipp_local.port),
			sizeof(ipp_local.port));
	partPush(participants[REMOTE_PART], &(ipp_remote.host), 0); /*ANYHOST???*/
	partPush(participants[REMOTE_PART], &(ipp_remote.port), 0); /*ANYPORT???*/

//	add_delegate(Self,iobuf_user);
//	ret = setup_iobuf_user(Self,buf_mgr);

	/*
	 * Cannot let the network code wait on the incoming stream.
	 *
	 * XXX Need a better strategy to drop old packets instead
	 * of new ones.
	 */
//	new_object(Local(incoming_stream),recordstream);
//	ret = setup_recordstream(Local(incoming_stream),
//				Local(usx_iobuf_mgr),usudp_clts_bufsize,
//				IOS_ENABLED|IOS_WAIT_ALLOWED,IOS_ENABLED);
	incoming_stream = new recordstream(buf_mgr, usudp_clts_bufsize,
				IOS_ENABLED|IOS_WAIT_ALLOWED,IOS_ENABLED);
	if (*ret != ERR_SUCCESS) {
		return;
	}

	net_options_null_init(&Local(null_options));
}

static mach_error_t new_clts(ns_mgr_id_t mgr_id, access_table *_acctab,
			usx_iobuf_mgr *_buf_mgr, usx_dir *_protocol_dir,
			net_addr_t *_localaddr, 
			agency **endpt, ns_type_t *endpt_type)
{
	mach_error_t	ret;

	*endpt = new usudp_clts(mgr_id, _acctab, _buf_mgr, _protocol_dir,
			_localaddr, &ret);
	*endpt_type = NST_CLTS_RECS;
	return(ret);
}

usudp_clts::usudp_clts()
:
 protocol_dir(0), buf_mgr(0), incoming_stream(0)
{
	net_addr_null_init(&Local(localaddr));
	net_options_null_init(&Local(null_options));
}

usudp_clts::~usudp_clts()
{
	mach_object_dereference(Local(protocol_dir));
	mach_object_dereference(Local(buf_mgr));
	mach_object_dereference(Local(incoming_stream));

	net_addr_destroy(&Local(localaddr));
	net_options_destroy(&Local(null_options));
}


char* usudp_clts::remote_class_name() const
{
	return "usNetCLTS_proxy";
}

mach_error_t usudp_clts::ns_tmp_cleanup_for_shutdown()
{
	mach_error_t		ret;

	/*
	 * Everything should already have been taken care of
	 * when the last agent was destroyed.
	 */

	ret = tmp_agency::ns_tmp_cleanup_for_shutdown();
	return(ret);
}

mach_error_t usudp_clts::ns_register_agent(ns_access_t access)
{
	mach_error_t		ret;

	if (access & NSR_READ) {
		XKERNEL_MASTER();

		Local(readers_count)++;

		if (Local(readers_count) == 1) {
			ret = protocol_dir->usx_openenable_internal(this,
							&(Local(ipp_local)),
							Local(participants));
			if (ret != ERR_SUCCESS) {
				XKERNEL_RELEASE();
				return(ret);
			}

			/*
			 * XXX Need an openenable for broadcast packets?
			 */
		}

		XKERNEL_RELEASE();
	}

//	ret = invoke_super_with_base(Super,Base,
//				mach_method_id(ns_register_agent),access);
	ret = tmp_agency::ns_register_agent(access);

	return(ret);
}


mach_error_t usudp_clts::ns_unregister_agent(ns_access_t access)
{
	mach_error_t		ret;
	int			i;

//	ret = invoke_super_with_base(Super,Base,
//				mach_method_id(ns_unregister_agent),access);
	ret = tmp_agency::ns_unregister_agent(access);

	if (access & NSR_READ) {
		XKERNEL_MASTER();

		Local(readers_count)--;

		if (Local(readers_count) == 0) {
			for (i = 0; i < SESSION_POOL_SIZE; i++) {
				SESSPOOL_CLEAN(i);
			}

//			Local(participants)[1].address = NULL;
//			Local(participants)[1].length = 0;
			(void) protocol_dir->usx_opendisable_internal(
					this, &(Local(ipp_local)),
					Local(participants));

			(void) incoming_stream->io_flush_stream();
		}

		XKERNEL_RELEASE();
	}

	return(ret);
}


/*
 * Exported client interface.
 */

mach_error_t usudp_clts::net_get_localaddr(net_addr_t *addr)
{
	net_addr_copy(&Local(localaddr),addr);

	return(ERR_SUCCESS);
}


mach_error_t usudp_clts::net_connect(net_addr_t		*peeraddr,
				     net_options_t	*options,/* inout */
				     char		*in_udata,
				     unsigned int	in_udatalen,
				     char		*out_udata,/* out */
				     unsigned int	*out_udatalen,/* inout */
				     ns_prot_t		prot,
				     unsigned int	protlen,
				     ns_access_t	access,
				     usItem		**newobj,/* out */
				     ns_type_t		*newtype)/* out */
{
	mach_error_t	ret;
	agency		*endpt = 0;
	std_cred	*cred = 0;

#define	ABORT(_ret) {							\
	mach_object_dereference(endpt);					\
	mach_object_dereference(cred);					\
	mach_object_dereference(*newobj);				\
	*newobj = NULL;					\
	*newtype = NST_INVALID;						\
	return(_ret);							\
}

	*newobj = NULL;
	*out_udatalen = 0;

	/*
	 * No use doing anything if not returning an agent, since
	 * the endpoint would just disappear at once.
	 */
	if (access == 0) {
		*newtype = NST_INVALID;
		return(US_UNSUPPORTED);
	}

	/*
	 * Call on the "protocol" directory to initiate the connection.
	 */
	XKERNEL_MASTER();
	ret = protocol_dir->usx_get_connection_endpt(this,
						   &Local(localaddr),peeraddr,
						   prot,protlen, NULL, &endpt);
	XKERNEL_RELEASE();

	if (ret != ERR_SUCCESS) {
		ABORT(ret);
	}

	/*
	 * Setup the OUT arguments.
	 */
	ret = agent::base_object()->ns_get_cred_obj(&cred);
	if (ret != ERR_SUCCESS) {
		us_internal_error(
				"net_accept: cannot get current credentials",
				ret);
		ABORT(US_INTERNAL_ERROR);
	}
	agent *agent_obj;
	ret = endpt->ns_create_agent(access,cred,&agent_obj);
	if (ret != ERR_SUCCESS) ABORT(ret);
	*newtype = NST_COTS_RECS;
	*newobj = agent_obj;

	mach_object_dereference(endpt);
	mach_object_dereference(cred);

	return(ERR_SUCCESS);

#undef	ABORT
}

mach_error_t 
usudp_clts::net_listen(io_mode_t mode, 
		       net_addr_t* peeraddr,
		       net_options_t* options, 
		       char* buf,
		       unsigned int* len, 
		       int* num)
{
  return MACH_OBJECT_NO_SUCH_OPERATION;
}

mach_error_t 
usudp_clts::net_accept(int num, 
		       net_options_t* options, 
		       char* buf,
		       unsigned int len, 
		       ns_prot_t prot,
		       unsigned int len2, 
		       ns_access_t access,
		       usItem** itm, 
		       ns_type_t* typt)
{
  return MACH_OBJECT_NO_SUCH_OPERATION;
}

mach_error_t 
usudp_clts::net_reject(int num, char* buf, unsigned int len)
{
  return MACH_OBJECT_NO_SUCH_OPERATION;
}
  
mach_error_t 
usudp_clts::net_get_connect_qinfo(int* num, int* len)
{
  return MACH_OBJECT_NO_SUCH_OPERATION;
}

mach_error_t 
usudp_clts::net_set_connect_qmax(int num)
{
  return MACH_OBJECT_NO_SUCH_OPERATION;
}


mach_error_t 
usudp_clts::net_clts_read1rec(io_mode_t		mode,
			      char		*buf,
			      unsigned int	*len,	/* INOUT */
			      io_recnum_t	*recnum,/* OUT */
			      net_addr_t	*peeraddr,/* OUT */
			      net_options_t	*options)/* OUT */
{
	mach_error_t		ret;
	usx_recinfo_t		recinfo;

	io_recinfo_init_default(&recinfo);

	ret = incoming_stream->io_read1rec_seq_with_info(mode,buf,len,
						      recnum,
						      (io_recinfo_t*)&recinfo);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	if ((mode & IOM_PROBE) == 0) {
		net_addr_copy(&recinfo.addr,peeraddr);
		net_options_copy(&recinfo.options,options);
	}

	io_recinfo_destroy(&recinfo);

	return(ERR_SUCCESS);
}


mach_error_t 
usudp_clts::net_clts_write1rec(io_mode_t		mode,
			       char			*buf,
			       unsigned int		len,
			       io_recnum_t		*recnum,/* OUT */
			       net_addr_t		*peeraddr,
			       net_options_t		*options)
{
	mach_error_t		ret;
	io_record_t		rec;

	if ((mode & IOM_PROBE) == 0) {
		/*
		 * XXX Should construct an x-kernel message directly out of the
		 * user memory when possible, with assistance from the RPC
		 * system (i.e. just keep IPC receive buffers around...).
		 */

		ret = buf_mgr->io_alloc_record(len,&rec);
		if (ret != ERR_SUCCESS) {
			return(ret);
		}

		bcopy(buf,ioblk_start(rec->first_block),len);
		rec->first_block->end_offset += len;
	} else {
		rec = NULL;
	}

	ret = net_clts_put1rec(mode,rec,recnum,peeraddr,options);

	return(ret);
}


mach_error_t 
usudp_clts::net_clts_get1rec(io_mode_t		mode,
			     io_record_t	*data,		/* inout */
			     io_recnum_t	*recnum,	/* out */
			     net_addr_t		*peeraddr,	/* out */
			     net_options_t	*options)	/* out */
{
	mach_error_t		ret;
	io_record_t		rec = NULL;
	io_count_t		count = 1;
	usx_recinfo_t		*recinfo;

	ret = incoming_stream->io_getrec_seq(mode,&rec,&count,recnum);
 	if (ret != ERR_SUCCESS) {
 		return(ret);
 	}

	recinfo = (usx_recinfo_t *) &rec->recinfo;

	if (mode & IOM_PROBE) {
 		/*
 		 * XXX Should there be a way to find-out how big the record
 		 * is, and whether it would fit in a user-specified buffer?
  		 */
		return(ERR_SUCCESS);
 	}

	/*
 	 * XXX Should there be a way to get the control info
	 * without also getting the data?
	 *
	 * No need to destroy the recinfo now; it will be taken care
	 * of when the whole record is freed.
  	 */
	net_addr_copy(&recinfo->addr,peeraddr);
	net_options_copy(&recinfo->options,options);

	*data = rec;

	return(ERR_SUCCESS);
}


mach_error_t 
usudp_clts::net_clts_put1rec(io_mode_t		mode,
			     io_record_t	data,
			     io_recnum_t	*recnum,	/* out */
			     net_addr_t		*peeraddr,
			     net_options_t	*options)
{
	Msg			xmsg;
	IPPaddr			ipp_addr;
	mach_error_t		ret;
	xkern_return_t		xret;
	int			i;

	if (mode & ~(IOM_WAIT | IOM_PROBE | IOM_TRUNCATE)) {
		return(IO_INVALID_MODE);
	}

#ifdef	NOTDEF
	if (! (Local(write_strategy) & IOS_ENABLED)) {
		return(IO_REJECTED);
	}
#endif	NOTDEF

	if (mode & IOM_PROBE) {
		XKERNEL_MASTER();
		*recnum = Local(outgoing_recnum);
		XKERNEL_RELEASE();
		return(ERR_SUCCESS);
	}

	/*
	 * Prepare x-kernel message.
	 */
	ret = buf_mgr->usx_convert_record_to_xmsg(data,&xmsg);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	XKERNEL_MASTER();

	/*
	 * Look for existing session.
	 */
	SESSPOOL_FIND_ADDR(peeraddr,&i);
	if (i == XK_FAILURE) {
		/*
		 * Open a new session.
		 *
		 * XXX Should we check to prohibit sending to privileged ports?
		 */
		XObj			s;

		if (!net_addr_inet_p(peeraddr)) {
			XKERNEL_RELEASE();
			msgDestroy(&xmsg);
			return(NET_INVALID_ADDR_FLAVOR);
		}
		USX_CVT_NETADDR(peeraddr,&ipp_addr);

		partInit(participants, 2);
		partPush(participants[LOCAL_PART], &(ipp_local.host),
				sizeof(ipp_local.host));
		partPush(participants[LOCAL_PART], &(ipp_local.port),
			sizeof(ipp_local.port));
		partPush(participants[REMOTE_PART], &(ipp_addr.host), 
				sizeof(ipp_addr.host));
		partPush(participants[REMOTE_PART], &(ipp_addr.port),
				sizeof(ipp_addr.port));
		ret = protocol_dir->usx_open_internal(this,FALSE,
						      Local(participants),&s);

		if (ret != ERR_SUCCESS) {
			XKERNEL_RELEASE();
			msgDestroy(&xmsg);
			return(ret);
		}

		SESSPOOL_FIND_EMPTY(&i);
		Local(session_pool)[i].s = s;
		net_addr_destroy(&Local(session_pool)[i].peeraddr);
		net_addr_copy(peeraddr,&Local(session_pool)[i].peeraddr);
	}

	/*
	 * Send the message.
	 */
	xret = xPush(Local(session_pool)[i].s,&xmsg);
	if (xret == XK_FAILURE) {
		us_internal_error("xPush",convert_xkernel_error(x_errno));
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
usudp_clts::ns_get_attributes(ns_attr_t attr, int *attrlen)
{
	mach_error_t		ret;

//	ret = invoke_super(Super,mach_method_id(ns_get_attributes),
//							attr,attrlen);
	ret = tmp_agency::ns_get_attributes(attr,attrlen);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	attr->type = NST_CLTS_RECS;

	/*
	 * XXX Should return the number of available records.
	 */

	return(ERR_SUCCESS);
}


xkern_return_t usudp_clts::usx_pop_internal(XObj		s,
					    Msg			*xmsg)
{
	int			i;
	io_record_t		rec;
	io_count_t		count = 1;
	io_recnum_t		recnum;
	mach_error_t		ret;
	xkern_return_t		xret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */
	SESSPOOL_FIND_SESSION(s,&i);

	if (i == XK_FAILURE) {
		/*
		 * This should not happen: all incoming messages should
		 * have an active record in the session pool, since we
		 * close the (only) reference to each session when
		 * removing it from the pool.
		 *
		 * Report the problem and quit.
		 */
		IPPaddr			ipp_addr;
		net_addr_t		netaddr;
		char			addrstring[80];
		char			errmsg[200];
		int			tmp;

		net_addr_null_init(&netaddr);

		xret = usx_get_IPPaddr(s,REMOTE_PART, &ipp_addr);
		if (xret == XK_FAILURE) {
			us_internal_error(
				"usx_pop_internal.usx_get_IPPaddr",
				convert_xkernel_error(x_errno));
		}
		USX_CVT_IPPADDR(ipp_addr,&netaddr);
		(void) net_addr_inet_get_stringname(&netaddr,addrstring,80);
		sprintf(errmsg,
			"usx_pop_internal: cannot find cached session for %s",
								addrstring);
		us_internal_error(errmsg,US_UNKNOWN_ERROR);
		(void) protocol_dir->usx_close_internal(this,FALSE,s);
//		msgDestroy(xmsg);

		return(XK_FAILURE);

#ifdef	NOTDEF
		/*
		 * Try to recover by re-creating a session record.
		 */
		XObj			new_s;

		us_internal_error(
	"usx_pop_internal cannot find cached session -- creating a new one",
							US_UNKNOWN_ERROR);

		partPush(participants[REMOTE_PART], &(ipp_addr.host));
		partPush(participants[REMOTE_PART], &(ipp_addr.port));
		ret = protocol_dir->usx_open_internal(this,FALSE,
						Local(participants),&new_s);
		if (ret != ERR_SUCCESS) {
			us_internal_error("usx_pop_internal.usx_open_internal",
									ret);
//			msgDestroy(xmsg);
/*			xClose(s);	/* XXX ??? */
			return(XK_FAILURE);
		}
		if (new_s != s) {
			us_internal_error(
			"usx_pop_internal cannot re-create same session",
							US_UNKNOWN_ERROR);
			(void) protocol_dir->usx_close_internal(this,
								FALSE,new_s);
//			msgDestroy(xmsg);
/*			xClose(s);	/* XXX ??? */
			return(XK_FAILURE);
		}
		SESSPOOL_FIND_EMPTY(&i);
		Local(session_pool)[i].s = s;
		USX_CVT_IPPADDR(ipp_addr,&Local(session_pool)[i].peeraddr);

		/*
		 * XXX usx_open_internal() got a new reference to
		 * the session. Should we assume that we had an old one?
		 */
/*		xClose(s);	/* XXX ??? */
#endif	NOTDEF
	}

	ret = buf_mgr->usx_convert_xmsg_to_record(xmsg,&rec);
	if (ret != ERR_SUCCESS) {
//		msgDestroy(xmsg);
		return(XK_FAILURE);
	}

	usx_recinfo_init(&rec->recinfo,
		&(Local(session_pool)[i].peeraddr),&Local(null_options));

	ret = incoming_stream->io_putrec_seq(0,&rec,&count,&recnum);
	if (ret != ERR_SUCCESS) {
		(void) buf_mgr->io_free_record(rec);
		return(XK_FAILURE);
	}

	return(XK_SUCCESS);
}

xkern_return_t
usudp_clts::usx_opendone_internal(XObj		lls,
				 XObj			llp,
				 XObj			hlpType)
{
	int			i;
	xkern_return_t		xret;
	IPPaddr			ipp_addr;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	xret = usx_get_IPPaddr(lls,REMOTE_PART, &ipp_addr);
	if (xret == XK_FAILURE) {
		us_internal_error(
			"usudp_clts::usx_opendone_internal.usx_get_IPPaddr",
				convert_xkernel_error(x_errno));
		return(XK_FAILURE);
	}

	SESSPOOL_FIND_SESSION(lls,&i);

	if (i != XK_FAILURE) {
		/*
		 * We already have a record for this session.
		 */
		net_addr_t		peeraddr;
		ns_name_t		newname;
		ns_name_t		oldname;
		char			errmsg[200];

		net_addr_null_init(&peeraddr);

		USX_CVT_IPPADDR(ipp_addr,&peeraddr);
		(void) net_addr_inet_get_stringname(&peeraddr,newname,
							sizeof(ns_name_t));
		if (net_addr_equal(&peeraddr,
				&Local(session_pool)[i].peeraddr)) {
			sprintf(errmsg,
			"usx_opendone_internal: duplicate record for %s",
			newname);
			us_internal_error(errmsg,US_UNKNOWN_ERROR);
			return(XK_FAILURE);
		} else {
			(void) net_addr_inet_get_stringname(
					&Local(session_pool)[i].peeraddr,
					oldname,sizeof(ns_name_t));
			sprintf(errmsg,
		"usx_opendone_internal: mismatched records for %s (old %s)",
			newname,oldname);
			us_internal_error(errmsg,US_UNKNOWN_ERROR);
			return(XK_FAILURE);
		}
	}

	SESSPOOL_FIND_EMPTY(&i);
	Local(session_pool)[i].s = lls;
	
	USX_CVT_IPPADDR(ipp_addr,	&Local(session_pool)[i].peeraddr);

	return(XK_SUCCESS);
}

xkern_return_t
usudp_clts::usx_closedone_internal(XObj lls)
{
  return XK_SUCCESS;
}

