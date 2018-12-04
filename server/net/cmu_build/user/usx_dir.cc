/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992 Carnegie Mellon University
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_dir.cc,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Manager for endpoints supported by the x-kernel.
 *
 * HISTORY
 * $Log:	usx_dir.cc,v $
 * Revision 2.3  94/07/13  18:06:51  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:19  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  12:47:39  jms]
 * 
 * Revision 2.4  92/07/05  23:33:54  dpj
 * 	Added explicit definition of remote_class_name()
 * 	under GXXBUG_VIRTUAL1.
 * 	[92/06/29  17:26:04  dpj]
 * 
 * 	Converted to new C++ RPC package.
 * 	[92/05/10  01:28:11  dpj]
 * 
 * Revision 2.3  92/03/05  15:10:05  jms
 * 	Upgrade to use no MACH_IPC_COMPAT features.  New IPC only!
 * 	[92/02/26  19:02:04  jms]
 * 
 * Revision 2.2  91/11/06  14:14:42  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:09:18  pjg]
 * 
 * Revision 2.2  91/05/05  19:31:03  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:32  dpj]
 * 
 * 	First really working version.
 * 	[91/04/28  10:49:53  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:48:22  dpj]
 * 
 *
 */

/*
 * In the normal operation of this module, entries in the demux_map
 * that correspond to CLTS endpoints are simply tossed-out when a
 * COTS endpoint is established for the same set of addresses. If that
 * COTS endpoint is later removed and the CLTS endpoint happens to
 * have another reference for the same session, a special lookup is
 * performed in the openenable_map to try and find that CLTS endpoint
 * and restore the original mapping.
 *
 * The USE_SECONDARY_DEMUX_MAP conditional corresponds to an alternative
 * mode of operation, where CLTS entries are not simply lost when they
 * are overridden by a COTS entry. Instead, CLTS entries are entered in
 * a "secondary" map, from which they are automatically restored when
 * the COTS entry is removed.
 *
 * The USE_SECONDARY_DEMUX_MAP mode is more complicated, untested, and
 * it is normally off. To turn it on, define the conditional in the
 * _ifc.h file.
 */


#include	<usx_dir_ifc.h>
#include	<usx_iobuf_mgr_ifc.h>
#include	<agent_ifc.h>

extern "C" {
#include	<base.h>
#include	<ns_types.h>
#include	<usx_internal.h>
#include	"upi.h"
#include	"process.h"
}



/*
 * Convenience symbols.
 */
#define	USPROT	Local(protocol)
#define PROT	(USPROT->down[0])
#define IP	(PROT->down[0])

#define BASE net_dir_base
DEFINE_CLASS(usx_dir)

void usx_dir::init_class(usClass* class_obj)
{
	BASE::init_class(class_obj);

	BEGIN_SETUP_METHOD_WITH_ARGS(usx_dir);
	SETUP_METHOD_WITH_ARGS(usx_dir,ns_list_types);
	SETUP_METHOD_WITH_ARGS(usx_dir,net_create);
	SETUP_METHOD_WITH_ARGS(usx_dir,net_lookup);
	SETUP_METHOD_WITH_ARGS(usx_dir,net_cots_lookup);
	END_SETUP_METHOD_WITH_ARGS;
}


usx_dir::usx_dir(ns_mgr_id_t mgr_id, access_table *acctab, XObj _protocol,
		usx_new_clts_fun_t _usx_new_clts,
		usx_new_cots_fun_t _usx_new_cots
)
:
 net_dir_base(mgr_id, acctab, 0),
 protocol(_protocol)
{
	int			ret;

	usx_new_clts = _usx_new_clts;
	usx_new_cots = _usx_new_cots;

	DEBUG1(usx_debug, (0, "usx_dir::usx_dir\n"));

//	new_object(Local(usx_iobuf_mgr),usx_iobuf_mgr);
//	(void) io_set_rec_infosize(Local(usx_iobuf_mgr),sizeof(usx_recinfo_t));

	buf_mgr = new usx_iobuf_mgr;
	(void) buf_mgr->io_set_rec_infosize(sizeof(usx_recinfo_t));
	
	XKERNEL_MASTER();

