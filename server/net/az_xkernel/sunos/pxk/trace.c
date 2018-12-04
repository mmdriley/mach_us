/*
 * trace.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 23:54:26 $
 */

#include "platform.h"
#include "xk_debug.h"
#include "compose.h"
			 
void
xTraceInit()
{
    initTraceLevels();
}

/* 
 * If we start having problems with interleaved trace statements on
 * this platform we'll want to have the traceLocks actually do
 * something.  But for now ...
 */

void
xTraceLock() 
{
}


void
xTraceUnlock()
{
}
