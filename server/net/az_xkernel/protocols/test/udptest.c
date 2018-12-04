/*     
 * $RCSfile: udptest.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.26 $
 * $Date: 1993/02/01 22:27:11 $
 */


/*
 * Ping-pong test of UDP
 */

#include "part.h"
#include "udp.h"
#include "site.h"

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC udptest_init
#define TRACE_VAR udptestp
#define PROT_STRING "udp"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

static long	serverPort = 2001;

#define TRIPS 100
#define TIMES 1
#define DELAY 3
/*
 * Define to do timing calculations
 */
#define TIME


/* 
 * It may be convenient to send between different UDP ports on the
 * same host, so we'll have custom participant assignment
 */
#define CUSTOM_ASSIGN

static void
clientSetPart( p )
    Part	*p;
{
    partInit(p, 1);
    partPush(p[0], &ServerAddr, sizeof(IPhost));
    partPush(p[0], &serverPort, sizeof(long));
    /* 
     * If we don't specify the second participant, UDP will select a port
     * for us. 
     */
#if 0
    {
	long	clientPort = ANY_PORT;
	/* 
	 * NOTE -- if you use two participants, make sure the second argument to 
	 * the above call to partInit is 2
	 */
	partPush(p[1], ANY_HOST, 0);
	partPush(p[1], &clientPort, sizeof(long));
    }
#endif
}

static void
serverSetPart( p )
    Part 	*p;
{
    partInit(p, 1);
    partPush(*p, ANY_HOST, 0);
    partPush(*p, &serverPort, sizeof(long));
}

static int lens[] = { 
  1, 1000, 2000, 4000, 8000, 16000
};

/* #define SAVE_SERVER_SESSN */

#include "common_test.c"


static void
testInit()
{
}
