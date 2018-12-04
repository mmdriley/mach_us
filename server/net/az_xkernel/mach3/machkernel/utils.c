/*
 * utils.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.9 $
 * $Date: 1993/02/02 00:07:37 $
 */

#include <sys/varargs.h>
#include "assert.h"
#include "xk_debug.h"
#define IN_MALLOC_FILE
#include "xk_malloc.h"

int tracebcopy = 0;
int tracemalloc = 0;

#ifdef XK_MEMORY_THROTTLE
int memory_unused = XK_MEMORY_LIMIT;
int tracememoryusage = 0;
#endif XK_MEMORY_THROTTLE

void
Kabort(s)
    char *s;
{
      panic(s);
}


/* forward decl */
#ifdef __STDC__
void bcopy(register unsigned from,
	register unsigned to,
	register unsigned bcount);

void bzero(register unsigned addr,
	register unsigned bcount);
#else
void bcopy();
void bzero();
#endif __STDC__

int *
malloc( size )
   int	size;
{
	int	*where;

	where = (int *)kalloc( size + sizeof( int ) );

	if( where == 0 )
		panic( "malloc -- out of memory in kalloc" );

	*where = size;

	return( &where[ 1 ] );
}


void
free( where )
   int	*where;
{

#ifdef XK_MEMORY_THROTTLE  
        xIfTrace(memoryusage, TR_MAJOR_EVENTS) {
	  if (where[-1] > 4096 || where[-1] <= 0)
	    printf("free: returning %d bytes from %x", where[-1], where);
	}
        memory_unused += where[-1];
        xIfTrace(memoryusage, TR_MAJOR_EVENTS) {
	  if (where[-1] > 4096 || where[-1] <= 0)
	    printf(" new usage  %d\n", memory_unused);
	}
#endif XK_MEMORY_THROTTLE  
	kfree( &where[ -1 ], where[ -1 ] + sizeof( int ) );
}


int
xFree( where )
    int	*where;
{
    free(where);
    return 0;
}
    


char *
calloc(nelem, elsize)
   unsigned nelem, elsize;
{
	char	*where;

	where = (char *)malloc( nelem * elsize );
	bzero( where, nelem * elsize );
	return( where );
}

char *
realloc(ptr, size)
   char *ptr;
   unsigned size;
{
	char	*where;

	where = (char *)malloc( size );

	bcopy( ptr, where, 
			((int *)ptr)[ -1 ] < size  ?  ((int *)ptr)[ -1 ]  :  size );
	
	free( ptr );

	return( where );

}


char *
xMalloc( unsigned s )
{
    char *p;

    xTrace1(malloc, TR_EVENTS, "malloc %u bytes", s);
    xIfTrace(malloc, TR_MAJOR_EVENTS)
      { if (s > 4096 && tracemalloc < TR_EVENTS)
	  printf("malloc %u bytes", s);
      }
    if ( p = (char *)malloc(s) ) {
#ifdef XK_MEMORY_THROTTLE
    memory_unused -= s;
    xAssert((memory_unused > 0));
    xAssert((((int *)p)[-1]) == s);
    xIfTrace(memoryusage, TR_EVENTS) {
      if (!(report_count++ % XK_MEMORY_REPORT_INTERVAL))
	printf("malloc: memory available %d\n", memory_unused);
    }
#endif XK_MEMORY_THROTTLE
	return p;
  }
    Kabort("malloc failed");
    return 0;
}


/*
 *	Object:
 *		bcopy				EXPORTED function
 *
 *		Memory copy
 *
 */
void
bcopy(from, to, bcount)
	register unsigned from;
	register unsigned to;
	register unsigned bcount;
{
	register int    i;

	xTrace3(bcopy,TR_EVENTS,"bcopy: from %x to %x count %d (decimal)",from,to,bcount);
	if ((from & 3) != (to & 3)) {
		/* wont align easily */
		while (bcount--)
			*((char *) to++) = *((char *) from++);
		return;
	}
	switch (to & 3) {
	    case 1:
		*((char *) to++) = *((char *) from++);
		if (--bcount == 0)
			return;
	    case 2:
		*((char *) to++) = *((char *) from++);
		if (--bcount == 0)
			return;
	    case 3:
		*((char *) to++) = *((char *) from++);
		if (--bcount == 0)
			return;
	    default:
		break;
	}

	for (i = bcount >> 2; i; i--, to += 4, from += 4)
		*((int *) to) = *((int *) from);

	switch (bcount & 3) {
	    case 3:
		*((char *) to++) = *((char *) from++);
	    case 2:
		*((char *) to++) = *((char *) from++);
	    case 1:
		*((char *) to++) = *((char *) from++);
	    default:
		break;
	}
}

void
xMallocInit()
{
}


/*
 *	Object:
 *		bzero				EXPORTED function
 *
 *		Clear memory locations
 *
 *	Optimize for aligned memory ops, if possible and simple.
 *	Might need later recoding in assembly for better efficiency.
 */
void
bzero(addr, bcount)
	register unsigned addr;
	register unsigned bcount;
{
	register int    i;

	if (bcount == 0)	/* sanity */
		return;
	switch (addr & 3) {
	    case 1:
		*((char *) addr++) = 0;
		if (--bcount == 0)
			return;
	    case 2:
		*((char *) addr++) = 0;
		if (--bcount == 0)
			return;
	    case 3:
		*((char *) addr++) = 0;
		if (--bcount == 0)
			return;
	    default:
		break;
	}

	for (i = bcount >> 2; i; i--, addr += 4)
		*((int *) addr) = 0;

	switch (bcount & 3) {
	    case 3: *((char*)addr++) = 0;
	    case 2: *((char*)addr++) = 0;
	    case 1: *((char*)addr++) = 0;
	    default:break;
	}
}


int
bcmp( a, b, count )
   char	*a, *b;
   int	count;
{
	while( count-- )
		if( *a == *b ) {
			++a; ++b;
		}
		else
			return( a - b );

	return( 0 );
}

putchar( c )
   char c;
{
	cnputc( c );
}


SetPromiscuous()
{
}

unsigned short
htons( s )
   unsigned short	s;
{
	return( ((0x00FF & s) << 8) | (((0xFF00 & s) >> 8) & 0x00FF) );
}

unsigned short
ntohs( s )
   unsigned short	s;
{
	return( htons( s ) );
}

static char *copybyte_str;

static char *
copybyte(byte)
char byte;
{
  *copybyte_str++ = byte;
  *copybyte_str = '\0';
}

/*VARARGS1*/
char *
sprintf(str, fmt, va_alist)
	char *	fmt, *str;
	va_dcl
{
	va_list	listp;
	va_start(listp);
	copybyte_str = str;
	_doprnt(fmt, &listp, copybyte, 16);
	va_end(listp);
	return str;
}

void
strcat(buf, newstuff)
char *buf, *newstuff;
{
  register char a;

  while(*buf++);
  buf--;
  while(a = *newstuff++) *buf++ = a;
}



char *
mach_error_string( kr )
    kern_return_t	kr;
{
    static char		buf[80];

    sprintf(buf, "%d", kr);
    return buf;
}
