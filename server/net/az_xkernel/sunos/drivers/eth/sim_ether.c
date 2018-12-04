/* 
 * sim_ether.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.52 $
 * $Date: 1993/02/01 23:48:38 $
 */

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <netinet/in.h>
#include "x_stdio.h"
#include "xkernel.h"
#include "eth.h"
#include "eth_i.h"
#include "sim_ether_i.h"
#include "arp.h"
#include "machine.h"	/* For routines to establish interrupt handler */

int	tracesimethp;

#define SIMETH_STATS	XK_DEBUG

static block_pool	pool;
#ifdef SIMETH_STATS
static int		activeThreads;
static int		activeHighWater;
#endif
static char		dummyBuf[4096];
static Msg		emptymsg;
static ETHhost		ethBcastHost = ETH_BCAST_HOST;
static int		instance;

extern int	bind( int, struct sockaddr *, int );
extern int	gethostname( char *, int );
extern int	getpid( void );
extern int	getsockopt( int, int, int, char *, int * );
extern char *	inet_ntoa( struct in_addr );
extern int	recvfrom( int, char *, int, int, struct sockaddr *, int * );
extern int	sendto( int, char *, int, int, struct sockaddr *, int );
extern int	setsockopt( int, int, int, char *, int );
extern int	socket( int, int, int );

static int		arpForEachFunc( ArpBinding *, VOID * );
static void		block_handler( Event, VOID * );
static void		init_eth_blocks( void );
static int		initSocket( int );
static void		internalDemux( block * );
static bool		msg2Buf( char *, long, VOID * );
static block *		nextBlock( void );
static void		processRomFile( XObj );
static int		read_ether( int, char *, int );
static int		readether2demux( VOID * );
static void		sendOnSocket( int, ETHhost *, char *, int );
static int		simethControl( XObj, int, char *, int );
static xkern_return_t	simethOpenEnable( XObj, XObj, XObj, Part * );
static xmsg_handle_t	simethPush( XObj, Msg * );
static void		writeBcast( PState *, char *, int );



/* changes to support full internet addressing in rom files */

void simEth2sock(ethAddr, sockAddr)
    ETHhost ethAddr;
    struct sockaddr_in *sockAddr;
{
  bzero((char *)sockAddr, sizeof (struct sockaddr_in));
  sockAddr->sin_family = AF_INET;
  /* 
   * IP address is in the first 4 bytes of the ethernet address
   * UDP port is in the 5th and 6th bytes of the ethernet address
   */
  sockAddr->sin_addr = *(struct in_addr *)&ethAddr;
  sockAddr->sin_port = (*(u_short *)((char *)&ethAddr + 4));
}


/* changed inAddr to pass-by-value to make sparc and 
   sun3 compatible - cjt 5/15 */

void sock2simEth(ethAddr, inAddr, udpPort)
     char *ethAddr;
     struct in_addr inAddr;
     int udpPort;
{
    char *cp1, *cp2;
    short tmpshrt;
    int i;
    
    cp1 = ethAddr;
    cp2 = (char *) &inAddr;		/* passed by value now - cjt */
    for (i=0; i<4; i++) *cp1++ = *cp2++;
    tmpshrt = htons((u_short)udpPort);
    cp2 = (char *) &tmpshrt;
    for (i=0; i<2; i++) *cp1++ = *cp2++;
}


static int
arpForEachFunc( ab, arg )
    ArpBinding	*ab;
    VOID 	*arg;
{
    EthMsg	*m = (EthMsg *)arg;

    sendOnSocket(m->sock, &ab->hw, m->buf, m->len);
    return TRUE;
}


/* 
 * We simulate broadcast by having ARP perform a callback
 * for every host it has in its table.  We then do a direct send for
 * each ARP entry.
 */
static void
writeBcast( ps, buf, len )
    PState	*ps;
    char	*buf;
    int		len;
{
    EthMsg	m;
    ArpForEach	afe;
    
    if ( ! ps->arp ) {
	xError("eth bcast write fails -- no arp protocol");
	return;
    }
    m.buf = buf;
    m.len = len;
    m.sock = ps->sock;
    afe.f = arpForEachFunc;
    afe.v = &m;
    xControl(ps->arp, ARP_FOR_EACH, (char *)&afe, sizeof(ArpForEach));
}	


static void
sendOnSocket( sock, dest, buf, len )
    int		sock;
    ETHhost	*dest;
    char	*buf;
    int		len;
{  
    extern unsigned long inet_addr();
    struct  sockaddr_in	addr;

    simEth2sock(*dest, &addr);
    xTrace3(simethp, TR_FUNCTIONAL_TRACE,
	    "write_ether: sending %d bytes to <%d,%s>",
	    len, ntohs(addr.sin_port), inet_ntoa(addr.sin_addr));
    
    while (sendto(sock, buf, len, 0, (struct sockaddr *)&addr,
		  sizeof(struct sockaddr)) != len) {
	xTrace0(simethp, TR_ERRORS, "write_ether: error in sendto");
	xError("sim_ether: sendto");
	/*    exit(1);  */
    }
}


