/* 
 * arp_mach3.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:19:09 $
 */


#include "xkernel.h"


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
    ETHhost	ethHost;
    IPhost	ipHost;
    int		i;
    
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
	    if ( str2ipHost(&ipHost, rom[i][1]) == XK_FAILURE ) {
		sprintf(errBuf,
			"ARP ROM file format error (IPhost) in line %d", i+1 );
		xError(errBuf);
		continue;
	    }
	    if ( str2ethHost(&ethHost, rom[i][2]) == XK_FAILURE ) {
		sprintf(errBuf,
			"ARP ROM file format error (ETHhost) in line %d",
			i + 1 );
		xError(errBuf);
		continue;
	    }
	    arpSaveBinding( ((PSTATE *)self->state)->tbl,
			    (IPhost *)&ipHost, &ethHost );
	    xTrace1(arpp, 5, "arp: loaded (%s) from rom file",
		    ipHostStr(&ipHost));
	    xTrace1(arpp, 5, "arp: corresponding eth address: %s",
		    ethHostStr(&ethHost));
	}
    }
}
