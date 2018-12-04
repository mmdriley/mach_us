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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/ustcp_dir.cc,v $
 *
 * Author: J. Mark Stevenson
 *
 * Purpose: Manager for all TCP endpoints supported by the x-kernel.
 *
 * HISTORY
 * $Log:	ustcp_dir.cc,v $
 * Revision 2.3  94/07/13  18:06:24  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:09:56  jms
 * 	Initial Version
 * 	[94/01/10  11:46:18  jms]
 * 
 *
 */

#include	<ustcp_dir_ifc.h>
#include	<ustcp_connector_ifc.h>
#include	<ustcp_cots_ifc.h>

#define BASE usx_dir
DEFINE_CLASS(ustcp_dir)

void ustcp_dir::init_class(usClass* class_obj)
{
	BASE::init_class(class_obj);

	BEGIN_SETUP_METHOD_WITH_ARGS(ustcp_dir);
	SETUP_METHOD_WITH_ARGS(ustcp_dir,ns_list_types);
	END_SETUP_METHOD_WITH_ARGS;
}


ustcp_dir::ustcp_dir(ns_mgr_id_t mgr_id, access_table *acctab, XObj _protocol)
:
 usx_dir(mgr_id, acctab, _protocol, ustcp_new_connector, ustcp_new_cots)
{
	0;
}


ustcp_dir::ustcp_dir()
:
	usx_dir()
{}


#ifdef	GXXBUG_VIRTUAL1
char* ustcp_dir::remote_class_name() const
	{ return net_dir_base::remote_class_name(); }
#endif	GXXBUG_VIRTUAL1


/*
 * Standard name service interface.
 */

mach_error_t 
ustcp_dir::ns_list_types(ns_type_t		**types,	/* out */
			 int			*count)		/* out */
{
	mach_error_t		ret;
	vm_address_t		data;

	*count = 2;

	/*
	 * Get space for the reply.
	 */
	data = NULL;
	DEBUG1(usx_debug, (0, "ustcp_dir::ns_list_types\n"));

	ret = vm_allocate(mach_task_self(),&data,*count * sizeof(ns_type_t),TRUE);
	if (ret != KERN_SUCCESS) {
		*count = 0;
		*types = NULL;
		return(ret);
	}

	/*
	 * Prepare the reply.
	 */
	((ns_type_t *)data)[0] = NST_CONNECTOR;
	((ns_type_t *)data)[1] = NST_COTS_BYTES;

	*types = (ns_type_t *)data;

	return(NS_SUCCESS);
}

/*
 * Front-ends for x-kernel methods, called from the endpoints.
 */

mach_error_t 
ustcp_dir::usx_open_internal(usx_endpt_base*	endpt,
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
	DEBUG1(usx_debug, (0, "ustcp_dir::usx_open_internal\n"));

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
		us_internal_error("ustcp_close_internal.xClose",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	return(ERR_SUCCESS);
}

mach_error_t 
ustcp_dir::usx_close_internal(usx_endpt_base*	endpt,
			      boolean_t		cots,
			      XObj		sessn)
{
	mach_error_t		ret;
	xkern_return_t		xret;

	/* MUST BE CALLED UNDER XKERNEL_MASTER() */
	DEBUG1(usx_debug, (0, "ustcp_dir::usx_close_internal: sessn 0x%x\n", sessn));

	/*
	 * Remove the mapping from the demux map.
	 */
	xret = mapUnbind(Local(demux_map),&sessn);
	if (xret == XK_FAILURE) {
		us_internal_error("ustcp_close_internal.xClose",
					convert_xkernel_error(x_errno));
		return(US_INTERNAL_ERROR);
	}

	/*
	 * Close the sessn.
	 */
	DEBUG1(usx_debug, (0, "ustcp_dir::usx_close_internal\n"));

	ret = usx_dir::usx_close_internal(endpt, cots, sessn);
	if (ERR_SUCCESS != ret) {
		return(ret);
	}


	return(ERR_SUCCESS);
}