static int
read_ether( int sock, char *msg, int len )
{
    struct sockaddr_in	from;
    int			size, n;
    
    xTrace0(simethp, TR_EVENTS, "read_ether");
    size = sizeof(from);
    if ((n = recvfrom(sock, msg, len, 0, (struct sockaddr *)&from, &size)) < 0)
      return -1;
    xTrace3(simethp, TR_FUNCTIONAL_TRACE,
	    "read_ether: receiving %d bytes from <%d,%s>",
	    n, ntohs(from.sin_port), inet_ntoa(from.sin_addr));
    return n;
}


static int
initSocket( port )
    int	port;
{
    int			s, bufSize, bufSizeSize;
    struct sockaddr_in	addr;
    int			on = 1;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((u_short)port);
    if ( (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	Kabort("init_ether: cannot open socket");
    }	
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on));
#ifdef SO_BROADCAST
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof (on));
#endif
    /*
     * increase receive buffer sizes from default
     */
    bufSize = RCVBUFSIZE;
    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&bufSize,
		   sizeof(bufSize))) {
	xTrace1(simethp, TR_ERRORS,
		"Could not set size of ethernet receive buffer to %d",
		bufSize);
    }
    bufSizeSize = sizeof(bufSize);
    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&bufSize, &bufSizeSize)) {
	xTrace0(simethp, TR_ERRORS,
		"Could not get size of ethernet receive buffer");
    } else {
	xTrace1(simethp, TR_GROSS_EVENTS,
		"Receive buffer of ethernet socket: %d", bufSize);
    }
    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&bufSize, &bufSizeSize)) {
	xTrace0(simethp, TR_ERRORS,
		"Could not get size of ethernet send buffer");
    } else {
	xTrace1(simethp, TR_GROSS_EVENTS,
		"Send buffer of ethernet socket: %d", bufSize);
    }
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr))) {
	Kabort("init_ether: cannot bind socket");
    }
    return s;
}


/* 
 * Sets the 'port' field of the protocol state
 */
static void
processRomFile( self )
    XObj	self;
{
    PState	*ps = (PState *)self->state;
    int 	i;
    char	instStr[80];

    strcpy(instStr, self->name);
    if ( strlen(self->instName) > 0 ) {
	strcat(instStr, "/");
	strcat(instStr, self->instName);
    }
    ps->port = -1;
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], instStr) ) {
	    if ( ! rom[i][1] || sscanf(rom[i][1], "%d", &ps->port) < 1 ) {
		sprintf(errBuf, "ETH format error in line %d of the rom file",
			i + 1);
		xError(errBuf);
		break;
	    }
	}
    }
}


void
simeth_init( self )
    XObj	self;
{
    struct hostent	*h;
    struct in_addr 	*in;
    char		name[100];
    int			namelen=100;
    PState		*ps;
    
    xTrace0(simethp, TR_MAJOR_EVENTS, "init_ether");
    if ( instance == 0 ) {
	init_eth_blocks();		
    }
    if ( instance > SIMETH_MAX_INSTANCES ) {
	Kabort("simeth -- too many instances");
    }
    ps = X_NEW(PState);
    bzero((char *)ps, sizeof(PState));
    self->state = (VOID *)ps;
    processRomFile(self);
    if ( ps->port == -1 ) {
	xTrace1(simethp, TR_ERRORS,
		"No port specified for simeth instance %d", instance);
	Kabort("simeth -- port specification error");
    }
    xTrace1(simethp, TR_MAJOR_EVENTS,
	    "init_ether: listening on port %d", ps->port);
    if ( (ps->sock = initSocket(ps->port)) == 0 ) {
	Kabort("init_ether: problems creating socket");
    }
    installSignalHandler(ps->sock, readether2demux, self);
    if ( fcntl(ps->sock, F_SETFL, (FASYNC | FNDELAY)) == -1 ) {
	Kabort("fcntl async");
    }
    if ( fcntl(ps->sock, F_SETOWN, getpid()) == -1 ) {
	Kabort("fcntl setown");
    }
    /* 
     * Determine my host address
     */
    gethostname(name,namelen);
    h = gethostbyname(name);
    in = (struct in_addr *) h->h_addr;
    sock2simEth((char *)&ps->myHost, *in, ps->port);
    xTrace1(simethp, TR_GROSS_EVENTS,
	    "init_ether: ethernet started with address %s",
	    ethHostStr(&ps->myHost));

    self->push = simethPush;
    self->control = simethControl;
    self->openenable  = simethOpenEnable;
    self->up = 0;
    instance++;
}


