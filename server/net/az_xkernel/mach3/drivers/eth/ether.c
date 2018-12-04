/*     
 * ether.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.36 $
 * $Date: 1993/02/02 00:01:33 $
 */

#include <sys/file.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <netinet/in.h>

#include <mach.h> 
#include <cthreads.h> 
#include <device/device.h>
#include <device/net_status.h> 
#include <device/device_request.h>
#include <mach_error.h>
#include <mach/mach_host.h>
#include <mach/mach_traps.h>

#include "assert.h"
#include "upi.h"
#include "xk_debug.h"
#include "xkernel.h"
#include "eth.h"
#include "eth_i.h"
#include "list.h"
#include "process_msg.h"
#include "xk_mach.h"

#ifdef __STDC__
xkern_return_t	ethdrv_init( XObj self );

char		*mach_error_string( mach_error_t );

#endif


#define ETHDRVMAPSIZE 11

struct net_status ether_net_status;
#define ETHER_MAX_PACKET_SIZE 1600

/*
 * Set default filter priority. >100 means noone else gets the packet.
 */
#define FILTER_PRIORITY_DEFAULT 20

#define XK_INTERFACE_PORT_QLIMIT	MACH_PORT_QLIMIT_MAX

/*
 * Defining TEST_TYPE_FILTER will enable code which filters packets on
 * type before creating processes.  The types in the filter can only
 * be configured once at boot time.
 */
/* #define TEST_TYPE_FILTER */

/* the device string is now part of the protocol name, e.g. ethdrv/SE0 */
#define DEVICE_STR "SE"
#define MAX_DEVICE_STRING 5

/*
 *  The state structure for the ethernet driver;
 *    each harward interface needs a separate state block
 *
 */
typedef struct estate {
  mach_port_t	interface_port;
  mach_port_t	filter_port;
#ifdef TEST_TYPE_FILTER
  Map           typeMap;
#endif
  int	        ethFilterPriority;
  ETHhost	bcast;
  ETHhost	myetheraddr;
  bool		initialized;
  char          devname[MAX_DEVICE_STRING];
} PState;

/* global data of ethernet protocol */

int	traceethdrvp; 	/* ethdrv == ethernet driver */

static  char	errBuf[100];
ETHhost	stdbcast = BCAST_ETH_AD;


#ifdef __STDC__

#  ifdef XK_DEBUG
static void 		dumpIncomingPacket( struct mach_hdrs *, int );
#  endif
static xmsg_handle_t	eth_push( XObj self, Msg *msg );
static int 		eth_control( XObj self, int op, char *buf, int len );

#else

#  ifdef XK_DEBUG
static void 		dumpIncomingPacket();
#  endif
static xmsg_handle_t	eth_push();
static int 		eth_control();

#endif




/*
 *  ether2demux --- the permanent reading thread
 *    this is not master_locked.
 */
