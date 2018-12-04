/* 
 * part.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 23:57:27 $
 */

#include "xtype.h"
#include "part.h"
#include "assert.h"
#include "platform.h"
#include "x_util.h"
#include "x_libc.h"

void
partInit(p, n)
    Part *p;
    int n;
{
    int i;

    for (i=0; i < n; i++) {
	p[i].stack.top = 0;
	p[i].len = n;
    }
}


void
partStackPush(s, data, len)
    PartStack *s;
    VOID *data;
    int	 len;
{
    xAssert( s->top >= 0 );
    if ( s->top >= PART_MAX_STACK ) {
	Kabort("participant stack overflow");
    }
    s->arr[s->top].ptr = data;
    s->arr[s->top].len = len;
    s->top++;
}


VOID *
partStackPop(s)
    PartStack *s;
{
    xAssert( s->top >= 0 );
    if ( s->top == 0 || s->top > PART_MAX_STACK ) {
	return 0;
    }
    return s->arr[--s->top].ptr;
}


/* 
 * Very quick, very dirty ...
 *
 * External participant representation:
 *
 *	number of Participants
 * 		number in part1 stack
 *			stack elem 1 len	stack elements from bottom-to-top
 *			stack elem 1 data
 *			stack elem 2 len	
 *			stack elem 2 data
 *			...
 * 		number in part2 stack
 *			stack elem 1 len	stack elements from bottom-to-top
 *			stack elem 1 data
 *			stack elem 2 len	
 *			stack elem 2 data
 *		...
 */


#define	CHECK_BUF_LEN( _incLen )							\
  	if ( (char *)buf + (_incLen) - (char *)bufStart > (*bufLen) ) {			\
	    return XK_FAILURE;								\
	}		

xkern_return_t
partExternalize( p, buf, bufLen )
    Part	*p;
    VOID	*buf;
    int		*bufLen;	/* length of buffer, number of bytes written */
{
    VOID	*bufStart = buf;
    int		i, j, len, numParts;

    CHECK_BUF_LEN(sizeof(int));
    *(int *)buf = numParts = partLen(p);
    buf += sizeof(int);
    for ( i=0; i < numParts; i++, p++ ) {
	/* 
	 * For each participant
	 */
	CHECK_BUF_LEN(sizeof(int));
	*(int *)buf = p->stack.top;
	buf += sizeof(int);
	for ( j=0; j < p->stack.top; j++ ) {
	    /* 
	     * For each stack element
	     */
	    CHECK_BUF_LEN(sizeof(int));
	    len = p->stack.arr[j].len;
	    *(int *)buf = len;
	    buf += sizeof(int);
	    if ( len ) {
		CHECK_BUF_LEN(len);
		bcopy((char *)p->stack.arr[j].ptr, buf, len);
		buf += len;
	    } else {
		/* 
		 * len == 0 indicates a special-valued pointer which
		 * must be preserved. 
		 */
		CHECK_BUF_LEN(sizeof(VOID *));
		*(VOID **)buf = p->stack.arr[j].ptr;
		buf += sizeof(VOID *);
	    }
	    while ( ! LONG_ALIGNED(buf) ) {
		buf++;
	    }
	}
    }
    return XK_SUCCESS;
}



void
partInternalize( p, buf )
    Part	*p;
    VOID	*buf;
{
    int		i, j, len, numInStack;

    partInit(p, *(int *)buf);
    buf += sizeof(int);
    for ( i=0; i < partLen(p); i++ ) {
	/* 
	 * For each participant
	 */
	numInStack = *(int *)buf;
	buf += sizeof(int);
	for ( j=0; j < numInStack; j++ ) {
	    /* 
	     * For each stack element
	     */
	    len = *(int *)buf;
	    buf += sizeof(int);
	    if ( len ) {
		partPush(p[i], buf, len);
		buf += len;
	    } else {
		partPush(p[i], *(VOID **)buf, 0);
		buf += sizeof(VOID *);
	    }
	    while ( ! LONG_ALIGNED(buf) )
	        buf++;
	}
    }
}


