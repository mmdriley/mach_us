/*     
 * $RCSfile: tcptest.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.23 $
 * $Date: 1993/02/01 22:26:44 $
 */

/*
 * Ping-pong test of TCP
 */

#include "xkernel.h"
#include "tcp.h"
#include "ip.h"
#include "site.h"

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC tcptest_init
#define TRACE_VAR tcptestp
#define PROT_STRING "tcp"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

#define TRIPS 100
#define TIMES 1
#define DELAY 3
/*
 * Define to do timing calculations
 */
#define TIME


long	serverPort = 2001;
long	clientPort = ANY_PORT;

#define CUSTOM_ASSIGN

static void
clientSetPart( p )
    Part	*p;
{
    partInit(p, 1);
    partPush(p[0], &ServerAddr, sizeof(IPhost));
    partPush(p[0], &serverPort, sizeof(long));
    /* 
     * If we don't specify the second participant, TCP will select a port
     * for us. 
     */
#if 0
    /* 
     * NOTE -- if you use two participants, make sure the second argument to 
     * the above call to partInit is 2
     */
    partPush(p[1], ANY_HOST, 0);
    partPush(p[1], &clientPort, sizeof(long));
#endif
}

static void
serverSetPart( p )
    Part	*p;
{
    partInit(p, 1);
    partPush(*p, ANY_HOST, 0);
    partPush(*p, &serverPort, sizeof(long));
}

static int lens[] = { 
  1, 4000, 8000, 16000
};


#define BUFFER_SIZE	4096

#define CUSTOM_OPENDONE
#define CLIENT_OPEN_DONE

static xkern_return_t
customOpenDone( self, s, llp, hlpType )
    XObj self, s, llp, hlpType;
{
    u_short space = BUFFER_SIZE;

    xTrace0(prottest, TR_MAJOR_EVENTS, "tcp test program openDone");
    xDuplicate(s);

    if (xControl(s, TCP_SETRCVBUFSIZE, (char*)&space, sizeof(space)) < 0) {
	xError("saveServerSessn: TCP_SETRCVBUFSIZE failed");
    } /* if */
    if (xControl(s, TCP_SETRCVBUFSPACE, (char*)&space, sizeof(space)) < 0){
	xError("saveServerSessn: TCP_SETRCVBUFSPACE failed");
    } /* if */

    return XK_SUCCESS;
}
    

#define CUSTOM_CLIENT_DEMUX

static xkern_return_t
customClientDemux( self, lls, dg )
    XObj	self, lls, dg;
{
    u_short space = BUFFER_SIZE;

    xTrace1(prottest, TR_DETAILED, "TCP custom demux resets buffer to size %d",
	    BUFFER_SIZE);
    if (xControl(lls, TCP_SETRCVBUFSPACE, (char*)&space, sizeof(space)) < 0) {
	xError("TCP custom test_demux: TCP_SETRCVBUFSPACE failed");
    } /* if */
    return XK_SUCCESS;
}

#define CUSTOM_SERVER_DEMUX

static xkern_return_t
customServerDemux( self, lls, dg )
    XObj	self, lls, dg;
{
    return customClientDemux( self, lls, dg );
}

#define STREAM_TEST

#include "common_test.c"

static void
testInit()
{
}
