/* 
 * ipRouteTest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:26:14 $
 */

/* 
 * Test protocol to exercise IP routing in a rudimentary fashion
 */


#include "xkernel.h"
#include "ip.h"

typedef struct {
    IPhost	net;
    IPhost	gw;
} Route;

static IPhost	myHost;
static IPhost	localPeer = { 192,12,69,50 };
static IPhost	newGw = { 192,12,69,33 };
static IPhost	remPeer1 = { 192,55,33,22 };
static IPhost	remPeer2 = { 192,99,98,123 };	/* On route1 */
static Route	route1 = { { 192,99,98,0 }, { 192,12,69,12 } };
static Route	route1_2 = { { 192,99,98,0 }, { 192,12,69,54 } };
static Route	route2 = { { 11,0,0,0 }, { 192,12,69,12 } };

int	traceiproutetestp = 0;


static void
setPart( Part *p, IPhost *remHost )
{
    partInit(p, 1);
    partPush(*p, remHost);
}


int
iproutetest_init( XObj self )
{
    XObj	ip, lls;
    Part	p[2];

    xTrace0(iproutetestp, 0, "IP route test init");

    ip = xGetProtlByName("ip");
    xAssert(ip != ERR_XOBJ);
    xControl(ip, GETMYHOST, (char *)&myHost, sizeof(myHost));

    /* 
     * open session with peer on local network
     */
    xTrace0(iproutetestp, 0, "Local session open");
    setPart(p, &localPeer);
    lls = xOpen(self, self, ip, p);
    xAssert(lls != ERR_XOBJ);

    /* 
     * open session with peer on remote network -- should go through
     * default gateway
     */
    xTrace0(iproutetestp, 0, "Remote session open (default gateway)");
    setPart(p, &remPeer1);
    lls = xOpen(self, self, ip, p);
    xAssert(lls != ERR_XOBJ);

    /* 
     * Add a route for a new network -- shouldn't affect existing sessions
     */
    xTrace0(iproutetestp, 0, "Adding route 1");
    xControl(ip, IP_REDIRECT, (char *)&route1, sizeof(route1));

    /* 
     * open session with peer on new route
     */
    xTrace0(iproutetestp, 0, "Remote session open (new route)");
    setPart(p, &remPeer2);
    lls = xOpen(self, self, ip, p);
    xAssert(lls != ERR_XOBJ);

    /* 
     * Add a route for a new network -- shouldn't affect existing sessions
     */
    xTrace0(iproutetestp, 0, "Adding route 2");
    xControl(ip, IP_REDIRECT, (char *)&route2, sizeof(route2));

    /* 
     * Change route #1
     */
    xTrace0(iproutetestp, 0, "Changing route 1");
    xControl(ip, IP_REDIRECT, (char *)&route1_2, sizeof(route1_2));

    /* 
     * Change default route (this is a gross layering violation)
     */
    xTrace0(iproutetestp, 0, "Changing default route");
    rt_add_def(ip->state, &newGw);

    xTrace0(iproutetestp, 0, "IP route test completes");
    return 0;
}