	Local(openenable_map) = mapCreate(100, sizeof(IPPaddr));
	if (Local(openenable_map) == ((Map)-1)) {
		us_internal_error("usx_dir.mapCreate()",
					convert_xkernel_error(x_errno));
	}

	Local(demux_map) = mapCreate(100, sizeof(void*));
	if (Local(demux_map) == ((Map)-1)) {
		us_internal_error("usx_dir.mapCreate()",
					convert_xkernel_error(x_errno));
	}

#if	USE_SECONDARY_DEMUX_MAP
	Local(secondary_demux_map) = mapCreate(100, sizeof(void*));
	if (Local(secondary_demux_map) == ((Map)-1)) {
		us_internal_error("usx_dir.mapCreate(secondary_demux_map)",
					convert_xkernel_error(x_errno));
	}
#endif	USE_SECONDARY_DEMUX_MAP

	bzero(&Local(default_ipaddr),sizeof(IPPaddr));
	ret = xControl(IP,GETMYHOST,
		(char *)(&((Local(default_ipaddr)).host)), sizeof(IPhost));

	if (ret != sizeof(IPhost)) {
		us_internal_error("usx_dir.xcontrolprotl(GETMYHOST)",
					convert_xkernel_error(x_errno));
		bzero(&Local(default_ipaddr),sizeof(IPPaddr));
	}
	
	XKERNEL_RELEASE();

	Local(next_free_port) = 2000;
}


usx_dir::usx_dir()
:
 openenable_map(0), buf_mgr(0), protocol(0)
{}


usx_dir::~usx_dir()
{
	DEBUG1(usx_debug, (0, "usx_dir::usx_dir\n"));

	if (openenable_map == 0) return;

	XKERNEL_MASTER();

	(void) mapClose(Local(openenable_map));
	(void) mapClose(Local(demux_map));

#if	USE_SECONDARY_DEMUX_MAP
	(void) mapClose(Local(secondary_demux_map));
#endif	USE_SECONDARY_DEMUX_MAP

	mach_object_dereference(Local(buf_mgr));

	(void) xClose(Local(protocol));

	XKERNEL_RELEASE();
}

#ifdef	GXXBUG_VIRTUAL1
char* usx_dir::remote_class_name() const
	{ return net_dir_base::remote_class_name(); }
#endif	GXXBUG_VIRTUAL1


/*
 * Standard name service interface.
 */

mach_error_t 
usx_dir::ns_list_types(ns_type_t		**types,	/* out */
			 int			*count)		/* out */
{
	DEBUG1(usx_debug, (0, "usx_dir::ns_list_types\n"));

	*count = 0;
	*types = NULL;
	return(US_NOT_IMPLEMENTED);
}


/*
 * Standard network service interface.
 */

mach_error_t 
usx_dir::net_create(net_addr_t	*localaddr,	/* INOUT */
		      int		*qmax,		/* INOUT */
		      ns_prot_t		prot,
		      int		protlen,
		      ns_access_t	access,
		      usItem		**newobj,	/* OUT */
		      ns_type_t		*newtype,	/* OUT */
		      net_info_t	*info)		/* OUT */
{
	IPPaddr			ipp_addr;
	xkern_return_t		xret;
	ns_name_t		newname;
	agency			*endpt = NULL;
	ns_type_t		endpt_type;
	int			tag = 0;
	mach_error_t		ret;

#define	ABORT(_ret) {							\
	if (tag) (void) ns_cancel_entry(tag);				\
	XKERNEL_RELEASE();						\
	mach_object_dereference(endpt);					\
	mach_object_dereference(*newobj);				\
	*newobj = NULL;					\
	*newtype = NST_INVALID;						\
	return(_ret);							\
}

	DEBUG1(usx_debug, (0, "usx_dir::net_create\n"));

	*newobj = NULL;

	/*
	 * No use doing anything if not returning an agent, since
	 * the endpoint would just disappear at once.
	 */
	if (access == 0) {
		return(US_UNSUPPORTED);
	}

	XKERNEL_MASTER();

	/*
	 * Establish the effective local address.
	 */
	if (!net_addr_inet_p(localaddr)) ABORT(NET_INVALID_ADDR_FLAVOR);
	if (net_addr_inet_ipaddr_default_p(localaddr)) {
		(void) net_addr_inet_set_ipaddr(localaddr,
				*(ipaddr_t *)&((Local(default_ipaddr)).host));
	}
	if (net_addr_inet_port_default_p(localaddr)) {
		(void) net_addr_inet_set_port(localaddr,
				htons(Local(next_free_port)++)); /* XXX */
	}

	/*
	 * Create a new endpoint.
	 */
	ret = net_addr_inet_get_stringname(localaddr,newname,
							sizeof(ns_name_t));
	if (ret != ERR_SUCCESS) {
		us_internal_error(
		"net_create: cannot get stringname for local address",
		ret);
		ABORT(US_INTERNAL_ERROR);
	}

	ret = ns_reserve_entry(newname,&tag);
	if (ret != ERR_SUCCESS) ABORT(ret);