static
int ether2demux(self)
     XObj self;
{
    kern_return_t 	kr;
    struct input_buffer *buf;
    struct mach_hdrs    *mh;
#ifdef XK_DEBUG
    int			packetCount = 0;
#endif    
    
    cthread_set_name(cthread_self(),"ether_thread");
    /* 
     * If the xkernel is limiting the number of cthreads, increase
     * this limit by 1 (for the kernel thread we will wire to this
     * input cthread.)
     */
    if ( cthread_kernel_limit() ) {
	cthread_set_kernel_limit( cthread_kernel_limit() + 1 );
	xTrace2(ethdrvp, TR_MAJOR_EVENTS,
		"%s increased xkernel cthread limit to %d",
		self->name, cthread_kernel_limit());
    } else {
	xTrace0(ethdrvp, TR_MAJOR_EVENTS,
		"ethdrv -- xkernel is not limiting number of cthreads");
    }
    cthread_wire();
    max_cthread_priority();

    while (1) {
      int received_size;

	pqRemove( xkFreeQueue, buf );
	buf->driverProtl = self;
	mh = (struct mach_hdrs *)buf->data;  /* hope this is aligned */
	mh->msg_hdr.msgh_local_port = ((PState *)self->state)->filter_port;
	mh->msg_hdr.msgh_size = ROUND4(sizeof(struct mach_hdrs)
				       + ROUND4(ether_net_status.max_packet_size));
	kr = mach_msg_receive((mach_msg_header_t *)mh);
	if (kr != MACH_MSG_SUCCESS) {
	    xTrace2(ethdrvp, TR_ERRORS,
		    "ether.ether2demux.mach_msg_receive() failed: %d (0x%x)\n",kr, kr);
	    pqAppend( xkFreeQueue, buf );
	    continue;
	}
#ifdef XK_DEBUG
	dumpIncomingPacket(mh, ++packetCount);
#endif	
#ifdef TEST_TYPE_FILTER
	/*
	 * Make sure the type is one that we accept
	 */
	{
	    ETHtype	type = ntohs(((ETHhdr *)mh->header)->type);

	    if ( mapResolve(((PState *)self->state)->typeMap, &type, 0) == XK_SUCCESS ) {
		xTrace1(ethdrvp, TR_EVENTS,
			"eth type %x blocked by type filter", type);
		pqAppend( xkFreeQueue, buf );
		continue;
	    } else {
		xTrace1(ethdrvp, TR_MORE_EVENTS,
			"eth type %x passes the type filter", type);
	    }
	}
#endif
	bcopy((char *)&mh->header, (char *)&buf->hdr, NET_HDW_HDR_MAX); /* set the eth hdr */
        received_size = mh->msg_hdr.msgh_size - sizeof(struct mach_hdrs);
	/*
	 * pop off the machipc headers; guarantee no underflow
	 */
	msgPopDiscard(&buf->upmsg, sizeof(struct mach_hdrs));
	msgTruncate(&buf->upmsg, received_size);
	msgSetAttr(&buf->upmsg, 0, &buf->hdr, sizeof(ETHhdr));
	
	/* 
	 * Add the buffer to the incoming packet queue.  It will be
	 * returned to the free queue by the shepherd thread.
	 */
	xTrace1(ethdrvp, TR_EVENTS, "enqueuing packet %d", packetCount);
	pqAppend( xkInQueue, buf );
    }
}


#ifdef XK_DEBUG

static void
dumpIncomingPacket( mh, packetCount )
    struct mach_hdrs	*mh;
    int			packetCount;
{
    xTrace5(ethdrvp,TR_FUNCTIONAL_TRACE,
	    "ether in pkt(%d): msg bits=%d size=%d kind=%d id=%d",
	    packetCount,
	    mh->msg_hdr.msgh_bits,
	    mh->msg_hdr.msgh_size,
	    mh->msg_hdr.msgh_kind,
	    mh->msg_hdr.msgh_id);
    xTrace3(ethdrvp,TR_FUNCTIONAL_TRACE,
	    "ether in pkt(%d): pkt length=%d type=%d",
	    packetCount,
	    mh->packet_header.length,
	    mh->packet_header.type);
    xTrace4(ethdrvp,TR_FUNCTIONAL_TRACE,
	    "ether in pkt(%d): dest 0x%04x 0x%04x 0x%04x",
	    packetCount,
	    ((ETHhdr *)mh->header)->dst.high,
	    ((ETHhdr *)mh->header)->dst.mid,
	    ((ETHhdr *)mh->header)->dst.low);
    xTrace4(ethdrvp,TR_FUNCTIONAL_TRACE,
	    "ether in pkt(%d): src 0x%04x 0x%04x 0x%04x",
	    packetCount,
	    ((ETHhdr *)mh->header)->src.high,
	    ((ETHhdr *)mh->header)->src.mid,
	    ((ETHhdr *)mh->header)->src.low);
    xTrace2(ethdrvp,TR_FUNCTIONAL_TRACE,
	    "ether in pkt(%d): type 0x%04x",
	    packetCount,
	    ((ETHhdr *)mh->header)->type);
    xTrace4(ethdrvp,TR_FULL_TRACE,
	    "ether in pkt(%d): hdr 0x%02x 0x%02x 0x%02x",
	    packetCount,
	    (u_char)mh->header[0], (u_char)mh->header[1], (u_char)mh->header[2]);
    xTrace4(ethdrvp,TR_FULL_TRACE,
	    "ether in pkt(%d): hdr 0x%02x 0x%02x 0x%02x",
	    packetCount,
	    (u_char)mh->header[3], (u_char)mh->header[4], (u_char)mh->header[5]);
    xTrace4(ethdrvp,TR_FULL_TRACE,
	    "ether in pkt(%d): hdr 0x%02x 0x%02x 0x%02x",
	    packetCount,
	    (u_char)mh->header[6], (u_char)mh->header[7], (u_char)mh->header[8]);
    xTrace4(ethdrvp,TR_FULL_TRACE,
	    "ether in pkt(%d): hdr 0x%02x 0x%02x 0x%02x",
	    packetCount,
	    (u_char)mh->header[9], (u_char)mh->header[10], (u_char)mh->header[11]);
    xTrace3(ethdrvp,TR_FULL_TRACE,
	    "ether in pkt(%d): hdr 0x%02x 0x%02x",
	    packetCount,
	    (u_char)mh->header[12], (u_char)mh->header[13]);
}

