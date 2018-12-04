/* 
 * rrx_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.10 $
 * $Date: 1993/02/01 22:35:00 $
 */
 
/* 
 * Mach Receive Right Transfer protocol (RRX)
 */


#ifndef rrx_i_h
#define rrx_i_h

#include "machripc_internal.h"
#include "machripc_xfer.h"
#include "xfer.h"
#include "rrx.h"
#include "bidctl.h"

extern	int	tracerrxp;

/* 
 * request codes
 */
typedef enum {
    RECEIVE_LOCK_REQ = 1,
    RECEIVE_UNLOCK_REQ,
    RECEIVE_RIGHT_TRANSFER
} RrxMsgType;

#define rrxMsgTypeStr(_t) 					\
  ((_t) == RECEIVE_LOCK_REQ ? "RECEIVE LOCK" :			\
   (_t) == RECEIVE_UNLOCK_REQ ? "RECEIVE UNLOCK" :		\
   (_t) == RECEIVE_RIGHT_TRANSFER ? "RECEIVE TRANSFER" :	\
   "UNKNOWN")


/* 
 * reply codes
 */
typedef enum {
    RRX_SUCCESS	= 1,
    RRX_FAILURE,
    RRX_REJECTED
} RrxReplyCode;


typedef struct {
    XferHost	xh;
    int		sendCount;
} RrxSendHost;


/* 
 * Used to temporarily represent a list of sending hosts and relevant
 * information.  
 */
typedef struct listElement {
    struct listElement	*next;
    struct listElement	*prev;
    RrxSendHost		sh;
    bool		valid;
    XObj		lls;
} ListElement;


/* 
 * This represents an incoming RRX request, generated by rrxLoadRequest.
 */
typedef struct {
    RrxMsgType		type;
    NetPortNumber	portNumber;
    mn_arch_tag_t	archTag;
    union {
	struct {
	    XferHost	nrr;		
	} unlock;
	struct {
	    MsgId	msgId;
	    u_short	numSenders;	
	    RrxSendHost	*senders;	
	    XferHost	orr;
	} transfer;
    } u;
} RrxReqMsg;
  

/* 
 * This structure is used to form an outgoing RRX request, passed to
 * rrxStoreRequest. 
 */
typedef struct {
    RrxMsgType	type;
    mnetport	*port;
    union {
	struct {
	    XferHost	nrr;		
	} unlock;
	struct {
	    XferHost	orr;
	    ListElement	*senders;	
	    MsgId	msgId;
	} transfer;
    } u;
} RrxReqOutgoing;


/* 
 * Both incoming and outgoing reply messges
 */
typedef struct {
    RrxMsgType		type;
    RrxReplyCode	replyCode;
} RrxRepMsg;

#define RRX_REPMSG_NETLEN	4



#ifdef __STDC__


long		rrxLoadReply( VOID *, char *, long, VOID * );
void		rrxStoreReply( VOID *, char *, long, VOID * );
xkern_return_t	rrxLoadRequest( RrxReqMsg *, Msg * );
void		rrxStoreRequest( RrxReqOutgoing *, Msg * );

/* 
 * Headers loaded with rrxLoadRequest include dynamically allocated
 * storage.  rrxReqDispose should be called for each of these
 * headers. 
 */
void		rrxReqDispose( RrxReqMsg * );

#else

long		rrxLoadReply();
void		rrxStoreReply();
xkern_return_t	rrxLoadRequest();
void		rrxStoreRequest();

/* 
 * Headers loaded with rrxLoadRequest include dynamically allocated
 * storage.  rrxReqDispose should be called for each of these
 * headers. 
 */
void		rrxReqDispose();

#endif


#endif  ! rrx_i_h
