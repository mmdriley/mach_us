/*
 * ip_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.32 $
 * $Date: 1993/02/01 22:22:40 $
 */

#ifndef ip_i_h
#define ip_i_h

#ifndef ip_h
#include "ip.h"
#endif

#ifndef vnet_h
#include "vnet.h"
#endif


#define	IPHLEN	(sizeof(IPheader))

#define IPVERS	(4 << 4)
#define VERS_MASK 0xf0
#define HLEN_MASK 0x0f
#define GET_HLEN(h) ((h)->vers_hlen & HLEN_MASK)
#define GET_VERS(h) ( ((h)->vers_hlen & VERS_MASK) >> 4 )

#define IPMAXINTERFACES  10
#define IPOPTPACKET 512
#define IPMAXPACKET 65000*4	/* I mean maximum possible for protocol */
#define IPDEFAULTDGTTL  30
#define IP_GC_INTERVAL 30 * 1000 * 1000		/* 30 seconds */

/* fragment stuff */
#define DONTFRAGMENT  0x4000
#define MOREFRAGMENTS  0x2000
#define FRAGOFFMASK   0x1fff
#define FRAGOFFSET(fragflag)  ((fragflag) & FRAGOFFMASK)
#define COMPLETEPACKET(hdr) (!((hdr).frag & (FRAGOFFMASK | MOREFRAGMENTS)))
#define INFINITE_OFFSET      65000

#define IP_ACTIVE_MAP_SZ	101
#define IP_FORWARD_MAP_SZ	101
#define IP_PASSIVE_MAP_SZ	23
#define IP_PASSIVE_SPEC_MAP_SZ	23
#define IP_FRAG_MAP_SZ		23

typedef struct ipheader {
  	u_char 	vers_hlen;	/* high 4 bits are version, low 4 are hlen */
	u_char  type;
	u_short	dlen;
	u_short ident;
	u_short frag;
	u_char  time;
	u_char  prot;
	u_short checksum;
  	IPhost	source;		/* source address */
  	IPhost	dest;		/* destination address */
} IPheader; 



typedef struct pstate {
    XObj	self;
    Map 	activeMap;
    Map		fwdMap;
    Map  	passiveMap;
    Map  	passiveSpecMap;
    Map		fragMap;
    int		numIfc;
} PState;

typedef struct sstate {
    IPheader	hdr;
    int		mtu;		   /* maximum transmission unit on intface */
} SState;

/*
 * The active map is keyed on the local and remote hosts rather than
 * the lls because the lls may change due to routing while the hosts
 * in the IP header will not.
 */
typedef struct {
    long	protNum;
    IPhost	remote;	/* remote host  */
    IPhost	local;	/* local host	*/
}	ActiveId;

typedef IPhost	FwdId;

typedef long	PassiveId;

typedef struct {
    long	prot;
    IPhost	host;
} PassiveSpecId;


/*
 * fragmentation structures
 */

typedef struct {
    IPhost source, dest;
    u_char prot;
    u_char pad;
    u_short seqid;
} FragId;

typedef struct hole_ent {
    u_short first, last;
} Hole_ent;

#define RHOLE  1
#define RFRAG  2

typedef struct fragif {
    u_char type;
    union {
	Hole_ent  *hole;
	Msg	  frag;
    } data;
    struct fragif *next, *prev;
} Fraginfo;

typedef struct Fragtable {
    u_short  	nholes;
    Fraginfo 	*listhead;
    Bind     	binding;
    bool	gcMark;
} Fragtable;

#define ERR_FRAG ((Fragtable *)-1)

#include "route.h"

#ifdef __STDC__

int		ipControlProtl( XObj, int, char *, int );
int		ipControlSessn( XObj, int, char *, int );
XObj		ipCreatePassiveSessn( XObj, XObj, ActiveId *, FwdId * );
xkern_return_t	ipDemux( XObj, XObj, Msg * );
void 		ipDumpHdr( IPheader * );
void 		ipDumpHdr( IPheader * );
Enable *	ipFindEnable( XObj, int, IPhost * );
xkern_return_t 	ipForwardPop( XObj, XObj, Msg *, VOID * );
void		ipFreeFragtable( Fragtable * );
xkern_return_t 	ipFwdBcastPop( XObj, XObj, Msg *, VOID * );
int		ipGetHdr( Msg *, IPheader *, char * );
void		ipHdrStore( VOID *, char *, long, VOID * );
int		ipHostOnLocalNet( PState *, IPhost *);
int		ipIsMyAddr( XObj, IPhost * );
xkern_return_t	ipMsgComplete( XObj, XObj, Msg *, VOID * );
void		ipProcessRomFile( void );
xkern_return_t	ipReassemble( XObj, XObj, Msg *, IPheader * );
int		ipRemoteNet( PState *, IPhost *, route *);
void		ipRouteChanged( PState *, route *,
			       int (*)(PState *, IPhost *, route *) );
int		ipSameNet( PState *, IPhost *, route *);
xmsg_handle_t	ipSend( XObj s, XObj lls, Msg *msg, IPheader *hdr );
xkern_return_t	ipStdPop( XObj, XObj, Msg *, VOID * );
void		scheduleIpFragCollector( PState * );

#else

int		ipControlProtl();
int		ipControlSessn();
XObj		ipCreatePassiveSessn();
xkern_return_t	ipDemux();
Enable *	ipFindEnable();
xkern_return_t 	ipForwardPop();
void		ipFreeFragtable();
xkern_return_t 	ipFwdBcastPop();
int		ipGetHdr();
void		ipHdrStore();
int		ipIsMyAddr();
xkern_return_t	ipMsgComplete();
void		ipProcessRomFile();
xkern_return_t	ipReassemble();
int		ipRemoteNet();
void		ipRouteChanged();
int		ipSameNet();
xmsg_handle_t	ipSend();
xkern_return_t	ipStdPop();
void		scheduleIpFragCollector();

#endif

extern int 	traceipp;
extern IPhost	ipSiteGateway;

#endif /* ip_i_h */