//	endpt = new usudp_clts(mgr_id, access_tab, buf_mgr, this, localaddr,
//			       &ret);
	ret = usx_new_clts(mgr_id, access_tab, buf_mgr, this, localaddr,
			       &endpt, &endpt_type);

	if (ret != ERR_SUCCESS) ABORT(ret);

	XKERNEL_RELEASE();

	agent *new_agent;
	ret = ns_create_common(tag,endpt,endpt_type,prot,protlen,
			       access, &new_agent);

	mach_object_dereference(endpt);

	if (ret != ERR_SUCCESS) {
		(void) ns_cancel_entry(tag);
		*newobj = NULL;
		*newtype = NST_INVALID;
		return(ret);
	}
	*newobj = new_agent;
	*newtype = endpt_type;
	net_info_copy(&netinfo,info);

	return(ERR_SUCCESS);

#undef	ABORT
}


mach_error_t 
usx_dir::net_lookup(net_addr_t	*localaddr,	/* INOUT */
		      ns_access_t	access,
		      usItem		**newobj,	/* OUT */
		      ns_type_t		*newtype,	/* OUT */
		      net_info_t	*info)		/* OUT */
{
	mach_error_t		ret;

	/*
	 * Establish the effective local address.
	 */
	DEBUG1(usx_debug, (0, "usx_dir::net_lookup\n"));

	if (!net_addr_inet_p(localaddr)) return(NET_INVALID_ADDR_FLAVOR);
	if (net_addr_inet_ipaddr_default_p(localaddr)) {
		(void) net_addr_inet_set_ipaddr(localaddr,
				*(ipaddr_t *)&((Local(default_ipaddr)).host));
	}

	/*
	 * Let the standard base object do all the work.
	 */
//	ret = invoke_super_with_base(Super,Base,
//					mach_method_id(net_lookup),
//					localaddr,access,newobj,newtype,info);

	ret = net_dir_base::net_lookup(localaddr,access,newobj,newtype,info);
	return(ret);
}


mach_error_t 
usx_dir::net_cots_lookup(net_addr_t		*localaddr,	/* INOUT */
			   net_addr_t		*peeraddr,
			   ns_access_t		access,
			   usItem		**newobj,	/* OUT */
			   ns_type_t		*newtype,	/* OUT */
			   net_info_t		*info)		/* OUT */
{
	mach_error_t		ret;

	/*
	 * Establish the effective local address.
	 */
	DEBUG1(usx_debug, (0, "usx_dir::net_cots_lookup\n"));

	if (!net_addr_inet_p(localaddr)) return(NET_INVALID_ADDR_FLAVOR);
	if (net_addr_inet_ipaddr_default_p(localaddr)) {
		(void) net_addr_inet_set_ipaddr(localaddr,
				*(ipaddr_t *)&((Local(default_ipaddr)).host));
	}

	/*
	 * Let the standard base object do all the work.
	 */
//	ret = invoke_super_with_base(Super,Base,
//					mach_method_id(net_cots_lookup),
//					localaddr,peeraddr,
//					access,newobj,newtype,info);

	ret = net_dir_base::net_cots_lookup(localaddr,peeraddr,
					    access,newobj,newtype,info);

	return(ret);
}


/*
 * Calls from the CLTS/CONNECTOR objects.
 */

/*
 * Build an endpoint for a new connection using a connector,
 * Add it to the demux map if sess != NULL (passive connections).
 */
