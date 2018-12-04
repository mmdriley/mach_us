/* 
 * srx_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.10 $
 * $Date: 1993/02/01 22:34:49 $
 */
 
/* 
 * Mach Send Right Transfer protocol (SRX)
 */


#ifndef srx_i_h
#define srx_i_h

#include "machripc_internal.h"
#include "machripc_xfer.h"
#include "xfer.h"
#include "srx.h"
#include "bidctl.h"

/* 
 * request codes
 */
typedef enum {
    SEND_LOCK_REQ = 1,
    UNLOCK_NO_TRANSFER,
    UNLOCK_WITH_TRANSFER,
    SEND_RIGHT_TRANSFER
} SrxMsgType;

#define srxMsgTypeStr(_t) 					\
  ((_t) == SEND_LOCK_REQ ? "LOCK REQUEST" :			\
   (_t) == UNLOCK_NO_TRANSFER ? "UNLOCK NO XFER" :		\
   (_t) == UNLOCK_WITH_TRANSFER ? "UNLOCK WITH XFER" :		\
   (_t) == SEND_RIGHT_TRANSFER ? "TRANSFER MSG" :		\
   "UNKNOWN")


/* 
 * reply codes
 */
typedef enum {
    SRX_SUCCESS	= 1,
    SRX_FAILURE
} SrxReplyCode;



typedef struct {
    SrxMsgType	type;
    mnetport	*port;
    union {
	struct {
	    XferHost	nsr;
	} lock;
	struct {
	    MsgId	msgId;
	    XferHost	rr;
	    int		sendCount;
	} xfer;
    } u;
} SrxReqOutgoing;

typedef struct {
    SrxMsgType		type;
    NetPortNumber	portNumber;
    XferHost		osr;
    mn_arch_tag_t	archTag;
    union {
	struct {
	    XferHost	nsr;
	} lock;		
	struct {
	    MsgId	msgId;
	    XferHost	rr;
	    int		sendCount;
	} xfer;
    } u;
} SrxReqMsg;


typedef struct {
    SrxMsgType		type;
    SrxReplyCode	replyCode;
    union {
	struct {
	    int		sendCount;
	} lock;
    } u;
} SrxRepMsg;



typedef enum {
    SRX_TRANSFERRED,
    SRX_LOCKING_FAILURE,
    SRX_DESTINATION_FAILURE
} srx_return_t;


extern	XferHost	srxMyXferHost;
extern	int		tracesrxp;

#ifdef __STDC__

xkern_return_t	srxLoadReply( SrxRepMsg *, Msg * );
xkern_return_t	srxLoadRequest( SrxReqMsg *, Msg * );
void		srxStoreReply( SrxRepMsg *, Msg * );
void		srxStoreRequest( SrxReqOutgoing *, Msg * );

#else

xkern_return_t	srxLoadReply();
xkern_return_t	srxLoadRequest();
void		srxStoreReply();
void		srxStoreRequest();


#endif


#endif  ! srx_i_h
