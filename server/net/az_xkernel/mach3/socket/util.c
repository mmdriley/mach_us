/*
 * $RCSfile: util.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:09:17 $
 * $Author: menze $
 *
 * $Log: util.c,v $
 * Revision 1.3  1993/02/02  00:09:17  menze
 * copyright change
 *
 * Revision 1.2  1993/01/26  08:09:43  menze
 * Added some include files
 *
 * Revision 1.1  1992/12/01  22:13:19  menze
 * Initial revision
 *
 */
#include <mach/mach_host.h>
#include "xk_mach.h"
#include "util.h"

void
fixed_priority_scheduling(int priority)
{
    kern_return_t kr;
    processor_set_t set;
    processor_set_name_t name;

    /* a priority of 0 is interpreted as no fixed priority scheduling: */
    if (!priority) {
	return;
    } /* if */

    kr = thread_get_assignment(mach_thread_self(), &name);
    if (kr != KERN_SUCCESS) {
	quit(1, "fixed_priority_scheduling: thread_get_assignment(): %s\n",
	     mach_error_string(kr));
    } /* if */

    kr = host_processor_set_priv(mach_host_priv_self(), name, &set);
    if (kr != KERN_SUCCESS) {
	quit(1, "fixed_priority_scheduling: host_processor_set_priv(): %s\n",
	     mach_error_string(kr));
    } /* if */

    kr = processor_set_policy_enable(set, POLICY_FIXEDPRI);
    if (kr != KERN_SUCCESS) {
	quit(1,
	     "fixed_priority_scheduling: processor_set_policy_enable(): %s\n",
	     mach_error_string(kr));
    } /* if */

    kr = thread_policy(mach_thread_self(), POLICY_FIXEDPRI, priority);
    if (kr != KERN_SUCCESS) {
	quit(1, "fixed_priority_scheduling: thread_policy: %s\n",
	     mach_error_string(kr));
    } /* if */
} /* fixed_priority_scheduling */

			/*** end of util.c ***/
