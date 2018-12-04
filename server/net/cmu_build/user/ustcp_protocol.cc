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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/ustcp_protocol.cc,v $
 *
 * Purpose: x-kernel protocol for TCP endpoints in the multi-server system.
 *
 * HISTORY
 * $Log:	ustcp_protocol.cc,v $
 * Revision 2.4  94/07/13  18:06:32  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:42  jms
 * 	Correct "opendone" forward decl return type
 * 	[94/04/29  11:47:53  jms]
 * 
 * Revision 2.2  94/01/11  18:09:59  jms
 * 	Initial Version
 * 	[94/01/10  11:46:49  jms]
 * 
 *
 */

#include	<ustcp_dir_ifc.h>
#include	<usx_internal.h>


extern "C" {
void ustcp_init(XObj);
void ustcp_install_mgr(XObj, usTop*);
xkern_return_t  ustcp_demux(XObj, XObj, Msg *);
xkern_return_t ustcp_opendone(XObj, XObj, XObj, XObj);
void ustcp_getproc(XObj, XObjType);
}

void ustcp_install_mgr(XObj		self,
		       usTop		*mgr)
{
	mach_object_reference(mgr);
	self->state = (char *) mgr;
}

xkern_return_t ustcp_demux(XObj	xself,
		XObj	s,
		Msg	*msg)
{
	ustcp_dir *dir_obj = ustcp_dir::castdown((usTop*)xself->state);

	DEBUG1(usx_debug, (0, "ustcp_demux: sess 0x%x\n", s));

#if	PARANOID
	if (dir_obj == NULL) return(-1);
#endif	PARANOID

	return(dir_obj->usx_demux_internal(s,msg));
}


xkern_return_t ustcp_opendone(XObj		xself,
		    XObj			lls,
		    XObj			llp,
		    XObj			hlpType)
{
	ustcp_dir *dir_obj;
	usx_endpt_base *base_obj;

	DEBUG1(usx_debug, (0, "ustcp_opendone: sess 0x%x\n", lls));

	dir_obj = ustcp_dir::castdown((usTop*)xself->state);
	if (dir_obj) {
		return(dir_obj->usx_opendone_internal(lls, llp, hlpType));
	}
	/* XXX castdown failure should not be used to determine obj type */

	base_obj = usx_endpt_base::castdown((usTop*)xself->state);
	if (base_obj) {
		return(base_obj->usx_opendone_internal(lls,llp,hlpType));
	}
	return (XK_FAILURE);
}

xkern_return_t ustcp_closedone(XObj		sess)

{
	ustcp_dir *dir_obj;

	DEBUG1(usx_debug, (0, "ustcp_closedone: sess 0x%x\n", sess));

	/* XXX there shuld have ben a xself arg to closedone */
	dir_obj = ustcp_dir::castdown((usTop*)(xMyProtl(xGetUp(sess))->state));
	if (dir_obj) {
		return(dir_obj->usx_closedone_internal(sess));
	}
	return (XK_FAILURE);
}

void ustcp_getproc(XObj p,
		   XObjType type)
{
	/* Null Them all first */
	p->open = (Pfo)NULL;
	p->close = (Pfk)NULL;
	p->closedone = (Pfk)NULL;
	p->openenable = (Pfk)NULL;
	p->opendisable = (Pfk)NULL;
	p->opendisableall = (Pfk)NULL;
	p->opendone = (Pfk)NULL;
	p->demux = (Pfk)NULL;
	p->calldemux = (Pfk)NULL;
	p->pop = (Pfk)NULL;
	p->callpop = (Pfk)NULL;
	p->push = (Pfh)NULL;
	p->call = (Pfk)NULL;
	p->control = (Pfi)NULL;
	p->duplicate = (Pfk)NULL;
	p->shutdown = (Pfk)NULL;

	if (type == Protocol) {
		p->opendone = (Pfk) ustcp_opendone;
		p->closedone = (Pfk) ustcp_closedone;
		p->demux = (Pfk) ustcp_demux;
	}
}

void ustcp_init(XObj self)
{
    ustcp_getproc(self, Protocol);
}

