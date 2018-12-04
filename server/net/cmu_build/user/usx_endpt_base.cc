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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_endpt_base.cc,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Base class for udp end-points.
 *
 * HISTORY
 * $Log:	usx_endpt_base.cc,v $
 * Revision 2.3  94/07/13  18:06:57  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:25  jms
 * 	Revised with the introduction of common "usx_" logic and xkernel v3.2
 * 	[94/01/10  12:50:12  jms]
 * 
 * Revision 2.2  91/11/06  14:14:20  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:07:56  pjg]
 * 
 */

#include	<usx_endpt_base_ifc.h>

#define BASE net_endpt_base
DEFINE_ABSTRACT_CLASS(usx_endpt_base)

usx_endpt_base::usx_endpt_base(ns_mgr_id_t mgr_id, access_table * acctab)
:
 net_endpt_base(mgr_id, acctab)
{}

