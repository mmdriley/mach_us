/*
 * blast_hdr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:20:29 $
 */

#include "blast_internal.h"
 

#define HDR	((BLAST_HDR *)hdr)
long
blastHdrLoad(hdr, src, len, arg)
    VOID *hdr;
    char *src;
    long int len;
    VOID *arg;
{
    xAssert( len == sizeof(BLAST_HDR) );
    bcopy( src, hdr, len );
    HDR->prot_id = ntohl(HDR->prot_id);
    HDR->seq = ntohl(HDR->seq);
    HDR->num_frag = ntohs(HDR->num_frag);
    BLAST_MASK_NTOH(HDR->mask, HDR->mask);
    HDR->len = ntohl(HDR->len);
    return len;
}


void
blastHdrStore(hdr, dst, len, arg)
    VOID *hdr;
    char *dst;
    long int len;
    VOID *arg;
{
    BLAST_HDR	h;

    xAssert( len == sizeof(BLAST_HDR) );
    h = *(BLAST_HDR *)hdr;
    h.prot_id = htonl(h.prot_id);
    h.seq = htonl(h.seq);
    h.num_frag = htons(h.num_frag);
    BLAST_MASK_HTON(h.mask, h.mask);
    h.len = htonl(h.len);
    bcopy( (char *)&h, dst, len );
}