#endif  XK_DEBUG


#ifdef TEST_TYPE_FILTER

/*
 * readHex -- read a number from 's' which may be either hex (prefixed
 * with an x as in x34b2) or decimal.
 *
 * non-zero on success, zero on failure
 */
static int
readHex( s, n )
    char *s;
    int *n;
{
    if ( *s == 'x' ) {
	return ( sscanf(s + 1, "%x", n) == 1 );
    } else {
	return ( sscanf(s, "%d", n) == 1 );
    }
}

#endif


static void
processRomFile( XObj self )
{
    int 	i;
    PState	*ps = (PState *)self->state;
    char	fullName[80];

    xTrace0(ethdrvp, TR_FUNCTIONAL_TRACE, "ether: processRomFile");
    sprintf(fullName, "%s%c%s", self->name, *self->instName ? '/' : 0,
	    self->instName);
    for ( i=0; rom[i][0]; i++ ) {
	if ( ! strcmp(rom[i][0], fullName) ) {
	    if ( rom[i][1] ) {
		if ( ! strcmp(rom[i][1], "priority") ) {
		    if ( rom[i][2] &&
			 sscanf(rom[i][2], "%d", &ps->ethFilterPriority) == 1 ) {
			continue;
		    }

		} else if ( ! strcmp(rom[i][1], "block") ) {
#ifdef TEST_TYPE_FILTER
		    int		n;
		    ETHtype	type;

		    if ( rom[i][2] && readHex(rom[i][2], &n) ) {
			type = n;
			mapBind(ps->typeMap, (char *)&type, 0);
			xTrace1(ethdrvp, TR_GROSS_EVENTS,
				"eth driver blocking type %x",
				type);
			continue;
		    }
#else
		    xTrace1(ethdrvp, TR_GROSS_EVENTS,
			    "eth driver ignoring type filter on line %d",
			    i+1);
		    continue;
#endif
		}
		sprintf(errBuf, "ETH format error in line %d of rom file",
			i + 1);
		xError(errBuf);
	    }
	}
    }
}


/*
 *    Support for generic xkernel operations
 *     The openenable interface allows the driver 
 *     to get the address of the higher level protocol to which it interfaces
 */
static xkern_return_t
eth_openenable(self, hlp, hlptype, part)
     XObj self, hlp, hlptype;
     Part part;
{
  cthread_t th;
  
  /* this xSetUp allows the self protocol object to be used as a session
     in a later xDemux call */
  xSetUp(self, hlp);

/* Since ether2demux does not use any xkernel routines, this should be 
   allowed to remain a cthread, in spite of the sledgehammer concurrency
   control otherwise enforced in the xkernel.

   The ether reading thread runs forever
 */
  th = cthread_fork((any_t)ether2demux, (void *)self);
  cthread_detach(th);

  return XK_SUCCESS;
}



#define checkResult(_r, _msg)						\
    if ( kr != (_r) ) {							\
	sprintf(errBuf, "%s fatal initialization error: %s (%s)",	\
		self->name, (_msg), mach_error_string(_r));		\
	xError(errBuf);							\
	if ( state->interface_port ) {					\
	    device_close(state->interface_port);			\
	}								\
	return XK_FAILURE;						\
    }			


/*
 *  etherInit
 *
 *  graph.comp: protocol name/device-unit name
 *
 */
