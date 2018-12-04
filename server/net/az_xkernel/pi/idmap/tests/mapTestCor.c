/* 
 * $RCSfile: mapTestCor.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 23:56:31 $
 */

/* 
 * Tests the equivalence of different map hash functions.  Note that
 * this test program includes the idmapper source code.
 */


#include <string.h>
#include "idmap.c"
#include "assert.h"

extern char *	rindex( char *, char );


#define MAX_KEYSIZE 50
static int	keySize = 20;

extern char 	*malloc(unsigned);

int	( * hashFuncs[MAX_KEYSIZE])() = {
    0,0,hash2,0,hash4,0,hash6,0,hash8,0,
    hash10,0,hash12,0,0,0,hash16,0,0,0,
    hash20,0,0,0,hash24,0,0,0,hash28,0,
    0,0,hash32,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0
};
      

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
    char		key1[MAX_KEYSIZE], offKey[MAX_KEYSIZE+1];
    int			i, j, h;
    xkern_return_t	xkr;
    Bind		b;
    VOID		*ptr;
    char		c;
    extern char 	*optarg;
    int			(* hashFunc)();
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
    hashFunc = hashFuncs[keySize];
    if ( verboseFlag ) {
	printf("Key size == %d\n", keySize);
	if ( hashFunc == 0 ) {
	    printf("No key-specific hash function\n");
	}
	printf("testing hash functions\n");
    }
    map_init();
    m = mapCreate(11, keySize);
    for ( j=0; j < 1000; j++ ) {
	randomKey(key1, keySize);
	h = generichash(key1, 511, keySize);
	bcopy(key1, offKey+1, keySize);
	if ( (hashFunc && h != hashFunc(key1, 511)) ||
	     h != generichash(offKey+1, 511, keySize) ) {
	    for ( i=0; i < keySize; i++ ) {
		printf("%2x ", (u_char)key1[i]);
	    }
	    printf("\n");
	    if ( hashFunc ) {
		printf("key-specific hash: %d\n", hashFunc(key1, 511));
	    }
	    printf("generic hash: %d\n", generichash(key1, 511, keySize));
	    printf("offset generic hash: %d\n",
		   generichash(offKey+1, 511, keySize));
	    exit(1);
	}
    }
}
