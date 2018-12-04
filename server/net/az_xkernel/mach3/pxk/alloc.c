/*
 * alloc.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:06:06 $
 */

#include "platform.h"
#include "assert.h"
#include "trace.h"
#include "xk_debug.h"
#include "x_util.h"
#include "x_libc.h"
#include "event.h"

int	tracemalloc;

#ifdef __STDC__

extern void	free( char * );
extern char *	malloc( unsigned );

#endif


#ifdef XK_DEBUG_MALLOC

extern int	qsort( char *, int, int, int (*)() );


#define I_MALLOC	i_malloc
#define I_FREE		i_free
#define MALLOC_SCOPE	static

static	char *	I_MALLOC( unsigned );
static  int	I_FREE( char * );
static  int	compareblocks( long **, long ** );
static  void	dumpBlocks( Event, void * );
static	void	fillPCs( long * );
static  void	lockInit( void );
static  void	xkBreakpoint();

#define verboseDump	TR_DETAILED

#define MALLOC_EXTRAS 	3	/* 2 tags + block index */
#define MALLOC_NPCS	5

#define MALLOC_STUFF	(MALLOC_NPCS + MALLOC_EXTRAS)

/*
 * The number of malloc blocks to store
 */
#define MAXBLOCKS		8192
#define FIRST_MALLOC_TAG	0x441199ee
#define SECOND_MALLOC_TAG	0x55ff0011

#define DUMP_BLOCK_INTERVAL	5 * 1000 * 1000  /* 5 seconds */
#define OCCURANCE_THRESHOLD	10


long		*xkBackTraceArr;

static long 	*blocks[MAXBLOCKS];
static int 	nextblock = 1;
static int	numMallocs = 0;
static int	numFrees = 0;
static struct mutex	malloc_mutex;
static int	lockInitialized;

char *
xMalloc(num)
    unsigned int num;
{
    register long *mp;
    int startblock = nextblock;
    
    if ( ! lockInitialized ) {
	lockInit();
    }
    mutex_lock(&malloc_mutex);
    numMallocs++;
    xTrace1(malloc, TR_FUNCTIONAL_TRACE, "xMalloc(%d) called", num);
    mp = (long *) I_MALLOC (num + MALLOC_STUFF * sizeof(long));
    xTrace1(malloc, TR_DETAILED, "xk internal malloc returns %x", mp);
    while (blocks[nextblock]) {
	if (++nextblock >= MAXBLOCKS) nextblock = 1;
	if (nextblock == startblock) {
	    nextblock = 0;
	    xError("malloc debugging block array overflow");
	    break;
	}
    }
    xTrace1(malloc, TR_DETAILED, "xMalloc adding block to index %d",
	    nextblock);
    blocks[nextblock] = mp;
    
    *mp++ = FIRST_MALLOC_TAG;
    *mp++ = SECOND_MALLOC_TAG;
    
    *mp++ = nextblock;
    
    /*
     * Allocators PCs
     */
    fillPCs(mp);
    mp += MALLOC_NPCS;
    
    xAssert( ! ( (int)mp % 4 ) );
    xTrace1(malloc, TR_DETAILED, "xMalloc returns %x", mp);
    mutex_unlock(&malloc_mutex);
    return (char *)mp;
}


static void
tagError()
{
    long	pcArray[ MALLOC_NPCS ];
    xError("xFree tag error");
    xError("free stack");
    tracemalloc = TR_FULL_TRACE;
    fillPCs(pcArray);
    xAssert(0);
}


int
xFree(p)
    char *p;
{
    long	*baseptr = (long *)p - MALLOC_STUFF;
    long	*lmp;
    long 	block;
    
    mutex_lock(&malloc_mutex);
    numFrees++;
    if ( baseptr[0] != FIRST_MALLOC_TAG || baseptr[1] != SECOND_MALLOC_TAG ) {
	tagError();
    }
    lmp = baseptr + 2;
    block = *lmp++;
    xAssert( blocks[block] );
    blocks[block] = 0;
    mutex_unlock(&malloc_mutex);
    return I_FREE((char *)baseptr);
}


static int
compareblocks(ap, bp)
    long **ap, **bp;
{
    long *a = *ap, *b = *bp;
    int i;
    if (a == b) return 0;
    if (!a) return  1;
    if (!b) return -1;
    
    a += MALLOC_EXTRAS;
    b += MALLOC_EXTRAS;
    for (i = 0; i < MALLOC_NPCS; i++) {
	if (*a < *b) return -1;
	if (*a > *b) return 1;
	a++; b++;
    }
    return 0;
}


