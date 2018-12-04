/*     
 * shepherd.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/02 00:02:56 $
 */

#include <mach/kern_return.h>

enum xkShepType_e {  XK_SHEP_BLOCKING, XK_SHEP_NONBLOCKING  };
typedef enum xkShepType_e xkShepType_t;


/*
 * Initialize a pool of shepherd threads
 * 
 * Start up count threads
 *
 */
kern_return_t
#ifdef __STDC__
shepInit( int count );
#else
shepInit();
#endif __STDC__

/*
 * Invoke a shepherd thread
 *
 * Invoke a shepherd thread of type type requesting it to
 *  run (*func)( arg )
 *
 * type == XK_SHEP_NONBLOCKING
 *   non-blocking, non-queuing, failure implies no available threads
 *     
 * type == XK_SHEP_BLOCKING
 *   blocking, queuing, failure implies kernel problems
 *     
 * The Mach ccom dies if arg is void *
 *
 */
kern_return_t
#ifdef __STDC__
xInvokeShepherd(xkShepType_t type, void (*func)( char * ), char *arg  );
#else
xInvokeShepherd();
#endif __STDC__