static xkern_return_t
etherInit(self)
     XObj self;
{
    kern_return_t kr;
    unsigned int if_stat_count;
    long	if_addr[4];
    unsigned int if_addr_count;
    filter_t filter[40];
    int filter_index = 0;
    mach_port_t	masterPort;
    char *p;
    PState *state = (PState *)xMalloc(sizeof(PState));
    
    xTrace0(ethdrvp, TR_GROSS_EVENTS, "etherInit");
    masterPort = mach_master_device_port();
    self->state = state;
    state->ethFilterPriority = FILTER_PRIORITY_DEFAULT;
    state->bcast = stdbcast;
    state->initialized = FALSE;
    
    /* set default device name "SE0" */
    strcpy(state->devname, DEVICE_STR);
    state->devname[strlen(DEVICE_STR)] = '0';
    state->devname[strlen(DEVICE_STR)+1] = '\0';
    if (p = self->instName) strncpy(state->devname, p, MAX_DEVICE_STRING);
#ifdef TEST_TYPE_FILTER
    state->typeMap = mapCreate(ETHDRVMAPSIZE, sizeof(ETHtype));
#endif
    if ( state->devname[strlen(state->devname)-1] != '0') {
	printf("will not attempt to open device %s because of a bug in the driver\n", state->devname);
	return XK_FAILURE;
    }
    processRomFile(self);
    xTrace1(ethdrvp, TR_DETAILED, "etherInit: master port == %x", masterPort);
    xTrace1(ethdrvp, TR_DETAILED, "opening device %s", state->devname);
    kr = device_open(masterPort, 0, state->devname, &state->interface_port); 
    checkResult(D_SUCCESS, "device_open fails");
    xTrace0(ethdrvp, TR_MAJOR_EVENTS, "ether.device_open() completed");
    
    if_stat_count = NET_STATUS_COUNT;
    kr = device_get_status(state->interface_port,NET_STATUS,
			   (dev_status_t)&ether_net_status, &if_stat_count);
    checkResult(D_SUCCESS, "device_get_status (NET_STATUS) failed");
    xTrace0(ethdrvp,TR_MAJOR_EVENTS,"ether.device_get_status() returned D_SUCCESS");
    
    if (ether_net_status.header_format != HDR_ETHERNET) {
	printf("ether: unsupported device header format: %d\n",
	       ether_net_status.header_format);
	return XK_FAILURE;
    }
    if (ether_net_status.max_packet_size > ETHER_MAX_PACKET_SIZE) {
	printf(
	       "ether: invalid device max packet size: %d (max %d)\n",
	       ether_net_status.max_packet_size,ETHER_MAX_PACKET_SIZE);
	return XK_FAILURE;
    }
    if (ether_net_status.header_size != sizeof(ETHhdr)) {
	printf("ether: invalid device header size: %d\n",
	       ether_net_status.header_size);
	return XK_FAILURE;
    }
    if (ether_net_status.address_size != sizeof(ETHhost)) {
	printf("ether: invalid device address size: %d\n",
	       ether_net_status.address_size);
	return XK_FAILURE;
    }
    if (ether_net_status.mapped_size) {
	printf("error: Ethernet mapped\n");
	return XK_FAILURE;
    }
    
    if_addr_count = sizeof(if_addr);
    kr = device_get_status(state->interface_port,NET_ADDRESS,
			   (dev_status_t)if_addr,&if_addr_count);
    checkResult(D_SUCCESS, "get_status(NET_ADDRESS) failed");
    
    if_addr[0] = ntohl(if_addr[0]);
    if_addr[1] = ntohl(if_addr[1]);
    xTrace1(ethdrvp,TR_GROSS_EVENTS,"ether: interface address (1) %s",
	    ethHostStr((ETHhost *)if_addr));
    bcopy((char *)if_addr, (char *)&(state->myetheraddr), sizeof(ETHhost));
    xTrace1(ethdrvp,TR_GROSS_EVENTS,"ether: interface address %s", ethHostStr(&state->myetheraddr));
    xTrace3(ethdrvp,TR_GROSS_EVENTS,"ether: interface address 0x%04x 0x%04x 0x%04x",
	    state->myetheraddr.high,state->myetheraddr.mid,state->myetheraddr.low);
    
    kr = mach_port_allocate(mach_task_self(),
			    MACH_PORT_RIGHT_RECEIVE, &state->filter_port);
    checkResult(KERN_SUCCESS, "cannot allocate filter port");
    xTrace0(ethdrvp,TR_MAJOR_EVENTS,"ether.mach_port_allocate() completed");
    
    kr = mach_port_set_qlimit(mach_task_self(), state->filter_port,
				MACH_PORT_QLIMIT_MAX);
    checkResult(KERN_SUCCESS, "cannot adjust filter limit");
    xTrace0(ethdrvp,TR_MAJOR_EVENTS,"ether.mach_port_set_qlimit() completed");
    
    
    xTrace1(ethdrvp, TR_GROSS_EVENTS, "Ethernet driver using packet filter priority %d",
	    state->ethFilterPriority);
    filter[filter_index++] = NETF_PUSHLIT;
    filter[filter_index++] = (filter_t) TRUE;
    kr = device_set_filter(state->interface_port,
			   state->filter_port,
			   MACH_MSG_TYPE_MAKE_SEND,
			   state->ethFilterPriority,
			   filter,
			   filter_index);
    checkResult(KERN_SUCCESS, "device_set_filter failed");
    
    self->openenable = eth_openenable;
    self->push = eth_push;
    self->control = eth_control;
    
    xTrace0(ethdrvp,TR_MAJOR_EVENTS,"ether.device_set_filter completed");
    return XK_SUCCESS;
}


