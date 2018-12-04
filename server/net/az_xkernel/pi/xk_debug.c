/* 
 * debug.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.16 $
 * $Date: 1993/02/01 23:58:18 $
 */

#include "xk_debug.h"
#include "platform.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#endif XKMACHKERNEL

#ifdef XK_DEBUG

int
  tracebuserror,
  tracecustom,
  traceether,
  traceevent,
  tracefixme,
  traceidle,
  traceie,
  tracememoryinit,
  traceprocesscreation,
  traceprocessswitch,
  traceprotocol,
  traceprottest,
  tracetick,
  tracetrap,
  traceuser;

char	errBuf[250];

char assertMessage[] = "Assertion failed: file %s, line %d\n";


#endif XK_DEBUG

void
xError( msg )
    char	*msg;
{
    xTraceLock();
#ifndef XKMACHKERNEL
    fprintf(stderr, "%s\n", msg);
#else
    printf("%s\n", msg);
#endif XKMACHKERNEL
    xTraceUnlock();
}
