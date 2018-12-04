/* 
 * platform.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.28 $
 * $Date: 1993/02/01 23:53:12 $
 */

/*
 * "sunos" (sparc simulator) version of platform.h
 */

#ifndef platform_h
#define platform_h

#include "xk_debug.h"

#if defined(__GNUC__) && ! defined(XK_DEBUG)
#  define XK_USE_INLINE
#endif

#include <sys/types.h>
#include <netinet/in.h>
#include "process.h"
#include "msg_s.h"
#include "xtype.h"


char *strcpy();
int free();

#ifdef __STDC__

u_short ocsum( u_short *buf, int count );
u_short inCkSum( Msg *m, u_short *buf, int len );
void sock2simEth( char *ethAddr, struct in_addr inAddr, int udpPort );
int	atoi( char * );

int	CreateKernelProcess();

extern	char *	xMalloc( unsigned );

#else

extern	char *	xMalloc( );

#endif

#define xFree(buf)  (free(buf) ? 0 : -1)

typedef long *Unspec;
typedef unsigned long ProcessId;
typedef unsigned long ContextId;

#ifndef NULL
#define NULL	0
#endif
#define MAXUNSIGNED	((unsigned) (-1)

#define splx(x)
#define spl7() 1
#define splnet spl7
#define splclk spl7

#define INIT_STACK_SIZE 1024

#define BYTES_PER_WORD	4

#define CLICKS_PER_SEC 100	/* Clock interrupts per second */

#define ROUNDUP(A, B)   ((((Bit32) (A)) + ((B) - 1)) & (~((B)-1)))
#define ROUNDDOWN(A, B)  (((Bit32) (A)) & (~((B)-1)))
#define BETWEEN(A,B,C) ((A) <= (B) && (B) < (C))

typedef	char	*mapKeyPtr_t;
typedef	int	mapVal_t;

typedef char	*statePtr_t;

#ifndef	TRUE
#define	TRUE	1
#define FALSE	0
#endif

#define	SUCCESS_RET_VAL		0
#define	FAILURE_RET_VAL		(-1)

#define	LO_BYTE_OF_2(word)	 ((u_char) (0xff & (unsigned) word))
#define HI_BYTE_OF_2(word)	 ((u_char) (((unsigned) word) >> 8 ))
#define CAT_BYTES(hiByte,loByte) ((((unsigned)hiByte)<<8) + (unsigned)loByte)

/* 
 * LONG_ALIGNMENT_REQUIRED indicates that int's must be 32-bit aligned
 * on this architecture.
 */
#define LONG_ALIGNMENT_REQUIRED
#define LONG_ALIGNED( address )  (! ((int)(address) & 0x3))
#define SHORT_ALIGNED( address ) ( ! ((int)(address) & 0x1))

#define ROM_MAX_LINES 	100 	/* Up to 100 table entries */
#define ROM_MAX_FIELDS 	20

extern	char *rom[ROM_MAX_LINES + 1][ROM_MAX_FIELDS + 1];  


#endif
