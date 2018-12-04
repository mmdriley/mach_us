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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usudp_dir.cc,v $
 *
 * Author: J. Mark Stevenson
 *
 * Purpose: Manager for all UDP endpoints supported by the x-kernel.
 *
 * HISTORY
 * $Log:	usudp_dir.cc,v $
 * Revision 2.3  94/07/13  18:06:44  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:13  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  11:55:29  jms]
 * 
 *
 */
#include	<usudp_dir_ifc.h>
#include	<usudp_clts_ifc.h>
#include	<usudp_cots_ifc.h>

#define BASE usx_dir
DEFINE_CLASS(usudp_dir)

void usudp_dir::init_class(usClass* class_obj)
{
	BASE::init_class(class_obj);

	BEGIN_SETUP_METHOD_WITH_ARGS(usudp_dir);
	SETUP_METHOD_WITH_ARGS(usudp_dir,ns_list_types);
	END_SETUP_METHOD_WITH_ARGS;
}


usudp_dir::usudp_dir(ns_mgr_id_t mgr_id, access_table *acctab, XObj _protocol)
:
 usx_dir(mgr_id, acctab, _protocol, usudp_new_clts, usudp_new_cots)
{
}


usudp_dir::usudp_dir()
:
	usx_dir()
{}

#ifdef	GXXBUG_VIRTUAL1
char* usudp_dir::remote_class_name() const
	{ return net_dir_base::remote_class_name(); }
#endif	GXXBUG_VIRTUAL1


/*
 * Standard name service interface.
 */

mach_error_t 
usudp_dir::ns_list_types(ns_type_t		**types,	/* out */
			 int			*count)		/* out */
{
	mach_error_t		ret;
	vm_address_t		data;

	*count = 2;

	/*
	 * Get space for the reply.
	 */
	data = NULL;
	ret = vm_allocate(mach_task_self(),&data,*count * sizeof(ns_type_t),TRUE);
	if (ret != KERN_SUCCESS) {
		*count = 0;
		*types = NULL;
		return(ret);
	}

	/*
	 * Prepare the reply.
	 */
	((ns_type_t *)data)[0] = NST_CLTS_RECS;
	((ns_type_t *)data)[1] = NST_COTS_RECS;

	*types = (ns_type_t *)data;

	return(NS_SUCCESS);
}

/*
 * x-kernel protocol handler methods.
 */

int 
usudp_dir::usx_demux_internal(XObj			s,
			      Msg			*msg)
{
	usx_endpt_base		*endpt;
	IPPaddr			ipp_addr;
	xkern_return_t		xret;
	Bind			bret;
	int			ret;

#define	ABORT {				\
	/* xClose(s); XXX */		\
	return(-1);			\
}
//	msgDestroy(msg);

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	xret = mapResolve(Local(demux_map),&s,&endpt);
	if (xret == XK_FAILURE) {
#if 0
		if (x_errno != CANNOT_RESOLVE) {
			us_internal_error(
				"usx_demux_internal.mapResolve(demux_map)",
				convert_xkernel_error(x_errno));
			ABORT;
		}
#endif
		/*
		 * Could not find a corresponding endpoint in the
		 * primary demux map. This may happen if a mapping
		 * for a COTS endpoint, which overrides any CLTS
		 * mapping, is removed while a CLTS endpoint is
		 * active with the same local address.
		 *
		 * Look for a CLTS endpoint with the appropriate
		 * local address in the openenable map.
		 */
		xret = usx_get_IPPaddr(s,LOCAL_PART, &ipp_addr);
		if (xret == XK_FAILURE) {
			us_internal_error(
				"usx_demux_internal.usx_get_IPPaddr",
				convert_xkernel_error(x_errno));
			ABORT;
		}
		xret = mapResolve(Local(openenable_map),
							&ipp_addr, &endpt);
		if (xret == XK_FAILURE) {
			char			errmsg[200];

			sprintf(errmsg,
		"usx_demux_internal cannot find endpoint for 0x%x,0x%x",
						ipp_addr.host,ipp_addr.port);
			us_internal_error(errmsg,
					convert_xkernel_error(x_errno));
			ABORT;
		}

		bret = mapBind(Local(demux_map),&s,endpt);
		if (bret == ERR_BIND) {
			us_internal_error(
				"usx_demux_internal.mapBind(demux_map)",
				convert_xkernel_error(x_errno));
			ABORT;
		}
	}

	/*
	 * No need to get a reference for endpt, since we are protected
	 * by the XKERNEL_MASTER lock.
	 */

	return(endpt->usx_pop_internal(s,msg));

#undef	ABORT
}

/*
 * Front-ends for x-kernel methods, called from the endpoints.
 */

