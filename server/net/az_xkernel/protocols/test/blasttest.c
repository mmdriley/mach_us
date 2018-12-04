/*
 * blasttest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.11 $
 * $Date: 1993/02/01 22:26:01 $
 */

/*
 * Ping-pong test of BLAST
 */

#include "xkernel.h"
#include "blast.h"

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC blasttest_init
#define TRACE_VAR blasttestp
#define PROT_STRING "blast"

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


static int lens[] = { 
  1, 1000, 2000, 4000, 8000, 16000
};


#define USE_KILL_TICKET

#ifdef USE_KILL_TICKET
#define CUSTOM_CLIENT_DEMUX
#define CUSTOM_SERVER_DEMUX
void	customClientDemux();
void	customServerDemux();
#endif USE_KILL_TICKET



#define SAVE_SERVER_SESSN


#include "common_test.c"

static void
testInit()
{
}

#ifdef USE_KILL_TICKET

void
customClientDemux( self, lls, dg )
    XObj	self, lls;
    Msg		*dg;
{
    if ( clientPushResult == 0 ) return;
    if (xControl(clientDownSes, FREERESOURCES,
		 (char *)&clientPushResult, sizeof(int))) {
	printf("FREERESOURCES on client session failed\n");
    }
}


void
customServerDemux( self, lls, dg )
    XObj	self, lls;
    Msg		*dg;
{
    if ( serverPushResult == 0 ) return;
    if (xControl(lls, FREERESOURCES,
		 (char *)&serverPushResult, sizeof(int))) {
	printf("FREERESOURCES on server session failed\n");
    }
}

#endif USE_KILL_TICKET
