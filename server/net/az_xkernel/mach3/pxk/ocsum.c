/*
 * ocsum.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.10 $
 * $Date: 1993/02/02 00:05:12 $
 */

#include "platform.h"	
#include "assert.h"
#include "xk_debug.h"
#include "msg.h"
#include "x_util.h"

int tracecksum = 0;


/*
 * ocsum -- return the 1's complement sum of the 'count' 16-bit words
 * pointed to by 'hdr'.  
 */
u_short
ocsum( hdr, count )
    u_short *hdr;
    int count;
{
    register u_long acc = 0;
    
    while ( count-- ) {
	acc += *hdr++;
	if ( acc & UNSIGNED(0xFFFF0000) ) {
	    /*
	     * Carry occurred -- wrap around
	     */
	    acc &= 0xFFFF;
	    acc++;
	}
    }
    return acc & UNSIGNED(0xFFFF);
}


typedef struct {
    u_long 	sum;
    int		odd;
} ckSum_t;


/*
 * cksum_helper -- perform the checksum over the data.  's' should point to
 * a ckSum_t with the current state of the checksum.  The data pointer does
 * not have to be aligned.
 */
static bool
cksum_helper( data, len, s ) 
    char 	*data;
    long 	len;
    VOID	*s;
{
    bool oddAddr;
    register unsigned long sum = 0;
    int	save_len;

    save_len = len;
    /*
     * Add first byte if necessary to put 'data' on an even address
     */
    if (oddAddr = ((int)data % 2)) {
#if ENDIAN == LITTLE
	sum = *(u_char *)data++ << 8;
#else
	sum = *(u_char *)data++;
#endif
	len--;
    }
    /*
     * Sum all bytes two at a time, leaving the last one if there are an
     * odd number of bytes
     */
#define USEOCSUM
#ifdef USEOCSUM
    {
	sum += ocsum((u_short *)data, len / 2);
	data += len & ~1;
	len = len & 1;
    }
#else
    while (len > 1) {
	sum += *((u_short *)data)++;
	len -= 2;
    }
#endif
    /*
     * Add in last byte if there is one
     */
    if (len) {
#if ENDIAN == LITTLE
	sum += *(u_char *)data;
#else
	sum += *(u_char *)data << 8;
#endif
    }
    /*
     * Swap bytes in the sum if necessary
     */
    if (oddAddr ^ ((ckSum_t *)s)->odd) {
	u_char hiByte;

	/*
	 * Wrap possible overflow
	 */
	sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
#ifndef USEOCSUM
	sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
#endif
	xAssert(!(sum & UNSIGNED(0xffff0000)));
	hiByte = (sum & 0xff00) >> 8;
	sum = (sum & 0xff) << 8;
	sum += hiByte;
    }
    /*
     * Add sum to old sum and indicate whether an odd or even total number
     * of bytes have been processed
     */
    ((ckSum_t *)s)->odd = ((ckSum_t *)s)->odd ^ (save_len % 2);
    ((ckSum_t *)s)->sum += sum;
    return TRUE;
}


u_short
inCkSum( m, buf, len )
    Msg *m;
    u_short *buf;
    int len;
{
    ckSum_t s;
    
    s.odd = 0;
    xTrace1(cksum, 8, "in_cksum:  msg_len = %d", msgLen(m));
    /*  xAssert(len == msg_len(m)); */
    xAssert(! (len % 2));
    s.sum = ocsum(buf, len / 2);
    xTrace1(cksum, 4, "Buf Checksum: %x\n", s.sum);
    msgForEach(m, cksum_helper, &s);
    s.sum = (s.sum & 0xffff) + ((s.sum >> 16) & 0xffff);
    s.sum = (s.sum & 0xffff) + ((s.sum >> 16) & 0xffff);
    xTrace1(cksum, 4, "Total checksum: %x\n", s.sum);
    xAssert(!(s.sum >> 16));
    return ~s.sum & 0xffff;
}


