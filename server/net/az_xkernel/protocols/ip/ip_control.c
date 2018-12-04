/*     
 * ip_control.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.19 $
 * $Date: 1993/02/01 22:22:33 $
 */

#include "xkernel.h"
#include "ip.h"
#include "ip_i.h"
#include "route.h"


#define IPHOSTLEN	sizeof(IPhost)


/*
 * ip_controlsessn
 */
int
ipControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
    SState        *sstate;
    PState        *pstate;
    IPheader      *hdr;
    
    xAssert(xIsSession(s));
    
    sstate = (SState *)s->state;
    pstate = (PState *)s->myprotl->state;
    
    hdr = &(sstate->hdr);
    switch (opcode) {
	
      case GETMYHOST :
	checkLen(len, IPHOSTLEN);
	*(IPhost *)buf = sstate->hdr.source;
	return IPHOSTLEN;
	
      case GETPEERHOST :
	checkLen(len, IPHOSTLEN);
	*(IPhost *)buf = sstate->hdr.dest;  
	return IPHOSTLEN;
	
      case GETMYHOSTCOUNT:
      case GETPEERHOSTCOUNT:
	checkLen(len, sizeof(int));
	*(int *)buf = 1;
	return sizeof(int);

      case GETMYPROTO :
      case GETPEERPROTO :
	checkLen(len, sizeof(long));
	*(long *)buf = sstate->hdr.prot;
	return sizeof(long);
	
      case GETMAXPACKET :
	checkLen(len, sizeof(int));
	*(int *)buf = IPMAXPACKET;
	return sizeof(int);
	
      case GETOPTPACKET :
	checkLen(len, sizeof(int));
	*(int *)buf = sstate->mtu - IPHLEN;
	return sizeof(int);
	
      case GETPARTICIPANTS:
	/* 
	 * Since we completely rewrite the participant when we open
	 * the lower session, we'll just construct the participants
	 */
	{
	    Part	p[2];

	    partInit(p, 2);
	    /* 
	     * Remote host
	     */
	    partPush(p[0], &sstate->hdr.dest, sizeof(IPhost));
	    /* 
	     * Local host
	     */
	    partPush(p[1], &sstate->hdr.source, sizeof(IPhost));
	    return (partExternalize(p, buf, &len) == XK_FAILURE) ? -1 : len;
	}

      case IP_REDIRECT:
	return ipControlProtl(s->myprotl, opcode, buf, len);
	
      default : 
	xTrace0(ipp,3,"Unrecognized opcode -- forwarding");
	return xControl(xGetDown(s, 0), opcode, buf, len);
    }
}



/*
 * ip_controlprotl
 */
int
ipControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    PState	*pstate;
    IPhost 	net, mask, gw, dest;
    route 	*rt;
    
    xAssert(xIsProtocol(self));
    pstate = (PState *) self->state;
    
    switch (opcode) {
	
      case IP_REDIRECT :
	{
	    checkLen(len, 2*IPHOSTLEN);
	    net = *(IPhost *)buf;
	    netMaskFind(&mask, &net);
	    gw = *(IPhost *)(buf + IPHOSTLEN);
	    xTrace3(ipp, 4, "IP_REDIRECT : net = %s, mask = %s, gw = %s",
		    ipHostStr(&net), ipHostStr(&mask), ipHostStr(&gw));
	    /*
	     * find which interface reaches the gateway
	     */
	    rt_add(pstate, &net, &mask, &gw, -1, RTDEFAULTTTL);
	    return 0;
	}

	/* test control ops - remove later */
      case IP_GETRTINFO :
	/* get route info for a given dest address :
	   in : IP host address 
	   out : route structure for this address
	   */
	checkLen(len, sizeof(route));
	dest = *(IPhost *)buf;
	rt = rt_get(&dest);
	*(route *)buf = *rt;
	rt_free(rt);
	return (sizeof(route));

      default:
	xTrace0(ipp,3,"Unrecognized opcode");
	return xControl(xGetDown(self, 0), opcode, buf, len);
    }
}


