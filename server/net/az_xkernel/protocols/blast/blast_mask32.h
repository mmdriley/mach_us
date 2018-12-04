/* 
 * blast_mask32.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 22:21:14 $
 */

/* 
 * Header file for using 32-bit masks
 */

#ifndef blast_mask32_h
#define blast_mask32_h
 
/* 
 * non-ANSI compilers gripe about the _n##U usage while GCC gives
 * warning messages about the u_long cast.
 */
#ifdef __STDC__
#   define UNSIGNED(_n)	_n##U
#else
#   define UNSIGNED(_n)	((u_long)(_n))
#endif

typedef u_long	BlastMask;
#define BLAST_MASK_PROTOTYPE	BlastMask

#define BLAST_MAX_FRAGS	32
#define BLAST_FULL_MASK(_m, _n)		(_m) = ( UNSIGNED(0xffffffff) >> (32 - (_n)) )
#define BLAST_MASK_SHIFT_LEFT(_m, _n)	(_m) <<= (_n)
#define BLAST_MASK_SHIFT_RIGHT(_m, _n)	(_m) >>= (_n)
#define BLAST_MASK_IS_BIT_SET(_m, _n)	( *(_m) & ( 1 << ((_n)-1 ) ) )
#define BLAST_MASK_SET_BIT(_m, _n)	( *(_m) |= ( 1 << ((_n)-1) ) )
#define BLAST_MASK_CLEAR(_m)		(_m) = 0
#define BLAST_MASK_IS_ZERO(_m)		( (_m) == 0 )
#define BLAST_MASK_OR(_m1, _m2)		(_m1) |= (_m2)
#define BLAST_MASK_EQ(_m1, _m2)		( (_m1) == (_m2) )
#define BLAST_MASK_NTOH(_tar, _src)	(_tar) = ntohl(_src);
#define BLAST_MASK_HTON(_tar, _src)	(_tar) = htonl(_src);

#define BLAST_PADDING	1

#endif ! blast_mask32_h

