/* 
 * $RCSfile: mapTestCor2.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 23:56:38 $
 */

/* 
 * Tests equivalence of map operations for identical keys with
 * different offsets  
 */



#include "xkernel.h"

#define MAX_KEYSIZE 50
static int	keySize = 20;

extern char 	*malloc(unsigned);
extern char	*rindex( char *, char );


static Map	m;
static Bind	bind[1500];

char *
xMalloc( unsigned int size )
{
    return malloc(size);
}


void
xTraceLock()
{
}


void
xTraceUnlock()
{
}


static void
formKey( char *key, char frag )
{
    int	i;

    for ( i=0; i < keySize; i++ ) {
	bcopy(&frag, key + i, sizeof(char));
    }
}


static void
store( int key, int value )
{
    char	realKey[MAX_KEYSIZE];

    formKey(realKey, key);
    bind[key] = mapBind(m, (char *)&realKey, (VOID *)value);
}


static int
displayElem( void *key, void * value, void *index )
{
    printf("Element %d:	  k = %d  val = %d\n",
	   ++*(int *)index, *(u_char *)key, (int)value);
    return 1;
}


static void
mapDump( Map m )
{
    int 	i;
    MapElement	*e;

    for ( i=0; i < m->tableSize; i++ ) {
	printf("bucket %d: ", i);
	if ( (e = m->table[i]) == 0 ) {
	    printf("no elements\n");
	} else {
	    do {
		printf("(%d -> %d)  ", *(char *)e->externalid, e->internalid);
		e = e->next;
	    } while ( e != 0 );
	    printf("\n");
	}
    }
}


static void
display()
{
    MapElement	*e;

    int	i = 0;
    printf("\n");
    mapDump(m);
    mapForEach(m, displayElem, &i);
    i = 0;
    printf("freeList:\n");
    for ( e = m->freelist; e != 0; e = e->next ) {
	displayElem(e->externalid, e->internalid, &i);
    }
}


static void
remove( int key)
{
#if 0
    mapUnbind(m, &key);
#endif
    if ( mapRemoveBinding(m, bind[key]) ) {
	printf("Remove binding of key %d failed!!\n", key);
    } else {
	printf("Remove binding of key %d succeeded\n", key);
    }
}




static void
randomKey( key, len )
    char	*key;
    int		len;
{
    struct timeval tp;
    struct timezone tzp;
    int		i;
    
    gettimeofday(&tp, &tzp);
    srandom(getpid() + tp.tv_sec + tp.tv_usec);
    for ( i=0; i < len; i++ ) {
	key[i] = random() & 0xff;
    }
}

static void
usage( arg )
    char	*arg;
{
    char	*name;

    name = rindex(arg, '/');
    if ( name ) {
	name++;
    } else {
	name = arg;
    }
    fprintf(stderr, "usage: %s [-h] [-v] [-k keySize] \n", name);
}


main( int argc, char **argv )
{
    char		key1[MAX_KEYSIZE], key2[MAX_KEYSIZE], offBuf[MAX_KEYSIZE+1];
    char		*offKey = offBuf + 1;
    int			i, j, h;
    xkern_return_t	xkr;
    Bind		b;
    VOID		*ptr;
    char		c;
    extern char 	*optarg;
    int			verboseFlag = 0;

    while ((c = getopt(argc, argv, "k:hv")) != -1 ) {
	switch (c) {
	  case 'k':
	    keySize = atoi(optarg);
	    break;
	  case 'v':
	    verboseFlag = 1;
	    break;
	  case '?':
	  case 'h':
	    usage(*argv);
	    exit(1);
	}
    }
    xAssert(keySize < MAX_KEYSIZE && keySize > 0);
    map_init();
    if ( verboseFlag ) {
	printf("Key size == %d\n", keySize);
	traceidmap = TR_FULL_TRACE;
    }
    m = mapCreate(11, keySize);
    
    randomKey(key1, keySize);
    bcopy(key1, offKey, keySize);

    b = mapBind(m, key1, 22);
    xAssert( b != ERR_BIND );
    xkr = mapResolve(m, key1, &ptr);
    xAssert( xkr == XK_SUCCESS );
    xAssert( ptr == (VOID *)22 );
    bzero(key2, keySize);
    xkr = mapResolve(m, key2, &ptr);
    xAssert( xkr == XK_FAILURE );
    b = mapBind(m, offKey, 25);
    xAssert( b == ERR_BIND );
    xkr = mapUnbind(m, key1);
    xAssert( xkr == XK_SUCCESS );
    b = mapBind(m, offKey, 25);
    xAssert( b != ERR_BIND );
    xkr = mapResolve(m, key1, &ptr);
    xAssert( xkr == XK_SUCCESS );
    xkr = mapResolve(m, offKey, &ptr);
    xAssert( xkr == XK_SUCCESS );
}