static void
ethMsgStore( void *hdr, char *netHdr, long len, void *arg )
{
    xAssert(len == sizeof(ETHhdr));
    ((ETHhdr *)hdr)->type = htons(((ETHhdr *)hdr)->type);
    bcopy(hdr, netHdr, sizeof(ETHhdr));
}


static long
ethMsgLoad( void *hdr, char *netHdr, long len, void *arg )
{
    xAssert(len == sizeof(ETHhdr));
    bcopy(netHdr, (char *)hdr, sizeof(ETHhdr));
    ((ETHhdr *)hdr)->type = ntohs(((ETHhdr *)netHdr)->type);
    return sizeof(ETHhdr);
}


static xmsg_handle_t
simethPush( self, msg )
    XObj	self;
    Msg 	*msg;
{
    ETHhdr	*hdr = msgGetAttr(msg, 0);
    char	buffer[EMAXPAK];
    char	*bufPtr = buffer;
    int		len;
    PState	*ps = (PState *)self->state;

    xTrace0(simethp, TR_EVENTS, "simethPush");
    xAssert(hdr);
    msgPush(msg, ethMsgStore, hdr, sizeof(ETHhdr), NULL);
    if ( (len = msgLen(msg)) > EMAXPAK ) {
	xTrace2(simethp, TR_SOFT_ERRORS,
		"sim ether driver: msgLen (%d) is larger than max (%d)",
		len, EMAXPAK);
	return XMSG_ERR_HANDLE;
    }
    /*
     * Place message contents in buffer
     */
    msgForEach(msg, msg2Buf, &bufPtr);

    if ( ETH_ADS_EQUAL(hdr->dst, ethBcastHost) ) {
	writeBcast(ps, buffer, len);
    } else {
	sendOnSocket(ps->sock, &hdr->dst, buffer, len);
    }
    return XMSG_NULL_HANDLE;
}    


static void
internalDemux( block *blockp )
{
    ETHhdr	hdr;
    Msg		msg;
    Msg		*oldmsg;
    
    xTrace0(simethp, TR_EVENTS, "in eth internal demux");
    oldmsg = blockp->msg;
    msgConstructCopy(&msg, oldmsg);
    msgTruncate(&msg, blockp->cur_len);
    msgAssign(oldmsg, &emptymsg);
    msgConstructAllocate(oldmsg, MAX_ETH_SIZE, &(blockp->data));
    
    if (! msgPop(&msg, ethMsgLoad, (void *)&hdr, sizeof(ETHhdr), NULL)) {
	xTrace0(simethp, TR_SOFT_ERRORS,
		"eth_demux: incoming message too small!");
	msgDestroy(&msg);
	return;
    }
    if ( ! blockp->self->up ) {
	xTrace0(simethp, TR_ERRORS, "eth_demux: no upper protocol!");
	msgDestroy(&msg);
	return;
    }
    msgSetAttr(&msg, 0, &hdr, sizeof(ETHhdr));
    xDemux(blockp->self, &msg);
    CLEAR_REF(blockp);
}


static void
block_handler(ev, arg)
    Event	ev;
    VOID 	*arg;
{
     block *bp = (block *)arg;
     
     for (;;) {
	 semWait(&bp->sem);		/* wait for incoming data */
	 xAssert(bp->cur_len > 0 && bp->cur_len <= MAX_ETH_SIZE);
	 xTrace1(simethp, TR_MORE_EVENTS, "shepherd thread for block %x runs",
		 (int)bp);
	 internalDemux(bp);
#ifdef SIMETH_STATS
	 if ( --activeThreads > activeHighWater ) {
	     activeHighWater = activeThreads;
	 }
#endif
     }
}


static void
init_eth_blocks()
{
    int 	i;
    block 	*bp;
    Msg 	*msg;
    static int	initialized = 0;
    
    if ( initialized ) {
	xTrace0(simethp, TR_EVENTS, "eth blocks already initialized");
	return;
    }
    initialized = 1;
    pool.total_blocks = MAX_ETH_BLOCKS;
    pool.next_block = 0;
    
    bp = pool.blocks;
    
    msgConstructEmpty(&emptymsg);
    
    for (i=0; i<MAX_ETH_BLOCKS; i++) {
	/*
	  if (!(bp->data = (char *) xMalloc(MAX_ETH_SIZE)))
	  Kabort("Malloc of ethernet packets");    
	  */
	msg = (Msg *)xMalloc(sizeof(Msg));
	msgConstructAllocate(msg, MAX_ETH_SIZE, &(bp->data));
	bp->msg = msg;
	bp->ref = NOREF;
	bp->id = i;
	bp->cur_len = 0;
	semInit(&bp->sem, 0);
	
	/* would be nice to get process id into struct */
	evDetach( evSchedule(block_handler, bp, 0));
	bp++;
    }
}


