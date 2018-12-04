/*     
 * $RCSfile: upi.h,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.67 $
 * $Date: 1993/02/01 22:39:53 $
 */

#ifndef upi_h
#define upi_h

#include "xtype.h"
#include "idmap.h"
#include "msg_s.h"
#include "part.h"

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif


typedef enum {Protocol,Session} XObjType;

/* default number of down protocols or sessions */
#define STD_DOWN 8

typedef	struct xobj {
  XObjType type;
  char  *name;
  char  *instName;
  char  *fullName;
  VOID	*state;
  Bind  binding;
  int   rcnt;
  int	id;
  int	*traceVar;
#if __STDC__
  struct xobj *
    (* open)( struct xobj *, struct xobj *, struct xobj *, Part * );
  enum xkret	(* close)( struct xobj * );
  enum xkret	(* closedone)( struct xobj * );
  enum xkret
    (* openenable)( struct xobj *, struct xobj *, struct xobj *, Part * );
  enum xkret
    (* opendisable)( struct xobj *, struct xobj *, struct xobj *, Part *);
  enum xkret	(* opendisableall)( struct xobj *, struct xobj * );
  enum xkret
    (* opendone)( struct xobj *, struct xobj *, struct xobj *, struct xobj * );
  enum xkret	(* demux)( struct xobj *, struct xobj *, Msg * );
  enum xkret 	(* calldemux)( struct xobj *, struct xobj *, Msg *, Msg * );
  enum xkret 	(* pop)( struct xobj *, struct xobj *, Msg *, void * );
  enum xkret 	(* callpop)( struct xobj *, struct xobj *, Msg *, void *, Msg * );
  xmsg_handle_t	(* push)( struct xobj *, Msg * );
  enum xkret 	(* call)( struct xobj *, Msg *, Msg * );
  int		(* control)( struct xobj *, int, char *, int );
  enum xkret	(* duplicate)( struct xobj * );
  enum xkret	(* shutdown)( struct xobj * );
#else
  Pfo	open;
  Pfk 	close;
  Pfk   closedone;
  Pfk	openenable;
  Pfk	opendisable;
  Pfk	opendisableall;
  Pfk	opendone;
  Pfk 	demux;
  Pfk 	calldemux;
  Pfk 	pop;
  Pfk 	callpop;
  Pfh 	push;
  Pfk 	call;
  Pfi 	control; 
  Pfk	duplicate;
  Pfk	shutdown;
#endif  
  int 	numdown;
  int	downlistsz;
  unsigned char	idle;
  struct xobj  *down[STD_DOWN];
  struct xobj **downlist;
  struct xobj  *myprotl;
  struct xobj  *up;
  struct xobj  *hlpType;
} *XObj;


typedef struct xenable {
  XObj	hlpRcv;
  XObj	hlpType;
  Bind	binding;
  int	rcnt;
} Enable;


typedef XObj Sessn;
typedef XObj Protl;

extern XObj protl_tab[];
extern int globalArgc;
extern char **globalArgv;

#include "msg.h"



/* error stuff */


#define ERR_XK_MSG	((xmsg_handle_t) XK_FAILURE)
#define	ERR_XOBJ	((XObj) XK_FAILURE)
#define	ERR_SESSN	((Sessn) XK_FAILURE)
#define	ERR_PROTL	((Protl) XK_FAILURE)
#define	ERR_BIND	((Bind) XK_FAILURE)
#define ERR_ENABLE	((Enable *) XK_FAILURE)
#define ERR_XMALLOC	0

/* protocol and session operations */

#ifdef __STDC__

extern	XObj xOpen( XObj hlpRcv, XObj hlpType, XObj llp, Part *p );
extern	xkern_return_t
  xOpenEnable( XObj hlpRcv, XObj hlpType, XObj llp, Part *p );
extern	xkern_return_t
  xOpenDisable( XObj hlpRcv, XObj hlpType, XObj llp, Part *p );
extern	xkern_return_t  xOpenDisableAll( XObj hlpRcv, XObj llp );
extern	xkern_return_t  xOpenDone( XObj hlp, XObj s, XObj llp );
extern	xkern_return_t  xCloseDone( XObj s );
extern	xkern_return_t  xClose( XObj s );
extern	xkern_return_t  xDemux( XObj s, Msg *msg );
extern	xkern_return_t  xCallDemux( XObj s, Msg *msg, Msg *returnmsg );
extern	xmsg_handle_t	xPush( XObj s, Msg *msg );
extern	xkern_return_t  xCall( XObj s, Msg *msg, Msg *returnmsg );
extern  xkern_return_t	xShutDown( XObj );

/* 
 * xPop and xCallPop prototypes are in upi_inline.h
 */

extern	int  		xControl( XObj s, int opcode, char *buf, int len );
extern  xkern_return_t  xDuplicate( XObj s );

typedef void	(* DisableAllFunc)( VOID *, Enable * );

xkern_return_t	defaultOpenDisable( Map, XObj, XObj, VOID * );
xkern_return_t	defaultOpenDisableAll( Map, XObj, DisableAllFunc);
xkern_return_t	defaultOpenEnable( Map, XObj, XObj, VOID * );
xkern_return_t	defaultVirtualOpenDisable( XObj, Map, XObj, XObj, XObj *,
					   Part *);
xkern_return_t	defaultVirtualOpenEnable( XObj, Map, XObj, XObj, XObj *,
					  Part *);

#else

