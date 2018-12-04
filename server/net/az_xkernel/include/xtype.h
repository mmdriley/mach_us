/* 
 * xtype.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.11 $
 * $Date: 1993/02/01 22:39:44 $
 */

#ifndef xtype_h
#define xtype_h

#include <sys/types.h>

typedef unsigned int	bool;

/*
 * upi internal error codes
 */
typedef enum xkret { XK_SUCCESS = 0, XK_FAILURE = -1 } xkern_return_t;

typedef int	xmsg_handle_t;
#define XMSG_NULL_HANDLE	0
#define XMSG_ERR_HANDLE		-1
#define XMSG_ERR_WOULDBLOCK	-2

#if defined(__STDC__) || defined(__GNUC__)
#   define VOID		void
#else
#   define VOID		char
#endif

typedef	xkern_return_t (*Pfk) ();
typedef	int (*Pfi) ();
typedef void (*Pfv)();
typedef struct session *(*Pfs)();
typedef struct xobj *(*Pfo)();
typedef xmsg_handle_t (*Pfh)();

#define XK_MAX_HOST_LEN		6

#endif ! xtype_h
