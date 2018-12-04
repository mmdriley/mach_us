/* 
 * sunrpc_hdr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:29:15 $
 */

#include "xrpc.h"
#include "xkernel.h"
#include "sunrpc_i.h"
#include "xrpc_print.h"


static void
sunrpcHdrStore(hdr, dst, len, arg)
    VOID *hdr;
    char *dst;
    long int len;
    VOID *arg;
{
    bcopy(hdr, dst, len);
}


int
sunrpcEncodeHdr(m, hdr)
    Msg *m;
    struct rpc_msg *hdr;
{
    static int first = 1;
    static XDR xdrs;
    static u_long xdrData[XDRHDRSIZE / 4]; /*=0;*/
    static int len;
    
    xIfTrace(sunrpcp,7) {
	prpchdr(*hdr, "encode_hdr");
    }
    if (first) {
	xdrmem_create(&xdrs, (char *)xdrData, XDRHDRSIZE, XDR_ENCODE);
	first = 0;
    }
    
    XDR_SETPOS(&xdrs,0);
    
    /* new stream for each message */
    if (hdr->rm_direction == CALL) { 
	if (! xdr_callmsg(&xdrs, hdr)) {
	    return -1;
	}
    } else {
	if (! xdr_replymsg(&xdrs, hdr)) {
	    return -1;
	}
    }
    len = (int) XDR_GETPOS(&xdrs);
    msgPush(m, sunrpcHdrStore, (char *)xdrData, len, 0);
    return 0;
}


long
sunrpcHdrLoad(hdr, src, len, arg)
    VOID *hdr;
    char *src;
    long int len;
    VOID *arg; 
{
    int 	end;
    static XDR	xdrs; /* = 0; */
    static u_long alignedSrc[ XDRHDRSIZE / 4 ];
    
    if (LONG_ALIGNED(src)) {
	xdrmem_create(&xdrs, src, len, XDR_DECODE);
    } else {
	bcopy(src, (char *)alignedSrc, len);
	xdrmem_create(&xdrs, (char *)alignedSrc, len, XDR_DECODE);
    }
    if (!xdr_callmsg(&xdrs, (struct rpc_msg *)hdr)) {
	XDR_SETPOS(&xdrs,0);
	if (!xdr_replymsg(&xdrs, (struct rpc_msg *)hdr)) {
	    *(int *)arg = -1;
	    return 0;
	}
    }
    end = (int) XDR_GETPOS(&xdrs);
    xTrace1(sunrpcp, 7, "decode hdr: popping=%d bytes \n", end);
    return end;
}


