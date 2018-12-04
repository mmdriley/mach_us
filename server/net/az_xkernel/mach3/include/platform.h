/* 
 * platform.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.23 $
 * $Date: 1993/02/02 00:02:43 $
 */

#ifndef platform_h
#define platform_h


#if defined(__GNUC__) && ! defined(XK_DEBUG)
#  define XK_USE_INLINE
#endif

#include <sys/types.h>

#ifndef XKMACHKERNEL
#include <netinet/in.h>
#include <ctype.h>
#endif XKMACHKERNEL

#include "process.h"
#include "msg_s.h"

#ifdef __STDC__

extern	char *	xMalloc( unsigned );
extern  int 	xFree( char * );
extern  void	xMallocInit( void );

#else

extern	char *	xMalloc(  );
extern  int 	xFree(  );
extern  void	xMallocInit();

#endif __STDC__


extern char *strcpy();


#ifdef __STDC__

void	evInit( int );
u_short inCkSum( Msg *m, u_short *buf, int len );
u_short ocsum( u_short *buf, int count );

#else

void	evInit();
u_short inCkSum( );
u_short ocsum(  );

#endif __STDC__


typedef long *Unspec;
typedef unsigned long ProcessId;
typedef unsigned long ContextId;

#ifndef NULL
#define NULL	0
#endif
#define MAXUNSIGNED	((unsigned) (-1)

#ifndef XKMACHKERNEL
#define splx(x)
#define spl7() 1
#define splnet spl7
#define splclk spl7
#endif XKMACHKERNEL

#define INIT_STACK_SIZE 1024

#define BYTES_PER_WORD	4

#define CLICKS_PER_SEC 100	/* Clock interrupts per second *//***/

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

/* Used for numbers? */

#define	LO_BYTE_OF_2(word)	 ((unsigned char) (0xff & (unsigned) word))
#define HI_BYTE_OF_2(word)	 ((unsigned char) (((unsigned) word) >> 8 ))

#define CAT_BYTES(hiByte,loByte) ((((unsigned)hiByte)<<8) + (unsigned)loByte)

#ifndef ENDIAN
/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE	1234		/* least-significant byte first (vax) */
#define	BIG	4321		/* most-significant byte first */
#define	PDP	3412		/* LSB first in word, MSW first in long (pdp) */
#if defined(vax) || defined(pmax)
#define	ENDIAN	LITTLE
#else
#define	ENDIAN	BIG		/* byte order on mc68000, tahoe, most others */
#endif
#endif ENDIAN

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
