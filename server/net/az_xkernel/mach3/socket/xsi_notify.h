/*
 * $RCSfile: xsi_notify.h,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:08:58 $
 * $Author: menze $
 *
 * $Log: xsi_notify.h,v $
 * Revision 1.3  1993/02/02  00:08:58  menze
 * copyright change
 *
 * Revision 1.2  1993/01/26  08:13:08  menze
 * minor tweak to parameter names to avoid shadowing
 *
 * Revision 1.1  1992/07/22  00:14:07  davidm
 * Initial revision
 *
 */
#ifndef xsi_notify_h
#define xsi_notify_h

#include <mach.h>

kern_return_t do_mach_notify_dead_name(mach_port_t notify_p,
				       mach_port_t name);
kern_return_t do_mach_notify_port_deleted(mach_port_t notify_p,
					  mach_port_t name);
kern_return_t do_mach_notify_msg_accepted(mach_port_t notify_p,
					  mach_port_t name);
kern_return_t do_mach_notify_port_destroyed(mach_port_t notify_p,
					    mach_port_t name);
kern_return_t do_mach_notify_no_senders(mach_port_t notify_p,
					mach_port_t name);
kern_return_t do_mach_notify_send_once(mach_port_t notify_p,
				       mach_port_t name);

#endif /* xsi_notify_h */
