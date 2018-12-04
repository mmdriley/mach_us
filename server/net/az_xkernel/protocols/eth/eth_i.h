/* 
 * eth_i.h 
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.9 $
 * $Date: 1993/02/01 22:30:26 $
 */

/*
 * Information shared between the ethernet protocol and the driver
 */

#ifndef eth_i_h
#define eth_i_h

#include "eth.h"

extern int traceethp;

/*
 * range of legal data sizes
 */

#define MIN_ETH_DATA_SZ		64

/* 
 * Moved to eth.h
 */
/* #define	MAX_ETH_DATA_SZ		1500 */


/*
 * Ethernet "types"
 *
 * Ether-speak for protocol numbers is "types."
 * Unfortunately, ether "types" are unsigned shorts,
 * while xkernel PROTLs are ints.
 */

typedef unsigned short				ETHtype, ethType_t;


typedef struct {
    ETHhost	dst;
    ETHhost	src;
    ethType_t	type;
} ETHhdr;


/*
 * Interface presented to protocol from driver
 */
extern ETHhost ethBcastHost;

#ifdef __STDC__

void	ethCtlrInit( ETHhost host );
void	getLocalEthHost( ETHhost *host );
int	SetPromiscuous( void );
void	ethCtlrXmit( Msg *msg, ETHhost *dst, int type );

#else

void	ethCtlrInit();
void	getLocalEthHost();
int	SetPromiscuous();
void	ethCtlrXmit();

#endif

/*
 * Interface presented to driver from protocol
 */

#ifdef __STDC__

xkern_return_t	eth_demux( Msg, int, ETHhost, ETHhost );
void		ethSendUp( Msg *, ETHhost *, int );

#else

xkern_return_t	eth_demux();
void		ethSendUp();

#endif

#endif  ! eth_i_h
