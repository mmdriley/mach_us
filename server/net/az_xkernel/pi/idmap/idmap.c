/* 
 * idmap.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.29 $
 * $Date: 1993/02/01 23:56:15 $
 */

#include "upi.h"
#include "idmap_internal.h"
#include "assert.h"
#include "xk_debug.h"
#ifndef XKMACHKERNEL
#include "x_libc.h"
#endif XKMACHKERNEL

static struct {
  xkern_return_t	(*resolve)();
  Bind			(*bind)();
  xkern_return_t 	(*unbind)();
  xkern_return_t 	(*remove)();
} map_functions[MAX_MAP_KEY_SIZE];
  
int	traceidmap;

#ifdef __STDC__

static xkern_return_t	mgenericresolve( Map, VOID *, VOID ** );
static Bind 		mgenericbind( Map, VOID *, VOID * );
static xkern_return_t  	mgenericunbind( Map, VOID * );
static xkern_return_t  	mgenericremove( Map, Bind );

#else

static xkern_return_t	mgenericresolve();
static Bind 		mgenericbind();
static xkern_return_t  	mgenericunbind();
static xkern_return_t  	mgenericremove();

#endif __STDC__

/*
 * Create and return a new map containing a table with nEntries entries 
 * mapping keys of size keySize to integers
 */

Map
mapCreate(nEntries, keySize)
    int nEntries;
    int keySize;
{
    register Map m;
    
    if (keySize > MAX_MAP_KEY_SIZE) {
	return (Map)-1;
    }
    m = (Map)xMalloc(sizeof(*m));
    m->tableSize = nEntries;
    m->keySize = keySize;
    m->cache = 0;
    m->freelist = 0;
    m->table = (MapElement **)xMalloc(nEntries * sizeof(MapElement *));
    bzero((char *)m->table, nEntries * sizeof(MapElement *));
    if (map_functions[keySize].resolve != 0) {
	m->resolve = map_functions[keySize].resolve;
	m->bind    = map_functions[keySize].bind   ;
	m->unbind  = map_functions[keySize].unbind ;
	m->remove  = map_functions[keySize].remove;
    } else {
	m->resolve = mgenericresolve;
	m->bind    = mgenericbind   ;
	m->unbind  = mgenericunbind ;
	m->remove  = mgenericremove;
    }    
    return m;
}


static void
removeList( e )
    MapElement	*e;
{
    MapElement	*next;

    while ( e != 0 ) {
	xTrace1(idmap, TR_FULL_TRACE, "mapClose removing element %x", (int)e);
	next = e->next;
	xFree((char *)e->externalid);
	xFree((char *)e);
	e = next;
    }
}


void
mapClose(m)
    Map m;
{
    int		i;

    xTrace1(idmap, TR_MAJOR_EVENTS, "mapClose of map %x", m);
    /* 
     * Free any elements still bound into the map
     */
    for ( i=0; i < m->tableSize; i++ ) {
	removeList(m->table[i]);
    }
    /* 
     * Remove freelist
     */
    removeList(m->freelist);
    xFree((char *)m->table);
    xFree((char *)m);
}


void
mapForEach(m, f, arg)
    Map m;
    MapForEachFun f;
    VOID *arg;
{
    char	key[MAX_MAP_KEY_SIZE];
    MapElement	*elem, *prevElem;
    int 	i;
    int		userResult;

    for ( i = 0; i < m->tableSize; i++ ) {
	prevElem = 0;
	do {
	    /* 
	     * Set elem to first item in the bucket
	     */
	    elem = m->table[i];
	    if ( prevElem != 0 ) {
		/*
		 * Set elem to the next element after the old elem in the
		 * same hash bucket.  If the previous element has been
		 * removed we will skip the rest of this bucket.  If
		 * the previous element has been removed and
		 * reinserted (with a possibly different key/value),
		 * we will skip it and everything before it in the
		 * list. 
		 */
		for ( ; elem != 0 ; elem = elem->next ) {
		    if ( prevElem == elem ) {
			elem = elem->next;
			break;
		    }
		}
		if ( userResult & MFE_REMOVE ) {
		    m->remove(m, prevElem);
		}
		prevElem = 0;
	    }
	    if ( elem != 0 ) {
		bcopy((char *)elem->externalid, key, m->keySize);
		userResult = f(key, elem->internalid, arg);
		if ( ! (userResult & MFE_CONTINUE) ) {
		    return;
		}
		prevElem = elem;
	    }
	} while ( elem != 0 );
    } 
}


