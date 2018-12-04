/* 
 * blast_mask16.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:21:07 $
 */

/* 
 * Header file for using 16-bit masks
 */

#ifndef blast_mask16_h
#define blast_mask16_h
 

typedef u_short	BlastMask;
#define BLAST_MASK_PROTOTYPE	int

#define BLAST_MAX_FRAGS	16
#define BLAST_FULL_MASK(_m, _n)		(_m) = ( (u_short)0xffff >> (16 - (_n)) )
#define BLAST_MASK_SHIFT_LEFT(_m, _n)	(_m) <<= (_n)
#define BLAST_MASK_SHIFT_RIGHT(_m, _n)	(_m) >>= (_n)
#define BLAST_MASK_IS_BIT_SET(_m, _n)	( *(_m) & ( 1 << ((_n)-1 ) ) )
#define BLAST_MASK_SET_BIT(_m, _n)	( *(_m) |= ( 1 << ((_n)-1) ) )
#define BLAST_MASK_CLEAR(_m)		( (_m) = 0 )
#define BLAST_MASK_IS_ZERO(_m)		( (_m) == 0 )
#define BLAST_MASK_OR(_m1, _m2)		( (_m1) |= (_m2) )
#define BLAST_MASK_EQ(_m1, _m2)		( (_m1) == (_m2) )
#define BLAST_MASK_NTOH(_tar, _src)	(_tar) = ntohs(_src)
#define BLAST_MASK_HTON(_tar, _src)	(_tar) = htons(_src)

#define BLAST_PADDING	3

#endif ! blast_mask16_h

