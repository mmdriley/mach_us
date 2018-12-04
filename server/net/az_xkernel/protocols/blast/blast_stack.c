/* 
 * blast_stack.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:21:26 $
 */


#include "xtype.h"
#include "assert.h"
#include "platform.h"
#include "xk_debug.h"
#include "blast_stack.h"

int	tracestack;

Stack
stackCreate( size )
    int	size;
{
    Stack	s;

    xAssert(size >= 0);
    s = (Stack)xMalloc(sizeof(struct stack));
    s->a = (VOID **)xMalloc(size * sizeof(VOID *));
    s->max = size;
    s->top = 0;
    return s;
}


VOID *
stackPop( s )
    Stack	s;
{
    xAssert(s);
    xAssert(s->top >= 0);
    xAssert(s->max >= 0);
    xTrace1(stack, TR_DETAILED, "stack pop -- size == %d", s->top);
    if ( s->top ) {
	xTrace1(stack, TR_DETAILED, "stack pop returning %x", s->a[s->top-1]);
	return s->a[--s->top];
    } else {
	xTrace0(stack, TR_DETAILED, "stack pop -- stack is empty");
	return 0;
    }
}


int
stackPush( s, v )
    Stack	s;
    VOID	*v;
{
    xAssert(s);
    xAssert(s->top >= 0);
    xAssert(s->max >= 0);
    xTrace1(stack, TR_DETAILED, "stack push -- size == %d", s->top);
    if ( s->top >= s->max ) {
	xTrace0(stack, TR_DETAILED, "stack push -- stack is full");
	return -1;
    }
    xTrace1(stack, TR_DETAILED, "stack push storing %x", v);
    s->a[s->top++] = v;
    return 0;
}


void
stackDestroy( s )
    Stack	s;
{
    xAssert(s);
    xAssert(s->a);
    xFree((char *)s->a);
    xFree((char *)s);
}
