/* 
 * chan_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.37 $
 * $Date: 1993/02/01 22:30:53 $
 */
 
#ifndef chan_internal_h
#define chan_internal_h

#include "chan.h"

#define CHANHLEN  		sizeof(CHAN_HDR) 
#define CHANEXTIDLEN 		sizeof(EXT_ID);
/* 
 * START_SEQ must be > 0
 */
#define START_SEQ 		1
 

/* 
 * Flags
 */
#define	FROM_CLIENT	0x01
#define	USER_MSG	0x02	/* valid user data */
#define ACK_REQUESTED	0x04
#define NEGATIVE_ACK	0x08


/* 
 * Protocol down vector indices
 */
#define CHAN_BIDCTL_I	1


typedef enum {
    /*--- Server states */
    SVC_EXECUTE = 1,	/* Sent msg up, waiting for answer */
    SVC_WAIT,		/* Waiting for an ACK of the reply */
    SVC_IDLE,
    /*--- Client states */
    CLNT_WAIT,
    CLNT_FREE,
    /*--- Client or server */
    DISABLED
} FsmState;


/*--- Message sequence possible status */
typedef enum {
  	old, current, new
} SEQ_STAT;

typedef u_int	SeqNum;
typedef u_short	Channel;

/*--- Channel Header structure */ 
typedef struct  {
    Channel	chan;
    u_char	unused;
    u_char 	flags;
    u_int	prot_id;
    SeqNum	seq;
    u_int	len;
} CHAN_HDR;


typedef struct {
    Msg	m;
    char valid;
} ChanMsg;


/*--- Channel State structure */
typedef struct {
        FsmState 	cur_state;
        int 		wait;
	int		waitParam;	/* user configurable wait value */
	int		maxWait;	/* user configurable wait value */
        Event 		event;
	CHAN_HDR 	hdr;
	ChanMsg		saved_msg;
	/* 
	 * client only
	 */
	xkern_return_t	replyValue;
        Msg 		*answer;
        Semaphore 	reply_sem;
        int 		ticket;
	IPhost		peer;
} CHAN_STATE, SState;

#define CHAN_IDLE_CLIENT_MAP_SZ		31
#define	CHAN_IDLE_SERVER_MAP_SZ		31
#define CHAN_ACTIVE_CLIENT_MAP_SZ	101
#define CHAN_ACTIVE_SERVER_MAP_SZ	101
#define CHAN_HLP_MAP_SZ		1

#define FIRST_CHAN	1

/*--- Protocol state structure */
typedef struct {
    Map		actCliHostMap, actCliKeyMap, idleCliMap;
    Map 	actSvrHostMap, actSvrKeyMap, idleSvrMap;
    Map 	passiveMap;
    Map		newChanMap;
    Semaphore	newSessnLock;
} PSTATE, PState;

/*--- Ids used to demux to active sessions */
typedef struct {
    int		chan;
    XObj	lls;
    u_int 	prot_id;
} ActiveID;

/*--- prot_id serves as passive key to demux to passive sessions */
typedef unsigned int PassiveID;


/*--- Delays are specified in microseconds! */
#define SERVER_WAIT_DEFAULT	8 * 1000 * 1000		/* 8 seconds */
#define SERVER_MAX_WAIT_DEFAULT	20 * 1000 * 1000	/* 20 seconds */
#define CLIENT_WAIT_DEFAULT	5 * 1000 * 1000		/* 5 seconds */
#define CLIENT_MAX_WAIT_DEFAULT	20 * 1000 * 1000


/*--- We still need to figure out a good delay, may need to query
      bottom protocol for hints. */
#define CHAN_CLNT_DELAY(m, p) 	(p)
#define CHAN_SVC_DELAY(m, p)  	(p)

#define msg_valid(M)	((M).valid = 1)
#define msg_clear(M)	((M).valid = 0)
#define msg_isnull(M)	((M).valid == 0)
#define msg_flush(M) 	{ if ((M).valid) {  msgDestroy(&(M).m);  \
			    	 	    (M).valid=0; } }

