/* 
 * assert.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.12 $
 * $Date: 1993/02/02 00:02:37 $
 */

#ifndef assert_h
#define assert_h

#if ! defined(FILE) && ! defined(XKMACHKERNEL)
#include "x_stdio.h"
#endif

#ifndef XKMACHKERNEL
#  ifdef __STDC__
extern void abort(void);
#  else
extern void abort();
#  endif __STDC__
#  define ABORT abort
#  define PRINT(A,B,C) fprintf(stderr, (A), (B), (C))
#else
#  define PRINT(A,B,C) printf((A), (B), (C))
extern int panic();
#  define ABORT() panic("xkernel assertion")
#endif XKMACHKERNEL

#define assertMessage "Assertion failed: file %s, line %d\n"

#ifdef XK_DEBUG
#   define _xAssert(ex) { if ( ! (ex) ) PRINT(assertMessage, __FILE__, __LINE__), ABORT(); }
#   define xAssert(ex) { if ( ! (ex) ) PRINT(assertMessage, __FILE__, __LINE__), ABORT(); }
#else
#   define _xAssert(ex) {}
#   define xAssert(ex) {}
#endif XK_DEBUG

#endif
