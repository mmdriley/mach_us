/*
 * xrpc_conc_test.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:27:41 $
 */

/*
 * Test concurrent operations on XRPC
 */

#include "xkernel.h"

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC xrpctest_init
#define PROT_STRING "xrpc"

static	HOST_TYPE ServerAddr = SITE_SERVER_IP;
static	HOST_TYPE ClientAddr = SITE_CLIENT_IP;
static	long	serverId = 1002;

#define TRIPS 100
#define TIMES 1
#define DELAY 3
/*
 * Define to do timing calculations
 */
#define TIME
#define SAVE_SERVER_SESSN
#define RPCTEST
#define CUSTOM_ASSIGN

static void
setServer( p )
    Part	*p;
{
    partPush(*p, &ServerAddr);	/* IP host 		*/
    partPush(*p, &serverId);
}

static void
serverSetPart( p )
    Part	*p;
{
    partInit(p, 1);
    setServer(p);
}


static void
clientSetPart( p )
    Part 	*p;
{
    partInit(p, 2);
    setServer(p);
#if 1
    partPush(p[1], ANY_HOST);
#endif    
}


static int lens[] = { 
  1000, 2000, 4000, 8000, 16000
};

#define CONCURRENCY	5

#include "common_test.c"

static int	argumentBase = 1;


static void
xrpcStore( hdr, dst, len, arg )
    long	*hdr, len;
    VOID	*dst, *arg;
{
    long	n;

    n = htonl(*hdr);
    bcopy((char *)&n, dst, sizeof(long));
}


static long
xrpcLoad( hdr, src, len, arg )
    long	*hdr, len;
    VOID	*src, *arg;
{
    bcopy((char *)src, (char *)hdr, sizeof(long));
    *hdr = ntohl(*hdr);
    return sizeof(long);
}


static int
xrpcTryCall( lls, times, length )
    XObj	lls;
    int 	times, length;
{
    xkern_return_t ret_val;
    int i;
    Msg	savedMsg, request, reply;
    int count = 0;
    long	num, base, square;
    
    msgConstructEmpty(&savedMsg);
    msgConstructEmpty(&reply);
    msgConstructEmpty(&request);
    num = base = argumentBase++;
    xTrace1(prottest, TR_MAJOR_EVENTS, "rpc square test using base %d", num);
    for (i=0; i < times; i++, num += CONCURRENCY) {
	msgAssign(&request, &savedMsg);
	msgPush(&request, xrpcStore, &num, sizeof(num), 0);
	ret_val = xCall(lls, &request, &reply);
	xIfTrace(prottest, 3) {
	    putchar('.');
	    if (! (++count % 50)) {
		putchar('\n');
	    }
	}
	if( ret_val == XK_FAILURE ) {
	    printf( "RPC call error %d\n" , ret_val );
	    goto abort;
	}
	if ( msgPop(&reply, xrpcLoad, (VOID *)&square, sizeof(square), 0)
	    			== FALSE ) {
	    xError("RPC call could not pop reply");
	    goto abort;
	}
	if ( square != num * num ) {
	    xError("RPC reply error");
	    xTrace2(prottest, TR_ERRORS, "%d is not square of %d",
		    square, num);
	} else {
	    xTrace2(prottest, TR_DETAILED, "square of %d is %d",
		    num, square);
	}
	msgTruncate(&reply, 0);
    }
    xTrace1(prottest, TR_MAJOR_EVENTS,
	    "rpc square test with base %d completes", base);
    return times;

abort:
    msgDestroy(&savedMsg);
    msgDestroy(&reply);
    msgDestroy(&request);
    return i;
}


static xkern_return_t
xrpcCallDemux( self, lls, dg, rMsg )
    XObj self, lls;
    Msg *dg, *rMsg;
{
    long	num;

/* #define TEST_DELAY	5 */
#ifdef TEST_DELAY
    static int	count = 0;
    if ( ! ( ++count % 5 ) ) {
	xTrace1(prottest, TR_MAJOR_EVENTS,
		"rpc test delaying %d seconds", TEST_DELAY);
	Delay( TEST_DELAY * 1000 );
    }
#endif
    if ( msgPop(dg, xrpcLoad, (VOID *)&num, sizeof(num), 0) == FALSE ) {
	xError("xrpcCallDemux -- could not pop value");
	return XK_FAILURE;
    }
    xTrace1(prottest, TR_DETAILED,
	    "rpc square server called with arg %d", num);
    num *= num;
    xTrace1(prottest, TR_DETAILED,
	    "rpc square server sending result %d", num);
    msgPush(rMsg, xrpcStore, &num, sizeof(num), 0);
    return XK_SUCCESS;
}


static void
testInit()
{
    tryCall = xrpcTryCall;
    serverCallDemux = xrpcCallDemux;
}

