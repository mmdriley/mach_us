/* 
 * debug.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/01 22:38:49 $
 */

#include "trace.h"
#ifndef debug_h
extern int
	tracebuserror,
	tracecustom,
  	traceethdrv,
 	traceether,
	traceevent,
	tracefixme,
	traceidle,
	traceie,
	traceinit,  
	tracememoryinit,
	tracemsg,  
	traceprocesscreation,
	traceprocessswitch,
	traceprotocol,
	traceptbl,
  	traceprottest,
	tracetick,
	tracetrap,
  	tracelock,
  	tracespl,
	tracesessngc,
	traceuser;

extern void trace();


#if !defined(NDEBUG) || defined(lint)

#ifdef __STDC__
#define XPASTE(X,Y) X##Y
#define PASTE(X,Y) XPASTE(X,Y)


#   define xIfTrace(t, l) \
	if (PASTE(trace,t) >= l)
#   define xTrace0(t, l, f) \
	if (PASTE(trace,t) >= l) trace(l, f, 0)
#   define xTrace1(t, l, f, arg1) \
	if (PASTE(trace,t) >= l) trace(l, f, 1, arg1)
#   define xTrace2(t, l, f, arg1, arg2) \
	if (PASTE(trace,t) >= l) trace(l, f, 2, arg1, arg2)
#   define xTrace3(t, l, f, arg1, arg2, arg3) \
	if (PASTE(trace,t) >= l) trace(l, f, 3, arg1, arg2, arg3)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	if (PASTE(trace,t) >= l) trace(l, f, 4, arg1, arg2, arg3, arg4)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	if (PASTE(trace,t) >= l) trace(l, f, 5, arg1, arg2, arg3, arg4, arg5)
#else
#define D___I(X) X


#   define xIfTrace(t, l) \
	if (D___I(trace)t >= l)
#   define xTrace0(t, l, f) \
	if (D___I(trace)t >= l) trace(l, f, 0)
#   define xTrace1(t, l, f, arg1) \
	if (D___I(trace)t >= l) trace(l, f, 1, arg1)
#   define xTrace2(t, l, f, arg1, arg2) \
	if (D___I(trace)t >= l) trace(l, f, 2, arg1, arg2)
#   define xTrace3(t, l, f, arg1, arg2, arg3) \
	if (D___I(trace)t >= l) trace(l, f, 3, arg1, arg2, arg3)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	if (D___I(trace)t >= l) trace(l, f, 4, arg1, arg2, arg3, arg4)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	if (D___I(trace)t >= l) trace(l, f, 5, arg1, arg2, arg3, arg4, arg5)

#endif /* __STDC__ */
#else

#   define xIfTrace(t, l) if (0)
#   define xTrace0(t, l, f)
#   define xTrace1(t, l, f, arg1)
#   define xTrace2(t, l, f, arg1, arg2)
#   define xTrace3(t, l, f, arg1, arg2, arg3)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5)

#endif	/* ! NDEBUG */

extern void	xError(
#ifdef __STDC__
		       char *
#endif
		       );

#endif	/* debug_h */
