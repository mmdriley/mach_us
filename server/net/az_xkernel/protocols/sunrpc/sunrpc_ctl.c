/* 
 * sunrpc_control.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.15 $
 * $Date: 1993/02/01 22:28:48 $
 */

#include "xkernel.h"
#include "sunrpc_i.h"
#include "xrpc.h"
#include "xrpc_print.h"


static void
pushComponent(p, value)
    Part *p;
    long int value;
{
    long	*l;

    l = (long *)xMalloc(sizeof(long));
    *l = value;
    partPush(*p, l, sizeof(long));
}


/* RPC requires many control opcodes  
   Those begining with RPC may be used
   by either client or server. Those
   begining with SVC are only defined for
   server. Thouse begining with CLNT are
   only defined for client.
   */
int
sunrpcControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    switch (opcode) {

      case SUNRPC_GETPORT:
	checkLen(len, sizeof(short));
	{
	    u_short port;
	    port = (u_short) sunrpcGetPort();
	    bcopy((char *)&port, buf, sizeof(short));
	    return(0);
	}
	break;
	
      default:
	return xControl(xGetDown(self, 0), opcode, buf, len);
    } 
}


#define TIMEVALLEN sizeof(struct timeval)
#define checkServer(s) { if ((s)->hdr.rm_direction != REPLY) return -1; }
#define checkClient(s) { if ((s)->hdr.rm_direction != CALL) return -1; }
#define getCred(s) ( (s)->hdr.rm_direction == CALL ?	\
		    	&(s)->hdr.rm_call.cb_cred : &(s)->s_cred )
#define getVerf(s) ( (s)->hdr.rm_direction == CALL ?		\
		    	&(s)->hdr.rm_call.cb_verf :		\
		        &(s)->hdr.rm_reply.rp_acpt.ar_verf )

