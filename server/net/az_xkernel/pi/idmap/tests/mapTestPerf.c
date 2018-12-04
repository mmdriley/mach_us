/* 
 * $RCSfile: mapTestPerf.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 23:56:44 $
 */

/* 
 * Tests performance of mapResolve
 */


#include "xkernel.h"

#define MAX_KEYSIZE	50
static int	keySize = 20;	/* default */
static int	tracemaptest = TR_MAJOR_EVENTS;

extern char 	*malloc(unsigned);
extern char	*rindex(char *, char);

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

#define INDEX	(keySize <= 4 ? (keySize - 1) : (keySize - (keySize % 4) - 1))


static void
formKey( char *key, char frag )
{
    int	i;

#if 0
    for ( i=0; i < keySize; i++ ) {
	bcopy(&frag, key + i, sizeof(char)); 
	/* 
	 * Try to get the same hash values independent of the key size. 
	 */
    }
#endif
    bzero(key, keySize);
    key[INDEX] = frag;
    xIfTrace(maptest, TR_FULL_TRACE) {
	printf("key: ");
	for ( i=0; i < keySize; i++ ) {
	    printf("%x ", (u_char)key[i]);
	}
	printf("\n");
    }
}


static void
store( char key, int value )
{
    char	realKey[MAX_KEYSIZE];

    formKey(realKey, key);
    bind[key] = mapBind(m, (char *)&realKey, (VOID *)value);
    xAssert( bind[key] != ERR_BIND );
}


static int
displayElem( void *key, void * value, void *index )
{
    u_char	frag;
    frag = ((u_char *)key)[INDEX];
    printf("Element %d:	  k = %d  val = %d\n",
	   ++*(int *)index, frag, (int)value);
    return 1;
}


static void
mapDump( Map m )
{
    int 	i;
    MapElement	*e;
    u_char	frag;

    for ( i=0; i < m->tableSize; i++ ) {
	printf("bucket %d: ", i);
	if ( (e = m->table[i]) == 0 ) {
	    printf("no elements\n");
	} else {
	    do {
		frag = ((u_char *)e->externalid)[INDEX];
		printf("(%d -> %d)  ", frag, e->internalid);
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
    fprintf(stderr, "usage: %s [-h] [-o] [-k keySize] [-t traceLevel]\n", name);
}


main(int argc, char **argv)
{
    int			buf1[MAX_KEYSIZE/4 + 1], buf2[MAX_KEYSIZE/4 + 1];
    char		*key1 = (char *)buf1;
    char		*key2 = (char *)buf2;
    int			i, j, h;
    xkern_return_t	xkr;
    int			initMap = 0;
    char		c;
    extern char 	*optarg;

    while ((c = getopt(argc, argv, "k:hot:")) != -1 ) {
	switch (c) {
	  case 'o':
	    initMap = 1;
	    break;
	  case 'k':
	    keySize = atoi(optarg);
	    break;
	  case 't':
	    tracemaptest = atoi(optarg);
	    break;
	  case '?':
	  case 'h':
	    usage(*argv);
	    exit(1);
	}
    }
    if ( initMap ) {
	xTrace0(maptest, TR_MAJOR_EVENTS,
		"Initializing key-specific functions");
	map_init(); 
    } else {
	xTrace0(maptest, TR_MAJOR_EVENTS, "Using only generic map functions");
    }
    xTrace1(maptest, TR_MAJOR_EVENTS, "Key size == %d", keySize);
    m = mapCreate(11, keySize);
    
    store(10, 110);
    store(12, 120);
    store(15, 150);

    xIfTrace(maptest, TR_FULL_TRACE) {
	display();
    }

    store(55, 11);
    store(17, 2345);
    store(22, 4567);
    store(66, 11);
    store(77, 2345);
    store(101, 4567);
    store(23, 11);
    store(150, 2345);
    store(76, 4567);

    xIfTrace(maptest, TR_FULL_TRACE) {
	display();
    }

    formKey(key1, 12);
    formKey(key2, 101);
    xkr = mapResolve(m, key1, 0);
    xAssert( xkr == XK_SUCCESS );
    xkr = mapResolve(m, key2, 0);
    xAssert( xkr == XK_SUCCESS );
    xTrace0(maptest, TR_EVENTS, "starting resolves");
    for ( i=0; i < 1E5; i++ ) {
	xkr = mapResolve(m, key1, 0);
	xkr = mapResolve(m, key2, 0);
    }
    xTrace0(maptest, TR_EVENTS, "finished resolves");

#if 0

    remove(101);
    remove(10);
    remove(12);
    remove(77);

    display();

    store(77, 11);

    display();

    traceidmap = 100;
    mapClose(m);
#endif
}
