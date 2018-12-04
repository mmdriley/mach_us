/*
 * common_test.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.51 $
 * $Date: 1993/02/01 22:27:24 $
 */

/*
 * This code implements a ping-pong test for another protocol.
 * The same protocol code is used for both the client and the server.  
 *
 * It expects the definitions of following macros which describe
 * the lower protocol:
 *
 *	HOST_TYPE (e.g., ETHhost)
 *	INIT_FUNC (e.g., ethtest_init)
 *	TRACE_VAR (e.g., ethtestp)
 *	PROT_STRING (e.g., "eth")
 *
 * It also needs declarations for Client and Server addresses, e.g.:
 *	HOST_TYPE ServerAddr = { 0xC00C, 0x4558, 0x04d2 };  
 *	HOST_TYPE ClientAddr = { 0xC00C, 0x4558, 0x2694 };  
 *
 * It also needs definitions for the following macros controlling the test:
 *	TRIPS  (number of round trips to make)
 *	TIMES  (number of tests for each length)
 *	DELAY  (number of seconds to delay between each test
 *		(and timeout for declaring failure))
 *
 * Define the macro TIME to do timing calculations.
 *
 * *Finally*, define an array of integers 'lens[]' with the lengths for the
 * tests:
 *
 *	static int lens[] = { 
 *		  1, 200, 400, 600, 800, 1000, 1200
 *	};
 *
 */

/* STREAM_TEST may not work if simul>1 */

#include "xkernel.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#else
#include "assert.h"
#endif ! XKMACHKERNEL
#ifdef XK_MEMORY_THROTTLE
#include "xk_malloc.h"
#endif XK_MEMORY_THROTTLE

#ifdef __STDC__

int	INIT_FUNC( XObj );

static	void 	client( Event, void * );
static 	void 	server( Event, void * );
static 	int	isServerDefault( void );
static 	int	isClientDefault( void );
static 	int	(*isServer)( void ) = isServerDefault;
static 	int	(*isClient)( void ) = isClientDefault;
static 	int	defaultRunTest( int );
static  void	testInit( void );



#else

static	void 	client();
static 	void 	server();
static 	int	isServerDefault();
static 	int	isClientDefault();
static 	int	(*isServer)() = isServerDefault;
static 	int	(*isClient)() = isClientDefault;
/* static 	int	asyncRunTest(); */
static 	int	defaultRunTest();
static  void	testInit();

#endif __STDC__

#ifdef RPCTEST

#ifdef __STDC__
static 	xkern_return_t	testCallDemux( XObj, XObj, Msg *, Msg * );
#else
static 	xkern_return_t	testCallDemux();
#endif __STDC__

#else

static 	int 	gotone;
static 	xmsg_handle_t	clientPushResult = XMSG_NULL_HANDLE;
static 	xmsg_handle_t	serverPushResult = XMSG_NULL_HANDLE;

#ifdef __STDC__
static 	xkern_return_t	test_clientdemux( XObj, XObj, Msg * );
static  xkern_return_t	defaultServerDemux( XObj, XObj, Msg * );
#else
static 	xkern_return_t	test_serverdemux();
static 	xkern_return_t	test_clientdemux();
static  xkern_return_t	defaultServerDemux();
#endif __STDC__

#endif RPCTEST

#ifndef STR2HOST
#  define STR2HOST str2ipHost
#endif


static	HOST_TYPE 	myHost;
static	XObj 		myProtl;
static	XObj	clientDownSes;

#ifdef USE_CONTROL_SESSN
static	XObj	controlSessn;
#endif

static 	int 	count;
static 	int	sentMsgLength;
static	int	serverParam, clientParam, testsSpecified, myTestSpecified;
static	char	*serverString;
static	int	trips = TRIPS;
static  int	simul = 1;  /* number of simultaneous messages circulating */
/* if simul>1, the trips are randomly divided among the messages */
static int		clientTotal;   

