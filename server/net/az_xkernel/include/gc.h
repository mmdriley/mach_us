/*
 * gc.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:38:54 $
 */


#ifndef gc_h
#define gc_h

#include "xkernel.h"
#include "idmap.h"

/*
 * initSessionCollector -- start a session garbage collector to run every
 * 'interval' microseconds, collecting  idle sessions on map 'm'
 * (an idle session is one whose ref count is zero.)  A session is collected
 * by calling the function 'destructor'.   Protocols mark a
 * session as 'non-idle' by clearing it's 'idle' field.
 *
 * 'msg' is a string used in trace statements to identify the collector.
 * (try to use a string unique to the map, such as a protocol name)
 * A null pointer is interpreted as an empty string.
 *
 * There is currently no way to shut down a session collector.
 */

void	initSessionCollector(
#ifdef __STDC__
			     Map m, int interval, Pfv destructor, char *msg
#endif
			     );


#endif	gc_h