mach_error_t 
usx_dir::usx_new_connection_endpt(vol_agency	*connector,
				  net_addr_t	*localaddr,
				  net_addr_t	*peeraddr,
				  XObj		sess,
				  agency	**endpt) /* OUT */
{
	mach_error_t		ret;
	Bind			bret;
	ns_type_t		endpt_type;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

#define	ABORT(_ret) {							\
	mach_object_dereference(*endpt);				\
	*endpt = NULL;					\
	return(_ret);							\
}

	DEBUG1(usx_debug, (0, "usx_dir::usx_new_connection_endpt\n"));

	*endpt = NULL;

	ret = usx_new_cots(mgr_id, access_tab, connector, buf_mgr, this,
				localaddr, peeraddr, sess, endpt, &endpt_type);
	if (ret != ERR_SUCCESS) ABORT(ret);

	/*
	 * If it was a "passive connection" then put the new endpoint into
	 * the demux map.
	 */
	if (NULL != sess) {
		bret = mapBind(Local(demux_map),&sess,*endpt);
		if (bret == ERR_BIND) {
			us_internal_error(
			"usx_opendisable_internal.mapBind(openenable_map)",
			convert_xkernel_error(x_errno));
			ABORT(US_INTERNAL_ERROR);
		}
	}

	return(ERR_SUCCESS);

#undef	ABORT
}

/*
 * Given an endpoint, put it into the needed directory
 */
mach_error_t 
usx_dir::usx_register_connection_endpt(agency	*endpt,
				  net_addr_t	*localaddr,
				  net_addr_t	*peeraddr,
				  ns_prot_t	prot,
				  int		protlen)
{
	mach_error_t		ret;
	Bind			bret;
	xkern_return_t		xret;
	ns_name_t		newname;
	unsigned int		namelen;
	int			tag = 0;
	ns_type_t		endpt_type;


#define	ABORT(_ret) {							\
	if (tag) (void) ns_cancel_entry(tag);				\
	return(_ret);							\
}

	DEBUG1(usx_debug, (0, "usx_dir::usx_register_connection_endpt\n"));
	/*
	 * Prepare the full endpoint name.
	 */
	ret = net_addr_inet_get_stringname(localaddr,newname,
							sizeof(ns_name_t));
	if (ret != ERR_SUCCESS) {
		us_internal_error(
	"usx_get_connection_endpt: cannot get stringname for local address",
			ret);
		ABORT(US_INTERNAL_ERROR);
	}
	namelen = strlen(newname);
	newname[namelen++] = '-';
	ret = net_addr_inet_get_stringname(peeraddr,&newname[namelen],
						sizeof(ns_name_t) - namelen);
	if (ret != ERR_SUCCESS) ABORT(ret);

	/*
	 * Save the endpt in the dir
	 */

	ret = ns_reserve_entry(newname,&tag);
	if (ret != ERR_SUCCESS) ABORT(ret);

	ret = (endpt)->ns_set_protection(prot,protlen);
	free(prot);
	if (ret != ERR_SUCCESS) ABORT(ret);

	ret = ns_install_entry(tag,endpt,endpt_type);
	if (ret != ERR_SUCCESS) ABORT(ret);
	tag = 0;

	return(ERR_SUCCESS);

#undef	ABORT
}

/*
 * Build an endpoint the protocol directory for a new connection using
 * a connector
 */
mach_error_t 
usx_dir::usx_get_connection_endpt(vol_agency	*connector,
				  net_addr_t	*localaddr,
				  net_addr_t	*peeraddr,
				  ns_prot_t	prot,
				  int		protlen,
				  XObj		sess,
				  agency	**endpt) /* OUT */
{
	mach_error_t		ret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

#define	ABORT(_ret) {							\
	mach_object_dereference(*endpt);				\
	*endpt = NULL;					\
	return(_ret);							\
}

	DEBUG1(usx_debug, (0, "usx_dir::usx_get_connection_endpt\n"));

	ret = usx_new_connection_endpt(connector,
				  localaddr, peeraddr, sess,
				  endpt);
	if (ERR_SUCCESS != ret) {
		return(ret);
	}

	ret = usx_register_connection_endpt(*endpt,
				  localaddr, peeraddr,
				  prot, protlen);

	if (ERR_SUCCESS != ret) {
		ABORT(ret);
	}

	return(ERR_SUCCESS);