static int
generichash(key, tableSize, keySize)
    char *key;
    int tableSize;
    int keySize;
{
    unsigned int	hash = 0;
    int			i;

    xTrace2(idmap, TR_FULL_TRACE,
	    "generic idmap hash -- tableSize %d, keySize %d",
	    tableSize, keySize);
    if ( ! LONG_ALIGNED(key) ) {
	xTrace0(idmap, TR_FULL_TRACE, "unaligned key");
	for (i = 0; i < keySize; i++) {
	    if ( i % 4 == 0 ) {
		hash <<= 2;
	    }
	    ((char *)&hash)[i % 4] ^= (* (unsigned char *)key);
	    key++;
	}
    } else {
	int	howmanylongs = keySize / 4;
	
	xTrace0(idmap, TR_FULL_TRACE, "aligned key");
	for (i = 0; i < howmanylongs; i++) {
	    hash = (hash << 2) ^ (* (unsigned long *)key);
	    key += 4;
	}
	if ( keySize % 4 ) {
	    hash <<= 2;
	    for ( i=0; i < keySize % 4; i++ ) {
		((char *)&hash)[i % 4] ^= (* (unsigned char *)key); 
		key++;
	    }
	}
    }
    xTrace1(idmap, TR_FULL_TRACE, "generic hash returns %d", hash % tableSize);
    return hash % tableSize;
}


#ifdef XK_USE_INLINE
#  define INLINE	__inline__
#else
#  define INLINE
#endif


#define KEYSIZE generic
#define BINDNAME mgenericbind
#define REMOVENAME mgenericremove
#define RESOLVENAME mgenericresolve
#define UNBINDNAME mgenericunbind
#define HASH(K, tblSize, keySize) generichash(K, tblSize, keySize)
#define GENHASH(K, T, keySize) generichash(K, tblSize, keySize)
#define COMPBYTES(s1, s2) (!bcmp(s1, s2, table->keySize))
#define GENCOMPBYTES(s1, s2) (!bcmp(s1, s2, table->keySize))
#define COPYBYTES(s1, s2) bcopy(s2, s1, table->keySize)
#define GENCOPYBYTES(s1, s2) bcopy(s2, s1, table->keySize)

#include "idmap_templ.c"

#define KEYSIZE 2
#define BINDNAME m2bind
#define REMOVENAME m2remove
#define RESOLVENAME m2resolve
#define UNBINDNAME m2unbind
static INLINE int
hash2(K, tblSize)
    VOID	*K;
    int		tblSize;
{
    u_int	h = 0;

    if ( SHORT_ALIGNED(K)) {
	*(u_short *)&h = *(u_short *)(K);
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "2 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K,tblSize,2);
    }
}
#define HASH(K, tblSize, keySize)        hash2(K, tblSize)
#define COMPBYTES(s1, s2) (((char *)(s1))[0] == ((char *)(s2))[0] && 	\
			   ((char *)(s1))[1] == ((char *)(s2))[1])
#define COPYBYTES(s1, s2) {((char *)(s1))[0] = ((char *)(s2))[0];	\
			   ((char *)(s1))[1] = ((char *)(s2))[1];}


#include "idmap_templ.c"

#define KEYSIZE 4
#define BINDNAME m4bind
#define REMOVENAME m4remove
#define RESOLVENAME m4resolve
#define UNBINDNAME m4unbind
static INLINE int
hash4(K, tblSize)
    VOID	*K;
    int		tblSize;
{
    u_int	h = 0;

    if ( LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "4 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 4);
    }
}

#define HASH(K, tblSize, keySize)        hash4(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
   ( *(int *)(s1) == (*(int *)(s2))) :			\
   GENCOMPBYTES((s1), (s2)) )

#define COPYBYTES(s1, s2) {				\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {		\
    	*(int *)(s1) = (*(int *)(s2)); 			\
  } else {						\
	GENCOPYBYTES(s1,s2);				\
  }							\
}

#include "idmap_templ.c"