#ifdef RPCTEST

#  ifdef __STDC__
static xkern_return_t	serverCallDemuxDefault( XObj, XObj, Msg *, Msg * );
static int		tryCallDefault( XObj, int, int );
#  else
static xkern_return_t	serverCallDemuxDefault();
static int		tryCallDefault();
#  endif

static xkern_return_t	(* serverCallDemux)() = serverCallDemuxDefault;
static int		(* tryCall)() = tryCallDefault;

#else

static Pfk	serverDemux = defaultServerDemux;

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



static Pfi	runTest = defaultRunTest;


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


#ifdef __STDC__
#define DOUBLEQUOTEWRAP(x) #x
#define STRINGIFY(z) DOUBLEQUOTEWRAP(z)
#endif

static void
print_compile_options()
{   printf("\nCompiled with options:\n");
    printf(
#ifdef __STDC__
    "HOST_TYPE "  STRINGIFY(HOST_TYPE)
    "; INIT_FUNC " STRINGIFY(INIT_FUNC)
    "; TRACEVAR "  STRINGIFY(TRACEVAR)  "; "
#endif __STDC__
    "PROT_STRING %s\n", PROT_STRING);
    printf("TRIPS = %d  TIMES = %d  DELAY = %d\n", TRIPS, TIMES, DELAY);
    printf("__STDC__ %s  PROFILE %s  TIME %s  XKMACHKERNEL %s\n",
#ifdef __STDC__
	   "on",
#else
	   "off",
#endif
#ifdef PROFILE
	   "on",
#else
	   "off",
#endif
#ifdef TIME
	   "on",
#else
	   "off",
#endif
#ifdef XKMACHKERNEL
	   "on"
#else
	   "off"
#endif
	   );
    printf("XK_MEMORY_THROTTLE %s  RPCTEST %s  STREAM_TEST %s\n",
#ifdef XK_MEMORY_THROTTLE
	   "on",
#else
	   "off",
#endif
#ifdef RPCTEST
	   "on",
#else
	   "off",
#endif
#ifdef STREAM_TEST
	   "on"
#else
	   "off"
#endif
	   );
    printf("FAILURE_LIMIT %d  CONCURRENCY ", FAILURE_LIMIT);
#ifdef CONCURRENCY
    printf("%d",CONCURRENCY);
#else
    printf("off");
#endif
    printf("  XK_INCOMING_MEMORY_MARK ");
#ifdef XK_INCOMING_MEMORY_MARK
    printf("%d", XK_INCOMING_MEMORY_MARK);
#else
    printf("undefined");
#endif
    printf("\n");
#if (defined(CUSTOM_ASSIGN) || defined(CUSTOM_OPENDONE) || \
     defined(CUSTOM_SERVER_DEMUX) || defined(CUSTOM_CLIENT_DEMUX))
    printf("with");
#ifdef CUSTOM_ASSIGN
	printf(" CUSTOM_ASSIGN");
#endif
#ifdef CUSTOM_OPENDONE
	printf(" CUSTOM_OPENDONE");
#endif
#ifdef CUSTOM_SERVER_DEMUX
	printf(" CUSTOM_SERVER_DEMUX");
#endif
#ifdef CUSTOM_CLIENT_DEMUX
	printf(" CUSTOM_CLIENT_DEMUX");
#endif
    printf("\n");
#endif
#if !(defined(CUSTOM_ASSIGN) && defined(CUSTOM_OPENDONE) && \
      defined(CUSTOM_SERVER_DEMUX) && defined(CUSTOM_CLIENT_DEMUX))
    printf("without");
#ifndef CUSTOM_ASSIGN
	printf(" CUSTOM_ASSIGN");
#endif
#ifndef CUSTOM_OPENDONE
	printf(" CUSTOM_OPENDONE");
#endif
#ifndef CUSTOM_SERVER_DEMUX
	printf(" CUSTOM_SERVER_DEMUX");
