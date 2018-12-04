/* 
 * vcache_list.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:36:14 $
 */

/* 
 * Routines for maintaining the collection of cached sessions
 */

#include "xkernel.h"
#include "vcache_i.h"


extern	int	insque();
extern	int	remque();


typedef struct XListElem {
    struct XListElem	*next, *prev;
    /* 
     * No data for the header element
     */
} XListElem;

typedef XListElem	*XList;

typedef enum {
    CREATE, NO_CREATE
} Create;

#ifdef __STDC__

static XList	findList( Map, IPhost *, XObj, Create );
static XList	findListForSessn( Map, XObj, Create );

#else

static XList	findList();
static XList	findListForSessn();

#endif __STDC__



#define xListFirst( head )	(( (head)->next == (head) ) ? 0 : (head)->next)
#define xListRemove( e )	remque(e)
#define xListInsert( head, e )	insque(e, head)

/* 
 * Allocates a new XList
 */
static XList
xListNew()
{
    XList	l;

    l = X_NEW(XListElem);
    l->next = l;
    l->prev = l;
    return l;
}


/* 
 * Description in vcache_i.h
 */
XObj
vcacheFindIdleSessn( ps, peer, hlp )
    PState	*ps;
    IPhost	*peer;
    XObj	hlp;
{
    XList	list;
    SState	*ss;

    if ( (list = findList(ps->cacheMap, peer, hlp, NO_CREATE)) ) {
	if ( (ss = (SState *)xListFirst(list)) != 0 ) {
	    vcacheRemIdleSessn(ps, ss->self);
	    xTrace1(vcachep, TR_MORE_EVENTS, "vcacheFindIdleSessn returns %x",
		    ss->self);
	    return ss->self;
	}
    }
    return ERR_XOBJ;
}


/* 
 * Description in vcache_i.h
 */
void
vcacheRemIdleSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    xkern_return_t	res;
    XObj		lls;

    if ( ps->symmetric || ((SState *)s->state)->type == VCACHE_CLIENT ) {
	xListRemove((XListElem *)s->state);
    }
    lls = xGetDown(s, 0);
    xAssert( xIsSession(lls) );
    res = mapUnbind(ps->collectMap, (char *)&lls);
    xAssert ( res == XK_SUCCESS );
}



static XList
findList( topMap, peer, hlp, create )
    Map		topMap;
    IPhost	*peer;
    XObj	hlp;
    Create	create;
{
    XList	list;
    Bind	bind;
    Map		hlpMap;

    if ( mapResolve(topMap, peer, &hlpMap) == XK_FAILURE ) {
	if ( create == NO_CREATE ) {
	    return 0;
	}
	xTrace1(vcachep, TR_EVENTS,
		"VCACHE findList creating new submap for peer %s",
		ipHostStr(peer));
	hlpMap = mapCreate(VCACHE_HLP_MAP_SZ, sizeof(XObj));
	bind = mapBind(topMap, peer, hlpMap);
	xAssert( bind != ERR_BIND );
    }
    if ( mapResolve(hlpMap, &hlp, &list) == XK_FAILURE ) {
	if ( create == NO_CREATE ) {
	    return 0;
	}
	xTrace1(vcachep, TR_EVENTS,
		"VCACHE findList creating new list for hlpType %s",
		hlp->name);
	list = xListNew();
	bind = mapBind(hlpMap, (char *)&hlp, (int)list);
	xAssert(bind != ERR_BIND);
    }
    return list;
}


/* 
 * Returns the appropriate XList for the given session's host/hlpType pair.
 * The third parameter controls whether the list is created if it
 * doesn't exist.
 */
static XList
findListForSessn( map, s, create )
    Map		map;
    XObj	s;
    Create	create;
{
    SState	*ss = (SState *)s->state;
    XObj	hlpType;

    xAssert(ss);
    hlpType = xHlpType(s);
    return findList(map, &ss->peer, hlpType, create);
}


/* 
 * Description in vcache_i.h
 */
void
vcacheAddIdleSessn( s )
    XObj	s;
{
    SState	*ss = (SState *)s->state;
    PState	*ps;
    XList	list;
#ifdef XK_DEBUG
    XObj	sessnCheck;
#endif
    XObj	lls;

    xTrace1(vcachep, TR_MAJOR_EVENTS, "vcacheAddIdleSessn(%x)", s);
    ps = (PState *)xMyProtl(s)->state;
    s->idle = FALSE;
    if ( ps->symmetric || ss->type == VCACHE_CLIENT ) {
	list = findList(ps->cacheMap, &ss->peer, xHlpType(s), CREATE);
	xAssert(list);
	xListInsert(list, (XListElem *)s->state);
    }
    xAssert(mapResolve(ps->collectMap, &lls, 0) == XK_FAILURE);
    lls = xGetDown(s, 0);
    xAssert(xIsSession(lls));
    mapBind(ps->collectMap, (char *)&lls, s);
    xAssert(mapResolve(ps->collectMap, &lls, &sessnCheck) == XK_SUCCESS &&
	    sessnCheck == s);
}


/* 
 * Add the session s to the active map
 */
void
vcacheAddActiveSessn( ps, s )
    PState	*ps;
    XObj	s;
{
    SState	*ss = (SState *)s->state;
    Bind	b;
    XList	list;
    XObj	lls;

    xTrace1(vcachep, TR_MAJOR_EVENTS, "vcacheAddActiveSessn(%x)", s);
    
    list = findListForSessn(ps->activeMap, s, CREATE);
    xAssert(list);
    xListInsert(list, (XListElem *)s->state);
    if ( (ps->symmetric || (ss->type == VCACHE_CLIENT)) &&
	 (ps->multiRefSessns && ! ps->simpleParticipants) ) {
	lls = xGetDown(s, 0);
	xAssert(xIsSession(lls));
	b = mapBind(ps->activeLlsMap, &lls, s);
	xAssert( b != ERR_BIND ) ;
    }
}


XObj
vcacheFindActiveSessn( ps, h, hlpType )
    PState	*ps;
    IPhost	*h;
    XObj	hlpType;
{
    XList	list;
    SState	*ss;

    if ( ! (list = findList(ps->activeMap, h, hlpType, NO_CREATE)) ) {
	if ( ss = (SState *)xListFirst(list) ) {
	    return ss->self;
	}
    }
    return ERR_XOBJ;
}


xkern_return_t
vcacheRemActiveSessn( s )
    XObj	s;
{
    PState	*ps;
    XObj	lls;

    ps = (PState *)xMyProtl(s)->state;
    xListRemove((XListElem *)s->state);
    if ( (ps->symmetric || (((SState *)s->state)->type == VCACHE_CLIENT)) && 
		( ps->multiRefSessns && ! ps->simpleParticipants ) ) {
	lls = xGetDown(s, 0);
	xAssert(xIsSession(lls));
	return mapUnbind(ps->activeLlsMap, &lls);
    }
    return XK_SUCCESS;
}