#define KEYSIZE 6
#define BINDNAME m6bind
#define REMOVENAME m6remove
#define RESOLVENAME m6resolve
#define UNBINDNAME m6unbind
static INLINE int
hash6(K, tblSize)
    VOID	*K;
    int		tblSize;
{
    u_int	h = 0;

    if ( LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	*(u_short *)&h ^= *((u_short *)(K) + 2);
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "6 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K,tblSize,6);
    }
}
#define HASH(K, tblSize, keySize) 	hash6(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
   ( (*(int *)(s1)) == (*(int *)(s2)) &&		\
     (*((short *)(s1)+2)) == (*((short *)(s2)+2)) ) :	\
   GENCOMPBYTES((s1), (s2)) )

#define COPYBYTES(s1, s2) {				\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {		\
    	*(int *)(s1) = (*(int *)(s2)); 			\
	*((short *)(s1)+2) = *((short *)(s2)+2); 	\
  } else {						\
	GENCOPYBYTES(s1,s2);				\
  }							\
}


#include "idmap_templ.c"

#define KEYSIZE 8
#define BINDNAME m8bind
#define REMOVENAME m8remove
#define RESOLVENAME m8resolve
#define UNBINDNAME m8unbind
static INLINE int
hash8( K, tblSize )
    VOID	*K;
    int		tblSize;
{
    u_int	h;

    if (LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "8 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 8);
    }
}
#define HASH(K, tblSize, keySize)	hash8(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	 (*((int *)(s1)+1)) == (*((int *)(s2)+1))) :	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}


#include "idmap_templ.c"

#define KEYSIZE 10
#define BINDNAME m10bind
#define REMOVENAME m10remove
#define RESOLVENAME m10resolve
#define UNBINDNAME m10unbind
static INLINE int
hash10(K, tblSize)
    VOID	*K;
    int		tblSize;
{
    u_int	h = 0;

    if ( LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h <<= 2;
	*(u_short *)&h ^= *((u_short *)(K) + 4);
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "10 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 10);
    }
}
#define HASH(K, tblSize, keySize) 	hash10(K, tblSize)

#define COMPBYTES(s1, s2) 				\
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
   ( (*(int *)(s1)) == (*(int *)(s2)) &&		\
     (*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
     (*((short *)(s1)+4)) == (*((short *)(s2)+4)) ) :	\
   GENCOMPBYTES((s1), (s2)) )

#define COPYBYTES(s1, s2) {				\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {		\
    	*(int *)(s1) = (*(int *)(s2)); 			\
	*((int *)(s1)+1) = *((int *)(s2)+1); 		\
	*((short *)(s1)+4) = *((short *)(s2)+4); 	\
  } else {						\
	GENCOPYBYTES(s1,s2);				\
  }							\
}

#include "idmap_templ.c"



#define KEYSIZE 12
#define BINDNAME m12bind
#define REMOVENAME m12remove
#define RESOLVENAME m12resolve
#define UNBINDNAME m12unbind
static INLINE int
hash12( K, tblSize )
    VOID	*K;
    int		tblSize;
{
    u_int	h;

    if (LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h <<= 2;
	h ^= (*((u_int *)(K) + 2));
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "12 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 12);
    }
}	
#define HASH(K, tblSize, keySize)	hash12(K, tblSize)


#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2))) : 	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}

#include "idmap_templ.c"



#define KEYSIZE 16
#define BINDNAME m16bind
#define REMOVENAME m16remove
#define RESOLVENAME m16resolve
#define UNBINDNAME m16unbind
static INLINE int
hash16( K, tblSize )
    VOID	*K;
    int		tblSize;
{
    u_int	h;

    if (LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h <<= 2;
	h ^= (*((u_int *)(K) + 2));
	h <<= 2; 
	h ^= (*((u_int *)(K) + 3));
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "16 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 16);
    }
}	
#define HASH(K, tblSize, keySize)	hash16(K, tblSize)


#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2)) && 	\
	(*((int *)(s1)+3)) == (*((int *)(s2)+3))) : 	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
	*((int *)(s1)+3) = *((int *)(s2)+3); 	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}

#include "idmap_templ.c"



#define KEYSIZE 20
#define BINDNAME m20bind
#define REMOVENAME m20remove
#define RESOLVENAME m20resolve
#define UNBINDNAME m20unbind
static INLINE int
hash20( K, tblSize )
    VOID	*K;
    int		tblSize;
{
    u_int	h;

    if (LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h <<= 2;
	h ^= (*((u_int *)(K) + 2));
	h <<= 2;
	h ^= (*((u_int *)(K) + 3));
	h <<= 2;
	h ^= (*((u_int *)(K) + 4));
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "20 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 20);
    }
}	
#define HASH(K, tblSize, keySize)	hash20(K, tblSize)