#undef	ABORT
}

/*
 * x-kernel protocol handler methods.
 */

int 
usx_dir::usx_demux_internal(XObj			s,
			      Msg			*msg)
{
	usx_endpt_base		*endpt;
	xkern_return_t		xret;

#define	ABORT {				\
	/* xClose(s); XXX */		\
	return(-1);			\
}
//	msgDestroy(msg);

	DEBUG1(usx_debug, (0, "usx_dir::usx_demux_internal\n"));

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	xret = mapResolve(Local(demux_map),&s,&endpt);
	if (xret == XK_FAILURE) {
		us_internal_error(
			"usx_demux_internal.mapResolve(demux_map)",
			convert_xkernel_error(x_errno));
		ABORT;
	}
	/*
	 * No need to get a reference for endpt, since we are protected
	 * by the XKERNEL_MASTER lock.
	 */

	return(endpt->usx_pop_internal(s,msg));

#undef	ABORT
}


xkern_return_t
usx_dir::usx_opendone_internal(XObj			lls,
				 XObj			llp,
				 XObj			hlpType)
{
	IPPaddr			ipp_addr;
	usx_endpt_base*		endpt;
	xkern_return_t		xret;
	Bind			bret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	DEBUG1(usx_debug, (0, "usx_dir::usx_opendone_internal\n"));

	/*
	 * Find the endpoint responsible for handling incoming msgs.
	 */
	xret = usx_get_IPPaddr(lls,LOCAL_PART, &ipp_addr);
	if (xret == XK_FAILURE) {
		us_internal_error(
			"usx_dir::usx_opendone_internal.usx_get_IPPaddr",
			convert_xkernel_error(x_errno));
		return(XK_FAILURE);
	}
	xret = mapResolve(Local(openenable_map),
						&ipp_addr, &endpt);




	if (xret == XK_FAILURE) {
		char			errmsg[200];

		sprintf(errmsg,
		"usx_opendone_internal cannot find endpoint for 0x%x,0x%x",
				ipp_addr.host,
				ipp_addr.port);
		us_internal_error(errmsg,convert_xkernel_error(x_errno));
/*		xClose(lls); XXX ??? */
		return(XK_FAILURE);
	}

	/*
	 * No need to get a reference for endpt, since we are protected
	 * by the XKERNEL_MASTER lock.
	 */

	/*
	 * Register this new session for future reference.
	 */

	xret = endpt->usx_opendone_internal(lls, llp, hlpType);
	if (xret == XK_FAILURE) {
		return(XK_FAILURE);
	}

#if BIND_UDP_ENDPT_XXX
	bret = mapBind(Local(demux_map),&lls,endpt);	/* XXX ONLY UDP! $$$ */
	if (bret == ERR_BIND) {
		us_internal_error(
				"usx_opendone_internal.mapBind(demux_map)",
				convert_xkernel_error(x_errno));

		/*
		 * There already was a binding ??? Then how come we got
		 * an opendone()?
		 *
		 * XXX Go on and hope this was an hallucination...
		 */

#if	USE_SECONDARY_DEMUX_MAP
		/*
		 * XXX ???
		 */
#endif	USE_SECONDARY_DEMUX_MAP

	return(XK_FAILURE);
	}
#endif BIND_UDP_ENDPT_XXX

	return(XK_SUCCESS);
}


/*
 * Front-ends for x-kernel methods, called from the endpoints.
 */

