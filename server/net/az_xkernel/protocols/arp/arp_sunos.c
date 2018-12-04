/* 
 * arp_sunos.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.9 $
 * $Date: 1993/02/01 22:19:39 $
 */

/*
 * initialize table from ROM entries
 */

#include "xkernel.h"
#include "arp.h"
#include "arp_i.h"
#include "arp_table.h"

#ifdef __STDC__

extern u_long	inet_addr( char * );
extern int	atoi( char * );

#else

extern u_long	inet_addr();
extern int	atoi();

#endif

static  char	errBuf[80];

void
arpPlatformInit(self)
    XObj self;
{
    int		simInetAddr;
    int 	realInetAddr;
    ETHhost	simEthAddr;
    int		udpPort;
    int		i;
    PSTATE	*pstate = (PSTATE *)self->state;
    
    xAssert(pstate->tbl);
    /*
     * Check the rom file for arp initialization 
     */
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], "arp") ) {
	    if ( ! rom[i][1] || ! rom[i][2] ) {
		sprintf(errBuf, "ARP ROM file format error in line %d", i+1 );
		xError(errBuf);
		continue;
	    }
	    simInetAddr = inet_addr(rom[i][1]);
	    realInetAddr = inet_addr(rom[i][2]);
	    if ( ! rom[i][3] || sscanf(rom[i][3], "%d", &udpPort) < 1 ) {
		sprintf(errBuf, "ARP ROM file format error in line %d", i+1 );
		xError(errBuf);
		continue;
	    }
	    sock2simEth((char *)&simEthAddr,
			*((struct in_addr *)&realInetAddr),
			udpPort);
	    arpSaveBinding( pstate->tbl, (IPhost *)&simInetAddr, &simEthAddr );
	    /* 
	     * The sunos simulator can't talk to its peers and thus can't
	     * refresh its cache, so we lock down all entries.
	     */
	    arpLock(pstate->tbl, (IPhost *)&simInetAddr);
	    xTrace1(arpp, 5, "arp: loaded (%s) from rom file",
		    ipHostStr((IPhost *)&simInetAddr));
	    xTrace1(arpp, 5, "arp: corresponding eth address: %s",
		    ethHostStr(&simEthAddr));
	}
    }
}
