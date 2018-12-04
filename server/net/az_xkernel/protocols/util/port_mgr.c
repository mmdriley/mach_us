/* 
 * port_mgr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.17 $
 * $Date: 1993/02/01 22:30:12 $
 */

/* 
 * Management of ports
 *
 * This file is designed to be included by another source file which
 * defines these macros:
 *
 *	PORT -- the port type 
 *	MAX_PORT -- maximum allowed port
 *	FIRST_USER_PORT -- the first port which may be handed out through
 *		'getFreePort'
 *	NAME -- token to prepend to the routine names
 *	PROT_NAME -- string of protocol name (for debugging)
 *	TRACE_VAR -- trace variable to use in tracing
 * 
 * NOTE -- this code assumes a port is no larger than an int.
 * 
 */

#include "xkernel.h"

#define new(Type) (Type *)xMalloc(sizeof(Type))


int	traceportmgr;
#define DUMP    xIfTrace(portmgr, TR_DETAILED) { displayMap(); }



typedef struct {
    int		rcnt;
    long	port;
} PortDesc;

static	Map		portMap;
static	unsigned long	nextPort = FIRST_USER_PORT;
static	char		msgBuf[200];

void
#ifdef __STDC__
PASTE(NAME,PortMapInit)
#else
NAME/**/PortMapInit  
#endif
  ()
{
    if ( ! portMap ) {
	portMap = mapCreate(PORT_MAP_SIZE, sizeof(long));
    }
}


static int
displayElem( key, value, idx )
    VOID *key;
    int value;
    VOID *idx;
{
    PortDesc	*pd = (PortDesc *)value;

    xAssert(pd);
    sprintf(msgBuf, 
	    "Element %d:	  port = %d  rcnt = %d",
	   ++*(int *)idx, pd->port, pd->rcnt);
    xError(msgBuf);
    return MFE_CONTINUE;
}


static void
displayMap()
{
    int	i = 0;
    sprintf(msgBuf, "dump of %s port map:", PROT_NAME);
    xError(msgBuf);
    mapForEach(portMap, displayElem, &i);
}



/* 
 * Binds 'port' into the map with the indicated reference count.
 * Returns 0 on a successful bind, 1 if the port could not be bound
 * (indicating that it was already bound.)
 */
static int
portBind( port, rcnt )
    long port;
    int rcnt;
{
    PortDesc	*pd;

    pd = new(PortDesc);
    pd->rcnt = rcnt;
    pd->port = port;
    if ( mapBind(portMap, (char *)&port, (int)pd) == ERR_BIND ) {
	xFree((char *)pd);
	return 1;
    } 
    return 0;
}


static void
portUnbind( pd )
    PortDesc *pd;
{
    xAssert( pd && pd != (PortDesc *) -1 );
    mapUnbind(portMap, &pd->port);
    xFree((char *)pd);
}


int
#ifdef __STDC__
PASTE(NAME,GetFreePort)
#else
NAME/**/GetFreePort
#endif
  ( port )
    long *port;
{
    unsigned long firstPort;

    xAssert(portMap);
    firstPort = nextPort;
    do {
	*port = nextPort;
	if (nextPort >= MAX_PORT) {
	    nextPort = FIRST_USER_PORT;
	} else {
	    nextPort++;
	} /* if */
	if ( portBind(*port, 1) == 0 ) {
	    /* 
	     * Found a free port
	     */
	    DUMP;
	    return 0;
	}
    } while ( nextPort != firstPort );
    return 1;
}


int
#ifdef __STDC__
PASTE(NAME,DuplicatePort)
#else
NAME/**/DuplicatePort
#endif
  ( port )
    long port;
{
    PortDesc	*pd;
    int		res;

    xAssert(portMap);
    if ( port > MAX_PORT ) {
	res = 2;
    } else {
	if ( mapResolve(portMap, &port, &pd) == XK_FAILURE ) {
	    /* 
	     * Port is not used, so we know portBind will succeed.
	     */
	    res = portBind(port, 1);
	} else {
	    pd->rcnt++;
	    res = 0;
	}
    }
    DUMP;
    return res;
}


void
#ifdef __STDC__
PASTE(NAME,ReleasePort)
#else
NAME/**/ReleasePort
#endif
  ( port )
    long port;
{
    PortDesc	*pd;

    xAssert(portMap);
    if ( mapResolve(portMap, &port, &pd) == XK_SUCCESS ) {
	if ( pd->rcnt > 0 ) {
	    if ( --pd->rcnt == 0 ) {
		portUnbind(pd);
	    }
	}
    }
    DUMP;
}
