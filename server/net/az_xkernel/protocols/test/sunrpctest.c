/*     
 * $RCSfile: sunrpctest.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/01 22:27:05 $
 */

/*
 * Ping-pong test of SUNRPC
 */

#include "xkernel.h"
#include <rpc/rpc.h>
#include "sunrpc.h"

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC sunrpctest_init
#define TRACE_VAR sunrpctestp
#define PROT_STRING "sunrpc"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

static	long	prog = 100, proc = 5, vers = 2;
static	long	clientPort = 2000, serverPort = 1002;

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
    partPush(*p, &ServerAddr, sizeof(IPhost));	/* IP host 		*/
    partPush(*p, &serverPort, sizeof(long));
    partPush(*p, &prog, sizeof(long));
    partPush(*p, &vers, sizeof(long));
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
    Part	*p;
{
    partInit(p, 2);
    setServer(p);
#if 1
    partPush(p[1], ANY_HOST, 0);
    partPush(p[1], &clientPort, sizeof(long));
#endif    
}


static int lens[] = { 
  1000, 2000, 4000, 8000, 16000
};


#include "common_test.c"


static void
nullPush( hdr, des, len, arg )
    VOID	*hdr, *arg;
    long	len;
    char	*des;
{
}


static xkern_return_t
sunrpcCallDemux( self, lls, dg, rMsg )
    XObj self, lls;
    Msg *dg, *rMsg;
{
    msgAssign(rMsg, dg);
    msgPush(rMsg, nullPush, 0, sizeof(long), 0);
    return XK_SUCCESS;
}


static void
testInit()
{
    serverCallDemux = sunrpcCallDemux;
}
