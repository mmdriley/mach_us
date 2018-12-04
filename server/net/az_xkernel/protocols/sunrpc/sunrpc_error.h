/*
 * sunrpc_error.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:29:06 $
 */

/*
 * Send the indicated error message to the rpc session's peer
 */
void
sunrpcSendError( int errorCode, XObj s, void *arg );

/*
 * Send the indicated error message out on the dual of session lls.
 * This should be used when a lower outgoing session has not been allocated.
 */
void
sunrpcSendErrorDual( int errorCode, XObj lls, struct rpc_msg *hdr );
