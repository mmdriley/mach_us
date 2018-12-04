/* 
 * idmap.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.18 $
 * $Date: 1993/02/01 22:39:10 $
 */

#ifndef idmap_h
#define idmap_h


#define MAX_MAP_KEY_SIZE	100


typedef struct mapelement {
  struct mapelement *next;
  VOID *internalid;
  VOID *externalid;
} MapElement, *Bind;


typedef struct map {
  int tableSize, keySize;
  MapElement *cache;
  MapElement *freelist;
  MapElement **table;
  xkern_return_t (*resolve)();
  Bind	(*bind)();
  xkern_return_t (*unbind)();
  xkern_return_t (*remove)();
} *Map;


#define mapResolve(map, key, valPtr) \
  (map)->resolve(map, (VOID *)key, (VOID **)valPtr)
#define mapBind(map, key, value)   (map)->bind(map, key, value)
#define mapRemoveBinding(map, binding)   (map)->remove((map), (binding))
#define mapUnbind(map, key)   (map)->unbind((map), (key))
#define mapKeySize(_map) ((_map)->keySize)

/* 
 * msgForEach return value flags
 */
#define MFE_CONTINUE	1
#define MFE_REMOVE	2

#ifdef __STDC__

typedef	int (*MapForEachFun)( void *key, void *value, void *arg );

extern void 	mapClose( Map );
extern Map 	mapCreate( int tableSize, int keySize );
extern void	mapForEach( Map, MapForEachFun, void * );

/* 
 * map_init is not part of the public map interface.  It is to be
 * called once at xkernel initialization time.
 */
extern void	map_init( void );

#else

typedef	int (*MapForEachFun)();

extern void 	mapClose();
extern Map 	mapCreate();
extern void	mapForEach();

extern void	map_init();

#endif __STDC__


#endif /* idmap_h */
