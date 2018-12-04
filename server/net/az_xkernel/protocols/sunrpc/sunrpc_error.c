/* 
 * sunrpc_error.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.11 $
 * $Date: 1993/02/01 22:29:36 $
 */

/*
 * Functions dealing with the transmission of error messages to
 * the RPC peer
 */

#include "xkernel.h"
#include "sunrpc_i.h"
#include "sunrpc.h"


/*
 * Send the indicated error message to the rpc session's peer
 */
void
sunrpcSendError(errorCode, lls, xid, arg)
    int errorCode;
    XObj lls;
    int xid;
    VOID *arg;
{
    SunrpcHdr	errHdr;
    Msg 	errMsg;

    xTrace0(sunrpcp, 4, "sunrpc send error");
    bzero((char *)&errHdr, sizeof(struct rpc_msg));
    errHdr.rm_xid = xid;
    errHdr.rm_direction = REPLY;
    switch( errorCode ) {

      case RPC_MISMATCH:
	xTrace0(sunrpcp, 5, "sunrpc error: RPC_MISMATCH");
	errHdr.rm_reply.rp_stat  = MSG_DENIED;
	errHdr.rm_reply.rp_rjct.rj_stat = RPC_MISMATCH;
	errHdr.rm_reply.rp_rjct.rj_vers.high =  RPC_VERS_HIGH;
	errHdr.rm_reply.rp_rjct.rj_vers.low =  RPC_VERS_LOW;
	break;
	
      case PROC_UNAVAIL:
	xTrace0(sunrpcp, 5, "sunrpc error: PROC_UNAVAIL");
	errHdr.rm_reply.rp_stat  = MSG_ACCEPTED;
	errHdr.rm_reply.rp_acpt.ar_stat = PROC_UNAVAIL;
	break;

      case GARBAGE_ARGS:
	xTrace0(sunrpcp, 5, "sunrpc error: GARBAGE_ARGS");
	errHdr.rm_reply.rp_stat  = MSG_ACCEPTED;
	errHdr.rm_reply.rp_acpt.ar_stat = GARBAGE_ARGS;
	if (arg) {
	    bcopy((char *)arg, (char *)&errHdr.rm_reply.rp_acpt.ar_verf,
		  sizeof(struct opaque_auth));
	}
	break;

      case AUTH_ERROR:
	xTrace0(sunrpcp, 5, "sunrpc error: AUTH_ERROR");
	errHdr.rm_reply.rp_stat = MSG_DENIED;
	errHdr.rm_reply.rp_rjct.rj_stat = AUTH_ERROR;
	bcopy((char *)arg, (char *)&errHdr.rm_reply.rp_rjct.rj_why,
	      sizeof(enum auth_stat));
	break;

      case SYSTEM_ERR:
	xTrace0(sunrpcp, 5, "sunrpc error: SYSTEM_ERR");
	errHdr.rm_reply.rp_stat = MSG_ACCEPTED;
	errHdr.rm_reply.rp_acpt.ar_stat = SYSTEM_ERR;
	xAssert(arg);
	bcopy((char *)arg, (char *)&errHdr.rm_reply.rp_acpt.ar_verf,
	      sizeof(struct opaque_auth));
	break;

      default:
	xTrace1(sunrpcp, 0, "Unknown error code %d passed to sunrpc sendError",
		errorCode);
	return;
    }
    msgConstructEmpty(&errMsg);
    if (sunrpcEncodeHdr(&errMsg, &errHdr) == -1) {
	xTrace1(sunrpcp, 1, "Couldn't encode error header for error %d",
		errorCode);
    }
    xPush(lls, &errMsg);
    msgDestroy(&errMsg);
}