#define MAX(x,y) 		((x)>(y) ? (x) : (y))
#define MIN(x,y) 		((x)<(y) ? (x) : (y))


typedef		int	(* MapChainForEachFun)(
#ifdef __STDC__
					       VOID *, VOID *
#endif					       
					       );

#ifdef __STDC__

typedef		XObj (ChanOpenFunc)( XObj, XObj, XObj, ActiveID *, int );

xkern_return_t	chanAddIdleSessn( Map, Map, Map, XObj );
xkern_return_t	chanBidctlRegister( XObj, IPhost * );
int		chanCheckMsgLen( u_int, Msg * );
SEQ_STAT 	chanCheckSeq( u_int, u_int );
void		chanClientIdleRespond( CHAN_HDR	*, XObj, u_int );
void		chanClientPeerRebooted( PSTATE *, IPhost * );
int		chanControlSessn( XObj, int, char *, int );
XObj 		chanCreateSessn( XObj, XObj, XObj, ActiveID *, Pfv, Map, Map );
void		chanDestroy( XObj );
void		chanDispKey( ActiveID * );
void		chanEventFlush( XObj );
void		chanFreeResources( XObj );
Map		chanGetChanMap( XObj, IPhost * );
Map 		chanGetMap( Map, XObj, long );
long		chanGetProtNum( XObj, XObj );
int		chanMapRemove( VOID *, VOID * );
xkern_return_t 	chanOpenEnable( XObj, XObj, XObj, Part * );
xkern_return_t 	chanOpenDisable( XObj, XObj, XObj, Part * );
void		chanRemoveActive( XObj, Map keyMap, Map hostMap );
int		chanRemoveIdleSessns( Map, IPhost * );
void 		chanReply( XObj, CHAN_HDR *, int );
xmsg_handle_t	chanResend( XObj, int, int );
void		chanServerPeerRebooted( PSTATE *, IPhost * );
char * 		chanStateStr( int );
void		chanTimeout( Event, VOID * );
ChanOpenFunc	chanSvcOpen;
void		chanHdrStore( void *, char *, long, void * );
XObj		chanOpen( XObj, XObj, XObj, Part * );
char * 		chanStatusStr( SEQ_STAT );
void 		pChanHdr( CHAN_HDR * );

/* 
 * chan_mapchain.c
 */
void		chanMapChainAddObject( VOID *, Map, IPhost *, long, int );
int		chanMapChainApply( Map, IPhost *, MapChainForEachFun );
Map		chanMapChainFollow( Map, IPhost *, long );

#else

typedef		XObj (ChanOpenFunc)();

xkern_return_t	chanAddIdleSessn();
xkern_return_t	chanBidctlRegister();
int		chanCheckMsgLen();
SEQ_STAT 	chanCheckSeq();
void		chanClientIdleRespond();
void		chanClientPeerRebooted();
int		chanControlSessn();
XObj 		chanCreateSessn();
void		chanDestroy();
void		chanDispKey();
void		chanEventFlush();
void		chanFreeResources();
Map 		chanGetChanMap();
Map 		chanGetMap();
long		chanGetProtNum();
void		chanHdrStore();
void		chanInternalClose();
int		chanMapRemove();
xkern_return_t 	chanOpenEnable();
xkern_return_t 	chanOpenDisable();
void		chanRemoveActive();
int		chanRemoveIdleSessns();
void 		chanReply();
xmsg_handle_t	chanResend();
char * 		chanStateStr();
void		chanServerPeerRebooted();
XObj		chanOpen();
char * 		chanStatusStr();
XObj		chanSvcOpen();
void		chanTimeout();
void 		pChanHdr();

void		chanMapChainAddObject();
int		chanMapChainApply();
Map		chanMapChainFollow();

#endif


extern	int	tracechanp;

#endif
