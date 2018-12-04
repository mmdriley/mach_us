/*
 * xfertest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:35:22 $
 */

#include "xkernel.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#else
#include "assert.h"
#endif ! XKMACHKERNEL
#ifdef XK_MEMORY_THROTTLE
#include "xk_malloc.h"
#endif XK_MEMORY_THROTTLE

#include "machripc_internal.h"
#include "machripc_xfer.h"
#include "srx.h"
#include "rrx.h"
#include "bidctl.h"
#include "vnet.h"


void	xfertest_init( XObj );


#ifdef __STDC__

static mnetport *createLocalPort( mportNetRep * );

static	void 	client( Event, void * );
static 	void 	server( Event, void * );
static 	int	isServerDefault( void );
static 	int	isClientDefault( void );
static 	int	(*isServer)( void ) = isServerDefault;
static 	int	(*isClient)( void ) = isClientDefault;

#else

static mnetport *createLocalPort();
static	void 	client();
static 	void 	server();
static 	int	isServerDefault();
static 	int	isClientDefault();
static 	int	(*isServer)() = isServerDefault;
static 	int	(*isClient)() = isClientDefault;
/* static 	int	asyncRunTest(); */
static 	int	defaultRunTest();
static  void	testInit();
static  xkern_return_t	defaultServerDemux();

#endif __STDC__

#ifdef RPCTEST

#ifdef __STDC__
static 	xkern_return_t	testCallDemux( XObj, XObj, Msg *, Msg * );
#else
static 	xkern_return_t	testCallDemux();
#endif __STDC__

#else


#ifdef __STDC__
#else
static 	xkern_return_t	test_serverdemux();
static 	xkern_return_t	test_clientdemux();
#endif __STDC__

#endif


#define HOST_TYPE	IPhost
#define PROT_STRING	"xfer"

static HOST_TYPE ServerAddr = SITE_SERVER_IP;
static HOST_TYPE ClientAddr = SITE_CLIENT_IP;

static	HOST_TYPE 	myHost;
static	XObj 		myProtl;
static	XObj 		bidctl;
static	int	serverParam, clientParam, testsSpecified, myTestSpecified;
static	char	*serverString;
static  int	simul = 1;  /* number of simultaneous messages circulating */
/* if simul>1, the trips are randomly divided among the messages */

#ifdef RPCTEST
static xkern_return_t	serverCallDemuxDefault();
static int		tryCallDefault();
static xkern_return_t	(* serverCallDemux)() = serverCallDemuxDefault;
static int		(* tryCall)() = tryCallDefault;

#else


#endif RPCTEST

#ifdef TIME
#  ifndef RPCTEST
static	XTime 	starttime;
#  endif
static 	void	subtime(
#ifdef __STDC__
			XTime *t1, XTime *t2, XTime *result
#endif
			);
#endif
#ifdef STREAM_TEST
static	int	receivedLength = 0;
#endif

#define FAILURE_LIMIT 2


#ifdef __STDC__

static void	addHostRef( IPhost * );

#endif


#ifdef XKMACHKERNEL
static int
sscanf1(str, format, a1)
char *str, format;
int *a1;
{
  int n;

  *a1=0;
  while (*str >= '0' && *str <= '9')
    *a1 = 10*(*a1) + (*str++ - '0');
  return(1);
}
#else
#define sscanf1 sscanf
#endif XKMACHKERNEL


static void
processOptions()
{
    int		i;
    char 	*arg;

#define argPrefix(str) (! strncmp(arg, str, strlen(str)))
#define argEq(str) (! strcmp(arg, str) )

    for (i=1; i < globalArgc; i++) {
	arg = globalArgv[i];
	if ( argEq("-s") ) {
	    serverParam = 1;
	} else if ( argPrefix("-c") ) {
	    clientParam = 1;
	    serverString = arg + 2;
	} else if ( argPrefix("-test") ) {
	    testsSpecified = 1;
	    arg += strlen("-test");
	    if ( argEq(PROT_STRING) ) {
		myTestSpecified = 1;
	    }
	} else if ( argPrefix("-simul=") ) {
	    sscanf1(arg + strlen("-simul="), "%d", &simul);
	    if (simul>100) {printf("simul clipped to 100\n"); simul=100;};
	    if (simul<1) {printf("simul increased to 1\n"); simul=1;};
	}
    }
#undef argPrefix
#undef argEq    
}


static mnetport	netPorts[10];
static int	numNetPorts;



