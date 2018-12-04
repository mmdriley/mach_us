/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990 Carnegie Mellon University
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usudp_protocol.cc,v $
 *
 * Purpose: x-kernel protocol for UDP endpoints in the multi-server system.
 *
 * HISTORY
 * $Log:	usudp_protocol.cc,v $
 * Revision 2.4  94/07/13  18:06:49  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  94/05/17  14:09:57  jms
 * 	Correct "opendone" forward decl return type
 * 	[94/04/29  13:35:54  jms]
 * 
 * Revision 2.2  94/01/11  18:10:17  jms
 * 	Massively revised/re-written for xkernel v3.2
 * 	[94/01/10  11:57:24  jms]
 * 
 * Revision 2.3  92/07/05  23:33:59  dpj
 * 	Converted to new C++ RPC package.
 * 	[92/05/10  01:28:22  dpj]
 * 
 * Revision 2.2  91/11/06  14:14:49  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:09:43  pjg]
 * 
 * Revision 2.2  91/05/05  19:31:09  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:46  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:48:59  dpj]
 * 
 *
 */

#include	<usudp_dir_ifc.h>
#include	<usx_internal.h>


extern "C" {
void usudp_init(XObj);
void usudp_install_mgr(XObj, usTop*);
xkern_return_t  usudp_demux(XObj, XObj, Msg *);
xkern_return_t  usudp_opendone(XObj, XObj, XObj, XObj);
void usudp_getproc(XObj, XObjType);
}

void usudp_install_mgr(XObj		self,
		       usTop		*mgr)
{
	mach_object_reference(mgr);
	self->state = (char *) mgr;
}

xkern_return_t usudp_demux(XObj	xself,
		XObj	s,
		Msg	*msg)
{
	usudp_dir *dir_obj = usudp_dir::castdown((usTop*)xself->state);

#if	PARANOID
	if (dir_obj == NULL) return(-1);
#endif	PARANOID

	return(dir_obj->usx_demux_internal(s,msg));
}


xkern_return_t usudp_opendone(XObj		xself,
		    XObj			lls,
		    XObj			llp,
		    XObj			hlpType)
{
	usudp_dir *dir_obj;
	usx_endpt_base *base_obj;

	dir_obj = usudp_dir::castdown((usTop*)xself->state);
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

void usudp_getproc(XObj p,
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
		p->opendone = (Pfk) usudp_opendone;
		p->demux = (Pfk) usudp_demux;
	}
}

void usudp_init(XObj self)
{
    usudp_getproc(self, Protocol);
}

