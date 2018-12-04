/* 
 * blast_stack.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:21:34 $
 */


typedef struct stack {
    VOID	**a;
    int		max;
    int		top;	/* First available entry.  0 => empty stack */
} *Stack;


#ifdef __STDC__

Stack	stackCreate( int size );
VOID *	stackPop( Stack );
int	stackPush( Stack, VOID *);
void	stackDestroy( Stack );

#else 

Stack	stackCreate();
VOID * 	stackPop();
int	stackPush();
void	stackDestroy();

#endif
