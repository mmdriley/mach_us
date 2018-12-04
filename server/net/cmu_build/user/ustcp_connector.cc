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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/ustcp_connector.cc,v $
 *
 * Author: J. Mark Stevenson
 *
 * Purpose: xkernel TCP endpoint for connection-less operation.
 *
 * HISTORY:
 * $Log:	ustcp_connector.cc,v $
 * Revision 2.4  94/07/13  18:06:13  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:31  jms
 * 	Need dummy implementations of virtual methods in class ustcp_connector
 * 		for 2.3.3 g++ -modh
 * 	[94/04/28  19:08:38  jms]
 * 
 * Revision 2.2  94/01/11  18:09:41  jms
 * 	Initial Version
 * 	[94/01/10  11:33:11  jms]
 * 
 */

#include	<ustcp_connector_ifc.h>
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
#include	"tcp.h"
}

/*
 * Routine for building new connector endpoints.
 */
static mach_error_t new_connector(ns_mgr_id_t mgr_id, access_table *_acctab,
			usx_iobuf_mgr *_buf_mgr, usx_dir *_protocol_dir,
			net_addr_t *_localaddr, 
			agency **endpt, ns_type_t *endpt_type);

usx_new_clts_fun_t ustcp_new_connector = new_connector;


DEFINE_CLASS_MI(ustcp_connector)
DEFINE_CASTDOWN2(ustcp_connector, usx_endpt_base, usNetConnector)

void ustcp_connector::init_class(usClass* class_obj)
{
	usNetConnector::init_class(class_obj);
	net_endpt_base::init_class(class_obj);

	BEGIN_SETUP_METHOD_WITH_ARGS(ustcp_connector);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,net_get_localaddr);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,net_connect);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,ns_get_attributes);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,net_listen);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,net_accept);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,net_reject);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,net_get_connect_qinfo);
	SETUP_METHOD_WITH_ARGS(ustcp_connector,net_set_connect_qmax);
	END_SETUP_METHOD_WITH_ARGS;
}

ustcp_connector::ustcp_connector(ns_mgr_id_t mgr_id, access_table *_acctab,
		       usx_iobuf_mgr *_buf_mgr, usx_dir *_protocol_dir,
		       net_addr_t *_localaddr, mach_error_t* ret)
:
 pendconn_count(0),
 pendconn_max(-1),
 usx_endpt_base(mgr_id, _acctab),
 protocol_dir(_protocol_dir), buf_mgr(_buf_mgr),
 readers_count(0)
{
	int			i;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	*ret = ERR_SUCCESS;

	DEBUG1(usx_debug, (0, "ustcp_connector::ustcp_connector\n"));

	dll_init(&Local(pendconn_queue));
	mutex_init(&Local(pendconn_lock));
	condition_init(&Local(pendconn_listen_cond));

	*ret = ERR_SUCCESS;
	mach_object_reference(_protocol_dir);
	mach_object_reference(_buf_mgr);

	net_addr_null_init(&Local(localaddr));
	net_addr_copy(_localaddr,&Local(localaddr));

	USX_CVT_NETADDR(_localaddr,&Local(ipp_local));
	Local(ipp_remote).host = (IPhost){0xff,0xff,0xff,0xff};	/* ANY_HOST??? */
	Local(ipp_remote).port = (unsigned int)ANY_PORT;

	partInit(&(participants[LOCAL_PART]), 1);
	partPush(participants[LOCAL_PART], &(ipp_local.host),
			sizeof(ipp_local.host));
	partPush(participants[LOCAL_PART], &(ipp_local.port),
			sizeof(ipp_local.port));
	net_options_null_init(&Local(null_options));
}

static mach_error_t new_connector(ns_mgr_id_t mgr_id, access_table *_acctab,
			usx_iobuf_mgr *_buf_mgr, usx_dir *_protocol_dir,
			net_addr_t *_localaddr, 
			agency **endpt, ns_type_t *endpt_type)
{
	mach_error_t	ret;

	*endpt = new ustcp_connector(mgr_id, _acctab, _buf_mgr, _protocol_dir,
			_localaddr, &ret);
	*endpt_type = NST_CONNECTOR;
	return(ret);
}

ustcp_connector::ustcp_connector()
:
 protocol_dir(0), buf_mgr(0), incoming_stream(0)
{
	DEBUG1(usx_debug, (0, "ustcp_connector::ustcp_connector\n"));

	net_addr_null_init(&Local(localaddr));
	net_options_null_init(&Local(null_options));
}

