/*
 * chan_mapchain.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:31:25 $
 */

#include "xkernel.h"
#include "chan_internal.h"


#ifdef __STDC__

static int		applyToChanMap( VOID *, VOID *, VOID * );
static int		applyToHlpMap( VOID *, VOID *, VOID * );

#else

static int		applyToChanMap();
static int		applyToHlpMap();
static void		mapChainAddObject();

#endif __STDC__


typedef struct {
    int			count;
    MapChainForEachFun	f;
} ChanMapS;


/* 
 * The number of objects at the end of the map chain (i.e., the number
 * of objects to which the functions is applied) is returned.
 */
int
chanMapChainApply( map, peer, f )
    Map			map;
    IPhost		*peer;
    MapChainForEachFun	f;
{
    Map		hlpMap;
    ChanMapS	mapStr;

    mapStr.count = 0;
    mapStr.f = f;
    if ( mapResolve(map, (char *)peer, &hlpMap) == XK_FAILURE ) {
	xTrace1(chanp, TR_MAJOR_EVENTS,
		"No map for peer %s exists", ipHostStr(peer));
    } else {
	xTrace1(chanp, TR_MAJOR_EVENTS, "Applying function for peer %s",
		ipHostStr(peer));
	mapForEach(hlpMap, applyToHlpMap, &mapStr);
	xTrace1(chanp, TR_MAJOR_EVENTS,
		"chanMapChainApply -- %d idle objects were processed",
		mapStr.count);
    }
    return mapStr.count;
}


/* 
 * This function must not block
 */
static int
applyToChanMap( key, val, arg )
    VOID	*key, *val, *arg;
{
    /* 
     * key == chan, val == XObj, arg == mapStr
     */
    ChanMapS	*mapStr = (ChanMapS *)arg;
    int		retVal;

    xAssert(key);
    xAssert(val);
    xAssert(arg);
    xTrace2(chanp, TR_MORE_EVENTS,
	    "applyToChanMap: key(chan) == %d, val(seq) == %d",
	    *(int *)key, val);
    retVal = mapStr->f(key, val);
    mapStr->count++;
    return retVal;
}


/* 
 * This function must not block
 */
static int
applyToHlpMap( key, val, arg )
    VOID	*key, *val, *arg;
{
    ChanMapS	*mapStr = (ChanMapS *)arg;
    /* 
     * key == hlpNum, val == chanMap
     */
    xAssert(key);
    xAssert(val);
    xAssert(arg);
    xTrace2(chanp, TR_MORE_EVENTS,
	    "clearHlpMap: key(hlpNum) == %d, val(chanMap) == %x",
	    *(int *)key, val);
    mapForEach((Map)val, applyToChanMap, (VOID *)mapStr);
    return MFE_CONTINUE;
}


    
/* 
 * Binds 'obj' in the idleMap series starting at m.  The map series is
 * keyed by { peer --> { hlp --> { chan --> obj } } }.  Maps are
 * created as necessary.
 */
void
chanMapChainAddObject( obj, m, peer, prot, c )
    VOID	*obj;
    Map		m;
    IPhost	*peer;
    long	prot;
    int		c;
{
    Map		peerMap, hlpMap;
    Channel	chan;
    Bind	b;
#ifdef XK_DEBUG
    Map		mapCheck;
#endif

    if ( mapResolve(m, peer, &peerMap) == XK_FAILURE ) {
	peerMap = mapCreate(1, sizeof(long));
	b = mapBind(m, (char *)peer, (int)peerMap);
	xAssert(b != ERR_BIND);
	xAssert( mapResolve(m, peer, &mapCheck) == XK_SUCCESS && 
		 mapCheck == peerMap);
    }
    if ( mapResolve(peerMap, &prot, &hlpMap) == XK_FAILURE ) {
	hlpMap = mapCreate(1, sizeof(Channel));
	b = mapBind(peerMap, (char *)&prot, (int)hlpMap);
	xAssert(b != ERR_BIND);
	xAssert(mapResolve(peerMap, &prot, &mapCheck) == XK_SUCCESS && 
		mapCheck == hlpMap);
    }
    chan = c;
    b = mapBind(hlpMap, (char *)&chan, (int)obj);
    xAssert(b != ERR_BIND);
}


Map
chanMapChainFollow( m, h, prot )
    Map		m;
    IPhost	*h;
    long	prot;
{
    Map		peerMap, hlpMap;

    if ( mapResolve(m, h, &peerMap) == XK_FAILURE ||
    	 mapResolve(peerMap, &prot, &hlpMap) == XK_FAILURE ) {
	return 0;
    }
    return hlpMap;
}
    


