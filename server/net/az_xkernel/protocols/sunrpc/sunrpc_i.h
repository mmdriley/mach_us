/* 
 * sunrpc_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.23 $
 * $Date: 1993/02/01 22:28:54 $
 */

#ifndef sunrpc_i_h
#define sunrpc_i_h

#include "xrpc.h"
#include "sunrpc.h"


#define RPCRPLYLEN	100  
#define RPC_WAIT        50000
#define RPC_TTOUT       200000
#define XDRBUFSIZE      1024 
#define RPCMAXLEN       1024 
#define CUR_RPC_VERS	2
#define RPC_VERS_LOW 	2
#define RPC_VERS_HIGH	2

/* older version included a third field, for procedure */
typedef struct {
    long	vers;
    long	prog;
} SunrpcPgm;


/* 
 * Both ActiveKey and PassiveKey are used to find servers.  
 */
typedef struct {
    SunrpcPgm	p;
    XObj	lls;
} ActiveKey;		


typedef struct {
    SunrpcPgm	p;
    long	port;
} PassiveKey;		



typedef struct rpc_msg	SunrpcHdr;
typedef struct opaque_auth	Auth;

typedef struct {
    SunrpcHdr	hdr;
    union {
	struct {
	    Event	timeoutEvent;
	    struct rpc_err	error;
	    Msg 	*replyMsg;
	    int		evAbort;
	    Semaphore 	replySem;
	    int       	tries;
	    u_int	tout;		/* time in microseconds */
	    u_int	wait;		/* time in microseconds */
	    Msg       	callMsg;
	} client;
	struct {
	    Auth	cred;		/* credentials of client */
	    long	proc;		/* called procedure number */
	} server;
    } u;
} SState;
#define c_returnStatus	u.client.returnStatus
#define c_timeoutEvent	u.client.timeoutEvent
#define c_error		u.client.error
#define c_replyMsg	u.client.replyMsg
#define c_evAbort	u.client.evAbort
#define c_replySem	u.client.replySem
#define c_tries		u.client.tries
#define c_tout		u.client.tout
#define c_wait		u.client.wait
#define c_callMsg	u.client.callMsg
#define s_cred		u.server.cred
#define s_proc		u.server.proc



typedef struct pstate {
    Map	replyMap;
    Map	passiveMap;
    Map	activeMap;
} PSTATE;


#define XDRHDRSIZE 4 * 38

extern int tracesunrpcp;
extern struct opaque_auth sunrpcAuthDummy;

#ifdef __STDC__

long		sunrpcHdrLoad( void *, char *, long, void * );
int		sunrpcEncodeHdr( Msg *, SunrpcHdr * );
int		sunrpcControlProtl( XObj, int, char *, int );
int 		sunrpcControlSessn( XObj, int, char *, int );
void		sunrpcAuthFree( struct opaque_auth * );
u_short		sunrpcGetPort( void );
xkern_return_t 	sunrpcClientDemux( XObj, XObj, Msg *, SunrpcHdr * );
xkern_return_t	sunrpcServerDemux( XObj, XObj, Msg *, SunrpcHdr * );
xkern_return_t	sunrpcPop( XObj self, XObj s, Msg *dg, VOID * );
int		sunrpcPush( XObj s, Msg *msg, Msg *reply_ptr );
xkern_return_t	sunrpcCall( XObj s, Msg *msg, Msg *reply_ptr );
void		sunrpcGetProcServer( XObj );
void		sunrpcSendError( int errorCode, XObj lls, int xid, void *arg );

#else

long		sunrpcHdrLoad();
int		sunrpcEncodeHdr();
int		sunrpcControlProtl();
int 		sunrpcControlSessn();
void		sunrpcAuthFree();
u_short		sunrpcGetPort();
xkern_return_t 	sunrpcClientDemux();
xkern_return_t	sunrpcServerDemux();
xkern_return_t	sunrpcPop();
int		sunrpcPush();
xkern_return_t	sunrpcCall();
void		sunrpcGetProcServer();
void		sunrpcSendError();

#endif __STDC__

#endif sunrpc_i_h