static void
displayLine( long **b, int i )
{
    int	k;

    if ( b[i] ) {
	printf("%d:\t", i);
	for ( k=0; k < MALLOC_NPCS; k++ ) {
	    printf("%x ", (b[i][MALLOC_EXTRAS + k]));
	}
	if ( b[i][0] != FIRST_MALLOC_TAG || b[i][1] != SECOND_MALLOC_TAG ) {
	    printf("tag violation (%x %x)", b[i][0], b[i][1]);
	}
	printf("\n");
    }
}


static void
dumpBlocks( ev, arg )
    Event	ev;
    void	*arg;
{
    int i, j, k;

    long 		last[MALLOC_NPCS], *b;
    static long	*	lblocks[MAXBLOCKS];

    xError("\n\n\nMalloc trace\n");
    mutex_lock(&malloc_mutex);
    bcopy((char *)blocks, (char *)lblocks, sizeof(blocks));
    xIfTrace(malloc, verboseDump) {	
	for ( i=0; i < MAXBLOCKS; i++, j++ ) {
	    displayLine(blocks, i);
	}
	xError("\n\n");
    }
    qsort((char *)lblocks, MAXBLOCKS, sizeof(long *), compareblocks);
    for (i = 0, j = 0; i < MAXBLOCKS; i++, j++) {
	b = lblocks[i];
	xIfTrace(malloc, verboseDump) {	
	    displayLine(lblocks, i);
	}
	if ( ! b || bcmp((char *)(b + MALLOC_EXTRAS), (char *)last,
			 MALLOC_NPCS * sizeof(long)) ) {
	    /* 
	     * Found a different block
	     */
	    if ( j >= OCCURANCE_THRESHOLD ) {
		static char	buf[80];
		static char	smallBuf[10];
		
		sprintf(buf, "%d at ", j);
		for (k = 0; k < MALLOC_NPCS; k++) {
		    sprintf(smallBuf, "%x ", last[k]);
		    strcat(buf, smallBuf);
		}
		xTrace1(malloc, TR_ALWAYS, "%s", buf);
	    }
	    if (b) bcopy((char *)(b + MALLOC_EXTRAS), (char *)last,
			 MALLOC_NPCS * sizeof(long));
	    j = 0;
	}
	if (!b) break;
    }
    xTrace2(malloc, TR_ALWAYS, "%d mallocs, %d frees", numMallocs, numFrees);
    xTrace1(malloc, TR_ALWAYS, "%d malloc debugging slots occupied", i);

    if ( arg ) {
	dumpBlocks(0, 0);
    }
    mutex_unlock(&malloc_mutex);
    evDetach( evSchedule(dumpBlocks, 0, DUMP_BLOCK_INTERVAL) );
}



static void
xkBreakpoint()
{
}


static void
fillPCs( arr )
    long	*arr;
{
    int	i;

    xkBackTraceArr = arr;
    arr[0] = 0;
    xkBreakpoint();
    /* 
     * GDB should have inserted the backtrace at this point
     */
    xAssert( arr[0] );
    xIfTrace( malloc, TR_DETAILED ) {
	static char	buf[80];
	static char	smallBuf[10];
	
	buf[0] = 0;
	for ( i=0; i < MALLOC_NPCS; i++ ) {
	    sprintf(smallBuf, "%x ", arr[i]);
	    strcat(buf, smallBuf);
	}
	xTrace1(malloc, TR_ALWAYS, "%s", buf);
    }
}

static void
lockInit()
{
    lockInitialized = 1;
    mutex_init( &malloc_mutex );
}


#else    XK_DEBUG_MALLOC

#define MALLOC_SCOPE	/* exported */
#define I_MALLOC	xMalloc
#define I_FREE		xFree

#endif	XK_DEBUG_MALLOC




/* 
 * Careful -- allocations can happen before xMallocInit is called.
 */
void
xMallocInit()
{
    xTrace0(malloc, TR_GROSS_EVENTS, "xkernel malloc init");
#ifdef XK_DEBUG_MALLOC
    if ( ! lockInitialized ) {
	lockInit();
    }
    evDetach( evSchedule(dumpBlocks, 0, DUMP_BLOCK_INTERVAL) );
#endif    
}


/* 
 * The actual allocation/freeing routines.  These are either called
 * directly as xMalloc / xFree or are called by the wrapper debugging
 * functions above.
 */

MALLOC_SCOPE
char *
I_MALLOC( unsigned s )
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


MALLOC_SCOPE
int
I_FREE( p )
    char	*p;
{
    free(p);
    return 0;
}
      