ustcp_connector::~ustcp_connector()
{
	DEBUG1(usx_debug, (0, "ustcp_connector::~ustcp_connector\n"));

	mach_object_dereference(Local(protocol_dir));
	mach_object_dereference(Local(buf_mgr));


	net_addr_destroy(&Local(localaddr));
	net_options_destroy(&Local(null_options));
}


char* ustcp_connector::remote_class_name() const
{
	return "usNetConnector_proxy";
}

mach_error_t ustcp_connector::ns_tmp_cleanup_for_shutdown()
{
	mach_error_t		ret;

	DEBUG1(usx_debug, (0, "ustcp_connector::ns_tmp_cleanup_for_shutdown\n"));

	/*
	 * Everything should already have been taken care of
	 * when the last agent was destroyed.
	 */

	ret = tmp_agency::ns_tmp_cleanup_for_shutdown();
	return(ret);
}

mach_error_t ustcp_connector::ns_register_agent(ns_access_t access)
{
	mach_error_t		ret;

	DEBUG1(usx_debug, (0, "ustcp_connector::ns_register_agent\n"));
	if (access & NSR_INSERT) {	/* XXX DPJ wanted NSR_READ, why? */
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


mach_error_t ustcp_connector::ns_unregister_agent(ns_access_t access)
{
	mach_error_t		ret;
	int			i;

	DEBUG1(usx_debug, (0, "ustcp_connector::ns_unregister_agent\n"));
	ret = tmp_agency::ns_unregister_agent(access);

	if (access & NSR_INSERT) {	/* XXX DPJ wanted NSR_READ, why? */
		XKERNEL_MASTER();

		Local(readers_count)--;

		if (Local(readers_count) == 0) {
			/* close the session? */
			(void) protocol_dir->usx_opendisable_internal(
					this, &(Local(ipp_local)),
					Local(participants));
		}

		XKERNEL_RELEASE();
	}

	return(ret);
}

xkern_return_t
ustcp_connector::usx_pop_internal(XObj lls, Msg* llm)
{
  return XK_FAILURE;
}

xkern_return_t
ustcp_connector::usx_opendone_internal(XObj		lls,
				 XObj			llp,
				 XObj			hlpType)
{
	int			i;
	xkern_return_t		xret;
	mach_error_t		ret;
	ustcp_conninfo_t	conninfo;

	net_addr_t		peeraddr;
	IPPaddr			ipp_peeraddr;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

#define	ABORT() {							\
	if (conninfo->endpt) mach_object_dereference(conninfo->endpt);	\
	free(conninfo);							\
	mutex_unlock(&Local(pendconn_lock));				\
	return(XK_FAILURE);						\
}

	DEBUG1(usx_debug, (0, "ustcp_connector::usx_opendone_internal\n"));

	/*
	 * Get the remote connection information
	 */
	xret = usx_get_IPPaddr(lls, REMOTE_PART, &ipp_peeraddr);
	if (xret == XK_FAILURE) {
		us_internal_error(
			"ustcp_connector::net_listen.usx_usx_get_IPPaddr",
				convert_xkernel_error(x_errno));
		return(XK_FAILURE);
	}

	USX_CVT_IPPADDR(ipp_peeraddr,&peeraddr)

	conninfo = (ustcp_conninfo_t)malloc(sizeof(struct ustcp_conninfo));
	dll_init(&(conninfo->chain));
	conninfo->state = USTCP_CONN_PENDING;
	conninfo->lls = lls;
	conninfo->llp = llp;
	conninfo->hlpType = hlpType;
	conninfo->endpt = NULL;

	mutex_lock(&Local(pendconn_lock));

	if (Local(pendconn_count) >= Local(pendconn_max)) {
		/* XXX Tell someone we are dropping stuff */
		ABORT();
	}

	dll_enter(&Local(pendconn_queue),conninfo,ustcp_conninfo_t,chain);
	Local(pendconn_count)++;

	/* Since we are returning, we need to hold a reference for ourselves */
	/* XXX shouldn't we be given a reverence by being called as an anchor? */
	xDuplicate(lls);

	/*
	 * Get the new passive side endpoint
	 * XXX Should do the creation of the endpoint in accept.
	 * 	but we cannot release the XKERNEL_MASTER until
	 *	the opendone is done (or we get repeats)
	 *	and a xDemux may/will come in before the
	 *	accept occures.  The danger in the current
	 *	method, is that we may demux many messages
	 *	before the accept comes (if it ever does).
	 * XXX
	 */
	ret = protocol_dir->usx_new_connection_endpt(this,
					&Local(localaddr),&peeraddr,
					lls, &(conninfo->endpt));

	if (ret != ERR_SUCCESS) {
		ABORT();
	}

	/*
	 * Wake-up any potential users waiting in net_listen().
	 */
	condition_signal(&Local(pendconn_listen_cond));
	mutex_unlock(&Local(pendconn_lock));

	return(XK_SUCCESS);
#undef	ABORT
}

xkern_return_t
ustcp_connector::usx_closedone_internal(XObj lls)
{
  return XK_FAILURE;
}

/*
 * Exported client interface.
 */

mach_error_t ustcp_connector::net_get_localaddr(net_addr_t *addr)
{
	net_addr_copy(&Local(localaddr),addr);

	return(ERR_SUCCESS);
}


mach_error_t ustcp_connector::net_connect(net_addr_t		*peeraddr,
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
						   prot,protlen,NULL,&endpt);
	if (ret != ERR_SUCCESS) {
		ABORT(ret);
	}
	XKERNEL_RELEASE();

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
	*newtype = NST_COTS_BYTES;
	*newobj = agent_obj;

	mach_object_dereference(endpt);
	mach_object_dereference(cred);

	return(ERR_SUCCESS);

#undef	ABORT
}

