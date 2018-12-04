/* 
 * trace.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:39:29 $
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

