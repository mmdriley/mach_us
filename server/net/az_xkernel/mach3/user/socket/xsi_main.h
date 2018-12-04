/*
 * $RCSfile: xsi_main.h,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/02 00:19:56 $
 * $Author: menze $
 *
 * $Log: xsi_main.h,v $
 * Revision 1.4  1993/02/02  00:19:56  menze
 * copyright change
 *
 * Revision 1.3  1992/12/04  01:16:55  menze
 * debugging flag changed from ! NDEBUG to XK_DEBUG
 *
 * Revision 1.2  1992/08/15  01:19:59  davidm
 * select() totally revised---didn't work when user passed fd_sets which
 * were smaller than sizeof(fd_set); it is common practice (?) to use
 * just an "int" instead of fd_set
 *
 * Support for signal handling added.  If a server call returns EINTR,
 * the library checks the BSD server for pending signals by executing
 * a NOP system call (sigblock(sigmask(SIGKILL))).
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#ifndef xsi_main_h
#define xsi_main_h

#define TRACE_SYSLOG

#include <mach.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/signal.h>
#include "xsi_user.h"

extern mach_port_t xsi_server;
extern mach_port_t alive_port;
extern fd_set is_xsi_fd;
extern int tracexsi;
extern int xsi_cid;

/*
 * Trace support:
 */
#define TR_NEVER		100 /* we'll never use this */
#define TR_FULL_TRACE		25  /* every subroutine entry (sometimes exit, too */
#define TR_DETAILED		 9  /* all functions plus dumps of data structures at strategic points */
#define TR_FUNCTIONAL_TRACE	 7  /* all the functions of the module and their parameters */
#define TR_MORE_EVENTS		 6  /* probably should have used 7, instead */
#define TR_EVENTS		 5  /* more detail than major_events */
#define TR_SOFT_ERRORS		 4  /* mild warning */
#define TR_SOFT_ERROR TR_SOFT_ERRORS
#define TR_MAJOR_EVENTS		 3  /* open, close, etc. */
#define TR_GROSS_EVENTS		 2  /* probably should have used 3, instead */
#define TR_ERRORS		 1  /* serious non-fatal errors, low-level trace (init, closesessn, etc. */
#define TR_ALWAYS		 0  /* normally only used during protocol development */

#if defined(XK_DEBUG) || defined(lint)

#ifdef __STDC__
# define XPASTE(X,Y) X##Y
# define PASTE(X,Y) XPASTE(X,Y)
#else
# define XPASTE(X,Y) X/**/Y
# define PASTE(X,Y) XPASTE(X,Y)
#endif

#ifdef TRACE_SYSLOG

#include <syslog.h>

#define PRETRACE	int old = tracexsi; tracexsi = 0
#define POSTTRACE	tracexsi = old

#   define xIfTrace(t, l) \
	if (PASTE(trace,t) >= l)
#   define xTrace0(t, l, f) 		\
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE;		\
	    	syslog(LOG_NOTICE, f); 		\
		POSTTRACE;		\
	    }				\
	}
#   define xTrace1(t, l, f, arg1) 	\
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE;		\
	    	syslog(LOG_NOTICE, f, arg1);	\
		POSTTRACE;		\
	    }				\
	}
#   define xTrace2(t, l, f, arg1, arg2) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE;		\
	    	syslog(LOG_NOTICE, f, arg1, arg2);	\
		POSTTRACE;		\
	    }				\
	}
#   define xTrace3(t, l, f, arg1, arg2, arg3) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE;		\
	    	syslog(LOG_NOTICE, f, arg1, arg2, arg3);	\
		POSTTRACE;		\
	    }				\
	}
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE;		\
	    	syslog(LOG_NOTICE, f, arg1, arg2, arg3, arg4);	\
		POSTTRACE;		\
	    }				\
	}
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE;		\
	    	syslog(LOG_NOTICE, f, arg1, arg2, arg3, arg4, arg5); \
		POSTTRACE;		\
	    }				\
	}

#else /* TRACE_SYSLOG */

#define PRETRACE(l) { int i=l; while(i--) putchar(' '); }
#define POSTTRACE putchar('\n')

#   define xIfTrace(t, l) \
	if (PASTE(trace,t) >= l)
#   define xTrace0(t, l, f) 		\
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f); 		\
	    	POSTTRACE;		\
	    }				\
	}
#   define xTrace1(t, l, f, arg1) 	\
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1);	\
	    	POSTTRACE;		\
	    }				\
	}
#   define xTrace2(t, l, f, arg1, arg2) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2);	\
	    	POSTTRACE;		\
	    }				\
	}
#   define xTrace3(t, l, f, arg1, arg2, arg3) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3);	\
	    	POSTTRACE;		\
	    }				\
	}
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3, arg4);	\
	    	POSTTRACE;		\
	    }				\
	}
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	{				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3, arg4, arg5); \
	    	POSTTRACE;		\
	    }				\
	}
#endif /* TRACE_SYSLOG */
#else

#   define xIfTrace(t, l) if (0)
#   define xTrace0(t, l, f)
#   define xTrace1(t, l, f, arg1)
#   define xTrace2(t, l, f, arg1, arg2)
#   define xTrace3(t, l, f, arg1, arg2, arg3)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5)

#endif	/* XK_DEBUG */

#define XSI(fun, args) \
({ \
    int res = 0; \
    if (xsi_server) { \
	int stat; \
	int rc = xsi_##fun args; \
 \
	if (rc != KERN_SUCCESS) { \
	    xsi_server = 0; \
	    quit(1, "xsi_%s: %s\n", #fun, mach_error_string(rc)); \
	} else if (stat != ESUCCESS) { \
	    if (errno == EINTR) { \
		sigblock(sigmask(SIGKILL)); /* fetch pending signals */ \
	    } /* if */ \
	    errno = stat; \
	    res = -1; \
	} /* if */ \
    } else { \
	errno = ENETDOWN; \
	res = -1; \
    } /* if */ \
    res; \
})

extern void xsi_setup(void);
extern void xsi_user_init(void);

#endif /* xsi_main_h */