mach_error_t 
ustcp_connector::ns_get_attributes(ns_attr_t attr, int *attrlen)
{
	mach_error_t		ret;

	DEBUG1(usx_debug, (0, "ustcp_connector::ns_get_attributes\n"));

//	ret = invoke_super(Super,mach_method_id(ns_get_attributes),
//							attr,attrlen);
	ret = tmp_agency::ns_get_attributes(attr,attrlen);
	if (ret != ERR_SUCCESS) {
		return(ret);
	}

	attr->type = NST_CONNECTOR;

	/*
	 * XXX Should return the number of available bytes.
	 */

	return(ERR_SUCCESS);
}

mach_error_t 
ustcp_connector::net_listen(io_mode_t mode, net_addr_t *peeraddr,
			      net_options_t *options, char *udata,
			      unsigned int *udatalen, int *seqno)
{
	ustcp_conninfo_t		conninfo;
	IPPaddr				ipp_peeraddr;
	mach_error_t			ret;
	xkern_return_t			xret;

	DEBUG1(usx_debug, (0, "ustcp_connector::net_listen\n"));

	/*
	 * Check mode. Only IOM_WAIT allowed for now.
	 */
	if (mode & ~(IOM_PROBE | IOM_WAIT)) {

		return(IO_INVALID_MODE);
	}

	mutex_lock(&Local(pendconn_lock));

	/*
	 * Quick exit for non-blocking option.
	 */
	if (((mode & IOM_WAIT) == 0) && (Local(pendconn_count) == 0)) {
		mutex_unlock(&Local(pendconn_lock));
		return(US_OBJECT_BUSY);
	}

	/*
	 * Wait until there is something on the queue.
	 */
	while (Local(pendconn_count) == 0) {
		ret = intr_cond_wait(&Local(pendconn_listen_cond),&Local(pendconn_lock));
		if (ret != ERR_SUCCESS) {
			mutex_unlock(&Local(pendconn_lock));
			return(EXCEPT_SOFTWARE);
		}
	}

	/*
	 * We are committed to handling this connection!!!
	 * Remove the record from the queue. Iff the caller asked for
	 * a seqno to identify it with later.
	 */
	if (! (IOM_PROBE & mode)) {
		dll_remove_first(&Local(pendconn_queue),conninfo,
					ustcp_conninfo_t,chain);
		Local(pendconn_count)--;
	}
	else {
		/* just look at the first one, dont pop it */
		conninfo = (ustcp_conninfo_t)dll_first(&Local(pendconn_queue));
	}

	/*
	 * Return the connection information to the user.
	 */
	if (NULL != peeraddr) {
		xret = usx_get_IPPaddr(conninfo->lls, REMOTE_PART, &ipp_peeraddr);
		if (xret == XK_FAILURE) {
			us_internal_error(
				"ustcp_connector::net_listen.usx_usx_get_IPPaddr",
					convert_xkernel_error(x_errno));
			mutex_unlock(&Local(pendconn_lock));
			return(XK_FAILURE);
		}
		USX_CVT_IPPADDR(ipp_peeraddr,peeraddr)
	}

//	net_options_copy(conninfo->options,options);
//	bcopy(conninfo->in_udata,udata,conninfo->in_udatalen);
	*udatalen = 0;
	if (NULL != seqno) {
		*seqno = (int)conninfo;
	}

	mutex_unlock(&Local(pendconn_lock));

	return(ERR_SUCCESS);
}


