/* 
 * eth_host.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:41:26 $
 */


#ifndef eth_host_h
#define eth_host_h

/* Address types */
typedef struct {
    unsigned short	high;
    unsigned short	mid;
    unsigned short	low;
} ETHhost, ethAd_t;

#endif eth_host_h
