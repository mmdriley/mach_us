/*
 * icmptest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.21 $
 * $Date: 1993/02/01 22:26:59 $
 */

/*
 * This protocol is a ping-pong test for the ICMP echo request and reply
 * messages.
 *
 * The same protocol code is used for both the client and the server.  
 * (The server is null since ICMP automatically sends the echoes.)
 */

#include "xkernel.h"
#include "trace.h"
#ifndef XKMACHKERNEL
#include "x_stdio.h"
#endif ! XKMACHKERNEL
#include "icmp.h"

static IPhost myIpHost;

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static IPhost ServerAddr = { 0, 0, 0, 0 };
static IPhost ClientAddr = { 0, 0, 0, 0 };

static XObj myProtl;
static int client();
static int isClient();

static int count;

#define TRIPS 10
#define TIMES 2
#define DELAY 3

#define NEW_IP_EQUAL(_a, _b) ((_a.a == _b.a) && (_a.b == _b.b) && \
			      (_a.c == _b.c) && (_a.d == _b.d))

#ifdef __STDC__

int	icmptest_init( XObj );
static int	client( void );
static int	isClient( void );

#endif

static int
testShouldRun()
{
    int	testsSpecified = 0;
    int	i;

    for (i=0; i < globalArgc; i++) {
	if ( strncmp(globalArgv[i], "-test", strlen("-test")) == 0) {
	    testsSpecified = 1;
	    if ( strcmp(globalArgv[i], "-testicmp") == 0 ) {
		return 1;
	    }
	}
    }
    /* 
     * If we got here, our test was not specified.  Run only if no other
     * tests were specified.
     */
    return ! testsSpecified;
}


int
icmptest_init( self )
    XObj 	self;
{
    XObj 	ipProtl;
    
    if ( ! testShouldRun() ) {
	xTrace0(prottest, TR_FUNCTIONAL_TRACE, "Parameters indicate icmp test should not run");
	return 0;
    }
    printf("icmpTest_init\n");
    myProtl = self;
    if ((ipProtl = xGetProtlByName("ip")) == ERR_XOBJ) {
	xTrace0(prottest, TR_ALWAYS, "Couldn't get IP protocol");
	return -1;
    }
    if (xControl(ipProtl, GETMYHOST, (char *)&myIpHost, sizeof(IPhost)) < 0) {
	xTrace0(prottest, TR_ALWAYS, "Couldn't get my IP address");
	return -1;
    }
    xTrace1(prottest, TR_GROSS_EVENTS, "icmpTest: My ip addr is <%s>",
	    ipHostStr(&myIpHost));
    if ( isClient() ) {
	CreateKernelProcess(client, STD_PRIO+1, 0, 0, 0);
    } else {
	xTrace0(prottest, TR_ALWAYS, "icmpTest: I am not the client");
    }
    return 0;
}


static int
isClient()
{
    return NEW_IP_EQUAL(myIpHost, ClientAddr);
}


static int
client()
{
    XObj s;
    Part p;
    int test = 0;
#ifdef XK_DEBUG
    int	total = 0;
#endif
    int lenindex, len, i;
    
    static int lens[] = { 
	1, 1000, 2000, 4000, 8000, 16000
      };
    
    s = NULL;
    printf("I am the client, talking to <%s>\n", ipHostStr(&ServerAddr));
    
    partInit(&p, 1);
    partPush(p, &ServerAddr, sizeof(IPhost));
    
    if ((s = xOpen(myProtl, myProtl, xGetDown(myProtl, 0), &p)) == ERR_XOBJ) {
	xTrace0(prottest, TR_ERRORS, "Not sending, could not open session");
	return 0;
    } else {
#ifdef INF_LOOP
	for (lenindex = 0; ; lenindex++) {
	    if (lenindex >= sizeof(lens)/sizeof(long)) lenindex = 0;
#else
	for (lenindex = 0; lenindex < sizeof(lens)/sizeof(long); lenindex++) {
#endif
	    len = lens[lenindex];
	    for (test = 0; test < TIMES; test++) {
		xTrace2(prottest, TR_MAJOR_EVENTS, "Sending (%d)  len = %d ...\n",
			++total, len);
		for (i=0; i < TRIPS; i++) {
		    if (xControl(s, ICMP_ECHO_REQ, (char *)&len,
				 sizeof(len))) {
			xTrace1(prottest, TR_ERRORS, "\nfailed after %d trips", i);
			break;
		    }
		    xIfTrace(prottest, TR_MAJOR_EVENTS) {
			putchar('.');
			if (! (++count % 50)) {
			    putchar('\n');
			}
		    }
		}
		xTrace1(prottest, 1, "\n%d round trips completed", i);
		Delay(DELAY * 1000);
	    }
	}
#ifdef ABORT
        Kabort("End of test");
#endif
        return 0;
    }
}