mach_error_t 
usx_dir::usx_open_internal(usx_endpt_base*	endpt,
			     boolean_t		cots,
			     Part		*participants,
			     XObj		*sessn)		/* OUT */
{
	xkern_return_t		xret;
	Bind			bret;
	usx_endpt_base*		old_clts;


	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	DEBUG1(usx_debug, (0, "usx_dir::usx_open_internal\n"));

	/*
	 * First open the session.
 	 */
	*sessn = xOpen(USPROT, USPROT,PROT,participants);
	if (*sessn == ERR_XOBJ) {
		us_internal_error("usx_open_internal.xOpen",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	return(ERR_SUCCESS);
}

xkern_return_t
usx_dir::usx_closedone_internal(XObj			sess)
{
	IPPaddr			ipp_addr;
	usx_endpt_base*		endpt;
	xkern_return_t		xret;
	Bind			bret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	DEBUG1(usx_debug, (0, "usx_dir::usx_closedone_internal\n"));

	/*
	 * Find the endpoint responsible for handling incoming msgs.
	 */
	xret = mapResolve(Local(demux_map),&sess,&endpt);
	if (xret == XK_FAILURE) {
		/* Assume that it is already gone */
		DEBUG1(usx_debug, (0, "usx_dir::usx_closedone_internal: cannot find endpoint for 0x%x,0x%x\n",
				ipp_addr.host,
				ipp_addr.port));
		return(ERR_SUCCESS);
	}


	/*
	 * No need to get a reference for endpt, since we are protected
	 * by the XKERNEL_MASTER lock.
	 */

	/*
	 * UnRegister this session
	 */
	xret = mapUnbind(Local(demux_map),&sess);
	if (xret == XK_FAILURE) {
		us_internal_error("ustcp_close_internal.xClose",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	xret = endpt->usx_closedone_internal(sess);
	if (xret == XK_FAILURE) {
		return(XK_FAILURE);
	}
	return(XK_SUCCESS);
}
mach_error_t 
usx_dir::usx_close_internal(usx_endpt_base*	endpt,
			      boolean_t		cots,
			      XObj		sessn)
{
	xkern_return_t		xret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	DEBUG1(usx_debug, (0, "usx_dir::usx_close_internal, sess 0x%x\n", sessn));

	/*
	 * First close (release one reference) the session.
	 *
	 * We must do this even if the entry is not found in the
	 * demux map, to match the reference acquired earlier from
	 * usx_open_internal().
	 */
	xret = xClose(sessn);
	if (xret == XK_FAILURE) {
		us_internal_error("usx_close_internal.xClose",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	return(ERR_SUCCESS);
}


mach_error_t 
usx_dir::usx_openenable_internal(usx_endpt_base*		endpt,
				   IPPaddr		*ipp_addr,
				   Part			*participants)
{
	xkern_return_t		xret;
	Bind			bret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	DEBUG1(usx_debug, (0, "usx_dir::usx_openenable_internal\n"));

	xret = xOpenEnable(USPROT,USPROT,PROT,&(participants[LOCAL_PART]));
	if (xret == XK_FAILURE) {
		us_internal_error("usx_openenable_internal.xOpenEnable",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	bret = mapBind(Local(openenable_map), ipp_addr, endpt);
	if (bret == ERR_BIND) {
		us_internal_error(
			"usx_openenable_internal.mapBind(openenable_map)",
			convert_xkernel_error(x_errno));

		xret = xOpenDisable(USPROT,USPROT,PROT,participants);
		if (xret == XK_FAILURE) {
			us_internal_error(
				"usx_openenable_internal.x_opendisable",
				convert_xkernel_error(x_errno));
		}
		return(US_INTERNAL_ERROR);
	}

	return(ERR_SUCCESS);
}


mach_error_t 
usx_dir::usx_opendisable_internal(usx_endpt_base*		endpt,
				    IPPaddr		*ipp_addr,
				    Part		*participants)
{
	xkern_return_t		xret;
	Bind			bret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	DEBUG1(usx_debug, (0, "usx_dir::usx_opendisable_internal\n"));

	xret = mapUnbind(Local(openenable_map), ipp_addr);
	if (xret == XK_FAILURE) {
		us_internal_error(
			"usx_opendisable_internal.mapUnbind(openenable_map)",
			convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	xret = xOpenDisable(USPROT,USPROT,PROT,&(participants[LOCAL_PART]));
#if X_OPEN_DISABLE_FAILURE_NOT_OK
	if (xret == XK_FAILURE) {
		us_internal_error(
				"usx_opendisable_internal.x_opendisable",
				convert_xkernel_error(x_errno));

		bret = mapBind(Local(openenable_map), ipp_addr, endpt);
		if (bret == ERR_BIND) {
			us_internal_error(
			"usx_opendisable_internal.mapBind(openenable_map)",
			convert_xkernel_error(x_errno));
		}
		return(US_INTERNAL_ERROR);
	}
#endif X_OPEN_DISABLE_FAILURE_NOT_OK

	return(ERR_SUCCESS);
}

