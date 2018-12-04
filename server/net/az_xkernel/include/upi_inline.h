/* 
 * upi_inline.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/01 22:39:24 $
 */

#ifndef upi_inline_h
#define upi_inline_h

#include "platform.h"

/*
 * definitions of xPop and xCallPop
 *
 * If XK_USE_INLINE is defined, these are inline functions.  If not,
 * these are the prototypes for regular functions.  One source file
 * should define UPI_INLINE_INSTANTIATE before including this file.
 * This will cause the actual functions to be instantiated in that
 * source file.
 */

#ifdef XK_USE_INLINE
#  define FUNC_TYPE static __inline__ xkern_return_t
#else
#  define FUNC_TYPE xkern_return_t

#  ifdef __STDC__

FUNC_TYPE	xPop( XObj, XObj, Msg *, void * );
FUNC_TYPE	xCallPop( XObj, XObj, Msg *, void *, Msg * );

#  else 

FUNC_TYPE	xPop();
FUNC_TYPE	xCallPop();

#  endif __STDC__
#endif XK_USE_INLINE


#if defined(XK_USE_INLINE) || defined(UPI_INLINE_INSTANTIATE)

/*
 * OP_COUNTS controls whether session reference counts are raised and
 * lowered around each operation in order to count the number of
 * operations "outstanding" on that session
 */
#define OP_COUNTS	



#ifdef OP_COUNTS
#define INC_REF_COUNT(sessn, func) 					\
  {									\
    (sessn)->rcnt++;							\
    xTrace4(protocol, 5, "%s increased ref count of %x[%s] to %d",	\
	    (func), sessn, (sessn)->myprotl->name, (sessn)->rcnt);	\
  }
#else
#define INC_REF_COUNT(s, func) 1
#endif
  	

#define DEC_REF_COUNT_UNCOND(sessn, func)				\
  {									\
    if (--(sessn)->rcnt <= 0) {						\
	xTrace4(protocol, 5,						\
		"%s -- ref count of %x[%s] is %d, calling close",	\
		(func), s, (sessn)->myprotl->name, (sessn)->rcnt);	\
	(*(sessn)->close)(sessn);					\
    } else {								\
	xTrace4(protocol, 5,						\
		"%s -- decreased ref count of %x[%s] to %d",		\
		(func), s, (sessn)->myprotl->name, (sessn)->rcnt);	\
    }									\
  }


#ifdef OP_COUNTS
#define DEC_REF_COUNT(s, func) DEC_REF_COUNT_UNCOND((s), (func))
#else
#define DEC_REF_COUNT(s, func) 1
#endif  


FUNC_TYPE
xPop(s, ds, msg, hdr)
    XObj 	s;
    XObj 	ds;
    Msg 	*msg;
    VOID	*hdr;
{
    xkern_return_t retVal;
    
    xAssert(!(int)ds || xIsXObj(ds));
    INC_REF_COUNT(s, "xPop");
    xTrace2(protocol, 3, "Calling pop[%s], %d bytes", s->myprotl->name,
	    msgLen(msg));
    retVal = (*s->pop)(s, ds, msg, hdr);
    DEC_REF_COUNT(s, "xPop");
    return retVal;
}


FUNC_TYPE
xCallPop(s, ds, msg, hdr, replyMsg)
    XObj 	s;
    XObj 	ds;
    Msg 	*msg;
    Msg 	*replyMsg;
    VOID	*hdr;
{
    xkern_return_t retVal;
    
    xAssert(!(int)ds || xIsXObj(ds));
    INC_REF_COUNT(s, "xCallPop");
    xTrace2(protocol, 3, "Calling callpop[%s], %d bytes", s->myprotl->name,
	    msgLen(msg));
    retVal =  (*s->callpop)(s, ds, msg, hdr, replyMsg);
    xTrace2(protocol, 3, "callpop[%s] returns %d bytes", s->myprotl->name,
	    msgLen(replyMsg));
    DEC_REF_COUNT(s, "xCallPop");
    return retVal;
}

#endif defined(XK_USE_INLINE) || defined(UPI_INLINE_INSTANTIATE)

#endif	! upi_inline_h