/*
 * ether_init
 * 
 *   The protocol initialization entry point
 * 
 */
xkern_return_t
ethdrv_init( XObj self)
{
    xTrace0(ethdrvp, TR_GROSS_EVENTS, "ethCtlrInit");    
    if ( !self->state || ! ((PState *)self->state)->initialized )  {
	if (etherInit(self) == XK_SUCCESS) {
	  ((PState *)self->state)->initialized = TRUE;
	  return XK_SUCCESS;
	}
	else return XK_FAILURE;
      }
    xTrace0(ethdrvp, TR_ERRORS, "ether driver already initialized");
    return XK_FAILURE;
}


/*
 *  eth_control
 *
 *   Control operations
 *
 */
int
eth_control(self, op, buf, len)
     XObj  self;
     int   op;
     char *buf;
     int   len;
{
    
    xTrace1(ethdrvp, TR_FULL_TRACE, "eth_control: operation %x", op);
    if (op == GETMYHOST) {
	checkLen(len, sizeof(ETHhost));
	bcopy((char *)(&((PState *)self->state)->myetheraddr), buf, sizeof(ETHhost));
	return sizeof(ETHhost);
    }
    return -1;
}



static bool
msg2Buf(char *msgPtr, long len, void *bufPtr)
{
  bcopy(msgPtr, *(char **)bufPtr, len);
  *(char **)bufPtr += len;
  return TRUE;
}


/* 
 *  eth_push
 *
 *     take a message from the generic layer and push it out to the net
 */
static xmsg_handle_t
eth_push(s, msg)
     XObj s;
     Msg *msg;
{
    ETHhdr  		*hdr = (ETHhdr *)msgGetAttr(msg, 0);
    char		buffer[ETHER_MAX_PACKET_SIZE];
    char		*bufPtr;
    int			len;
    kern_return_t 	kr;


    xTrace1(ethdrvp, TR_EVENTS, "ethCtlrXmit destination %s", ethHostStr(&hdr->dst));
    xAssert(hdr);
    xTrace3(ethdrvp, TR_MORE_EVENTS, "outgoing header:  src: %s  dst %s  type %x",
	    ethHostStr(&hdr->src), ethHostStr(&hdr->dst), hdr->type);
    bcopy((char *)hdr, buffer, sizeof(ETHhdr));
    len = msgLen(msg) + sizeof(ETHhdr);
    if ( len > ETHER_MAX_PACKET_SIZE ) {
	return -1;
    }
    xTrace1(ethdrvp, TR_EVENTS, "write_ether sending %d bytes", len);
    /*
     * Place message contents in buffer
     */
    bufPtr = buffer + sizeof(ETHhdr);
    msgForEach(msg, msg2Buf, &bufPtr);
    kr = device_write_request(((PState *)s->state)->interface_port,
			      MACH_PORT_NULL, 0, 0, buffer, len);
    if (kr != KERN_SUCCESS) {
	sprintf(errBuf, "ether.device_write_request() failed: %d\n", kr);
	xError(errBuf);
    }
    return(XMSG_NULL_HANDLE);
}


