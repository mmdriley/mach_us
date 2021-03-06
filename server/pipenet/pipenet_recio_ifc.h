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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/pipenet/pipenet_recio_ifc.h,v $
 *
 * Author: Daniel P. Julin
 *
 * Purpose: Defines the record-oriented IO protocol for pipenet objects
 *
 * HISTORY
 * $Log:	pipenet_recio_ifc.h,v $
 * Revision 2.4  94/07/13  17:21:48  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  92/07/05  23:35:17  dpj
 * 	Removed undef of __cplusplus in extern "C" declarations.
 * 	[92/04/17  16:56:34  dpj]
 * 
 * Revision 2.2  91/11/06  14:23:30  jms
 * 	Initial C++ revision.
 * 	[91/09/27  15:55:54  pjg]
 * 
 */

#ifndef	_pipenet_recio_ifc_h
#define	_pipenet_recio_ifc_h

#include	<us_recio_ifc.h>

extern "C" {
#include	<io_types.h>
#include	<io_types2.h>
#include	<net_types.h>
}


class pipenet_recio: public virtual usRecIO {
      public:
	DECLARE_LOCAL_MEMBERS(pipenet_recio);
	virtual mach_error_t pipenet_clts_write1rec_upcall(io_mode_t, char *,
					   unsigned int, io_recnum_t *,
					   net_addr_t *, net_options_t *) =0;
	virtual mach_error_t pipenet_clts_put1rec_upcall(io_mode_t,io_record_t,
					   io_recnum_t *, net_addr_t *,
					   net_options_t *) =0;
};

#endif	_pipenet_recio_ifc_h