#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2)) && 	\
	(*((int *)(s1)+3)) == (*((int *)(s2)+3)) && 	\
	(*((int *)(s1)+4)) == (*((int *)(s2)+4))) :	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
	*((int *)(s1)+3) = *((int *)(s2)+3); 	\
	*((int *)(s1)+4) = *((int *)(s2)+4);	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}


#include "idmap_templ.c"

#define KEYSIZE 24
#define BINDNAME m24bind
#define REMOVENAME m24remove
#define RESOLVENAME m24resolve
#define UNBINDNAME m24unbind
static INLINE int
hash24( K, tblSize )
    VOID	*K;
    int		tblSize;
{
    u_int	h;

    if (LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h <<= 2;
	h ^= (*((u_int *)(K) + 2));
	h <<= 2;
	h ^= (*((u_int *)(K) + 3));
	h <<= 2;
	h ^= (*((u_int *)(K) + 4));
	h <<= 2;
	h ^= (*((u_int *)(K) + 5));
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "24 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 24);
    }
}	
#define HASH(K, tblSize, keySize)	hash24(K, tblSize)


#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2)) && 	\
	(*((int *)(s1)+3)) == (*((int *)(s2)+3)) && 	\
	(*((int *)(s1)+4)) == (*((int *)(s2)+4)) &&	\
	(*((int *)(s1)+5)) == (*((int *)(s2)+5))) :	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
	*((int *)(s1)+3) = *((int *)(s2)+3); 	\
	*((int *)(s1)+4) = *((int *)(s2)+4);	\
	*((int *)(s1)+5) = *((int *)(s2)+5);	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}

#include "idmap_templ.c"



#define KEYSIZE 28
#define BINDNAME m28bind
#define REMOVENAME m28remove
#define RESOLVENAME m28resolve
#define UNBINDNAME m28unbind
static INLINE int
hash28( K, tblSize )
    VOID	*K;
    int		tblSize;
{
    u_int	h;

    if (LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h <<= 2;
	h ^= (*((u_int *)(K) + 2));
	h <<= 2;
	h ^= (*((u_int *)(K) + 3));
	h <<= 2;
	h ^= (*((u_int *)(K) + 4));
	h <<= 2;
	h ^= (*((u_int *)(K) + 5));
	h <<= 2;
	h ^= (*((u_int *)(K) + 6));
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "28 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 28);
    }
}	
#define HASH(K, tblSize, keySize)	hash28(K, tblSize)

#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2)) && 	\
	(*((int *)(s1)+3)) == (*((int *)(s2)+3)) && 	\
	(*((int *)(s1)+4)) == (*((int *)(s2)+4)) && 	\
	(*((int *)(s1)+5)) == (*((int *)(s2)+5)) && 	\
	(*((int *)(s1)+6)) == (*((int *)(s2)+6))) :	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
	*((int *)(s1)+3) = *((int *)(s2)+3); 	\
	*((int *)(s1)+4) = *((int *)(s2)+4);	\
	*((int *)(s1)+5) = *((int *)(s2)+5);	\
	*((int *)(s1)+6) = *((int *)(s2)+6);	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}

#include "idmap_templ.c"


#define KEYSIZE 32
#define BINDNAME m32bind
#define REMOVENAME m32remove
#define RESOLVENAME m32resolve
#define UNBINDNAME m32unbind

static INLINE int
hash32( K, tblSize )
    VOID	*K;
    int		tblSize;
{
    u_int	h;

    if (LONG_ALIGNED(K)) {
	h = *(u_int *)(K);
	h <<= 2;
	h ^= (*((u_int *)(K) + 1));
	h <<= 2;
	h ^= (*((u_int *)(K) + 2));
	h <<= 2;
	h ^= (*((u_int *)(K) + 3));
	h <<= 2;
	h ^= (*((u_int *)(K) + 4));
	h <<= 2;
	h ^= (*((u_int *)(K) + 5));
	h <<= 2;
	h ^= (*((u_int *)(K) + 6));
	h <<= 2;
	h ^= (*((u_int *)(K) + 7));
	h %= tblSize;
	xTrace1(idmap, TR_FULL_TRACE, "32 byte key optimized hash returns %d",
		h);
	return h;
    } else {
	return GENHASH(K, tblSize, 32);
    }
}	
#define HASH(K, tblSize, keySize)	hash32(K, tblSize)

