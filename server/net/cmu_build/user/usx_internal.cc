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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_internal.cc,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: internal declarations for the x-kernel multi-server interface.
 *
 * HISTORY
 * $Log:	usx_internal.cc,v $
 * Revision 2.3  94/07/13  18:07:05  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:29  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  12:52:41  jms]
 * 
 * Revision 2.2  91/11/06  14:14:54  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:09:59  pjg]
 * 
 * Revision 2.2  91/05/05  19:31:11  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:50  dpj]
 * 
 * 	First working version.
 * 	[91/04/28  10:50:47  dpj]
 * 
 *
 */

#ifndef lint
char * usx_internal_rcsid = "$Header: usx_internal.cc,v 2.3 94/07/13 18:07:05 mrt Exp $";
#endif	lint

#include	<usx_internal.h>

extern "C" {
#include	<base.h>

void usx_recinfo_destroy(usx_recinfo_t*);
void usx_recinfo_copy(usx_recinfo_t*, usx_recinfo_t*);
int usx_recinfo_size(usx_recinfo_t*);
}

boolean_t usx_debug = TRUE;

int x_errno = 0;	/* XXX doesn't exist in most recent version but don't want to pull code references yet */

/************************************************************************\
 *									*
 *		Record information					*
 *									*
\************************************************************************/

void usx_recinfo_destroy(usx_recinfo_t	*recinfo)
{
	net_addr_destroy(&recinfo->addr);
	net_options_destroy(&recinfo->options);
}

void usx_recinfo_copy(usx_recinfo_t	*from,
		      usx_recinfo_t	*dest)
{
	io_recinfo_destroy(dest);
	usx_recinfo_init(dest,&from->addr,&from->options);
}

int usx_recinfo_size(usx_recinfo_t	*recinfo)
{
	return(sizeof(usx_recinfo_t));
}

io_recinfo_ops_t usx_recinfo_ops = {
	usx_recinfo_destroy,
	usx_recinfo_copy,
	usx_recinfo_size
};

/************************************************************************\
 *									*
 *		Misc Utility Stuff					*
 *									*
\************************************************************************/

/*
 * usx_get_IPPaddr:  Given a session, and a flavor, get the appropriate IPPaddr
 *			Flavor should be either (LOCAL_PART, REMOTE_PART)
 */
xkern_return_t usx_get_IPPaddr(XObj lls, int flavor, IPPaddr *out_addr)
{
	Part	ippParts[2];
	Part	ipParts[2];
	char	ipParts_buf[sizeof(ipParts)];
	int	ret;

	partInit(ippParts, 2);
	ret = xControl(lls, GETPARTICIPANTS, (void *)ippParts, sizeof(ippParts));
	if (ret < 1) {
		return(XK_FAILURE);
	}

	ret = xControl(xGetDown(lls, 0), GETPARTICIPANTS, (void *)ipParts_buf, sizeof(ipParts_buf));
	if (ret < 1) {
		return(XK_FAILURE);
	}
	partInit(ipParts, 2);
	partInternalize(ipParts, ipParts_buf);

	out_addr->port = *(unsigned int *) partPop(ippParts[flavor]);
	out_addr->host = *(IPhost *) partPop(ipParts[flavor]);
	return(XK_SUCCESS);
}