static void
loadPorts()
{
    int		i, j;
    mnetport	*npd;
    mportNetRep	nr;
    
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], "machport") ) {
	    if ( ! rom[i][1] || ! rom[i][2] ) {
		xError("loadPorts syntax error");
		continue;
	    }
	    sscanf(rom[i][1], "%d", &nr.net_port_number);
	    if ( str2ipHost(&nr.receiver_host_addr, rom[i][2]) == XK_FAILURE ) {
		xError("loadPorts syntax error");
		continue;
	    }
	    nr.make_send_count = 12;
	    if ( rom[i][3] && ! strcmp(rom[i][3], "senders") ) {
		nr.net_port_rights = MACH_PORT_TYPE_RECEIVE;
		npd = createLocalPort(&nr);
		for ( j=4; rom[i][j]; j++ ) {
		    IPhost	host;
		    
		    if ( str2ipHost(&host, rom[i][j]) == XK_FAILURE ) {
			xError("loadPorts syntax error");
			continue;
		    }
		    addNewSender(npd, host, 12);
		}
	    } else {
		if ( rom[i][3] ) {
		    sprintf(errBuf, "loadPorts syntax error in line %d",
			    i+1);
		    xError(errBuf);
		}
		nr.net_port_rights = MACH_PORT_TYPE_SEND;
		npd = createLocalPort(&nr);
	    }
	}
    }
}




void
xfertest_init( self )
    XObj self;
{

    xError("xfertest_init");
    myProtl = self;
    bidctl = xGetDown(self, 0);
    xAssert( xIsProtocol(bidctl) && bidctl == xGetProtlByName("bidctl") );
    processOptions();
    loadPorts();
    if ( testsSpecified && ! myTestSpecified ) {
	xTrace1(prottest, TR_SOFT_ERRORS, "Parameters indicate %s test should not run",
		PROT_STRING);
	return;
    }
    xControl(xGetDown(self, 0), GETMYHOST, (char *)&myHost, sizeof(HOST_TYPE));
    xTrace1(prottest, TR_GROSS_EVENTS, "I am host %s", ipHostStr(&myHost));
    printf("%s test\n", PROT_STRING);
    if ((*isServer)()) {
	evDetach( evSchedule(server, 0, 0) );
    } else if ( (*isClient)()) {
	evDetach( evSchedule(client, 0, 0) );
    } else {
	printf("%stest: I am neither server nor client\n", PROT_STRING);
    }
}


static int
isServerDefault()
{
    if ( serverParam ) {
	return TRUE;
    }
    return ! bcmp((char *)&myHost, (char *)&ServerAddr, sizeof(HOST_TYPE));
}


static int
isClientDefault()
{
    
    if ( clientParam ) {
	str2ipHost(&ServerAddr, serverString);
	ClientAddr = myHost;
	return TRUE;
    }
    return ! bcmp((char *)&myHost, (char *)&ClientAddr, sizeof(HOST_TYPE));
}



#define MESSAGE_ID	12345
/* #define MESSAGE_ID_2	23456 */

static void
msgArrivedStub( ev, arg )
    Event	*ev;
    VOID	*arg;
{
    rrxTransferComplete(ClientAddr, (int)arg);
    srxTransferComplete(ClientAddr, (int)arg);
}


static void
server( ev, foo )
    Event	ev;
    VOID 	*foo;
{
    printf("I am the server\n");
    evSchedule(msgArrivedStub, (VOID *)MESSAGE_ID, 45 * 1000 * 1000);  
#ifdef MESSAGE_ID_2
    evSchedule(msgArrivedStub, (VOID *)MESSAGE_ID_2, 50 * 1000 * 1000);  
#endif
}


/* 
 * The client sends all of its ports (indicated in the ROM file) to
 * the server.   
 */
static void
client( ev, foo )
    Event	ev;
    VOID 	*foo;
{
    mnetport	*sendRightList[10];
    mnetport	*rcvRightList[10];
#ifdef MESSAGE_ID_2
    mnetport	*sendRightList2[10];
#endif
    int		i, si, ri;
    mnetport	*p;
    
    printf("I am the client\n");
    for ( i=0, si=0, ri=0; i < numNetPorts; i++ ) {
	p = &netPorts[i];
	if ( p->amReceiver ) {
	    rcvRightList[si++] = p;
	} else {
	    sendRightList[ri++] = p;
	}
    }
    rcvRightList[si] = sendRightList[ri] = 0;
#ifdef MESSAGE_ID_2
    /* 
     * Repeat one of the msg1 ports
     */
    sendRightList2[0] = sendRightList[0];
    sendRightList2[1] = 0;
#endif
#if 1
    printf("client delays\n");
    Delay(5 * 1000);
#endif
    srxMoveSendRights(ServerAddr, MESSAGE_ID, sendRightList);
#ifdef MESSAGE_ID_2
    srxMoveSendRights(ServerAddr, MESSAGE_ID_2, sendRightList2);
#endif
    rrxMoveReceiveRights(ServerAddr, MESSAGE_ID, rcvRightList);
    xError("test client returns");
}