#define COMPBYTES(s1, s2) \
  ( (LONG_ALIGNED(s1) && LONG_ALIGNED(s2)) ? 		\
       ((*(int *)(s1)) == (*(int *)(s2)) && 	    	\
	(*((int *)(s1)+1)) == (*((int *)(s2)+1)) && 	\
	(*((int *)(s1)+2)) == (*((int *)(s2)+2)) && 	\
	(*((int *)(s1)+3)) == (*((int *)(s2)+3)) && 	\
	(*((int *)(s1)+4)) == (*((int *)(s2)+4)) && 	\
	(*((int *)(s1)+5)) == (*((int *)(s2)+5)) && 	\
	(*((int *)(s1)+6)) == (*((int *)(s2)+6)) && 	\
	(*((int *)(s1)+7)) == (*((int *)(s2)+7))) :	\
    GENCOMPBYTES(s1, s2) )
	  

#define COPYBYTES(s1, s2) {			\
  if ( LONG_ALIGNED(s1) && LONG_ALIGNED(s2) ) {	\
    	*(int *)(s1) = (*(int *)(s2)); 		\
	*((int *)(s1)+1) = *((int *)(s2)+1); 	\
	*((int *)(s1)+2) = *((int *)(s2)+2); 	\
	*((int *)(s1)+3) = *((int *)(s2)+3); 	\
	*((int *)(s1)+4) = *((int *)(s2)+4);	\
	*((int *)(s1)+5) = *((int *)(s2)+5);	\
	*((int *)(s1)+6) = *((int *)(s2)+6);	\
	*((int *)(s1)+7) = *((int *)(s2)+7);	\
  } else {					\
	GENCOPYBYTES(s1,s2);			\
  }						\
}

#include "idmap_templ.c"


void
map_init()
{
#define KEYSIZE 2
#define BINDNAME m2bind
#define REMOVENAME m2remove
#define RESOLVENAME m2resolve
#define UNBINDNAME m2unbind
#include "idmap_init.c"
#define KEYSIZE 4
#define BINDNAME m4bind
#define REMOVENAME m4remove
#define RESOLVENAME m4resolve
#define UNBINDNAME m4unbind
#include "idmap_init.c"
#define KEYSIZE 6
#define BINDNAME m6bind
#define REMOVENAME m6remove
#define RESOLVENAME m6resolve
#define UNBINDNAME m6unbind
#include "idmap_init.c"
#define KEYSIZE 8
#define BINDNAME m8bind
#define REMOVENAME m8remove
#define RESOLVENAME m8resolve
#define UNBINDNAME m8unbind
#include "idmap_init.c"
#define KEYSIZE 10
#define BINDNAME m10bind
#define REMOVENAME m10remove
#define RESOLVENAME m10resolve
#define UNBINDNAME m10unbind
#include "idmap_init.c"
#define KEYSIZE 12
#define BINDNAME m12bind
#define REMOVENAME m12remove
#define RESOLVENAME m12resolve
#define UNBINDNAME m12unbind
#include "idmap_init.c"
#define KEYSIZE 16
#define BINDNAME m16bind
#define REMOVENAME m16remove
#define RESOLVENAME m16resolve
#define UNBINDNAME m16unbind
#include "idmap_init.c"
#define KEYSIZE 20
#define BINDNAME m20bind
#define REMOVENAME m20remove
#define RESOLVENAME m20resolve
#define UNBINDNAME m20unbind
#include "idmap_init.c"
#define KEYSIZE 24
#define BINDNAME m24bind
#define REMOVENAME m24remove
#define RESOLVENAME m24resolve
#define UNBINDNAME m24unbind
#include "idmap_init.c"
#define KEYSIZE 28
#define BINDNAME m28bind
#define REMOVENAME m28remove
#define RESOLVENAME m28resolve
#define UNBINDNAME m28unbind
#include "idmap_init.c"
#define KEYSIZE 32
#define BINDNAME m32bind
#define REMOVENAME m32remove
#define RESOLVENAME m32resolve
#define UNBINDNAME m32unbind
#include "idmap_init.c"
}
