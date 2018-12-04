/*     
 * ip_hdr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:23:06 $
 */


#include "xkernel.h"
#include "ip.h"
#include "ip_i.h"


#ifdef __STDC__

static long	ipStdHdrLoad( VOID *, char *, long, VOID * );
static long	ipOptionsLoad( VOID *, char *, long, VOID * );

#else

static long	ipStdHdrLoad();
static long	ipOptionsLoad();

#endif __STDC__


void
ipDumpHdr(hdr)
    IPheader *hdr;
{
    xTrace5(ipp, TR_ALWAYS,
	    "    version=%d,hlen=%d,type=%d,dlen=%d,ident=%d",
	   GET_VERS(hdr), GET_HLEN(hdr), hdr->type,hdr->dlen,hdr->ident);
    xTrace2(ipp, TR_ALWAYS, "    flags:  DF: %d  MF: %d",
	    (hdr->frag & DONTFRAGMENT) ? 1 : 0, 
	    (hdr->frag & MOREFRAGMENTS) ? 1 : 0);
    xTrace4(ipp, TR_ALWAYS, "    fragoffset=%d,time=%d,prot=%d,checksum=%d",
	   FRAGOFFSET(hdr->frag), hdr->time, hdr->prot, hdr->checksum);
    xTrace2(ipp, TR_ALWAYS, "    source address = %s  dest address %s", 
	    ipHostStr(&hdr->source), ipHostStr(&hdr->dest));
}



/*
 * Note:   *hdr will be modified
 */
void
ipHdrStore(hdr, dst, len, arg)
    VOID *hdr;
    char *dst;
    long int len;
    VOID *arg;
{
    /*
     * XXX -- this needs to be revised to include options
     */
    xAssert(len == sizeof(IPheader));
    ((IPheader *)hdr)->dlen = htons(((IPheader *)hdr)->dlen);
    ((IPheader *)hdr)->ident = htons(((IPheader *)hdr)->ident);
    ((IPheader *)hdr)->frag = htons(((IPheader *)hdr)->frag);
    ((IPheader *)hdr)->checksum = 0;
    ((IPheader *)hdr)->checksum = ~ocsum((u_short *)hdr, sizeof(IPheader) / 2);
    xAssert(! (~ ocsum( (u_short *)hdr, sizeof(IPheader) / 2 ) & 0xFFFF ));
    bcopy ( (char *)hdr, dst, sizeof(IPheader) );
}


/*
 * checksum over the the header is written into the checksum field of
 * *hdr.
 */
static long
ipStdHdrLoad(hdr, src, len, arg)
    VOID *hdr;
    char *src;
    long int len;
    VOID *arg;
{
    xAssert(len == sizeof(IPheader));
    bcopy(src, (char *)hdr, sizeof(IPheader));
    ((IPheader *)hdr)->checksum =
      ~ ocsum((u_short *)hdr, sizeof(IPheader) / 2) & 0xFFFF;
    ((IPheader *)hdr)->dlen = ntohs(((IPheader *)hdr)->dlen);
    ((IPheader *)hdr)->ident = ntohs(((IPheader *)hdr)->ident);
    ((IPheader *)hdr)->frag = ntohs(((IPheader *)hdr)->frag);
    return sizeof(IPheader);
}


static long
ipOptionsLoad(hdr, netHdr, len, arg)
    VOID *hdr;
    char *netHdr;
    long int len;
    VOID *arg;
{
    bcopy(netHdr, (char *)hdr, len);
    *(u_short *)arg = ~ocsum((u_short *)hdr, len / 2);
    return len;
}



/*
 * This is a bit ugly.  The checksum and network-to-host byte conversion
 * can't nicely take place in the load function because options are possible,
 * the checksum is calculated over the entire header (including options),
 * and the checksum is done over the data in network-byte order.
 */
int
ipGetHdr(msg, h, options)
    Msg *msg;
    IPheader *h;
    char *options;
{
    u_char hdrLen;

    if (! msgPop(msg, ipStdHdrLoad, h, IPHLEN, NULL)) {
	xTrace0(ipp, 3, "ip getHdr: msg too short!");
	return -1;
    }
    xIfTrace(ipp, 7) {
	xTrace0(ipp, 7, "ip getHdr: received header:");
	ipDumpHdr(h);
    }
    hdrLen = GET_HLEN(h);
    if (hdrLen == 5) {
	/*
	 * No options
	 */
	if (h->checksum) {
	    xTrace0(ipp, 3, "ip getHdr: bad checksum!");
	    return -1;
	}
	return 0;
    }
    if (hdrLen > 5) {
	/*
	 * IP options
	 */
	u_short cksum[2];
	int optBytes;
	
	optBytes = (hdrLen - 5) * 4;
	cksum[0] = h->checksum;
	if (! msgPop(msg, ipOptionsLoad, options, optBytes, &cksum[1])) {
	    xTrace0(ipp, 3, "ip getHdr: options component too short!");
	    return -1;
	}
	/*
	 * Add the regular header checksum with the options checksum
	 */
	if ( ~ocsum( cksum, 2 ) ) {
	    xTrace0(ipp, 3, "ip getHdr: bad checksum (with options)!");
	    return -1;
	}
	return 0;
    }
    xTrace1(ipp, 3, "ip getHdr: hdr length (%d) < 5 !!", hdrLen);
    return -1;
}