#endif
#ifndef CUSTOM_CLIENT_DEMUX
	printf(" CUSTOM_CLIENT_DEMUX");
#endif
    printf("\n");
#endif
    printf("SAVE_SERVER_SESSN %s  CLIENT_OPENABLE %s\n",
#ifdef SAVE_SERVER_SESSN
	   "on",
#else
	   "off",
#endif
#ifdef CLIENT_OPENABLE
	   "on"
#else
	   "off"
#endif
	   );
    printf("USE_CHECKSUM %s  INF_LOOP %s  XK_TEST_ABORT %s\n",
#ifdef USE_CHECKSUM
	   "on",
#else
	   "off",
#endif
#ifdef INF_LOOP
	   "on",
#else
	   "off",
#endif
#ifdef XK_TEST_ABORT
	   "on"
#else
	   "off"
#endif
	   );
  }



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
	} else if ( argPrefix("-trips=") ) {
	    sscanf1(arg + strlen("-trips="), "%d", &trips);
	} else if ( argPrefix("-simul=") ) {
	    sscanf1(arg + strlen("-simul="), "%d", &simul);
	    if (simul>100) {printf("simul clipped to 100\n"); simul=100;};
	    if (simul<1) {printf("simul increased to 1\n"); simul=1;};
	}
    }
    if (trips%simul) {
      trips = simul * (1 + trips/simul);
      printf("Rounded trips up to %d, a multiple of simul(%d).\n",
	     trips, simul);
    };
    printf("Running with trips=%d, simul=%d.\n",trips,simul);
    if (simul>1)
     printf(
      "Be sure that server has a simul value <= %d, and is a divisor of %d.\n",
	    simul, trips);
#undef argPrefix
#undef argEq    
}


int
INIT_FUNC( self )
    XObj self;
{
    XObj	llp;

    xIfTrace(prottest, TR_ERRORS) { print_compile_options(); };
    processOptions();
    if ( testsSpecified && ! myTestSpecified ) {
	xTrace1(prottest, TR_SOFT_ERRORS, "Parameters indicate %s test should not run",
		PROT_STRING);
	return 0;
    }
    printf("%s timing test\n", PROT_STRING);
    myProtl = self;
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	Kabort("Test protocol has no lower protocol");
    }
    xControl(xGetDown(self, 0), GETMYHOST, (char *)&myHost, sizeof(HOST_TYPE));
    /* 
     * Call the per-test initialization function which gives the test
     * the opportunity to override the default functions
     */
    testInit();
    if ((*isServer)()) {
	evDetach( evSchedule(server, 0, 0) );
    } else if ( (*isClient)()) {
#ifdef CONCURRENCY	
	int	i;

	for ( i=0; i < CONCURRENCY; i++ )
#endif
	    evDetach( evSchedule(client, 0, 0) );
    } else {
	printf("%stest: I am neither server nor client\n", PROT_STRING);
    }
    return 0;
}


#ifndef RPCTEST

static void
checkHandle( h, str )
    xmsg_handle_t	h;
    char		*str;
{
    switch ( h ) {
      case XMSG_ERR_HANDLE:
      case XMSG_ERR_WOULDBLOCK:
	sprintf(errBuf, "%s returns error handle %d", str, h);
	Kabort(errBuf);
      default:
	;
    }
}

#endif


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
	STR2HOST(&ServerAddr, serverString);
	ClientAddr = myHost;
	return TRUE;
    }
    return ! bcmp((char *)&myHost, (char *)&ClientAddr, sizeof(HOST_TYPE));
}


#ifndef CUSTOM_ASSIGN

static void
clientSetPart( p )
    Part *p;
{
    partInit(p, 1);
    partPush(*p, &ServerAddr, sizeof(IPhost));
}

static void
serverSetPart( p )
    Part *p;
{
    partInit(p, 1);
    partPush(*p, ANY_HOST, 0);
}