int
sunrpcControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
    SState	*state;
    
    xTrace1(sunrpcp, 3, "in rpc_control with session=%x\n", s); 
    
    state = (SState *)s->state;

    switch (opcode) {

	/* copy high level protocol to  buf */
	/* wild hack to help server demux figure */
	/* out who he is. */
      case SUNRPC_SVCGETHLP: 
	checkServer(state);
	checkLen( len, sizeof(s->up));
	bcopy((char *)&s->up, buf, sizeof(s->up));
	return(0);
	break;
	
	/* copy argument's xdr byte string to buf */
	/* copy RPC error status structure to buf */
      case SUNRPC_CLNTGETERROR: 
	checkClient(state);
	checkLen( len, sizeof(struct rpc_err));
	bcopy((char *)&state->c_error, buf, sizeof(struct rpc_err));
	return(0);
	break;
	
#if 0
	/* copy callers UDP address to buf */
      case SUNRPC_SVCGETCALLER:
	checkServer(state);
	return(xControl(xGetDown(s, 0), GETPEERADDR, buf, len));
	break;
	
	/* copy servers RPC address to buf */
      case SUNRPC_SVCGETSERVER:
	checkServer(state);
	checkLen( len, SUNRPCADLEN);
	xIfTrace(sunrpcp, 9) {
	    prpcaddr(state->server);
	}
	bcopy((char *)&state->server, buf, SUNRPCADLEN);
	return(0);
	break;
#endif
	
	/* set ttout the clients timeout value */ 
      case SUNRPC_CLNTSETTOUT:
	checkClient(state);
	checkLen( len, TIMEVALLEN);
	bcopy(buf, (char *)&state->c_tout, TIMEVALLEN);
	return(0);
	break;
	
	/* copy ttout to buf */
      case SUNRPC_CLNTGETTOUT:
	checkClient(state);
	checkLen( len, TIMEVALLEN);
	bcopy((char *)&state->c_tout, buf, TIMEVALLEN);
	return(0);
	break;
	
	/* set clients wait value */
      case SUNRPC_CLNTSETWAIT:
	checkClient(state);
	checkLen( len, TIMEVALLEN);
	bcopy(buf, (char *)&state->c_wait, TIMEVALLEN);
	return(0);
	break;
	
	/* copy wait to buf */
      case SUNRPC_CLNTGETWAIT:
	checkClient(state);
	checkLen( len, TIMEVALLEN);
	bcopy((char *)&state->c_wait, buf, TIMEVALLEN);
	return(0);
	break;
	
	/* get authentication credentials type*/
      case SUNRPC_GETCREDTYPE:
	{
	    Auth	*cred = getCred(state);

	    checkLen( len, sizeof(int));
	    bcopy((char *)&cred->oa_flavor, buf, sizeof(int));
	    return(0);
	    break;
	}
	
	/* get authentication credentials */
      case SUNRPC_GETCRED:
	{
	    Auth	*cred = getCred(state);

	    checkLen( len, cred->oa_length); 
	    bcopy((char *)&cred->oa_base, buf, cred->oa_length);
	    return(0);
	    break;
	}
	
	/* set authentication credentials type*/
      case SUNRPC_SETCREDTYPE:
	{
	    Auth	*cred = getCred(state);

	    checkLen( len, sizeof(int));
	    sunrpcAuthFree(cred);
	    bcopy(buf, (char *)&cred->oa_flavor, sizeof(int));
	    return(0);
	    break;
	}
	
	/* set authentication credentials data*/
      case SUNRPC_SETCRED:
	{
	    Auth	*cred = getCred(state);

	    if (cred->oa_base != 0) {
		sunrpcAuthFree(cred);
	    }
	    cred->oa_length = len;
	    cred->oa_base = (caddr_t) xMalloc(len+2);
	    bcopy(buf, cred->oa_base, len);
	    {
		char *mname;
		struct authunix_parms *au_ptr;
		
		/* copy over the string, so we can possibly avoid user/kernel
		 * boundary problems at levels below rpc.  Dave 4-22 */
		au_ptr = (struct authunix_parms *) cred->oa_base;
		xAssert(au_ptr != NULL);
		mname = au_ptr->aup_machname;  /*existing machname-user ptr? */
	    }
	    return(0);
	    break;
	}
	
	/* get authentication verification type */
      case SUNRPC_GETVERFTYPE:
	{
	    Auth	*verf = getVerf(state);

	    checkLen( len, sizeof(int));
	    bcopy((char *)verf->oa_flavor, buf, sizeof(int));
	    return(0);
	    break;
	}
	
	/* get authentication verification */
      case SUNRPC_GETVERF:
	{
	    Auth	*verf = getVerf(state);
	    
	    checkLen( len, verf->oa_length); 
	    bcopy((char *)&verf->oa_base, buf, verf->oa_length);
	    return(0);
	    break;
	}
	
	/* set authentication verification type*/
      case SUNRPC_SETVERFTYPE:
	{
	    Auth	*verf = getVerf(state);
	    
	    checkLen( len, sizeof(int));
	    sunrpcAuthFree(verf);
	    bcopy(buf, (char *)&verf->oa_flavor, sizeof(int));
	    return(0);
	    break;
	}	

	/* set authentication verification data*/
      case SUNRPC_SETVERF:
	{
	    Auth	*verf = getVerf(state);
	    
	    if (verf->oa_base != 0) {
		sunrpcAuthFree(verf);
	    }
	    verf->oa_length = len;
	    verf->oa_base = (caddr_t) xMalloc(len+2);
	    bcopy(buf, (char *)&verf->oa_base, len);
	    return(0);
	    break;
	}
	
	/* cause an authentication error message to be sent. */
      case SUNRPC_SVCAUTHERR:
	{
	    checkServer(state);
	    checkLen(len , sizeof(enum auth_stat)) ;
	    sunrpcSendError(AUTH_ERROR,
			    xGetDown(s, 0), state->hdr.rm_xid, buf);
	    return(0);
	    break;
	}
	
	/* cause a garbage arguments error message to be sent. */
      case SUNRPC_SVCGARBAGEARGS:
	{
	    checkServer(state);
	    sunrpcSendError(GARBAGE_ARGS, xGetDown(s, 0), state->hdr.rm_xid,
			    &state->hdr.rm_reply.rp_acpt.ar_verf);
	    return(0);
	    break;
	}
	
	/* cause a system error message to be sent. */
      case SUNRPC_SVCSYSTEMERR:
	{
	    checkServer(state);
	    sunrpcSendError(SYSTEM_ERR, xGetDown(s, 0), state->hdr.rm_xid,
			    &state->hdr.rm_reply.rp_acpt.ar_verf);
	    return(0);
	    break;
	}
	
      case GETPARTICIPANTS:
	{
	    Part	*p = (Part *)buf;
	    int		retLen;

	    if ( state->hdr.rm_direction != CALL ) {
		return -1;
	    }
	    retLen = xControl(xGetDown(s, 0), opcode, buf, len);
	    if ( retLen < sizeof(Part) ) {
		return -1;
	    }
	    pushComponent(p, state->hdr.rm_call.cb_prog);
	    pushComponent(p, state->hdr.rm_call.cb_vers);
	    return retLen;
	}

      case GETMAXPACKET:
      case GETOPTPACKET:
	{
	    if ( xControl(xGetDown(s, 0), opcode, buf, len) < sizeof(int) ) {
		return -1;
	    }
	    *(int *)buf -= sizeof(struct rpc_msg);
	    return sizeof(int);
	}

      default:
	return xControl(xGetDown(s, 0), opcode, buf, len);
    }
    return(-1);
}




