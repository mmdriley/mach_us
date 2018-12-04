/*     
 * $RCSfile: xrpctest.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 22:27:36 $
 */

/*
 * Ping-pong test of XRPC
 */

#include "xkernel.h"

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC xrpctest_init
#define PROT_STRING "xrpc"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };
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
    partPush(*p, &ServerAddr, sizeof(IPhost));	/* IP host 		*/
    partPush(*p, &serverId, sizeof(long));
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
#endif    
}


static int lens[] = { 
  1000, 2000, 4000, 8000, 16000
};


#include "common_test.c"

static void
testInit()
{
}