extern	XObj xOpen();
extern	xkern_return_t  xOpenEnable();
extern	xkern_return_t  xOpenDisable();
extern	xkern_return_t  xOpenDone();
extern	xkern_return_t  xCloseDone();
extern	xkern_return_t  xClose();
extern	xkern_return_t  xDemux();
extern	xkern_return_t  xCallDemux();
extern	xmsg_handle_t 	xPush();
extern	xkern_return_t  xCall();
extern	int  		xControl();
extern  xkern_return_t  xDuplicate();
extern  xkern_return_t	xShutDown();

typedef void	(* DisableAllFunc)();

xkern_return_t	defaultOpenDisable();
xkern_return_t	defaultOpenEnable();
xkern_return_t	defaultVirtualOpenDisable();
xkern_return_t	defaultVirtualOpenEnable();

#endif

/* initialization operations */

#ifdef __STDC__

extern	XObj xCreateSessn(Pfv f, XObj hlpRcv, XObj hlpType, XObj llp,
			  int downc, XObj *downv);
extern	XObj xCreateProtl(Pfv f, char *nm, char *inst, int downc, XObj *downv);
extern	xkern_return_t  xDestroy(XObj s);
extern  void		upiInit( void );

#else

extern XObj 		xCreateSessn();
extern XObj 		xCreateProtl();
extern xkern_return_t	xDestroy();
extern void		upiInit();

#endif

/* utility routines */

#ifdef __STDC__

extern	XObj 	xGetProtlByName(char *name);
extern  bool	xIsValidXObj( XObj );
extern	xkern_return_t  xSetDown(XObj self, int i, XObj object);
extern	XObj 	xGetDown(XObj self, int i);
extern	void	xPrintXObj( XObj );
extern  void	xRapture( void );

#else

extern	XObj 	xGetProtlByName();
extern  bool	xIsValidXObj();
extern	xkern_return_t  xSetDown();
extern	XObj 	xGetDown();
extern	void	xPrintXObj();
extern  void	xRapture();

#endif __STDC__


/* object macros */

#define xSetUp(s, hlp)	((s)->up = (hlp))
#define xGetUp(s)	((s)->up)
#define xMyProtl(s) ((s)->myprotl)
#define xIsXObj(s) ((s) && (s) != ERR_XOBJ)
#define xIsSession(s) ((s) && (s) != ERR_XOBJ && (s)->type == Session)
#define xIsProtocol(s) ((s) && (s) != ERR_XOBJ && (s)->type == Protocol)
#define xHlpType(s)	((s)->hlpType)

/*
 * control operation definitions
 *
 * NOTE: if you change the standard control ops, make the
 * corresponding change to the controlops string array in upi.c
 */
enum {
    GETMYHOST = 0,	/* standard control operations        */
    GETMYHOSTCOUNT,	/* common to all protocols            */
    GETPEERHOST,
    GETPEERHOSTCOUNT,
    GETBCASTHOST,
    GETMAXPACKET,
    GETOPTPACKET,
    GETMYPROTO,
    GETPEERPROTO,
    RESOLVE,
    RRESOLVE,
    FREERESOURCES,
    GETPARTICIPANTS,
    SETNONBLOCKINGIO
};

#define	ARP_CTL		1	/* like a protocol number; used to    */
#define	BLAST_CTL	2	/* partition opcode space	      */
#define	ETH_CTL		3
#define	IP_CTL		4
#define	SCR_CTL		5
#define	VCHAN_CTL	6
#define	PSYNC_CTL	7
#define	SS_CTL		8
#define	SUNRPC_CTL	9
#define	NFS_CTL		10
#define	TCP_CTL		11
#define UDP_CTL		12
#define ICMP_CTL	13
#define VNET_CTL	14
#define BIDCTL_CTL	15
#define CHAN_CTL	16

#define	TMP0_CTL	100	/* for use by new/tmp protocols until */
#define	TMP1_CTL	101	/* a standard CTL number is assigned  */
#define	TMP2_CTL	102
#define	TMP3_CTL	103
#define	TMP4_CTL	104

#define	MAXOPS		100	/* maximum number of ops per protocol */

/* Check the length of a control argument */
#define checkLen(A, B) { \
  if ((A) < (B)) { \
    return -1; \
  } \
}

#include "ip_host.h"
#include "eth_host.h"

#ifdef __STDC__

extern xkern_return_t	str2ipHost( IPhost *, char * );
extern xkern_return_t	str2ethHost( ETHhost *, char * );
extern char *	ipHostStr( IPhost * );
extern char *	ethHostStr( ETHhost * );

#else

extern xkern_return_t	str2ipHost();
extern xkern_return_t	str2ethHost();
extern char *	ipHostStr();
extern char *	ethHostStr();

#endif __STDC__


extern XObj	xNullProtl;


#define X_NEW(Type) (Type *)xMalloc(sizeof(Type))
#define FREE(X) (free(X), (X) = 0)

/*
 * Optimize xDemux, xCallDemux, xPush and xCall as macros.  The other critical
 * path upi functions (xPop and xCallPop) are defined as inline functions
 * in upi_inline.h
 */
#if ! defined(XK_DEBUG)

#   define xDemux(DS, M) ((*((DS)->up->demux))((DS)->up, (DS), (M)))
#   define xCallDemux(DS, M, RM) ((*((DS)->up->calldemux))((DS)->up, (DS), (M), (RM)))
#   define xPush(S, M) ((*((S)->push))((S), (M)))
#   define xCall(S, M, RM) ((*((S)->call))((S), (M), (RM)))

#endif ! XK_DEBUG


#endif

