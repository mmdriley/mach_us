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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_internal.h,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: internal declarations for the x-kernel multi-server interface.
 *
 * HISTORY
 * $Log:	usx_internal.h,v $
 * Revision 2.3  94/07/13  18:07:07  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:30  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  12:55:13  jms]
 * 
 * Revision 2.3  91/11/06  14:15:00  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:10:04  pjg]
 * 
 * Revision 2.2  91/05/05  19:31:13  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:54  dpj]
 * 
 * 	First really working version.
 * 	[91/04/28  10:51:18  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:49:17  dpj]
 * 
 */

#ifndef	_usx_internal_h
#define	_usx_internal_h


extern "C" {
#include	<io_types.h>
#include	<io_types2.h>
#include	<net_types.h>

#define this _this
#include	"upi.h"
#include	"process.h"
#undef this
}

extern boolean_t usx_debug;

/************************************************************************\
 *									*
 *		Macros xkernel interface KP				*
 *									*
\************************************************************************/
#define XKERNEL_MASTER() MASTER_LOCK
#define XKERNEL_RELEASE() MASTER_UNLOCK
extern mutex_t sledgehammer_concurrency_control; /* XXX fix in process.h */

/************************************************************************\
 *									*
 *		Macros for address management				*
 *									*
\************************************************************************/

#define	USX_CVT_NETADDR(netaddr,ipp_addr) {				\
	(ipp_addr)->host = * (IPhost *) &net_addr_inet_get_ipaddr(netaddr);\
	(ipp_addr)->port = (unsigned long)(ntohs(net_addr_inet_get_port(netaddr)));		\
}

#define	USX_CVT_IPPADDR(ipp_addr,netaddr) { \
	net_addr_inet_init_default((netaddr)); \
	net_addr_inet_set_ipaddr((netaddr),*(ipaddr_t*)&((ipp_addr).host)); \
	net_addr_inet_set_port((netaddr),htons((unsigned short)((ipp_addr).port))); \
}


/************************************************************************\
 *									*
 *		Record information					*
 *									*
\************************************************************************/

typedef struct usx_recinfo {
	io_recinfo_t		base;	
	net_addr_t		addr;
	net_options_t		options;
} usx_recinfo_t;

extern io_recinfo_ops_t		usx_recinfo_ops;

#define	usx_recinfo_init(_ri,_addr,_options)				\
MACRO_BEGIN								\
	io_recinfo_init(((usx_recinfo_t *)(_ri)),&usx_recinfo_ops);	\
	net_addr_copy((_addr),&((usx_recinfo_t *)(_ri))->addr);		\
	net_options_copy((_options),&((usx_recinfo_t *)(_ri))->options);\
MACRO_END

/*
 * An IP addr with a port.  Used for for both TCP and UDP xkern addrs.
 */
typedef struct ippaddr {
	unsigned int port;
	IPhost	host;
} IPPaddr;

/************************************************************************\
 *									*
 *		Misc Utility Stuff					*
 *									*
\************************************************************************/


/*
 * usx_get_IPPaddr:  Given a session, and a flavor, get the appropriate IPPaddr
 *			Flavor should be either (LOCAL_PART, REMOTE_PART)
 */
xkern_return_t usx_get_IPPaddr(XObj lls, int flavor, IPPaddr *out_addr);

/************************************************************************\
 *									*
 *		XXX temp hacks to make stuff work
 *									*
\************************************************************************/

extern int x_errno;	/* XXX doesn't exist in most recent version but don't want to pull code references yet */

#endif	_usx_internal_h
