/* 
 * assert.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 23:53:06 $
 */

#ifndef assert_h
#define assert_h

#include <stdio.h>

#ifdef __STDC__

extern int fprintf(FILE *, char *, ...);
extern void abort(void);

#else

extern int fprintf();
extern void abort();

#endif

#define PRINT(A,B,C) fprintf(stderr, (A), (B), (C))
#define assertMessage "Assertion failed: file %s, line %d\n"

# ifdef lint
   extern int assert__x_;
#  define xAssert(ex) (assert__x_ = (ex), assert__x_ = assert__x_ )
#  define _xAssert(ex)(assert__x_ = (ex), assert__x_ = assert__x_ )
# else
#  ifdef XK_DEBUG
#   define _xAssert(ex) ((ex) ? 1 : (PRINT(assertMessage, __FILE__, __LINE__), abort()))
#   define xAssert(ex)  ((ex) ? 1 : (PRINT(assertMessage, __FILE__, __LINE__), abort()))
#  else
#   define _xAssert(ex) {}
#   define xAssert(ex) {}
#  endif XK_DEBUG
# endif lint
#endif assert_h