xkern_return_t
findNetPort( port, tag, create, ptr )
    mportNetRep		*port;
    mn_arch_tag_t	tag;
    mnetport		**ptr;
    bool		create;
{
    int	i;

    for ( i=0; i < numNetPorts; i++ ) {
	if ( netPorts[i].net_port_number == port->net_port_number ) {
	    *ptr = &netPorts[i];
	    return XK_SUCCESS;
	}
    }
    if ( create ) {
	*ptr = createLocalPort(port);
	return XK_SUCCESS;
    } else {
	return XK_FAILURE;
    }
}


static void
addHostRef( h )
    IPhost	*h;
{
    BidctlBootMsg	bm;
    Part		part;

    xTrace1(prottest, TR_EVENTS, "xfertest -- adding ref for host %s",
	    ipHostStr(h));
    partInit(&part, 1);
    partPush(part, h, sizeof(IPhost));
    xOpenEnable(myProtl, myProtl, bidctl, &part);
    bm.h = *h;
    bm.id = 0;
    xControl(bidctl, BIDCTL_GET_PEER_BID_BLOCKING, (char *)&bm,
	     sizeof(bm));
    xTrace2(prottest, TR_EVENTS, "xfertest -- host %s's BID == %x",
	    ipHostStr(h), bm.id);
}


static mnetport *
createLocalPort( np )
    mportNetRep	*np;
{
    mnetport	*p;

    xTrace2(prottest, TR_MAJOR_EVENTS,
	    "adding local mach port id %d, rcv host %s",
	    np->net_port_number, ipHostStr(&np->receiver_host_addr));
    p = &netPorts[numNetPorts];
    p->net_port_number = np->net_port_number;
    p->receiver_host_addr = np->receiver_host_addr;
    p->senders_map = mapCreate(1, sizeof(IPhost));
    rwLockInit(&p->rwlock);
    p->amReceiver = ( np->net_port_rights == MACH_PORT_TYPE_RECEIVE );
    if ( p->amReceiver ) {
	xTrace0(prottest, TR_MAJOR_EVENTS, "I am this port's receiver");
    } else {
	addHostRef(&np->receiver_host_addr);
    }
    numNetPorts++;
    return p;
}


xkern_return_t
addSendRight( npd )
    mnetport	*npd;
{
    sprintf(errBuf, "addSendRight to port %d", npd->net_port_number);
    xError(errBuf);
    return XK_SUCCESS;
}


xkern_return_t
addReceiveRight( npd )
    mnetport	*npd;
{
    sprintf(errBuf, "addReceiveRight to port %d", npd->net_port_number);
    xError(errBuf);
    return XK_SUCCESS;
}


void
addNewSender( npd, sender, msc )
    mnetport	*npd;
    IPhost	sender;
    int		msc;
{
    int		oldMsc;
    Bind	b;

    xTrace2(prottest, TR_EVENTS, "adding reference to host %s for port %d",
	    ipHostStr(&sender), npd->net_port_number);
    xAssert(npd->senders_map);
    if ( mapResolve(npd->senders_map, &sender, &oldMsc) == XK_SUCCESS ) {
	mapUnbind(npd->senders_map, &sender);
	msc = oldMsc + 1;
    }
    xTrace2(prottest, TR_EVENTS, "New make-send-count for port %d id %d",
	    npd->net_port_number, msc);
    b = mapBind(npd->senders_map, &sender, msc);
    xAssert( b != ERR_BIND );
    addHostRef(&sender);
}


void
removeSender( npd, sender )
    mnetport	*npd;
    IPhost	sender;
{
    xTrace2(prottest, TR_EVENTS, "removing reference to host %s for port %d",
	    ipHostStr(&sender), npd->net_port_number);
}


void
removeSendRight( npd )
    mnetport	*npd;
{
    xTrace1(prottest, TR_EVENTS,
	    "xfertest remove send right for port %d",
	    npd->net_port_number);
}


void
removeReceiveRight( npd )
    mnetport	*npd;
{
    xTrace1(prottest, TR_EVENTS,
	    "xfertest remove receive right for port %d",
	    npd->net_port_number);
}



void
receiveRightDeallocated( npd )
    mnetport	*npd;
{
    xTrace1(prottest, TR_EVENTS,
	    "xfertest receives port death notification for port %d",
	    npd->net_port_number);
}



void
receiverMoved( npd )
    mnetport	*npd;
{
    sprintf(errBuf,
	    "xfertest receiver moved notification for port %d (new rcvr %s)",
	    npd->net_port_number, ipHostStr(&npd->receiver_host_addr));
    xError(errBuf);
    addHostRef(&npd->receiver_host_addr);
}



char *
mportNetRepStr( mportNetRep *mp )
{
    static char	buf[80];

    sprintf(buf, "%d", *mp);
    return buf;
}
