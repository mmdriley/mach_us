/*
 * $RCSfile: xsi_notify.c,v $
 *
 * Copyright (c) 1992  Arizona Board of Regents
 *
 * $Revision: 1.3 $
 * $Date: 1992/07/22 00:14:07 $
 * $Author: davidm $
 *
 * $Log: xsi_notify.c,v $
 * Revision 1.3  1992/07/22  00:14:07  davidm
 * Various bug fixes and improvements.
 *
 * Revision 1.2  1992/07/03  08:47:48  davidm
 * Dead name notification implemented.
 *
 * Revision 1.1  1992/06/30  21:26:22  davidm
 * Initial revision
 *
 */
#include "xsi.h"
#include "xsi_notify.h"

/*
 * Server functions for the notification interface.  We should only
 * get dead-name notifications.
 */

kern_return_t
do_mach_notify_dead_name(mach_port_t notify, mach_port_t name)
{
    xTrace1(xksocketp, TR_EVENTS, "do_mach_notify_dead_name: client %d died",
	    name);

    bye_bye(name);

    mach_port_destroy(mach_task_self(), name);
    return KERN_SUCCESS;
} /* do_mach_notify_dead_name */

kern_return_t
do_mach_notify_port_deleted(mach_port_t notify, mach_port_t name)
{
    quit(1, "do_mach_notify_port_deleted(name=%d)\n", name);
    return KERN_FAILURE;
} /* do_mach_notify_port_deleted */

kern_return_t
do_mach_notify_msg_accepted(mach_port_t notify, mach_port_t name)
{
    quit(1, "do_mach_notify_msg_accepted(name=%d)\n", name);
    return KERN_FAILURE;
} /* do_mach_notify_msg_accepted */

kern_return_t
do_mach_notify_port_destroyed(mach_port_t notify, mach_port_t name)
{
    quit(1, "do_mach_notify_port_destroyed(name=%d)\n", name);
    return KERN_FAILURE;
} /* do_mach_notify_port_destroyed */

kern_return_t
do_mach_notify_no_senders(mach_port_t notify, mach_port_t name)
{
    quit(1, "do_mach_notify_no_senders(name=%d)\n", name);
    return KERN_FAILURE;
} /* do_mach_notify_no_senders */

kern_return_t
do_mach_notify_send_once(mach_port_t notify, mach_port_t name)
{
    quit(1, "do_mach_notify_send_once(name=%d)\n", name);
    return KERN_FAILURE;
} /* do_mach_notify_send_once */

			/*** end of xsi_notify.c ***/
