/*
 * $RCSfile: xsi_main.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/02 00:08:50 $
 * $Author: menze $
 *
 * $Log: xsi_main.c,v $
 * Revision 1.7  1993/02/02  00:08:50  menze
 * copyright change
 *
 * Revision 1.6  1993/01/26  08:11:50  menze
 * Added include files and prototypes
 *
 * Revision 1.5  1992/12/01  22:17:46  menze
 * *** empty log message ***
 *
 * Revision 1.4  1992/07/24  04:15:03  davidm
 * minor changes
 *
 * Revision 1.3  1992/07/22  00:14:07  davidm
 * Various bug fixes and improvements.
 *
 * Revision 1.2  1992/07/03  08:47:48  davidm
 * Changed to x threads instead of C-threads.
 *
 * Revision 1.1  1992/06/30  21:26:22  davidm
 * Initial revision
 *
 */
#include "xsi.h"
#include "xsi_notify.h"
#include "xsi_bench.h"
#include "util.h"
#include "xk_mach.h"

/*
 * Expected number of sessions.  Should be bigger than the real number
 * of clients active at once to get good hashing performance.
 */
#define NUM_SESSIONS	256

/*
 * Expected number of clients.  Should be bigger than the real number
 * of clients active at once to get good hashing performance.
 */
#define NUM_CLIENTS	64

Map active_map = 0;
Map passive_map = 0;
Map client_map = 0;
Map port2cid_map = 0;

condition_t selectable_event;

mach_port_t service;
mach_port_t notify;

static mach_port_t pset;


static boolean_t
xsi_demux(mach_msg_header_t *request, mach_msg_header_t *reply)
{
    if (request->msgh_local_port == service) {
	/* create an x-Kernel process to process the request: */
	return xsi_server(request, reply);
    } else if (request->msgh_local_port == notify) {
	return notify_server(request, reply);
    } else {
	quit(1, "xsi_demux(): bad local port %x\n",
	     request->msgh_local_port);
    } /* if */
    return FALSE;	/* to keep gcc happy... */
} /* xsi_demux */


static void
server_thread(Event ev, void *arg)
{
    kern_return_t kr;

    fixed_priority_scheduling(xsi_fixed_prio);

    kr = xk_msg_server(xsi_demux, XSI_MAX_MSG_SIZE, pset);
    quit(1, "xk_msg_server(): %s\n", mach_error_string(kr));
} /* server_thread */


void
xsi_init(void)
{
    int i;

    xsi_bench_process_options();

    if (xsi_i_am_server || xsi_i_am_client) {
	evDetach(evSchedule(xsi_benchmark, (void*)0, 0));
    } else {
	kern_return_t kr;

	/* allocate a service port: */
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&service);
	if (kr != KERN_SUCCESS) {
	    quit(1, "port_allocate(): %s\n", mach_error_string(kr));
	} /* if */

	/* register x-kernel socket library server with name-server: */
	kr = netname_check_in(name_server_port, XSI_SERVER_NAME,
			      mach_task_self(), service);
	if (kr != KERN_SUCCESS) {
	    quit(1, "netname_check_in(): %s\n", mach_error_string(kr));
	} /* if */

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&notify);
	if (kr != KERN_SUCCESS) {
	    quit(1, "mach_port_allocate(): %s\n", mach_error_string(kr));
	} /* if */

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET,
				&pset);
	if (kr != KERN_SUCCESS) {
	    quit(1, "mach_port_allocate(): %s\n", mach_error_string(kr));
	} /* if */

	kr = mach_port_move_member(mach_task_self(), service, pset);
	if (kr != KERN_SUCCESS) {
	    quit(1, "mach_port_move_member(service): %s\n",
		 mach_error_string(kr));
	} /* if */

	kr = mach_port_move_member(mach_task_self(), notify, pset);
	if (kr != KERN_SUCCESS) {
	    quit(1, "mach_port_move_member(notify): %s\n",
		 mach_error_string(kr));
	} /* if */

	for (i = 0; i < 10; i++) {
	    evDetach(evSchedule(server_thread, (void*)0, 0));
	} /* for */
    } /* if */

    /* create maps: */
    active_map = mapCreate(NUM_SESSIONS, sizeof(XObj));
    passive_map = mapCreate(NUM_SESSIONS, sizeof(conid_t));
    client_map = mapCreate(NUM_CLIENTS, sizeof(int));
    port2cid_map = mapCreate(NUM_CLIENTS, sizeof(mach_port_t));

    selectable_event = condition_alloc();
} /* xsi_init */

			/*** end of xsi_main.c ***/
