/*
 * enable_test.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:27:17 $
 */

/*
 * Test of openenables / opendisables.
 *
 * This incarnation uses specific IP hosts on the stack and is mostly
 * geared towards testing IP.  The expected results are probably
 * incorrect for other protocols.
 */

#include "xkernel.h"

int	traceenabletestp;

static XObj	self, llp;
static IPhost	host1 = { 192, 12, 69, 45 };
static IPhost	wrongHost = { 192, 12, 11, 22 };
static IPhost	bcast1 = { 192, 12, 69, 255 };

#define protNum1	100
#define protNum2	150
#define SPEC_HOST_1	&host1
#define SPEC_HOST_2	&host2
#define BCAST_1		&bcast1
#define WRONG_HOST	&wrongHost

static void
check( f, h, expRes )
    Pfk		f;
    IPhost	*h;
    xkern_return_t	expRes;
{
    Part		p;
    xkern_return_t	res;
    static int		n = 1;

    xTrace1(enabletestp, 1, "Running openEnable test # %d", n++);
    partInit(&p, 1);
    partPush(p, h);
    res = f(self, self, llp, &p);
    if ( res != expRes ) {
	xTrace2(enabletestp, 0, 
		"EnableTest ERROR: Expected enable result %d got %d",
		expRes, res);
    } else {
	xTrace1(enabletestp, TR_EVENTS, "EnableTest got expected result %d",
		res);
    }
}


void
enabletest_init( this )
    XObj	this;
{
    self = this;
    llp = xGetDown(self, 0);
    xAssert( xIsProtocol(llp) );
    
    xTrace0(enabletestp, 0, "EnableTest_init");
    check(xOpenEnable, ANY_HOST, XK_SUCCESS);
    check(xOpenEnable, ANY_HOST, XK_SUCCESS);
    check(xOpenEnable, ANY_HOST, XK_SUCCESS);
    check(xOpenEnable, ANY_HOST, XK_SUCCESS);

    check(xOpenEnable, SPEC_HOST_1, XK_SUCCESS);
    check(xOpenEnable, WRONG_HOST, XK_FAILURE);

    check(xOpenDisable, ANY_HOST, XK_SUCCESS);
    check(xOpenDisable, SPEC_HOST_1, XK_SUCCESS);

    check(xOpenEnable, BCAST_1, XK_SUCCESS);

    check(xOpenDisable, WRONG_HOST, XK_FAILURE);
    check(xOpenDisable, ANY_HOST, XK_SUCCESS);
    check(xOpenDisable, ANY_HOST, XK_SUCCESS);
    check(xOpenDisable, ANY_HOST, XK_SUCCESS);
    check(xOpenDisable, ANY_HOST, XK_FAILURE);
    check(xOpenDisable, BCAST_1, XK_SUCCESS);
    xTrace0(enabletestp, 0, "End of EnableTest");
}