mach_error_t 
usudp_dir::usx_open_internal(usx_endpt_base*	endpt,
			     boolean_t		cots,
			     Part		*participants,
			     XObj		*sessn)		/* OUT */
{
	mach_error_t		ret;
	xkern_return_t		xret;
	Bind			bret;
	usx_endpt_base*		old_clts;


	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	/*
	 * First open the session.
	 */
	ret = usx_dir::usx_open_internal(endpt, cots, participants, sessn);
	if (ERR_SUCCESS != ret) {
		us_internal_error("ustcp_dir::usx_open_internal.xOpen",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	/*
	 * Enter the (session,endpoint) in the demux map.
	 *
	 * No need to get a new reference to endpt, because it is
	 * assumed that the endpt is already known in this directory,
	 * and will be correctly removed at GC time.
	 */

	bret = mapBind(Local(demux_map),sessn,endpt);
	if (bret == ERR_BIND) {
#if 0
		if (x_errno != INCONSISTENT_BIND) {
			us_internal_error(
				"usx_open_internal.mapBind(demux_map)",
				convert_xkernel_error(x_errno));
			return(US_INTERNAL_ERROR);
		}
#endif
		/*
		 * Binding in the primary demux map failed. See if we should
		 * overwrite a secondary (CLTS) entry with a primary (COTS)
		 * entry.
		 *
		 * Note: for any session, there may be at most one COTS and
		 * one CLTS entry.
		 */
		if (cots) {
			/*
			 * Find old CLTS entry.
			 */
			xret = mapResolve(
					Local(demux_map),sessn,&old_clts);
			if (xret == XK_FAILURE) {
				us_internal_error(
				"usx_open_internal cannot find old CLTS",
					convert_xkernel_error(x_errno));
				return(US_INTERNAL_ERROR);
			}

			/*
			 * Remove CLTS entry from primary map.
			 */
			xret = mapUnbind(Local(demux_map),sessn);
			if (xret == XK_FAILURE) {
				us_internal_error(
		"usx_open_internal cannot unbind old CLTS in primary map",
					convert_xkernel_error(x_errno));
				return(US_INTERNAL_ERROR);
			}

			/*
			 * Enter COTS entry in primary map.
			 */
			bret = mapBind(Local(demux_map),sessn,endpt);
			if (bret == ERR_BIND) {
				us_internal_error(
		"usx_open_internal cannot bind new COTS in primary map",
					convert_xkernel_error(x_errno));
				return(US_INTERNAL_ERROR);
			}

#if	USE_SECONDARY_DEMUX_MAP
			/*
			 * Enter CLTS entry in seconday map.
			 */
			bret = mapBind(Local(secondary_demux_map),
							sessn,old_clts);
			if (bret == ERR_BIND) {
				us_internal_error(
		"usx_open_internal cannot bind old CLTS in secondary map",
					convert_xkernel_error(x_errno));
				return(US_INTERNAL_ERROR);
			}
#endif	USE_SECONDARY_DEMUX_MAP

		} else {
			/*
			 * There is already a COTS entry. Let it be.
			 */

#if	USE_SECONDARY_DEMUX_MAP
			bret = mapBind(Local(secondary_demux_map),
								sessn,endpt);
			if (bret == ERR_BIND) {
				us_internal_error(
		"usx_open_internal cannot bind new CLTS in secondary map",
					convert_xkernel_error(x_errno));
				return(US_INTERNAL_ERROR);
			}
#endif	USE_SECONDARY_DEMUX_MAP

		}
	}

	return(ERR_SUCCESS);
}

mach_error_t 
usudp_dir::usx_close_internal(usx_endpt_base*	endpt,
			      boolean_t		cots,
			      XObj		sessn)
{
	xkern_return_t		xret;
	Bind			bret;
	mach_error_t		ret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */

	/*
	 * Remove the mapping from the demux map.
	 */
	xret = mapUnbind(Local(demux_map),&sessn);

	/*
	 * Close the sessn.
	 */
	ret = usx_dir::usx_close_internal(endpt, cots, sessn);
	if (ERR_SUCCESS != ret) {
		return(ret);
	}


	/* What if the unbind failed ? */
	if (xret == XK_FAILURE) {
		if (cots) {
			/*
			 * Could not find a COTS entry in the primary map.
			 * This cannot happen.
			 */
			us_internal_error(
			"usx_close_internal cannot unbind COTS in primary map",
					convert_xkernel_error(x_errno));
			return(US_INTERNAL_ERROR);
		} else {
			/*
			 * Could not find a CLTS entry in the primary map.
			 * This could happen if the CLTS entry was
			 * displaced by a COTS.
			 */

#if	USE_SECONDARY_DEMUX_MAP
			/*
			 * Look for the entry in the secondary map.
			 */
			bret = mapUnbind(Local(secondary_demux_map),
								&sessn);
			if (bret == ERR_BIND) {
				us_internal_error(
	"usx_close_internal cannot unbind CLTS in secondary map",
					convert_xkernel_error(x_errno));
				return(US_INTERNAL_ERROR);
			}
#endif	USE_SECONDARY_DEMUX_MAP

		}

#if	USE_SECONDARY_DEMUX_MAP
	} else {
		if (cots) {
			/*
			 * We have successfully removed a COTS entry from
			 * the primary map.
			 *
			 * Check if there is a CLTS entry in the secondary
			 * map, to be moved back to the primary map.
			 */
			xret = mapUnbind(Local(secondary_demux_map),
								&sessn);
			if (xret == XK_FAILURE) {
				/*
				 * No CLTS entry.
				 */
				return(ERR_SUCCESS);
			}

			bret = mapBind(Local(demux_map),&sessn,endpt);
			if (bret ==  ERR_BIND) {
				us_internal_error(
	"usx_close_internal cannot rebind old CLTS in primary map",
					convert_xkernel_error(x_errno));
				return(US_INTERNAL_ERROR);
			}
		} else {
			/*
			 * We have successfully removed a CLTS entry from
			 * the primary map.
			 *
			 * There is nothing more to do.
			 */
		}
#endif	USE_SECONDARY_DEMUX_MAP

	}

	return(ERR_SUCCESS);
}