mach_error_t 
ustcp_connector::net_accept(int seqno, net_options_t *options,
			      char *udata, unsigned int udatalen,
			      ns_prot_t prot, unsigned int protlen,
			      ns_access_t access, usItem **newobj,
			      ns_type_t *newtype)
{
	ustcp_conninfo_t		conninfo = (ustcp_conninfo_t)seqno;
	net_addr_t			peeraddr;
	IPPaddr				ipp_peeraddr;
	std_cred			*cred = 0;
	agency				*endpt = 0;
	mach_error_t			ret;
	xkern_return_t			xret;

#define	ABORT(_ret) {							\
	mach_object_dereference(*newobj);				\
	*newobj = NULL;					\
	*newtype = NST_INVALID;						\
	mach_object_dereference(conninfo->endpt);			\
	mach_object_dereference(cred);					\
	return(_ret);							\
}

	DEBUG1(usx_debug, (0, "ustcp_connector::net_accept\n"));
	*newobj = NULL;

	/*
	 * No use doing anything if not returning an agent, since
	 * the endpoint would just disappear at once.
	 */
	if (access == 0) {
		*newtype = NST_INVALID;
		return(US_UNSUPPORTED);
	}

	/*
	 * Get the remote connection information
	 */
	xret = usx_get_IPPaddr(conninfo->lls, REMOTE_PART, &ipp_peeraddr);
	if (xret == XK_FAILURE) {
		us_internal_error(
			"ustcp_connector::net_accept.usx_usx_get_IPPaddr",
				convert_xkernel_error(x_errno));
		return(XK_FAILURE);
	}

	USX_CVT_IPPADDR(ipp_peeraddr,&peeraddr)

	/*
	 * Register the connection in the dir
	 */
	ret = protocol_dir->usx_register_connection_endpt(conninfo->endpt,
					&Local(localaddr),&peeraddr,
					prot, protlen);
	/*
	 * Build an agent for the endpt
	 */
	ret = agent::base_object()->ns_get_cred_obj(&cred);
	if (ret != ERR_SUCCESS) {
		us_internal_error(
				"net_accept: cannot get current credentials",
				ret);
		ABORT(US_INTERNAL_ERROR);
	}
	agent *agent_obj;

	ret = conninfo->endpt->ns_create_agent(access,cred,&agent_obj);

	if (ret != ERR_SUCCESS) ABORT(ret);
	*newobj = agent_obj;
	mach_object_dereference(conninfo->endpt);
	mach_object_dereference(cred);
	cred = NULL;

	*newtype = NST_COTS_BYTES;
	return(ERR_SUCCESS);

#undef	ABORT
}


mach_error_t 
ustcp_connector::net_reject(int seqno, char *udata, unsigned int udatalen)
{
	ustcp_conninfo_t		conninfo = (ustcp_conninfo_t)seqno;
	mach_error_t			ret;

	/*
	 * Wake-up the active side.
	 */
	conninfo->state = USTCP_CONN_REJECTED;
	/* XXX KILL THE CONNECTION */

	DEBUG1(usx_debug, (0, "ustcp_connector::net_reject\n"));

	return(ERR_SUCCESS);
}


mach_error_t 
ustcp_connector::net_get_connect_qinfo(int *qsize, int *qmax)
{
	DEBUG1(usx_debug, (0, "ustcp_connector::net_get_connect_qinfo\n"));

	mutex_lock(&Local(pendconn_lock));

	*qsize = Local(pendconn_count);
	*qmax = Local(pendconn_max);

	mutex_unlock(&Local(pendconn_lock));

	return(ERR_SUCCESS);
}


mach_error_t 
ustcp_connector::net_set_connect_qmax(int qmax)
{
	ustcp_conninfo_t		conninfo;

	DEBUG1(usx_debug, (0, "ustcp_connector::net_set_connect_qmax\n"));

	if (qmax < 0) {
		return(US_INVALID_ARGS);
	}

	mutex_lock(&Local(pendconn_lock));

	Local(pendconn_max) = qmax + 1;

	while (Local(pendconn_count) > Local(pendconn_max)) {
		conninfo = (ustcp_conninfo_t) dll_first(&Local(pendconn_queue));
		dll_remove(&Local(pendconn_queue),conninfo,ustcp_conninfo_t,chain);
		Local(pendconn_count)--;

		mutex_unlock(&Local(pendconn_lock));

		conninfo->state = USTCP_CONN_REJECTED;
		/* XXX KILL THE CONNECTION */

		mutex_lock(&Local(pendconn_lock));
	}

	mutex_unlock(&Local(pendconn_lock));

	return(ERR_SUCCESS);
}
