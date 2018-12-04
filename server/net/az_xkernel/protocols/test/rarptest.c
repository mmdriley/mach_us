/*
 * rarpTest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:26:08 $
 */

/*
 * This protocol sends out a single RARP request.
 */

#include "xkernel.h"
#include "arp.h"

int tracerarptestp;
  
static ETHhost TARGET = SITE_SERVER_ETH;


int
rarptest_init(XObj self)
{
  int	buf[2];
  
  printf("rarpTest_init\n");
  *(ETHhost *)buf = TARGET;
  xTrace3(rarptestp, 2, "rarpTest resolving: %x:%x:%x",
	  TARGET.high, TARGET.mid, TARGET.low);
  if (xControl(xGetDown(self, 0), RRESOLVE, (char *)buf, sizeof(ETHhost))
      < 0) {
    xTrace0(rarptestp, 0, "RARP control op failed");
    return -1;
  }
  xTrace4(rarptestp, 2, "rarpTest: result <%d.%d.%d.%d>",
	  ((IPhost *)buf)->a, ((IPhost *)buf)->b,
	  ((IPhost *)buf)->c, ((IPhost *)buf)->d);
  return 0;
}


