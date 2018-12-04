/* 
 * xk_debug.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.26 $
 * $Date: 1993/02/01 22:39:59 $
 */

#ifndef xk_debug_h
#define xk_debug_h

#include "platform.h"
#include "x_stdio.h"
#include "trace.h"

extern int
	tracebuserror,
	tracecustom,
  	traceethdrv,
#ifdef XKMACHKERNEL
 	tracelance,
#endif XKMACHKERNEL
 	traceether,
	traceevent,
	tracefixme,
	traceidle,
  	traceidmap,
	traceie,
	traceinit,  
	tracememoryinit,
	tracemsg,  
	tracenetmask,
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

extern	char errBuf[];


#if ! defined(XK_DEBUG)
#    define NDEBUG
#endif


#ifdef __STDC__

void	xTraceLock( void );
void	xTraceUnlock( void );
void	xTraceInit( void );

#else

void	xTraceLock();
void	xTraceUnlock();
void	xTraceInit();

#endif


#if defined(XK_DEBUG) || defined(lint)

#define PRETRACE(l) { int __i=l; xTraceLock();  while(__i--) putchar(' '); }
#define POSTTRACE { putchar('\n'); xTraceUnlock(); }

#ifdef __STDC__
#define XPASTE(X,Y) X##Y
#define PASTE(X,Y) XPASTE(X,Y)


#   define xIfTrace(t, l) \
	if (PASTE(trace,t) >= l)
#   define xTrace0(t, l, f) 		\
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f); 		\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#   define xTrace1(t, l, f, arg1) 	\
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#   define xTrace2(t, l, f, arg1, arg2) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#   define xTrace3(t, l, f, arg1, arg2, arg3) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3, arg4);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3, arg4, arg5); \
	    	POSTTRACE;		\
	    }				\
	} while (0)
#else
#define D___I(X) X


#   define xIfTrace(t, l) \
	if (D___I(trace)t >= l)
#   define xTrace0(t, l, f) 			\
	do {					\
	    if (D___I(trace)t >= l) { 		\
	    	PRETRACE(l); 			\
	    	printf(f); 			\
	    	POSTTRACE; 			\
	    }					\
	} while (0)
#   define xTrace1(t, l, f, arg1) \
	do {					\
	    if (D___I(trace)t >= l) { 		\
	    	PRETRACE(l); 			\
		printf(f, arg1); 		\
	    	POSTTRACE; 			\
	    }					\
	} while (0)
#   define xTrace2(t, l, f, arg1, arg2) \
	do {					\
	    if (D___I(trace)t >= l) { 		\
	    	PRETRACE(l); 			\
	    	printf(f, arg1, arg2); 		\
	    	POSTTRACE; 			\
	    }					\
	} while (0)
#   define xTrace3(t, l, f, arg1, arg2, arg3) \
	do {					\
	    if (D___I(trace)t >= l) { 		\
	    	PRETRACE(l); 			\
	    	printf(f, arg1, arg2, arg3); 	\
	    	POSTTRACE; 			\
	    }					\
	} while (0)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	do {					\
	    if (D___I(trace)t >= l) { 		\
	    	PRETRACE(l); 			\
	    	printf(f, arg1, arg2, arg3, arg4); \
	    	POSTTRACE; 			\
	    }					\
	} while (0)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	do {					\
	    if (D___I(trace)t >= l) { 		\
	    	PRETRACE(l); 			\
	    	printf(f, arg1, arg2, arg3, arg4, arg5); \
	    	POSTTRACE; 			\
	    }					\
	} while (0)

#endif /* __STDC__ */
#else

#   define xIfTrace(t, l) if (0)
#   define xTrace0(t, l, f)
#   define xTrace1(t, l, f, arg1)
#   define xTrace2(t, l, f, arg1, arg2)
#   define xTrace3(t, l, f, arg1, arg2, arg3)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5)

#endif	/* XK_DEBUG */

extern void	xError(
#ifdef __STDC__
		       char *
#endif
		       );

#endif	/* xk_debug_h */