#endif ! CUSTOM_ASSIGN


#ifdef SAVE_SERVER_SESSN
#  ifdef __STDC__
static xkern_return_t	saveServerSessn( XObj, XObj, XObj, XObj );
#  endif

static xkern_return_t
saveServerSessn( self, s, llp, hlpType )
    XObj self, s, llp, hlpType;
{
    xTrace1(prottest, TR_MAJOR_EVENTS,
	    "%s test program duplicates lower server session",
	    PROT_STRING);
    xDuplicate(s);
    return XK_SUCCESS;
}
#endif SAVE_SERVER_SESSN


static xkern_return_t
closeDone( lls )
    XObj	lls;
{
    xTrace2(prottest, TR_MAJOR_EVENTS, "%s test -- closedone (%x) called",
	    PROT_STRING, lls);
#ifdef SAVE_SERVER_SESSION
    if ( (*isServer)() ) {
	xClose(lls);
    }
#endif
    return XK_SUCCESS;
}


static void
server( ev, foo )
    Event	ev;
    VOID 	*foo;
{
    Part p;
    
    printf("I am the  server\n");
#ifdef RPCTEST
    myProtl->calldemux = testCallDemux;
#else
    myProtl->demux = serverDemux;
#endif
#ifdef SAVE_SERVER_SESSN
    myProtl->opendone = saveServerSessn;
#endif 
#ifdef CUSTOM_OPENDONE
    myProtl->opendone = customOpenDone;
#endif 
    myProtl->closedone = closeDone;
    serverSetPart(&p);
    if ( xOpenEnable(myProtl, myProtl, xGetDown(myProtl, 0), &p)
		== XK_FAILURE ) {
	printf("%s test server can't openenable lower protocol\n",
	       PROT_STRING);
    } else {
	printf("%s test server done with xopenenable\n", PROT_STRING);
    }
    return;
}


