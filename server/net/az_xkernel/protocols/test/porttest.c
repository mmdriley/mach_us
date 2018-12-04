/*
 * porttest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:26:49 $
 */

/*
 * Test of UDP port management
 */

#include "xkernel.h"
#include "udp.h"

static XObj	lls[10];
static IPhost	ServerAddr = { 192, 12, 69, 45 };

#define numArbPorts 2
#define numSpecPorts 2

static int	api[numArbPorts] = { 0, 1 };	/* arbitrary port indices */
static int	spi[numSpecPorts] = { 2, 3 };	/* arbitrary port indices */
static int	specPort[numSpecPorts] = { 2000, 2005 };

static void
openArbPorts( XObj self, XObj llp )
{  
    int 	i, j;
    Part	p;

    for ( i=0; i < numArbPorts; i++ ) {
	j = api[i];
	partInit(&p, 1);
	partSetProt(p, 2001);
	partPush(p, &ServerAddr);
	lls[j] = xOpen(self, llp, &p);
	xAssert(lls[j] != ERR_XOBJ);
    }
}

    
static void
closeArbPorts()  
{
    int i;
    
    for ( i=0; i < numArbPorts; i++ ) {
	xClose(lls[api[i]]);
    }
}

static void
closeSpecPorts()  
{
    int i;
    
    for ( i=0; i < numSpecPorts; i++ ) {
	xClose(lls[spi[i]]);
    }
}


static void
openSpecPorts( XObj self, XObj llp, int shouldFail )
{  
    int 	i, j;
    Part	p[2];
    XObj	llsNew;

    for ( i=0; i < numSpecPorts; i++ ) {
	j = spi[i];
	partInit(p, 2);
	partSetProt(p[0], 2001);
	partPush(p[0], &ServerAddr);
	partSetProt(p[1], specPort[i]);
	partPush(p[1], ANY_HOST);
	llsNew = xOpen(self, llp, p);
	if ( shouldFail ) {
	    xAssert(llsNew == ERR_XOBJ);
	} else {
	    xAssert(llsNew != ERR_XOBJ);
	    lls[j] = llsNew;
	}
    }
}


void
porttest_init( XObj self )
{
    XObj	llp;

    llp = xGetDown(self, 0);
    xAssert(llp != ERR_XOBJ);

    openArbPorts(self, llp);
    closeArbPorts();

    openSpecPorts(self, llp, 0);
    openSpecPorts(self, llp, 1);
    closeSpecPorts();

    openArbPorts(self, llp);
    openSpecPorts(self, llp, 0);

    closeArbPorts();
    closeSpecPorts();
    xError("end of test");
}

