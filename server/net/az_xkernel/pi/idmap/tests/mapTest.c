/* 
 * $RCSfile: mapTest.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 23:56:20 $
 */

/* 
 * Exceedingly rudimentary test of map operations:
 *	mapBind
 *	mapUnbind
 *	mapForEach
 *	mapClose
 */


#include "xkernel.h"


extern char 	*malloc(unsigned);


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
store( int key, int value )
{
    bind[key] = mapBind(m, (char *)&key, (VOID *)value);
}


static int
displayElem( void *key, void * value, void *index )
{
    printf("Element %d:	  k = %d  val = %d\n",
	   ++*(int *)index, *(int *)key, (int)value);
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
		printf("(%d -> %d)  ", *(int *)e->externalid, e->internalid);
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


main()
{
    m = mapCreate(11, sizeof(int));
    
    store(10, 110);
    store(12, 120);
    store(15, 150);

    display();

    store(55, 11);
    store(17, 2345);
    store(22, 4567);
    store(66, 11);
    store(77, 2345);
    store(101, 4567);
    store(1004, 11);
    store(150, 2345);
    store(76, 4567);

    display();

    remove(101);
    remove(10);
    remove(12);
    remove(77);

    display();

    store(77, 11);

    display();

    traceidmap = 100;
    mapClose(m);
}
