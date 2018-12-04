/* 
 * idmap_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 23:55:59 $
 */

/* 
 * We do two malloc's for each element to guarantee that the external
 * ID field has no alignment problems.
 */
#define	GETMAPELEM(map, elem) { \
    if ((elem = (map)->freelist) == 0) { \
        elem = (MapElement*)xMalloc(sizeof(MapElement)); \
        elem->externalid = xMalloc((map)->keySize); \
    } else { \
        (map)->freelist = (elem)->next; \
    } \
}
#define	FREEIT(map, elem) { \
    (elem)->next = (map)->freelist; \
    (map)->freelist = (elem); \
}