static block *
nextBlock()
{
    block 	*bp;
    int		i;

    i = pool.next_block;
    do {
	bp = &(pool.blocks[i]);
	i++;
	i %= pool.total_blocks;
    } while ( bp->ref == INUSE && i != pool.next_block );
    if ( bp->ref == INUSE ) {
	xTrace0(simethp, TR_SOFT_ERRORS, "simeth: all threads are busy");
	return NULL;
    }
    xTrace2(simethp, TR_MORE_EVENTS, "nextBlock returns block %x (# %d)",
	    bp, i);
    bp->ref = INUSE;
    pool.next_block = i;
    return bp;
}


static void
showBlocks()
{
    int	i;
    for ( i=0; i < pool.total_blocks; i++ ) {
	printf("%d: %s  ", i,
	       pool.blocks[i].ref == INUSE ? "R" : "I");
    }
    printf("\n");
    printf("next_block == %d\n", pool.next_block);
}


/* sparc version: Must pass each field of the msg structure by
   value.  Look at eth_demux to see where these are put back together.
*/

int interrupts = 0;
int checked = 0;

/* 
 * readether2demux -- interrupt handler
 *
 * Note that if only one packet is available, two blocks will be used
 * (the read into the second buffer will fail.)  With an even number
 * of blocks, this may result in every other block never transporting
 * a packet.  This is not a problem (i.e., they will be used if they
 * are needed.)
 */
static int
readether2demux( arg )
    VOID	*arg;
{
    XObj	self = (XObj)arg;
    int		buflen = 0;
    block 	*bp;
    PState	*ps = (PState *)self->state;
    
    xTrace0(simethp, TR_FUNCTIONAL_TRACE, "readether2demux");
    do {
	if ( ! (bp = nextBlock()) ) {
	    xError("sim_ether ERROR: Can't get next buffer, dropping incoming packet");
	    xIfTrace(simethp, TR_SOFT_ERRORS) {
		showBlocks();
	    }
	    /* 
	     * Drop this packet
	     */
	    if (++ps->errorCount > MAX_ERROR_COUNT) {
		xAssert(0);
	    } else {
		read_ether(ps->sock, dummyBuf, EMAXPAK);
		return -1;
	    }
	} else if ((buflen = read_ether(ps->sock, bp->data, EMAXPAK)) != -1) {
	    bp->cur_len = buflen;
	    bp->self = (XObj)arg;
#ifdef SIMETH_STATS
	 if ( ++activeThreads > activeHighWater ) {
	     activeHighWater = activeThreads;
	 }
#endif
	    semSignal(&bp->sem);
	} else CLEAR_REF(bp);
    } while (buflen != -1);
    
    return 0;
}


static bool
msg2Buf(char *msgPtr, long len, void *bufPtr)
{
  bcopy(msgPtr, *(char **)bufPtr, len);
  *(char **)bufPtr += len;
  return TRUE;
}


static xkern_return_t
simethOpenEnable( self, hlpRcv, hlpType, p )
    XObj	self, hlpRcv, hlpType;
    Part	*p;
{
    if ( self->up ) {
	xError("simethOpenEnable called multiple times!");
	return XK_FAILURE;
    }
    self->up = hlpRcv;
    return XK_SUCCESS;
}


static int
simethControl( s, op, buf, len )
    XObj	s;
    int 	op, len;
    char 	*buf;
{
    PState	*ps = (PState *)s->state;

    switch (op) {

      case ETH_REGISTER_ARP:
	/* 
	 * ARP registers itself with us so we can ask it to perform
	 * callbacks in order to simulate hardware broadcast.  
	 */
	checkLen(len, sizeof(XObj));
	ps->arp = *(XObj *)buf;
	return 0;

      case ETH_DUMP_STATS:
#ifdef SIMETH_STATS	
	{
	    xTrace0(simethp, TR_ALWAYS, "SIMETH statistics:");
	    xTrace3(simethp, TR_ALWAYS,
		    "\tthreads: %d\tactive: %d\thigh-water: %d",
		    pool.total_blocks, activeThreads, activeHighWater);
	}
#endif

      case GETMYHOST:
	checkLen(len, sizeof(ETHhost));
	bcopy((char *) &ps->myHost, buf, sizeof(ETHhost));
	return (sizeof(ETHhost));

      default:
	return -1;

    }
}
    