static void
client( ev, foo )
    Event	ev;
    VOID 	*foo;
{
    Part	p[2];
    int 	lenindex;
    
#ifndef RPCTEST
    myProtl->demux = test_clientdemux;
#endif

    printf("I am the client\n");
#ifdef CLIENT_OPENENABLE
    serverSetPart(p);
    if ( xOpenEnable(myProtl, myProtl, xGetDown(myProtl, 0), p)
		== XK_FAILURE ) {
	printf("%s test client can't openenable lower protocol\n",
	       PROT_STRING);
	return;
    } else {
	printf("%s test client done with xopenenable\n", PROT_STRING);
    }
#endif
    clientSetPart(p);
    if ( clientDownSes == 0 ) {
    	clientDownSes = xOpen(myProtl, myProtl, xGetDown(myProtl, 0), p);
	if ( clientDownSes == ERR_XOBJ ) {
	    printf("%s test: open failed!\n", PROT_STRING);
	    Kabort("End of test");
	    return;
	}
    }
#ifdef CLIENT_OPENDONE
    myProtl->opendone(myProtl, clientDownSes, xGetDown(myProtl, 0), myProtl);
#endif    
#ifdef USE_CONTROL_SESSN
    clientSetPart(p);
    if ( xIsProtocol(xGetDown(myProtl, 1)) ) {
	controlSessn = xOpen(myProtl, myProtl, xGetDown(myProtl, 1), p);
	if ( ! xIsXObj(controlSessn) ) {
	    xError("test protl could not open control sessn");
	}
    } else {
	xTrace1(prottest, TR_EVENTS,
		"%s test client not opening control session",
		PROT_STRING);
    }
#endif USE_CONTROL_SESSN

#ifdef USE_CHECKSUM
    xControl(clientDownSes, UDP_ENABLE_CHECKSUM, NULL, 0);
#endif
#ifdef INF_LOOP
    for (lenindex = 0; ; lenindex++) {
	if (lenindex >= sizeof(lens)/sizeof(long)) {
	    lenindex = 0;
	}
#else
    for (lenindex = 0; lenindex < sizeof(lens)/sizeof(long); lenindex++) {
#endif INF_LOOP	
	sentMsgLength = lens[lenindex];
	if ( runTest(lens[lenindex]) ) {
	    break;
	}
    }
    printf("End of test\n");
    xClose(clientDownSes);
    clientDownSes = 0;
#ifdef XK_TEST_ABORT
    Kabort("test client aborts at end of test");
#endif
    xTrace0(prottest, TR_FULL_TRACE, "test client returns");
}




#ifdef RPCTEST


static int
tryCallDefault( sessn, times, length )
  XObj sessn;
  int times;
  int length;
{
    xkern_return_t ret_val;
    int i;
    Msg	savedMsg, request, reply;
    char *buf;
    int c = 0;
    
    msgConstructAllocate(&savedMsg, length, &buf);
    msgConstructEmpty(&reply);
    msgConstructEmpty(&request);
    for (i=0; i<times; i++) {
	msgAssign(&request, &savedMsg);
	ret_val = xCall(sessn, &request, &reply);
	xIfTrace(prottest, TR_MAJOR_EVENTS) {
	    putchar('.');
	    if (! (++c % 50)) {
		putchar('\n');
	    }
	}
	if( ret_val == XK_FAILURE ) {
	    printf( "RPC call error %d\n" , ret_val );
	    goto abort;
	}
	if (msgLen(&reply) != length) {
	    printf("Bizarre reply length.  Expected %d, received %d\n",
		   length, msgLen(&reply));
	    goto abort;
	}
	msgTruncate(&reply, 0);
    }
#ifdef USE_CONTROL_SESSN
    if ( xIsXObj(controlSessn) ) {
	Msg		m;
	char	*strbuf;

	msgConstructAllocate(&m, 80, &strbuf);
	strbuf[0] = '\n';
	sprintf(strbuf+1, "End of test %d", clientTotal);
	
	xCall(controlSessn, &m, 0);
	msgDestroy(&m);

    }
#endif USE_CONTROL_SESSN

abort:
    msgDestroy(&savedMsg);
    msgDestroy(&reply);
    msgDestroy(&request);
    return i;
}


/* 
 * RPC runTest
 */
static int
defaultRunTest( len )
    int len;
{
    int 	test;
    static int	noResponseCount = 0;
#ifdef TIME    
    XTime 	startTime, endTime, netTime;
#endif

    for (test = 0; test < TIMES; test++) {
	printf("Sending (%d) ...\n", ++clientTotal);
	count = 0;
#ifdef PROFILE
	startProfile();
#endif
#ifdef TIME
	xGetTime(&startTime);
#endif
	if ( (count = tryCall(clientDownSes, trips, len)) == trips ) {
#ifdef TIME
	    xGetTime(&endTime);
	    subtime(&startTime, &endTime, &netTime);
	    printf("\nlen = %4d, %d trips: %6d.%06d\n", 
		   len, trips, netTime.sec, netTime.usec);
#else
	    printf("\nlen = %4d, %d trips\n", len, trips);
#endif
	}
	if ( count == 0 ) {
	    if ( noResponseCount++ == FAILURE_LIMIT ) {
		printf("Server looks dead.  I'm outta here.\n");
		return 1;
	    }
	} else {
	    noResponseCount = 0;
	}
	Delay(DELAY * 1000);
#ifdef XK_MEMORY_THROTTLE
	{
	  extern int max_thread_pool_used, min_memory_avail,
	             max_xk_threads_inuse;
	  printf("used %d incoming threads; %d regular threads; max %d bytes memory",
		 max_thread_pool_used,
		 max_xk_threads_inuse,
		 min_memory_avail);
	}
#endif XK_MEMORY_THROTTLE
    }
    return 0;
}


static xkern_return_t
serverCallDemuxDefault(self, lls, dg, rMsg)
    XObj self, lls;
    Msg *dg, *rMsg;
{
    msgAssign(rMsg, dg);
    return XK_SUCCESS;
}


static xkern_return_t
testCallDemux( self, lls, dg, rMsg )
    XObj self, lls;
    Msg *dg, *rMsg;
{
    static int c = 0;
    
    xIfTrace(prottest, TR_MAJOR_EVENTS) {
	putchar('.');
	if (! (++c % 50)) {
	    putchar('\n');
	}
    }
    msgAssign(rMsg, dg);
    return serverCallDemux(self, lls, dg, rMsg);
}


#else	/* ! RPCTEST */



static int
defaultRunTest( len )
    int len;
{
    Msg		savedMsg;
    static Msg	msga[100];
    int 	test, m;
    char	*buf;
    static int	noResponseCount = 0;
    
    msgConstructAllocate(&savedMsg, len, &buf);
    for (test = 0; test < TIMES; test++) {
	for (m=0; m<simul; m++) {msgConstructCopy(&msga[m], &savedMsg);};
	printf("Sending (%d) ...\n", ++clientTotal);
	printf("msg length: %d\n", msgLen(&msga[0]));
	count = 0;
#ifdef PROFILE
	startProfile();
#endif
#ifdef TIME
	xGetTime(&starttime);
#endif
#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif XK_MEMORY_THROTTLE
	for (m=0; m<simul; m++) {
	    clientPushResult = xPush(clientDownSes, &msga[m]);
	    checkHandle(clientPushResult, "client push");
	}
	do {
	    gotone = 0;
	    Delay(DELAY * 1000);
	} while (gotone);
	if (count < trips) {
	    printf("Test failed after receiving %d packets\n", count);
#ifdef STREAM_TEST
	    receivedLength = 0;
#endif
	} 
	if ( count == 0 ) {
	    if ( noResponseCount++ == FAILURE_LIMIT ) {
		printf("Server looks dead.  I'm outta here.\n");
		return 1;
	    }
	} else {
	    noResponseCount = 0;
	}
	for (m=0; m<simul; m++) { msgDestroy(&msga[m]); };
    }
    msgDestroy(&savedMsg); 
    return 0;
}


static xkern_return_t
defaultServerDemux( self, lls, dg )
    XObj self, lls;
    Msg *dg;
{
    static int c = 0;
    static Msg msga[100]; static int msgi = 0; int i;

    xIfTrace(prottest, TR_MAJOR_EVENTS) {
	putchar('.');
	if (! (c++ % 50)) {
	    putchar('\n');
	}
    }
#ifdef CUSTOM_SERVER_DEMUX
    customServerDemux(self, lls, dg);
#endif CUSTOM_SERVER_DEMUX
#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif XK_MEMORY_THROTTLE
    /* if simul>1, save up a group of messages, then return them */
    /* if things get out of sync, they get really screwed up! */
    if (simul>1) {
      msgConstructCopy(&msga[msgi],dg); msgi++;
      if (msgi==simul) {
	for (i=0; i<simul; i++) {
	    serverPushResult = xPush(lls, &msga[i]);
	    checkHandle(serverPushResult, "server push");
	    msgDestroy(&msga[i]);
	}
	msgi=0;
      };
    } else {
	serverPushResult = xPush(lls, dg);
	checkHandle(serverPushResult, "server push");
    }
    return XK_SUCCESS;
}



#  ifdef STREAM_TEST


static xkern_return_t
test_clientdemux( self, lls, dg )
    XObj self, lls;
    Msg *dg;
{
#ifdef TIME
    XTime 	now, total;
#endif
    char		*buf;
    Msg			msgToPush;
    xmsg_handle_t	h;
    
    gotone = 1;

    xTrace1(prottest, TR_EVENTS, "R %d", msgLen(dg));
    receivedLength += msgLen(dg);
    xTrace1(prottest, TR_DETAILED, "total length = %d", receivedLength);
#ifdef CUSTOM_CLIENT_DEMUX
    customClientDemux(self, lls, dg);
#endif CUSTOM_CLIENT_DEMUX
    if (receivedLength == sentMsgLength) {
	/*
	 * Entire response has been received.
	 * Send another message
	 */
	if (++count < trips) {
	    xIfTrace(prottest, TR_MAJOR_EVENTS) { 
		putchar('.');
		if (! (count % 50)) {
		    putchar('\n');
		}
	    }
	  if (count+simul <= trips) {
	    msgConstructAllocate(&msgToPush, receivedLength, &buf);
	    receivedLength = 0;
	    xTrace0(prottest, TR_EVENTS, "S");
#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif XK_MEMORY_THROTTLE
	    h = xPush(lls, &msgToPush);
	    checkHandle(h, "client push");
	    msgDestroy(&msgToPush);
	  } else { receivedLength = 0; };
	} else {
#ifdef TIME
	    xGetTime(&now);
	    subtime(&starttime, &now, &total);
	    printf("\nlen = %4d, %d trips: %6d.%06d\n", 
		   receivedLength, trips, total.sec, total.usec);
#else
	    printf("\nlen = %4d, %d trips\n", receivedLength, trips);
#endif
	    receivedLength = 0;
#ifdef PROFILE
	    endProfile();
#endif
	}
    }
    return XK_SUCCESS;
}


#  else /* ! STREAM_TEST */


static xkern_return_t
test_clientdemux( self, lls, dg )
    XObj self, lls;
    Msg *dg;
{
#ifdef TIME
    XTime now, total;
#endif
    static Msg msga[100]; static int msgi = 0; int i;

/* note that customdemux is not called on final response message */
#ifdef CUSTOM_CLIENT_DEMUX
    customClientDemux(self, lls, dg);
#endif CUSTOM_CLIENT_DEMUX
    gotone = 1;
    if ( ++count < trips ) {
	xIfTrace(prottest, TR_MAJOR_EVENTS) {
	    putchar('.');
	    if (! (count % 50)) {
		putchar('\n');
	    }
	}
#ifdef XK_MEMORY_THROTTLE
	while ( memory_unused < XK_INCOMING_MEMORY_MARK)
	  Delay(DELAY * 1000);
#endif XK_MEMORY_THROTTLE
	if (simul>1) {
	  msgConstructCopy(&msga[msgi],dg); msgi++;
	  if (msgi==simul) {
	    for (i=0; i<simul; i++) {
		clientPushResult = xPush(clientDownSes, &msga[i]);
		checkHandle(clientPushResult, "client push");
		msgDestroy(&msga[i]);
	    }
	    msgi=0;
	  };
	}
	else {
	  /* if (count+simul<=trips) */
	  clientPushResult = xPush(clientDownSes, dg);
	  checkHandle(clientPushResult, "client push");
      }
    } else {
        /* should clear out msga when simul>1, but skip it for now */
        msgi=0;
#ifdef TIME
	xGetTime(&now);
	subtime(&starttime, &now, &total);
	printf("\nlen = %4d, %d trips: %6d.%06d\n", 
		msgLen(dg), trips, total.sec, total.usec);
#else
	printf("\nlen = %4d, %d trips\n", msgLen(dg), trips);
#endif
	
#ifdef PROFILE
	endProfile();
#endif
    }
    return XK_SUCCESS;
}


#  endif STREAM_TEST



#endif RPCTEST




#ifdef TIME
static void
subtime(t1, t2, t3)
    XTime *t1, *t2, *t3;
{
    t3->sec = t2->sec - t1->sec;
    t3->usec = t2->usec - t1->usec;
    if (t3->usec < 0) {
	t3->usec += 1000000;
	t3->sec -= 1;
    }
}
#endif


