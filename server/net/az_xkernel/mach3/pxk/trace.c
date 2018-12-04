/*
 * trace.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/02 00:06:00 $
 */

#include "platform.h"
#include "xk_debug.h"
#include "compose.h"
			 

#ifdef XK_TRACE_LOCKING
#  ifdef XKMACHKERNEL
			 
static decl_simple_lock_data(, lock);

#  else

static mutex_t lock;

#  endif
#endif


void
xTraceLock()
{
#ifdef XK_TRACE_LOCKING
#  ifdef XKMACHKERNEL
    simple_lock( &lock );
#  else
    mutex_lock( lock );
#  endif
#endif
}


void
xTraceUnlock()
{
#ifdef XK_TRACE_LOCKING
#  ifdef XKMACHKERNEL
    simple_unlock( &lock );
#  else
    mutex_unlock( lock );
#  endif
#endif
}


void
xTraceInit()
{

#ifdef XK_TRACE_LOCKING
#  ifdef XKMACHKERNEL
    simple_lock_init( &lock );
#  else
    lock = mutex_alloc();
#  endif
#endif

    initTraceLevels();
}

