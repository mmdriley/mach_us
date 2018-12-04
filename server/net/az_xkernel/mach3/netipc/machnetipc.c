/*
 *
 * machnetipc.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/03 20:20:31 $
 */

/*
 *
 * MACHNETIPC - an embedding of Mach3 IPC in a network
 *
 *
 *   See CAVEATS, below
 *
 *
 *  machripc has three functions: to convert messages to and from network form,
 *    to manage port transfers embedded in messages,
 *    and to interface upwards to the simple server protocol that
 *    bootstraps port transfers between machines
 *  the direct data transfer is the function of the protocol, but
 *  the port transfers are side-effects that alter the state of the
 *  machripc protocol and the port set of the Mach OS.
 *
 *  The interface to the SSR protocol is modelled as transfering a
 *  composite object: the message (just a service number) and a port (to be
 *  used for the reply).
 *
 */

/* 
    performs three functions:

    for ports with send rights, this must devote a thread to listening.
    
    for data to be sent to net, convert to network msg form and send
    to lower protocol
    	note transfers of receive rights to remote machine ...
	  notify external senders of the change
	convert message to network form and push to next protocol

    for data coming in from net
    	reconstitute ports
		establish listening threads
	demux on ssr vs. mach vs. port mgmt types
	send message to destination Mach port or xPop to ssr protocol
*/

/*
 *   CAVEATS: the network port identifiers are fragile for the time
 *    being.  They are based on the lower 24 bits of the IP address
 *    and an 8 bit sequence number.  This is inadequate and will be
 *    changed in the near future.
 *
 *   Behavior under heavy load or with machine reboots during complex
 *   port transfers have not been tested.
 *
 *   Embedded port right transfers have been tested only lightly.
 *
 *   Support for heterogeneous architectures is not included.  Some
 *   of the header load and store routines attempt to be architecture
 *   independent, but this work is still in progress.
 *
 */

#include <stdio.h>
#include <mach.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <cthreads.h>
#include "xkernel.h"
#include "ip.h"
#ifdef USING_UDP
#include "udp.h"
#endif USING_UDP
#include "rwlock.h"
#include "machripc_internal.h"

#define XK_MACH_MSG_TIMEOUT 10000  /* 10 seconds for an RPC reply */

#define pass_open_transport(part) (xOpenEnable(myProtl, myProtl, xGetDown(myProtl, MNI_TRANSPORT_DNUM), part))
#define pass_open_rpc_transport(part) (xOpenEnable(myProtl, myProtl, xGetDown(myProtl, MNI_RPC_TRANSPORT_DNUM), part))
#define active_open_transport(part) (xOpen(myProtl, myProtl, xGetDown(myProtl, MNI_TRANSPORT_DNUM), part))
#define active_open_rpc_transport(part) (xOpen(myProtl, myProtl, xGetDown(myProtl, MNI_RPC_TRANSPORT_DNUM), part))

#define LOCAL_PART  1
#define REMOTE_PART 0

#define ipaddr2int(foo) ((foo.a)<<24 + (foo.b)<<16 + (foo.c)<<8 + (foo.d))
#define int2iphost(foo, des)  { des.a = (foo>>24) & 0xFF; \
			        des.b = (foo>>16) & 0xFF; \
			        des.c = (foo>>8) & 0xFF;  \
			        des.d = foo & 0xFF; }

#define ONESEND(r) (r & (MACH_PORT_TYPE_SEND | MACH_PORT_TYPE_SEND_ONCE))
#define IP_NULL { 0, 0, 0, 0 };

int tracemachripcp;

static IPhost ip_null = IP_NULL;

static XObj myProtl;

static void getproc(XObj);

static XTime starttime;

#define IPEQUAL(_a, _b) ((_a.a == _b.a) && (_a.b == _b.b) && \
			      (_a.c == _b.c) && (_a.d == _b.d))

#define STATEPTR ((struct machr_state *)(myProtl->state))
#define STATESELFPTR ((struct machr_state *)(self->state))
#define SESSNSTATEPTR ((Mach_ActiveId *)(self->state))
#define SESSNSTATEVARPTR(session) ((Mach_ActiveId *)(session->state))

#define TYPE_MALLOC(type) (type *)xMalloc(sizeof(type))

typedef	long (*Pfl) ();

/* 
 *  Null_Netport  definition shared with port_maint
 *
*/
mnetport Null_Netport;  /* error return value */

/*
 *  Routines for copying data; ignoring alignment and representation
 *  problems for the moment.
 *
 */

static long
mycopypop(char *to, char *from, long len, void *arg) {
  if ((int)len == len) {
    xTrace3(machripcp, TR_FULL_TRACE, "copy pop %x to %x len %d", from, to, len);
    bcopy(from, to, len);
    return(len);
  }
  else xTrace1(machripcp, TR_ERRORS, "machr: copypop: Untenable len for copy function %ld", len);
  return((long)0);
}


/*
 *  machnetipc_msg_pop
 *
 *    gets the architecture and message type
 */
static long
machnetipc_msg_pop(char *to, char *from, long len, void *arg)
{
  mn_arch_tag_t arch;
  struct mnipchdr *hdr = (struct mnipchdr *)to;

  if (len < MN_ARCH_TAG_NETLEN + MACHIPCTYPE_NETLEN) {
    xTrace1(machripcp, TR_SOFT_ERROR, "machnetipc: machnetipc_msg_pop: message too short %d", len);
    return 0;
  }
  bcopy(from, (char *)&hdr->architecture_type, MN_ARCH_TAG_NETLEN);
  arch = hdr->architecture_type;
  if (arch == MN_ARCH_MARKER)
    bcopy(from+MN_ARCH_TAG_NETLEN, (char *)&hdr->machnetipcmsg_type, MACHIPCTYPE_NETLEN);
  else {
    int i = arch_unpermute_index(arch);
    hdr->architecture_type = unpermute_int32[i](from+MN_ARCH_TAG_NETLEN);
  }
  return len;
}

/*
 *  machnetipc_msg_push
 *
 *    puts the architecture and message type
 */
static void
machnetipc_msg_push(char *from, char *to, long len, void *arg)
{
  enum MACHIPCTYPE msgtype = (int)from;
  mn_arch_tag_t arch = MN_ARCH_MARKER;

  bcopy((char *)&arch, to, MN_ARCH_TAG_NETLEN);
  bcopy((char *)&msgtype, to+MN_ARCH_TAG_NETLEN, MACHIPCTYPE_NETLEN);
}

/* ssrpop
              get parameters from the ssr header to direct the
	      port transfer operations
*/
static long
ssrpop(char *to, char *from, long len, void *arg) {
  struct machripc_msg	*tobuf = (struct machripc_msg *)to;

  if ((int)len == len) {
    xTrace3(machripcp, TR_FULL_TRACE, "machnetipc: ssrpop %x to %x len %d", from, tobuf, len);
    bcopy(from, to, len);
    return len;
  }
  else xTrace1(machripcp, TR_ERRORS, "machnetipc: ssrpop: Untenable len for copy function %ld", len);
  return (long)0;
}

/* ssrpush
              get parameters into the ssr header to direct the
	      port transfer operations
*/
static void
ssrpush(char *from, char *to, long len, void *arg) {
  struct machripc_msg	*frombuf = (struct machripc_msg *)from;

  if ((int)len == len) {
    xTrace3(machripcp, TR_FULL_TRACE, "machnetipc: ssrpush %x to %x len %d", from, frombuf, len);
    bcopy(from, to, len);
  }
  else xTrace1(machripcp, TR_ERRORS, "machnetipc: ssrpush: Untenable len for copy function %ld", len);
}

/*
 *   mycopypop_andlie   ,   a simple copy function
 */
static long
mycopypop_andlie(char *to, char *from, long len, void *arg) {
  if ((int)len == len) {
    xTrace3(machripcp, TR_FULL_TRACE, "copy pop and lie %x to %x len %d", from, to, len);
    bcopy(from, to, len);
    return(0);
  }
  else xTrace1(machripcp, TR_ERRORS, "Untenable len for copy function %ld", len);
  return((long)0);
}

static long
mycopypush(char *from, char *to, long len, void *arg) {
  if ((int)len == len) {
    xTrace3(machripcp, TR_FULL_TRACE, "copy push %x to %x len %d", from, to, len);
    bcopy(from, to, len);
    return(len);
  }
  else xTrace1(machripcp, TR_ERRORS, "Untenable len for copy function %ld", len);
  return((long)0);
}


/* allows use of an xkernel msg buffer as an ordinary buffer;
   saves the beginning of data ptr */
static long
mypushlie(char *from, char *to, long len, void *arg) {

  arg = (void *)from;
  return(len);

}

#define mach_msg_receiveSafeSize( msg ) mach_msg( \
			 (mach_msg_header_t *)msg, \
			 MACH_RCV_MSG, \
			 MACH_RCV_LARGE, \
			 ((mach_msg_header_t *)msg)->msgh_size, \
			 ((mach_msg_header_t *)msg)->msgh_local_port,\
			 MACH_MSG_TIMEOUT_NONE, \
			 MACH_PORT_NULL)

#define mach_msg_receiveSafeSizeTime( msg ) mach_msg( \
			 (mach_msg_header_t *)msg, \
			 MACH_RCV_MSG, \
			 MACH_RCV_LARGE, \
			 ((mach_msg_header_t *)msg)->msgh_size, \
			 ((mach_msg_header_t *)msg)->msgh_local_port,\
			 XK_MACH_MSG_TIMEOUT, \
			 MACH_PORT_NULL)

Map xkMsgIdMap;
		      /* forward declarations */
#ifdef __STDC__
static XObj
machr_open(XObj self, XObj hlp, XObj hlptype, Part *P);

static void machr_session_init(XObj p, Mach_ActiveId *state);

/* external  */
mnetport convert_to_netport(mach_port_right_t port, mach_port_type_t right,
			    IPhost dest, msg_id_t msgid, mnportid_t pnn);

/* external  */
mach_port_t
convert_netport_to_mach_port(mn_netport_t *netport, XObj self, XObj lower_session, Msg *msg, msg_id_t msgid, IPhost sender);

/* external */
xkern_return_t
portm_port_remove_send_once(mnetport *port_desc);

void
port_mgmt(mn_arch_tag_t, Msg *);

static mach_msg_header_t *
convert_netmach_msg_to_actual_mach(Msg *netmsg,
				   mach_msg_header_t *outmsg,
				   int port_count,
				   int notinline_count,
				   int xfercompletion,
				   mn_arch_tag_t arch,
				   msg_id_t msgid,
				   IPhost sender,
				   mach_port_right_t lright,
				   bool rpc_p );

static void
convert_mach_msg_to_netmach_msg(struct mach_msg_big_t *netmsg, 
				Msg *xkmsg,
				machnetipc_hdr *outmsg,
				mnetport *remote_port,
				IPhost dest, enum MACHIPCTYPE msgtype);

void
machr_msg_receive(struct send_request *args);

static void
machr_notification_receive( );

void
machnetipc_start_msg_receive( struct send_request *args );

static void
machr_req_dn_notif(mach_port_t port);

/* external  */
mnetport *
quick_port_convert(mach_port_t *local_port);

static msg_id_t msgid_assign();

/* external  */
xkern_return_t
port_maint_init(XObj self);

static xkern_return_t
rpc_session_control(mach_port_t receive_port, mach_port_t reply_port,
		    XObj lower_session);

/* external  */
xkern_return_t portm_register(IPhost addr);

/* external  */
xkern_return_t portm_register_via_session(XObj session);

/* external */
mach_msg_header_t *
xk_ports_and_ool_convert(Msg			*netmsg,
			 mach_msg_header_t	*outmsg,
			 int			mach_netport_count,
			 int			mach_notline_count,
			 msg_id_t		msgid,
			 IPhost			srchost,
			 int			xfercompletion);

mach_msg_header_t *
xk_netmach_msg_to_mach(Msg			*netmsg,
		       mach_msg_header_t	*outmsg,
		       mach_port_right_t	local_right,
		       int			mach_netport_count,
		       int			mach_notline_count,
		       mn_arch_tag_t		arch_type);
void
xk_mach_msg_convert (
  struct mach_msg_big_t  *msg,
  Msg			 *fulloutmsg,
  machnetipc_hdr	 *nethdr,
  IPhost		  dest);

void portm_delete_local_receive( mach_port_t port );

void portm_delete_local_send( mach_port_t port );

static void machr_pop( XObj, Msg *);

void portm_generate_nms(mnetport *);

mnportid_t portm_get_netnum();

void portm_use_fast_send_once(mach_port_t, bool);

void
portm_add_sender(mnetport *, IPhost);

#else

static XObj
machr_open();

static void machr_session_init();

static msg_id_t msgid_assign();

/* external  */
mnetport convert_to_netport();

/* external  */
mach_port_t
convert_netport_to_mach_port();

/* external */
xkern_return_t
portm_port_remove_send_once();

void port_mgmt();

static mach_msg_header_t *
convert_netmach_msg_to_actual_mach();

static void
convert_mach_msg_to_netmach_msg();

void
machr_msg_receive();

static void
machr_notification_receive();

void
machnetipc_start_msg_receive( );

static void
machr_req_dn_notif( );
/* external  */
mnetport *
quick_port_convert();

/* external  */
xkern_return_t
port_maint_init();

static xkern_return_t
rpc_session_control();

/* external  */
xkern_return_t portm_register();

/* external  */
xkern_return_t portm_register_via_session();

/* external */
mach_msg_header_t *
xk_ports_and_ool_convert();

mach_msg_header_t *
xk_netmach_msg_to_mach();

void xk_mach_msg_convert ();

void portm_delete_local_receive( );

void portm_delete_local_send( );

void portm_generate_nms( );

static void machr_pop( );

mnportid_t portm_get_netnum();

void portm_use_fast_send_once();

void portm_add_sender();
#endif  __STDC__

#ifdef MNEVENTDB
extern traceevent;
int savedevent;
#endif


#ifdef MSGTEST
void myfree(char *ptr)
{
  printf("*** free %x\n", ptr);
}


bool myforeach(char *ptr, long len, void *arg)
{
  printf("*** foreach %x\n", ptr);
  return TRUE;
}

void
test_new_msg()
{
  Msg msg;
  char buffer[20];
  int i;

  for (i=0; i<20; i++) buffer[i] = 'a';
  printf("construct using %x\n", buffer);
  msgConstructInplace(&msg, buffer, 10, myfree);
  msgPush(&msg, (Pfv)mycopypush, (void *)buffer, 20, (void *)0);
  msgForEach(&msg, myforeach, (void *)0);
  printf ("test done\n");
}
#endif MSGTEST

#ifdef PORTLOCKS
/*
 *  unlocker
 *
 *       pop a port descriptor address and unlock the port
 */
static void
unlocker (char *to, char *from, long len, void *arg)
{
  mnetport *lkport;
  mnportid_t pnum;
  int pbytes = (int)arg;

  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: unlocker called");

  while (pbytes>0) {
    bcopy(from, (char *)&pnum, sizeof (mnportid_t));
    xTrace1(machripcp, TR_EVENTS, "machnetipc: unlocker called with port %x",
	    lkport);
    if (quick_netport_lookup(pnum, &lkport) != XK_FAILURE)
      readerUnlock(lkport->rwlock_ptr);
    from += sizeof(mnportid_t *);
  }
  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: unlocker exits");
}
#endif

/*
 * xAsyncThread
 *
 * platform dependent asynchronous thread startup
 * the thread cannot perform any xkernel operations until
 * it sets the xk_master_lock
 *
 */
static void
xAsyncThread(Pfv func, void *args, char *name)
{
  cthread_t child;

  child = cthread_fork((cthread_fn_t)func, args);
  cthread_set_name(child, name);
  cthread_detach(child);
  xTrace1(processcreation, TR_ERRORS, "machripc created client thread_id: %d",
	  child);
}

/* 
 * machr_init()
 *
 * establish communication with CHAN, or the first lower transport
 *    protocol.
 *
 */
xkern_return_t
machripc_init(self) XObj self;
{
  IPhost ipaddr, nametoip();
  struct hostent *hostEnt;
  Part whom[1];
  XObj ls;
  kern_return_t ret;
  
  xTrace0(machripcp, TR_FULL_TRACE, "machr init");
  xTrace0(machripcp, TR_MAJOR_EVENTS, "machr init");
  myProtl = self;
  getproc(myProtl);
  self->state = TYPE_MALLOC(struct machr_state);
  /* should do this for each lower protocol? */
  xControl(xGetDown(myProtl, MNI_TRANSPORT_DNUM), GETMYHOST, (void *)&ipaddr, sizeof(IPhost) );

  ((struct machr_state *)self->state)->local_source_addr = ipaddr;
  partInit(whom, 1);
#ifdef USING_UDP
  partPush(whom[REMOTE_PART], &machripc_udpport);
#endif /* USING_UDP */
  /* partPush(whom[REMOTE_PART], &(STATESELFPTR->local_source_addr)); */
  /* open_enable to udp; depends on global myProtl */
  /* shouldn't really be done until after all protocols are initialized */
  if (pass_open_transport(whom) == XK_FAILURE) {
    xTrace0(machripcp, TR_ERRORS, "machr server can't openenable lower protocol");
    _exit(0);  /* for intial debugging only */
  }

  /* make sure ssr gets started OK; a kludge that could be removed */
  semInit(&(STATESELFPTR->ssr_wait), 0);

  bzero((char *)&Null_Netport, sizeof(Null_Netport));

  /* get a port to use for notifications */
  if ((ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&(STATESELFPTR->notification_port)))
      != KERN_SUCCESS)
    {
      xTrace0(machripcp, TR_ERRORS, "Machripc: Could not initialize notification port; bailing out");
      return(XK_FAILURE);
    }
  else {
    xAsyncThread(machr_notification_receive, 0, "xk_notification_thread");
  }

  /* used to note which msgs need to have transfer bits set in hdr */
  xkMsgIdMap = mapCreate(MACHIPC_MSG_MAP, sizeof(MsgId));

  portmaint_init(self);
#ifdef MSGTEST
  test_new_msg();
#endif MSGTEST
  xTrace0(machripcp, TR_MAJOR_EVENTS, "machr init done");
  return(XK_SUCCESS);
}

/*
 * machr_new_sessn
 *
 * create a session structure so that we can xDemux the message
 */
static XObj
machr_new_sessn(XObj self, XObj hlp)
 {
   xTrace0(machripcp, TR_FULL_TRACE, "machr_new_sessn");

   /* need only one session for SSR */
   return(xCreateSessn(machr_session_init, hlp, hlp, self, 0, (void *)0));
 }

/*
 * calldemux_msg
 * 
 * This data structure is a hack to pass an argument from calldemux
 *  to demux, to machnetipc_netrcv
 * 
 */
static Msg *calldemux_msg;

/*
 *   machnetipc_netrcv()
 *
 *   A mach message type has been received from the net.
 *
 *   If it is non-rpc, we convert the ports and data for forwarding.
 *   If it is a rpc_request, we convert and forward, but the listening
 *       thread on the reply port will be used to signal a quick return
 *       from the calldemux, using the reply message.
 *   If it is an rpc_reply, we are returning to an xCall, and this
 *       is the reply message.  We know the reply port already, so
 *       all we need to do is convert and forward.  Deallocates the
 *       send_once right netport structure for the reply.
 *
 *   Will wait for queue to unblock, if necessary.
 *   On incoming, will need to check msgid to see if it completes
 *   any port transfers that are in the queue forwarding state.
 *
 *   For rpc requests, the reply port will be a fast send-once
 *   right.  The caller must do all fast send-once right manipulations;
 *   this routine will stay out of the way.
 *
 */
static xkern_return_t
machnetipc_netrcv(msg, netport, arch_type,
		  msgtype, isrpc_request, rpc_reply_port, mach_msg,
		  self, transport_s)
	Msg      		*msg;
	mnetport		*netport;
        enum SOURCE_BYTE_ARCH	arch_type;
        enum MACHIPCTYPE	msgtype;
	bool      		isrpc_request;
        mnportid_t		*rpc_reply_port;
        mach_msg_header_t	**mach_msg;
        XObj      		self, transport_s;
   {
      mach_port_t	 receive_port;
      mach_port_type_t	 local_right, remote_right;
      struct machnetipc  mach_net_msg;
      mach_msg_header_t  mach_header;
      kern_return_t      ret;
      Msg               *reply_msg = 0;
      int                net_port_number = 0;
      msg_id_t		 msgid;
      IPhost		 sender;
      mach_port_type_t	 local_port_type;
      bool		 isrpc_reply = msgtype==MACHRIPC_RPC_REPLY_MSG?TRUE:FALSE;

      xTrace2(machripcp, TR_FULL_TRACE, "machnetipc: netrcv called: port addr %x num %x",
	      netport,
	      netport?netport->net_port_number:0);

      if (transport_s != ERR_XOBJ)
	xControl(transport_s, GETPEERHOST, (char *)&sender, sizeof(IPhost));
      else {
	xTrace0(machripcp, TR_ERRORS, "machnetipc: netrcv: bad session");
	return XK_FAILURE;
      }

      if (msgLen(msg) < sizeof(struct machnetipc)) {
	xTrace0(machripcp, TR_DETAILED, "machnetipc: netrcv: null msg");
	return(XK_FAILURE);
      }

      /* get the ports, the message description */
      msgPop(msg, (Pfl)mycopypop, &mach_net_msg,
	     sizeof(struct machnetipc),
	     (void *)0);
      msgid = mach_net_msg.message_id;

      xTrace3(machripcp, TR_FUNCTIONAL_TRACE, "machnetipc: netrcv: msgtype %d msgarch %x id %d",
	      msgtype, arch_type, msgid);

      if (isrpc_reply && msgtype != MACHRIPC_RPC_REPLY_MSG) {
	xTrace2(machripcp, TR_ERRORS,
		"machnetipc: netrcv: type mismatch - expected %x got %x",
		MACHRIPC_RPC_REPLY_MSG, msgtype);
	return XK_FAILURE;
      }

      /* check the port this is trying to come in on */
      if ( isrpc_reply ) {
	net_port_number = netport->net_port_number;
	if (mach_net_msg.transport_port.net_port_number != netport->net_port_number)
	  {
	    xTrace4(machripcp, TR_ERRORS, "machrnetipc: netrcv: received machripc message on bogus netport %x hex (%d dec), expected %x hex (%d dec)",
		    mach_net_msg.transport_port.net_port_number,
		    mach_net_msg.transport_port.net_port_number,
		    netport->net_port_number,
		    netport->net_port_number );
	    return XK_FAILURE;
	  }
	/* the real Mach port will be filled in by the caller */
      }
      else 
	 if ((receive_port = 
		   convert_netport_to_mach_port(
	                   &mach_net_msg.transport_port,
	                   self, ERR_XOBJ, (Msg *) 0,
			   -1, sender))
		   == MACH_PORT_NULL)
	{
	  xTrace2(machripcp, TR_ERRORS, "machnetipc: netrcv: received machripc message on bogus netport %x hex (%d dec)",
		  mach_net_msg.transport_port.net_port_number,
		  mach_net_msg.transport_port.net_port_number);
	  return XK_FAILURE;
	  /* probably need to deallocate a few things? */
	}

      if (!isrpc_reply) {
	if (((ret=mach_port_type(mach_task_self(), receive_port, &remote_right))
	     != KERN_SUCCESS)) {
	  xTrace2(machripcp, TR_ERRORS, "machnetipc: netrcv: Cannot get type of remote port %x code %x before getting local port", receive_port, ret);
	  xTrace2(machripcp, TR_ERRORS, "machnetipc: netrcv: failed port id %x send port %x", net_port_number, netport->real_send_port);
	  return XK_FAILURE;
	}
	local_right = mach_net_msg.reply_port.net_port_rights;
	mach_header.msgh_remote_port = receive_port;
      }
      if (isrpc_request) { /* incoming fast send-once */
	mach_header.msgh_local_port =
	  portm_fast_send_once(mach_net_msg.reply_port.net_port_number,
			       sender);
	local_right = MACH_PORT_TYPE_SEND_ONCE;
	*rpc_reply_port = mach_net_msg.reply_port.net_port_number;
      }
      else
	if (isrpc_reply) {
	  mach_header.msgh_remote_port = MACH_PORT_NULL;;
	  mach_header.msgh_local_port = MACH_PORT_NULL;;
	}
	else
	    /* sets up listener, if needed */
	  mach_header.msgh_local_port = 
	    convert_netport_to_mach_port(&mach_net_msg.reply_port,
					 self,
					 isrpc_request?transport_s:ERR_XOBJ,
					 reply_msg,
					 msgid,
					 sender);
				       
      *mach_msg = convert_netmach_msg_to_actual_mach(
				     msg, &mach_header,
				     mach_net_msg.netport_count,
				     mach_net_msg.notinline_count,
				     (mach_net_msg.sequence_num & MNSEQMASK),
				     arch_type,
				     msgid,
				     sender,
				     local_right,
				     FALSE);
      return XK_SUCCESS;
    }

/*
 * machr_demux
 *
 * lower protocol will call this with message from net
 *
 *  Machripc must convert the entire message from network mach to real mach.
 *  It must also forward the data to the appropriate receive right.
 * 
 * 
 *  The entire converted message is xPop'd as an xkernel message
 *  to SSR, and SSR will remove its header and forward the remainder
 *  on to the registered server task, using mach ipc.
 *
 *
 */
static xkern_return_t
machr_demux(XObj self, XObj transport_s, Msg *msg)
{
  XObj			up_session;
  Part			part[2];
  XObj			session;
  mach_port_t		net_port;
  mach_port_t		rpc_receive_port;
  long			n;
  bool			mret;
  bool			isrpc_req = FALSE;
  mach_msg_header_t	*mach_msg;
  struct mnipchdr	hdr;
  enum SOURCE_BYTE_ARCH	arch_type;
  enum MACHIPCTYPE	msg_type;
  IPhost		sender;
  Msg			*reply_msg;
  mnportid_t		rpc_reply_port;

  xTrace0(machripcp, TR_MAJOR_EVENTS, "machr demux");
  n = msgLen(msg);
  if (n >= (sizeof(hdr))) {
    msgPop(msg, (Pfl)machnetipc_msg_pop, (void *)&hdr, (long)sizeof(hdr), (void *)0);
    arch_type = hdr.architecture_type;
    msg_type = hdr.machnetipcmsg_type;
    xTrace3(machripcp, TR_DETAILED, "machr demux %ld bytes of msg; type %x arch type %x id %d", n, msg_type, arch_type);
  }
  else msg_type = MACHRIPC_NULL_MSG;

  switch(msg_type) {
  case MACHRIPC_NULL_MSG:
    xTrace0(machripcp, TR_EVENTS, "machripc: machr_demux: null msg");
    return XK_SUCCESS;
    break;

  case SSR_MSG:
    xTrace0(machripcp, TR_EVENTS, "machr demux SSR");
    if (up_session = STATESELFPTR->ssr_service_session) {
      struct machripc_msg	local_buffer;

      n = msgLen(msg);
      mret = msgPop(msg, (Pfl)ssrpop,
		    (void *)&local_buffer.content.ssr_hdr,
		    (long)sizeof(local_buffer.content.ssr_hdr),
		    (void *)arch_type);
      /* SSR messages carry a mach reply port, only */
      xTrace2(machripcp, TR_FULL_TRACE, "machr demux length %d return code %d", n,  mret);
      xControl(transport_s, GETPEERHOST, (char *)&sender, sizeof(IPhost));
      if (local_buffer.content.ssr_hdr.xfer_completion)
	srxTransferComplete(local_buffer.content.ssr_hdr.msgid, sender);
      if ((net_port = 
	   convert_netport_to_mach_port
	   (&local_buffer.content.ssr_hdr.netreplyport,
	    self, ERR_XOBJ, (Msg *)0,
	    local_buffer.content.ssr_hdr.msgid,
	    sender))
	                                        != MACH_PORT_NULL)
	{
	  /* 
	    this will transfer the port name via xKernel IPC.
	    This does not do Mach port transfers; this protocol and
	    the upper one MUST be in the same Mach task
	   */

	  xTrace1(machripcp, TR_FULL_TRACE, "machr demux reply port %x\n", net_port);
	  msgPush(msg,
		  (Pfv)mycopypush,
		  (void *)&net_port,
		  (long)sizeof(mach_port_t),
		  (void *)0);
	  machr_pop(up_session, msg);
	  return XK_SUCCESS;
	}
      else {
	xTrace0(machripcp, TR_ERRORS, "machr_demux error in port conversion");
	return XK_FAILURE;
      }
    }
    else {
      xTrace0(machripcp, TR_EVENTS, "machr: no SSR protocol available yet");
      semWait(&(STATESELFPTR->ssr_wait));
      xTrace0(machripcp, TR_EVENTS, "machr: SSR protocol available now");
      return XK_SUCCESS;
    }
    break;

  case MACHRIPC_RPC_REPLY_MSG:
    xTrace0(machripcp, TR_EVENTS, "machripc demux MACHRIPC_RPC_REPLY_MSG");
    goto msg_processing;
    break;

  case MACHRIPC_RPC_REQUEST_MSG:
    xTrace0(machripcp, TR_EVENTS, "machripc demux MACHRIPC_RPC_REQUEST_MSG");

    isrpc_req = TRUE;
    reply_msg = calldemux_msg;
    calldemux_msg = (Msg *)NULL;
    /* now fall through */

  case MACHRIPC_MSG:
msg_processing:    
    xTrace0(machripcp, TR_DETAILED, "machripc demux MACHRIPC_MSG");

    /* for non-rpc's, probably want to keep a reference to the
       session, if we don't already have one
    */
    if (machnetipc_netrcv(
			  msg, (mnetport *)NULL,
			  arch_type,
			  msg_type,
			  isrpc_req,
			  &rpc_reply_port,
			  &mach_msg,
			  self,
			  transport_s)
	!= XK_FAILURE)
      {
	mach_msg_return_t retm;
	mach_port_t reply_port = mach_msg->msgh_local_port;

	/* if this is an rpc_request, should use SEND/RCV option */
	/*         note that reply will come back in same buffer */
	xk_master_unlock();
	if ((retm = mach_msg_send(mach_msg)) == MACH_MSG_SUCCESS) {
	  if (isrpc_req && reply_port != MACH_PORT_NULL) { 
	    struct send_request		rpcargs;
	    struct mach_msg_big_t      *msg_buffer;

	    bzero((char *)&rpcargs, sizeof(struct send_request));
	    rpcargs.port = reply_port;
	    rpcargs.deallocate = FALSE;
	    rpcargs.reply_msg = reply_msg;
	    rpcargs.netport = &Null_Netport;
	    rpcargs.validity = TRUE;
	    rpcargs.lower_session = transport_s;
	    rpcargs.port_net_number = rpc_reply_port;
	    xk_master_lock();
	    msgConstructAllocate(reply_msg, sizeof(struct mach_msg_big_t),
				 (char **)&rpcargs.msg);
	    xk_master_unlock();
	    machr_req_dn_notif(reply_port);
	    machr_msg_receive(&rpcargs);
	  }
	  xk_master_lock();
	  return XK_SUCCESS;
	}
	else
	  xk_master_lock();
	  xTrace3(machripcp, TR_ERRORS, "machr_demux send failed mach send, code %x hex(%d dec) port %x",
		  retm, retm, mach_msg->msgh_remote_port);
      }
    else
      xTrace0(machripcp, TR_ERRORS, "machr_demux send failed netrcv");
    break;

  case PORT_MGMT_MSG:
    xTrace0(machripcp, TR_ERRORS, "machr_demux PORT_MGMT_MSG");
    port_mgmt(arch_type, msg);
    return XK_SUCCESS;
    break;

  default:
    xTrace1(machripcp, TR_SOFT_ERROR, "machr_demux unknown message type %d", msg_type);

    return XK_FAILURE;
  }

  xTrace0(machripcp, TR_FULL_TRACE, "machr_demux fails");
  return XK_FAILURE;
}

/*
 *  machr_calldemux
 *
 *
 */
static xkern_return_t
machr_calldemux(XObj self, XObj transport_s, Msg *msg, Msg *return_msg)
{
  xTrace0(machripcp, TR_FULL_TRACE, "machripc calldemux called");
  calldemux_msg = return_msg;
  machr_demux(self, transport_s, msg);
  calldemux_msg = (Msg *)NULL;
  xTrace1(machripcp, TR_FULL_TRACE, "machripc calldemux returns msg len %d",
	  msgLen(return_msg));
  return XK_SUCCESS;
}

 /*
  * machr_pop
  *
  * forward a message up to SSR; called from demux
  *
  */
static void
machr_pop(XObj up_session, Msg *msg)
 {
   xTrace0(machripcp, TR_FULL_TRACE, "machr_pop");

   /* remember the upward protocol must be in the same Mach task as
      this one, because a Mach port is being sent in an
      xkernel message
    */
   xDemux(up_session, msg);
 }

/*
 * machr_openenable
 *
 *   the SSR service is the only thing that needs to open machnetipc.
 *   all other interactions are implicit through Mach port transfers.
 *
 *   we use a single session for all demux's up to the ssr service,
 *      because it only takes incoming messages, and because it
 *      replies without blocking.
 *
 */
static xkern_return_t
machr_openenable(XObj self, XObj hlp, XObj hlptype, Part *p)
 {
   xTrace0(machripcp, TR_FULL_TRACE, "machr_openenable");

   if (!STATESELFPTR->ssr_service) {
     STATESELFPTR->ssr_service = hlp;
     semSignal(&(STATESELFPTR->ssr_wait));
     STATESELFPTR->ssr_service_session = machr_new_sessn(self, hlp);
     xTrace0(machripcp, TR_MORE_EVENTS, "machr_openenable returning");
   }
   else {
     xTrace0(machripcp, TR_ERRORS, "machr_openenable called more than once, ignoring");
   }
   return(XK_SUCCESS);
 }

/*
 * machr_push
 *
 *  This is only used by upper protocols; the more common case
 *  of outgoing data is an intercepted Mach port send 
 *
 */
static xmsg_handle_t
machr_push(XObj self, Msg *msg)
 {
   Part whom[2];
   IPhost destination;
   IPhost source;
   XObj ls;
   mnetport outport;
   struct machripc_msg outmsg;
   mach_port_t reply_port = SESSNSTATEPTR->localport;
   mach_port_type_t reply_right;
   kern_return_t ret;

   xTrace1(machripcp, TR_FULL_TRACE, "machr_push %x", self);

   /* must be from SSR, just convert port to net form and send */
   /* note that we don't copy the operation code; it's assumed to be REQUEST */
   xTrace1(machripcp, TR_FULL_TRACE, "machr_push incoming msg len %d", msgLen(msg));
   bzero((char *)&outmsg, sizeof(outmsg));

   if ((ret=mach_port_type(mach_task_self(), reply_port, &reply_right))
       != KERN_SUCCESS)
     {
       xTrace2(machripcp, TR_ERRORS, "machr_push: could not get port type for port %x failure code %x hex", reply_port, ret);
     }
   else xTrace1(machripcp, TR_DETAILED, "machripc: machr_push: got port right of type %x for reply port", reply_right);
   if (reply_right & MACH_PORT_TYPE_SEND) reply_right = MACH_PORT_TYPE_SEND;

   outmsg.hdr.architecture_type = MN_ARCH_MARKER;
   outmsg.hdr.machnetipcmsg_type = SSR_MSG;
   outmsg.content.ssr_hdr.msgid = msgid_assign();
   outport = convert_to_netport(reply_port,  /* locks the port */
				ONESEND(reply_right),
				SESSNSTATEPTR->destination_host, 
				outmsg.content.ssr_hdr.msgid, 0);
   if (mapResolve(xkMsgIdMap, &outmsg.content.ssr_hdr.msgid, 0) == XK_SUCCESS){
       outmsg.content.ssr_hdr.xfer_completion = 1;
       mapUnbind(xkMsgIdMap, &outmsg.content.ssr_hdr.msgid);
     }
   if (outport.net_port_type == MN_INVALID)
     {
       xTrace0(machripcp, TR_ERRORS, "machr_push: port conversion failed");
       return(ERR_XK_MSG);
     }

   outmsg.content.ssr_hdr.netreplyport = *((mn_netport_t *)&outport);
   outmsg.content.ssr_hdr.netreplyport.net_port_rights =
     ONESEND(outmsg.content.ssr_hdr.netreplyport.net_port_rights);
   xTrace3(machripcp, TR_FULL_TRACE, "machr_push netport params: number %d rights %x type %x hostaddr.a %d",
	   outmsg.content.ssr_hdr.netreplyport.net_port_number,
	   outmsg.content.ssr_hdr.netreplyport.net_port_rights,
	   outmsg.content.ssr_hdr.netreplyport.receiver_host_addr.a);

   msgPush(msg, (Pfv)ssrpush, (void *)&outmsg,
	   sizeof(outmsg.content.ssr_hdr) + sizeof(struct mnipchdr),
	   (void *)0);

   partInit(whom, 1);
   destination = SESSNSTATEPTR->destination_host;
   
   xTrace5(machripcp, TR_FULL_TRACE, "machr_push dest %d.%d.%d.%d arch %x", destination.a, destination.b, destination.c, destination.d, PERM1);
   partPush(whom[REMOTE_PART], &(SESSNSTATEPTR->destination_host), sizeof(IPhost));
#ifdef USING_UDP
   partPush(whom[REMOTE_PART], &machripc_udpport);
#endif /* USING_UDP */
/*
   partSetProt(whom[LOCAL_PART], (long)MACHR_UDP_PORTNUM);
   partPush(whom[LOCAL_PART], ANY_HOST, 0);
*/
   if ((ls = active_open_transport(whom)) != ERR_XOBJ)
     {
       xkern_return_t ret;
       Msg rmsg, *locked_ports;

       xTrace0(machripcp, TR_EVENTS, "machr_push doing msgPush, xPush");
       xTrace1(machripcp, TR_FULL_TRACE, "machr_push net %d", outmsg.content.ssr_hdr.netreplyport.receiver_host_addr.a);

       locked_ports = (Msg *)msgGetAttr(msg, 0);
       msgConstructEmpty(&rmsg);
       ret = xCall(ls, msg, &rmsg);
       xTrace1(machripcp, TR_EVENTS, "machr_push xCall return code %d", ret);
#ifdef PORTLOCKS
       if (quick_netport_lookup(outport.net_port_number, 0) != XK_FAILURE)
	 readerUnlock(outport.rwlock_ptr);
       if (locked_ports) {
	 xTrace0(machripcp, TR_DETAILED, "machnetipc: machr_push: locked_ports");
	 msgPop(locked_ports, (Pfl)unlocker, 0, msgLen(locked_ports), 0);
       }
#endif PORTLOCKS	   
       /* reply should be null */
       machr_demux(self, ls, &rmsg);
       xTrace0(machripcp, TR_EVENTS, "machr_push doing xClose");
       xClose(ls);
 /*       msgDestroy(msg); */
       return(XK_SUCCESS);
     }
   else
     xTrace0(machripcp, TR_ERRORS, "machnetipc: open of transport service for client request failed");
   return(XK_FAILURE);
 }

/*
 * machr_open
 *
 * Open a Mach IPC network session
 *
 *    used only by SSR code to send requests to remote server via SSR
 *    the sessions don't need to be in a map because we
 *      don't need to match incoming messages.
 */
static XObj
machr_open(XObj self, XObj hlp, XObj hlptype, Part *part)
{
  /*                    where does this get freed ? */
  Mach_ActiveId *maid = TYPE_MALLOC(Mach_ActiveId);
  XObj newsessn;
  long  portnum;

  xTrace1(machripcp, TR_MAJOR_EVENTS, "machr_open, self %x", self);
  if ((portnum = *(long *)partPop(part[LOCAL_PART])) != relProtNum(hlp->myprotl, myProtl))
    {
      xTrace2(machripcp, TR_ERRORS, "machr bogus participant id %d %s\n", portnum, hlp->name);
      return(ERR_XOBJ);
    }
  bzero((char *)maid, sizeof(Mach_ActiveId));
  maid->localport =  *((mach_port_t *)(partPop(part[LOCAL_PART])));
  maid->destination_host = *((IPhost *)(partPop(part[REMOTE_PART])));
  /* could pop the service id off the remote part */
  xTrace4(machripcp, TR_FULL_TRACE, "machr_open %d-%d-%d-%d", maid->destination_host.a, maid->destination_host.b, maid->destination_host.c, maid->destination_host.d);  
  newsessn = xCreateSessn(machr_session_init, hlp, hlp, self, 0, (void *)maid);
  if (newsessn != ERR_XOBJ) {
    xTrace1(machripcp, TR_EVENTS, "machr_open new session %x", newsessn);
    newsessn->state = (char *)maid;
    return(newsessn);
  }
  else {
    xTrace0(machripcp, TR_ERRORS, "machr: could not create lower session");
    return(ERR_XOBJ);
    }
}

/*
 * machr_forward_send_request()
 *
 * Intercept msg_send's destinated for remote machines;
 *  convert them to netmsgs, and forward to the appropriate
 *  remote machine.  These are ports for which the machnetipc thread holds the
 *  receive rights; it is obligated to send them on to the actual receiver
 *  on some other machine.
 *
 * The mach msg is in the buffer portion of the xkernel msg
 *
 * This must run under the xk_master_lock
 *
 */
static void
machr_forward_send_request(struct send_request *args, int too_big)
{
  mach_msg_header_t *msg = (mach_msg_header_t *)args->msg;
  Msg               xkmsg = args->request_msg;  /* must copy */
  mnetport          *remport = args->netport;
  mnetport	    *reply_port;
  mnportid_t	    remport_number;

  Part whom[1];
  IPhost destination;
  IPhost source;
  XObj ls;
  machnetipc_hdr *outmsg = TYPE_MALLOC(machnetipc_hdr);
  IPhost iphost;
  xkern_return_t ret;
  kern_return_t kret;
  mach_port_t mach_reply_port;
  mach_port_type_t reply_right;
  mnportid_t reply_netport_number = NULL;
  bool is_rpc = FALSE;
  Msg *locked_ports;

  xTrace0(machripcp, TR_FULL_TRACE, "enter machr_forward_send_request");
  xTrace2(machripcp, TR_DETAILED, "machr forward send request remport addr %x arg addr %x",
	  remport, args);

  bzero((char *)outmsg, sizeof(machnetipc_hdr));

  if (too_big) {
    mach_msg_return_t mret;
    char              *newdata;
    Msg               oldmsg = args->request_msg;

    xTrace1(machripcp, TR_EVENTS, "machnetipc: forward_send starting larger recv %d", msg->msgh_size);
    msgConstructAllocate(&xkmsg, msg->msgh_size, &newdata);
    /* copy the msg header data */
    bcopy((char *)msg, (char *)newdata, sizeof(mach_msg_header_t));
    msg = (mach_msg_header_t *)newdata;
    msgDestroy(&oldmsg);
    if ((mret = mach_msg_receive(&newdata)) != MACH_MSG_SUCCESS) {
      xTrace1(machripcp, TR_EVENTS, "machnetipc: forward_send_request: receive failure %x", mret);
      /* cleanup */
      return;
    }
  }
  else
    msgTruncate(&xkmsg, msg->msgh_size);

  mach_reply_port = msg->msgh_remote_port;

  /* First determine where to send this message */
  if (remport == (mnetport *)NULL)
    {
      xTrace2(machripcp, TR_ERRORS, "machr forward send failed to find net descriptor for destination port %x, msg %x", msg->msgh_remote_port, msg);
      mach_msg_destroy(msg);
      msgDestroy(&xkmsg);
      xFree((void *)outmsg);
      return;
    }
  remport_number = remport->net_port_number;  /* need local copy */
  if (!(remport->net_port_rights & MACH_PORT_TYPE_SEND_ONCE) &&
      !(remport->net_port_rights & MACH_PORT_TYPE_SEND))
    {
      xTrace2(machripcp, TR_ERRORS, "machripc forward_send_request does not hold network send right for port %x addr %x",
	      remport->net_port_number,
	      msg->msgh_remote_port);
      mach_msg_destroy(msg);
      msgDestroy(&xkmsg);  /* side-effect xFree's msg */
      xFree((void *)outmsg);
      return;
    }
  iphost = remport->receiver_host_addr;

  if (kret = mach_port_type(mach_task_self(), mach_reply_port, &reply_right)
      != KERN_SUCCESS) 
    {
      xTrace2(machripcp, TR_ERRORS, "machnetipc: forward_send_request: bogus reply right %x ret %x", reply_right, kret);
      mach_port_deallocate(mach_task_self(), mach_reply_port);
      mach_reply_port = MACH_PORT_NULL;
    }
  
  xTrace1(machripcp, TR_DETAILED, "forward_send_request msglen %d", msgLen(&xkmsg));

    xTrace4(machripcp, TR_FULL_TRACE, 
	  "machr forward send request to host %d.%d.%d.%d",
	  iphost.a, iphost.b, iphost.c, iphost.d);

  /* CONVERSION: will look at first two mach headers but not copy */
  /* this will lock the remote port and all embedded ports, except
     the reply right if this is an rpc request */
  convert_mach_msg_to_netmach_msg(
	  (struct mach_msg_big_t *)msg, &xkmsg, outmsg, remport, iphost, 0);

  if (reply_right & MACH_PORT_TYPE_SEND_ONCE) {
    xTrace1(machripcp, TR_FUNCTIONAL_TRACE, "machnetipc: forward_send_request: reply right is send once for port %x", mach_reply_port);

    if (mach_reply_port != MACH_PORT_NULL)
      /* get a number, register sender so a reboot will kill the port */
      reply_netport_number = portm_get_netnum(mach_reply_port, iphost);
    outmsg->content.machmsg.reply_port.net_port_number = reply_netport_number;
    is_rpc = TRUE;
  }

  ls = ERR_XOBJ;
  /* once we have opened a session for a port, we must stick with it;
     otherwise we will lose the sequencing
     */
  if (remport->lower_session != (XObj)0) ls = remport->lower_session;
  else {
    /* get ready for xOpen */
    partInit(whom, 1);
    partPush(whom[REMOTE_PART], &iphost, sizeof(IPhost));
#ifdef USING_UDP
    partPush(whom[REMOTE_PART], &machripc_udpport, sizeof(IPhost));
#endif /* USING_UDP */
  }

  if ((ls != ERR_XOBJ) ||
      (remport->lower_session = ls
        = xOpen(myProtl, myProtl, xGetDown(myProtl, 0), whom)) != ERR_XOBJ)
    {
      Msg  xk_return_msg;

      xTrace0(machripcp, TR_EVENTS, "machr_forward_send_request xOpen succeeded");
      msgConstructEmpty(&xk_return_msg); /* can we pre-compute header? */
      msgPush(&xkmsg, (Pfv)mycopypush,
	      (char *)outmsg,
	      sizeof(machnetipc_hdr),
	      (void *)0);
      xTrace1(machripcp, TR_DETAILED, "machripc pushing message size %d",
	      msgLen(&xkmsg));

      if (outmsg->hdr.machnetipcmsg_type == MACHRIPC_RPC_REQUEST_MSG) {
	mnetport fast_reply_port;

	xTrace1(machripcp, TR_DETAILED, "machripc pushing rpc retmsg at %x",
	      &xk_return_msg);
	reply_port = &fast_reply_port;
	reply_port->net_port_number = reply_netport_number;
#ifdef PORTLOCKS
	locked_ports = (Msg *)msgGetAttr(&xkmsg, 0);
#endif PORTLOCKS

	ret = xCall(ls, &xkmsg, &xk_return_msg);

#ifdef PORTLOCKS
	if (locked_ports) {
	  xTrace0(machripcp, TR_DETAILED, "machnetipc: machr_forward_send_request: locked_ports");
	  msgPop(locked_ports, (Pfl)unlocker, 0, msgLen(locked_ports), 0);
	  msgDestroy(locked_ports);
	  xFree((char *)locked_ports);
	}
#endif PORTLOCKS	   
	if (ret == XK_FAILURE)
	  xTrace0(machripcp, TR_ERRORS, "machripc xCall failed");
	else 
	  { 
	    mach_msg_header_t	*retmsg = 0;
	    mach_msg_return_t	mret = 0;
	    Msg			*locked_ports;
	    struct mnipchdr	hdr;
	    bool		too_short = 0;

	    xTrace1(machripcp, TR_DETAILED,
		    "machr_forward_send_request returned from xCall with reply len %d", msgLen(&xk_return_msg));
	    if (msgLen(&xk_return_msg) >= sizeof(hdr)) {
	      msgPop(&xk_return_msg, (Pfl)machnetipc_msg_pop, (void *)&hdr,
		     (long)sizeof(hdr), (void *)0);
	    }
	    else {
	      xTrace0(machripcp, TR_DETAILED, "machnetipc: forward_send_request: rpc reply too short");
	      too_short = 1;
	    }
	    if (too_short ||
		((ret = machnetipc_netrcv(&xk_return_msg,
					reply_port,
					hdr.architecture_type,
					hdr.machnetipcmsg_type,
					FALSE, 0, &retmsg,
					args->self, ls))
	      == XK_FAILURE)) {
	      /* reply too short means RPC has been aborted */
	      xTrace4(machripcp, TR_EVENTS, "machnetipc: forward_send: mach send failed convert code %x msgptr %x with code %x for port %x",
		      ret,
		      retmsg,
		      mret,
		      retmsg?
		      retmsg->msgh_remote_port:MACH_PORT_NULL);
	      if (retmsg) xFree((void *)retmsg);
	      /* must convert the reply port into a full netport structure */
	      /* this will also retract the fast-send-once registration */
	      (void)convert_to_netport(mach_reply_port,
				       MACH_PORT_TYPE_SEND_ONCE,
				       iphost, 0, reply_netport_number);
	    }
	    else {
	      retmsg->msgh_remote_port = mach_reply_port;
	      retmsg->msgh_local_port = MACH_PORT_NULL;
	      retmsg->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE,
						 0);
	      if ((mret = mach_msg_send(retmsg)) != MACH_MSG_SUCCESS) {
		xTrace1(machripcp, TR_ERRORS, "machnetipc: forward_send: rpc reply message could not be delivered: %x code", mret);
	      }
	    }  /* end of RPC reply handling */
	    portm_use_netnum(mach_reply_port);

	  } /* end of RPC send success */
	msgDestroy(&xk_return_msg);  /* on the stack of this thread  */
	msgDestroy(&xkmsg);          /* allocated in machr-start-rec */
      } /* end of RPC handling */
      else {
	/* not an RPC; we send and leave, assuming that the return msg is null */
	xTrace3(machripcp, TR_EVENTS, "machripc: forward_send no reply, starting xCall, msg %x retmsg %x ls %x", &xkmsg, &xk_return_msg, ls);
	
	if (((ret = xCall(ls, &xkmsg, &xk_return_msg)) != XK_SUCCESS)
	    || (remport->local_status == MN_INVALID))
	  {
	    xTrace0(machripcp, TR_EVENTS, "machnetipc: forward_send_request: xCall fails or destination rebooted");
#ifdef PORTLOCKS
	    if (quick_port_convert(&args->port) != &Null_Netport &&
		remport->local_status != MN_INVALID)
	      readerUnlock(remport->rwlock_ptr);
	    if (mach_reply_port != MACH_PORT_NULL) 
	      reply_port = quick_port_convert(&mach_reply_port);
	    if (mach_reply_port != MACH_PORT_NULL && reply_port != &Null_Netport)
	      readerUnlock(reply_port->rwlock_ptr);
#endif PORTLOCKS
	    remport->lower_session = ERR_XOBJ;
	    remport->net_port_type = MN_INVALID;
	    xClose(ls);
	  }
#ifdef PORTLOCKS
	if (quick_port_convert(&args->port) != &Null_Netport &&
	    !(remport->net_port_rights & MACH_PORT_TYPE_SEND_ONCE))
	  readerUnlock(remport->rwlock_ptr);
	else
	  xFree((char *)remport);  /* this is NOT the rpc path! (why not?) */
	if (mach_reply_port != MACH_PORT_NULL) 
	  reply_port = quick_port_convert(&mach_reply_port);
	if (mach_reply_port != MACH_PORT_NULL && reply_port != &Null_Netport)
	  readerUnlock(reply_port->rwlock_ptr);
#endif PORTLOCKS
#ifdef PORTREFS
	mach_msg_destroy(msg);  /* think about this ... */
#endif PORTREFS
	xTrace0(machripcp, TR_EVENTS, "machripc: forward_send no reply done");
	msgDestroy(&xkmsg);   /* as side-effect frees msg */
      }
      xTrace0(machripcp, TR_EVENTS, "machr_forward_send_request finished");
    }
  else
    xTrace0(machripcp, TR_ERRORS, "machr_forward_send_request failed to open session to transport protocol for forward_send_rquest");
}

/*
 * machr_forward_rpc_reply()
 *
 * Intercept rpc replies,
 *  convert them to netmsgs, and forward to the appropriate
 *  remote machine.  These are ports for which the machr thread holds the
 *  receive rights; it is obligated to send them on to the actual receiver
 *  on some other machine.
 *
 * Must convert from mach message format to network and xkernel structures
 *
 * This routine is called as a result of machr_demux, which recognizes the
 * incoming RPC request.
 *
 */

static void
machr_forward_rpc_reply(rpcargs, too_big)
     struct send_request *rpcargs; int too_big;
{
  XObj self = rpcargs->self;
  XObj lower_session = rpcargs->lower_session;

  mach_msg_header_t *msg = (mach_msg_header_t *)rpcargs->msg;
  mnetport *remport = rpcargs->netport;
  machnetipc_hdr *outmsg = TYPE_MALLOC(machnetipc_hdr);
  kern_return_t ret;
  Msg           *locked_ports;

  xTrace1(machripcp, TR_FULL_TRACE, "machripc: machr_forward_rpc_reply called, msg ptr %x", msg);

  bzero((char *)outmsg, sizeof(machnetipc_hdr));

  if (too_big) {
    Msg newmsg;
    mach_msg_return_t mret;
    char *newdata;

    xTrace1(machripcp, TR_EVENTS, "machnetipc: forward_rcp_reply starting larger recv %d", msg->msgh_size);
    msgConstructAllocate(&newmsg, msg->msgh_size, &newdata);
    bcopy((char *)msg, newdata, sizeof(mach_msg_header_t));
    msg = (mach_msg_header_t *)newdata;
    rpcargs->msg = (struct mach_msg_big_t *)newdata;
    rpcargs->reply_msg = &newmsg;
    if ((mret = mach_msg_receive(newdata)) != MACH_MSG_SUCCESS) {
      xTrace1(machripcp, TR_EVENTS, "machnetipc: forward_rpc_reply: receive failure %x", mret);
      msgDestroy(&newmsg);
      return;
    }
  }
  else
      msgTruncate(rpcargs->reply_msg, msg->msgh_size);

  if (!rpcargs->validity) {
    xTrace0(machripcp, TR_ERRORS, "machnetipc rpc session invalid");
    return;
   }

  /*  msgAssign(rpcargs->reply_msg, &rpcargs->request_msg); */

  /* this sets up the machnetipc header, deallocates send_once netright */
  convert_mach_msg_to_netmach_msg((struct mach_msg_big_t *)msg,
				  rpcargs->reply_msg, outmsg,
				  remport, remport->receiver_host_addr,
				  MACHRIPC_RPC_REPLY_MSG);
  xTrace0(machripcp,TR_FULL_TRACE, "machripc forward_rpc_reply converted msg");
  xTrace1(machripcp,TR_DETAILED, "machr forward_rpc_reply size %d", msg->msgh_size); 

  outmsg->content.machmsg.transport_port.net_port_number =
    rpcargs->port_net_number;

#ifdef PORTLOCKS
  if (locked_ports = msgGetAttr(rpcargs->reply_msg, 0) 
      /* || msg->mmhdr->msgh_local_port != MACH_PORT_NULL */ )
    {
      /* have to abort the rpc and do an ordinary send operation */
      xTrace0(machripcp, TR_ERRORS, "machnetipc: rpc_reply: has locked ports");
      rpcargs->reply_msg = (Msg *)NULL;
      rpcargs->lower_session = ERR_XOBJ;
      rpcargs->deallocate = TRUE;
      xAsyncThread(machr_msg_receive, rpcargs, "machr msg receive thread");
      return;
    }
#endif PORTLOCKS

  /* push the machnetipc header onto it */
  msgPush(rpcargs->reply_msg, (Pfv)mycopypush,
	  (char *)outmsg,
	  sizeof(machnetipc_hdr),
	  (void *)0);
  xTrace1(machripcp, TR_FULL_TRACE, "machripc forward_rpc_reply constructed msg size %d", msgLen(rpcargs->reply_msg));

  if (lower_session && (SESSNSTATEVARPTR((lower_session)))->rebootedp == TRUE)
    {
      xTrace1(machripcp, TR_EVENTS, "machripc forward_rpc_reply found lower session %x rebooted", lower_session);
      /* is this necessary? */
      xClose(lower_session);
    }

  /* msgFree(rpcargs->request_msg); it's ok, reader won't try again */
  portm_use_fast_send_once(msg->msgh_remote_port, TRUE);
  xFree((void *)outmsg);
  xFree((void *)rpcargs);
  xTrace0(machripcp, TR_FULL_TRACE, "machripc: forward_rpc_reply exits"); 
}

/*
 *  machr_req_dn_notif
 *
 *     request a dead name notification for the port
 *
 */
static void
machr_req_dn_notif(mach_port_t port)
{
  mach_port_t previous_port;
  kern_return_t kret;

  kret =
    mach_port_request_notification(mach_task_self(),
				   port,
				   MACH_NOTIFY_DEAD_NAME,
				   0,
				   STATEPTR->notification_port,
				   MACH_MSG_TYPE_MAKE_SEND_ONCE,
				   &previous_port);
  if (kret != KERN_SUCCESS) xTrace1(machripcp, TR_ERRORS, "machnetipc: machr_req_dn_notif: failed %x", kret);
}

/*
 *  machr_handle_notification()
 *
 *    A send or send-once right has gone away
 *
 */
static void
machr_handle_notification(mach_port_t port, mach_msg_id_t not_type)
  {
    if (not_type != MACH_NOTIFY_DEAD_NAME )
      {
	xTrace1(machripcp, TR_ERRORS, "machr_handle_notification called with type %x; no action",
		not_type);
	return;
      }
    xTrace0(machripcp, TR_ERRORS, "machr_handle_notification port deleted or dead");
    /* may be local receiver died; must start protocol to notify
       the senders, delete local structures
       */
    xk_master_lock();
    evDetach(evSchedule((Pfv)portm_delete_local_receive, (void *)port, 0));
    xk_master_unlock();
  }

/*
 * machr_notification_receive()
 *
 * Receives local notifications about ports managed by machnetipc.
 * Is started by init, runs forever.
 *
 */
static void
machr_notification_receive()
{
  mach_msg_return_t ret;
  /* should be only dead_name */
  mach_dead_name_notification_t smsg;

  xTrace1(machripcp, TR_FULL_TRACE, "machr_notification_receive starting on port %x", STATEPTR->notification_port);

  smsg.not_header.msgh_local_port = STATEPTR->notification_port;
  smsg.not_header.msgh_size = sizeof(mach_dead_name_notification_t);

  while ((ret = mach_msg_receive(&smsg)) == MACH_MSG_SUCCESS)
    {
    xTrace3(machripcp, TR_EVENTS, "machnetipc: machr_notification_receive received notification id %x type %x for port %x",
	    smsg.not_header.msgh_id,
	    smsg.not_type,
	    smsg.not_port);

    machr_handle_notification(smsg.not_port, smsg.not_header.msgh_id);
    smsg.not_header.msgh_local_port = STATEPTR->notification_port;
    smsg.not_header.msgh_size = sizeof(mach_dead_name_notification_t);
    xTrace0(machripcp, TR_FUNCTIONAL_TRACE, "machnetipc: notification read");
  }
  xTrace1(machripcp, TR_ERRORS, "error from machr mach msg recv %x", ret);
  return;
}

/*
 *
 *  machnetipc_start_msg_receive (args)
 *
 *       primary job is to malloc storage for the message and start
 *       an asynchronous thread to do the listening.  It also sets
 *       up notifications.
 *
 *       Entry invariant: args->request_msg is free for use.
 *       Exit invariant: args->request_msg is a valid xkernel msg;
 *                       args->msg is a pointer to a buffer for
 *                       mach_msg_receive and is the xkernel buffer
 *                       area of args->request_msg
 *       Storage: args is re-used until the port dies.
 *
 */
void
machnetipc_start_msg_receive(args) struct send_request *args;
{
  mach_port_t		previous_port = MACH_PORT_NULL;
  mach_port_t		port = args->port;
  mach_port_mscount_t	sync = 0;

  /* the message space will be freed by forward_send_request */

  if (args->ask_for_dead_name) {
    machr_req_dn_notif(port);
    args->ask_for_dead_name = 0;
  }

  if (args->ask_for_nms) {
    mach_port_request_notification(mach_task_self(),
				   port,
				   MACH_NOTIFY_NO_SENDERS,
				   sync,
				   port,
				   MACH_MSG_TYPE_MAKE_SEND_ONCE,
				   &previous_port);
    args->ask_for_nms = 0;
  }

  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: start_rcv: starting machr_msg_rcv");
  msgConstructAllocate(&args->request_msg,
		       XK_BASIC_MACH_MSG_MAX, (void *)&args->msg);

  xk_master_unlock();
  machr_msg_receive(args);
  /* it is possible to start processing the message right here */
  xk_master_lock();

  while ( args->validity ) {
    xTrace1(machripcp, TR_FULL_TRACE, "machnetipc: start_rcv for port %x waits", port);
    xTrace1(machripcp, TR_FULL_TRACE, "machnetipc: start_rcv for port %x ends wait", port);
    /* set up the xkernel message and get a pointer to the buffer */
    msgConstructAllocate(&args->request_msg, 
			 XK_BASIC_MACH_MSG_MAX,
			 (void *)&args->msg);
    xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: start_rcv: starting machr_msg_rcv after semwait");
    xk_master_unlock();
    machr_msg_receive(args);
    xk_master_lock();
  }
  xFree((char *)args);
  xTrace1(machripcp, TR_EVENTS, "machnetipc: start_rcv for port %x terminates",
	  port);
}

/*
 *   xkmach_check_status
 *
 *	get the Mach kernel's veiw of the port status
 *
 */
static void
xkmach_check_status(struct send_request *args, mach_port_status_t *pstat)
{
  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: xkmach_check_status: entered");

    if (args->netport->net_port_type != MN_INVALID) {
      kern_return_t ret;

      if ((ret=
	   mach_port_get_receive_status(mach_task_self(), args->port, pstat))
	  != KERN_SUCCESS) {
	xTrace4(machripcp, TR_ERRORS, "machnetipc: mach_msg_receive: could not get port status for port %x netnum %x ret %x state",
		args->port,
		args->netport->net_port_number,
		ret, args->netport->net_port_type);
	return;
      }
      xTrace3(machripcp, TR_MAJOR_EVENTS, "machnetipc: xkmach_check_status: port state is %d (valid is %d) queue len %d",
	      args->netport->net_port_type,
	      MN_VALID,
	      pstat->mps_msgcount);
      /* this is where we should do queue forwarding */
      /* when the forwarding is done, check for nms  */
      args->netport->net_port_type = MN_VALID;
    }
  }


/*
 * machr_msg_receive (args)
 *
 * The message intermediary receives a message on a port for which
 * the receive rights are on another machine; the send right is local.
 * This is independent of the master xkernel control
 *
 */
void
machr_msg_receive(args) struct send_request *args;
{
  mach_msg_return_t ret = 0;
  mach_port_right_t port = args->port;
  struct mach_msg_big_t *smsg = args->msg;
  Event ev;
  int too_big = 0;
  mach_port_status_t pstat;
  int                queue_len;
  int		     mscount;  /* initial make send count */

  xTrace3(machripcp, TR_FULL_TRACE, "machr_msg_receive starting on port %x id %x arg addr %x",
	  port, args->netport->net_port_number, args);
  xTrace4(machripcp, TR_FULL_TRACE, "machr_msg_receive: remote port host %d.%d.%d.%d",
	  args->netport->receiver_host_addr.a,
	  args->netport->receiver_host_addr.b,
	  args->netport->receiver_host_addr.c,
	  args->netport->receiver_host_addr.d);

  if (!smsg) {
    xTrace1(machripcp, TR_ERRORS, "machr_msg_receive: no message space in arg block at %x", args);
    return;
  }

  if (port==MACH_PORT_NULL) {
    xTrace0(machripcp, TR_ERRORS, "machr_msg_receive: null port; abandoning");
    return;
  }

  xkmach_check_status(args, &pstat);
  queue_len = pstat.mps_msgcount;
  mscount   = pstat.mps_mscount;

 reread:
  smsg->mmhdr.msgh_local_port = port;
  smsg->mmhdr.msgh_size = sizeof(struct mach_msg_big_t);
  if (args->reply_msg != (Msg *)NULL)
	ret = mach_msg_receiveSafeSize(smsg);
  else
    ret = mach_msg_receiveSafeSizeTime(smsg);
  if ( ret  == MACH_MSG_SUCCESS
      ||
      ret == MACH_RCV_TOO_LARGE
      ||
      (ret == MACH_RCV_TIMED_OUT && args->reply_msg != (Msg *)NULL))
    {
      xTrace2(machripcp, TR_EVENTS, "machnetipc: machr_msg_receive received message on port %x, msg %x", port, smsg);
      
      /* the port has died; port mgr already knows about it */
      if (args->validity == FALSE) return;

      if (ret==MACH_RCV_TIMED_OUT) {
	/* return null reply, continue listening in non-rpc state */

	args->lower_session = ERR_XOBJ;
	args->reply_msg = (Msg *)NULL;
	args->deallocate = TRUE;
	goto reread;
      }

      xkmach_check_status(args, &pstat);

      if (smsg->mmhdr.msgh_id == MACH_NOTIFY_NO_SENDERS) {
	int mscount = ((mach_no_senders_notification_t *)smsg)->not_count;

	xTrace4(machripcp, TR_MAJOR_EVENTS, "machripcp: machr_msg_receive detected no more senders (msgh_id %x) on port %x mscount %ld current %d",
		smsg->mmhdr.msgh_id,
		port,
		pstat.mps_mscount,
		mscount);

	if (mscount && (mscount == pstat.mps_mscount) && !pstat.mps_sorights)
	  {
	    xk_master_lock();
	    xTrace1(machripcp, TR_MAJOR_EVENTS, "machnetipc: processing no more senders for port %x", args->netport);
#ifdef NOMORESENDERS	    
	    portm_generate_nms(args->netport);
	    xFree((char *)args);
	    xk_master_unlock();
	    return;
#endif NOMORESENDERS
       }
	{
	  mach_port_t previous_port = MACH_PORT_NULL;

	  /* reregister the nms notification */
	  mach_port_request_notification(mach_task_self(),
					 port,
					 MACH_NOTIFY_NO_SENDERS,
					 pstat.mps_mscount,
					 port,
					 MACH_MSG_TYPE_MAKE_SEND_ONCE,
					 &previous_port);
	  goto reread;
	}
      }

      if (smsg->mmhdr.msgh_id == MACH_NOTIFY_SEND_ONCE) {
	xTrace4(machripcp, TR_MAJOR_EVENTS, "machripcp: machr_msg_receive detected send_once deletion (msgh_id %x) on port %x mscount %ld",
		smsg->mmhdr.msgh_id,
		port,
		((mach_no_senders_notification_t *)smsg)->not_count,
		pstat.mps_mscount);
	args->validity = FALSE;
      }

      xTrace1(machripcp, TR_DETAILED, "machr msg rcv size %d ",
	    smsg->mmhdr.msgh_size); 
      /* 
	see if the port has changed state; could have died or the 
	receive right may have moved off campus
	*/

      /* 
	NB:The smsg space will be freed by forward_send_request/reply.
       */

      if (ret == MACH_RCV_TOO_LARGE) too_big = 1;

      xk_master_lock();
      if (args->reply_msg != (Msg *)NULL)
	machr_forward_rpc_reply(args, too_big);
      else
	machr_forward_send_request(args, too_big);
      xk_master_unlock();

      if (queue_len && !--queue_len) {
	xTrace1(machripcp, TR_EVENTS, "machnetipc: machr_msg_receive: port %x changed from forwarding to valid", port);
	if (args->netport && args->netport != &Null_Netport && 
	    args->netport->net_port_type == MN_FORWARDING)
	  args->netport->net_port_type = MN_VALID;
      }
    }
  else {
      xTrace1(machripcp, TR_ERRORS, "error from machr mach msg recv %x", ret);
      /* if the port is dead, then xFree the args area */
      if (ret == MACH_RCV_PORT_DIED ) {
	xTrace1(machripcp, TR_FUNCTIONAL_TRACE, "machnetipc: msg_rcv port %x died", port);
	args->validity = FALSE;
      }
      else xTrace2(machripcp, TR_ERRORS, "machnetipc: msg_rcv unexpected error %x on port %x",
		   ret, port);
    }
  if (args->deallocate)  /* a send_once right goes away immediately */
    {
      xk_master_lock();
      xFree((char *)args);
      xk_master_unlock();
    }
}

/*
 *  is_send(rright)
 *
 *   determine if an incoming net Mach message has legal header rights
 *
 */
#define is_send(rright) (rright & ( MACH_PORT_TYPE_SEND_ONCE | MACH_PORT_TYPE_SEND))

/*
 * convert_netmach_msg_to_actual_mach()
 *
 * Convert all transferred ports, out-of-line data, etc.
 * The two header ports have been converted by the caller
 * The incoming message has had the arch and type field and the two mnetport
 * structures popped off.  The "outmsg" is used only for the remote and
 * local ports.  This routine returns a pointer to fully formed outgoing
 * message.
 * 
 */
static mach_msg_header_t *
convert_netmach_msg_to_actual_mach(Msg *netmsg, mach_msg_header_t *outmsg,
				   int port_count, int notinline_count,
				   int xfercompletion,
				   mn_arch_tag_t msgarch,
				   msg_id_t msgid,
				   IPhost   srchost,
				   mach_port_right_t local_right,
				   bool rpc_p)
{
  long xk_msg_length;
  int  mach_msg_length;
  struct mach_msg_big_t *new_mach_msg;
  mach_port_type_t remote_right;
  mach_msg_header_t tmphdr;
  kern_return_t ret;

  xTrace0(machripcp, TR_FULL_TRACE, "convert_netmach_to_actual_mach");

  xk_msg_length = msgLen(netmsg);
  mach_msg_length = xk_msg_length;

  if (outmsg->msgh_remote_port == MACH_PORT_NULL) {
    local_right = MACH_PORT_TYPE_SEND_ONCE;
  }
  else {
    if ((ret=mach_port_type(mach_task_self(), outmsg->msgh_remote_port,
			   &remote_right))
      != KERN_SUCCESS) {
      xTrace2(machripcp, TR_ERRORS, "machripc: netmach_to_actualmach: could not get port type for port %x failure code %x hex", outmsg->msgh_remote_port, ret);
    }
    else xTrace2(machripcp, TR_DETAILED, "machripc: netmach_to_actualmach: got port right of type %x for remote port %x", remote_right, outmsg->msgh_remote_port);
    if (local_right != MACH_PORT_NULL && !is_send(local_right))
    {
      xTrace2(machripcp, TR_ERRORS, "machripc: convert_netmach: inappropriate port %x local rights %x",
	      outmsg->msgh_local_port, local_right);
      xk_msg_length = 0;
      xTrace1(machripcp, TR_ERRORS, "machripc: xkmsglen %d", msgLen(netmsg));
    }
  }
  if (xk_msg_length < 0) {
    xTrace2(machripcp, TR_ERRORS, "machnetipc: convert_netmach_msg_to_actual_mach: bogus length %d dec(%x hex)", xk_msg_length, xk_msg_length);
    xk_msg_length = 0;
  }

  if (msgarch == MN_ARCH_MARKER) {
    if (port_count || notinline_count)
      /* must convert ports and ool data */
      {
	xTrace2(machripcp, TR_ERRORS, "machripc: convert_netmach_msg_to_actual_mach: not simple - ports %d ool %d",
		port_count, notinline_count);
	new_mach_msg = (struct mach_msg_big_t *)
	  xk_ports_and_ool_convert(netmsg, &tmphdr, port_count,
				   notinline_count, msgid, srchost,
				   xfercompletion);
	if (!new_mach_msg) return 0;
      }
    else {
      msgPop(netmsg, (Pfl)mycopypop_andlie, (char *)&tmphdr, sizeof(tmphdr), (void *)0);
      if (tmphdr.msgh_size>0) {
	new_mach_msg = (struct mach_msg_big_t *) xMalloc( tmphdr.msgh_size );
	bzero((char *)new_mach_msg, tmphdr.msgh_size);
	msgPop(netmsg, (Pfl)mycopypop, (char *)new_mach_msg,
	       tmphdr.msgh_size, (void *) 0);
	xTrace3(machripcp, TR_DETAILED, "machripc convert netmach: msgLen %d, tmphdr.msgh_size %d, computed data msg_length %d",
		xk_msg_length, tmphdr.msgh_size, mach_msg_length);
	if (xk_msg_length != tmphdr.msgh_size)
	  xTrace2(machripcp, TR_SOFT_ERRORS, "machripc convert netmach: msg length mismatch: xk length %d mach length %d",
		  xk_msg_length, tmphdr.msgh_size);
      }
      else return 0;
    }
  }
  else {
    /* must convert each item according to architecture */
    xTrace2(machripcp, TR_ERRORS, "machripc: netdata different architecture:  ports %d ool %d",
	    port_count, notinline_count);
    new_mach_msg = (struct mach_msg_big_t *)
      xk_netmach_msg_to_mach(netmsg, &tmphdr, local_right,
			     port_count, notinline_count,
			     msgarch);
    if (!new_mach_msg) return 0;
  }

#define XKMOVERIGHT(right) ((right & MACH_PORT_TYPE_SEND_ONCE)? \
                                    MACH_MSG_TYPE_MOVE_SEND_ONCE: \
                                    MACH_MSG_TYPE_MOVE_SEND)
#define XKMOVEREPLYRIGHT(right) ((right & MACH_PORT_TYPE_SEND_ONCE)? \
                                    MACH_MSG_TYPE_MAKE_SEND_ONCE: \
                                    MACH_MSG_TYPE_MOVE_SEND)
  new_mach_msg->mmhdr.msgh_remote_port = outmsg->msgh_remote_port;
  new_mach_msg->mmhdr.msgh_local_port = outmsg->msgh_local_port;
  new_mach_msg->mmhdr.msgh_bits =
    MACH_MSGH_BITS(XKMOVERIGHT(remote_right), XKMOVEREPLYRIGHT(local_right));
  if (port_count || notinline_count) 
    new_mach_msg->mmhdr.msgh_bits != MACH_MSGH_BITS_COMPLEX;

  xTrace5(machripcp, TR_DETAILED, "machripc convert netmach: using msgh bits %x hex for remote port right %x local port right %x remote port %x local port %x",
	  new_mach_msg->mmhdr.msgh_bits,
	  remote_right, local_right,
	  new_mach_msg->mmhdr.msgh_remote_port,
	  new_mach_msg->mmhdr.msgh_local_port);
  xTrace2(machripcp, TR_DETAILED, "machripc convert netmach: received mach message length is %d dec (%x hex)",
	  new_mach_msg->mmhdr.msgh_size, new_mach_msg->mmhdr.msgh_size);

  return (mach_msg_header_t *)new_mach_msg;
}

/*
 *  msgid_assign()
 *
 *
 */
static msg_id_t
msgid_assign()
{
  static msg_id_t ident = 0;

  xTrace2(machripcp, TR_EVENTS, "msgid_assign %d (0x%x)", ident, ident);
  return ident++;
}


/*
 *  is_machhdr(msg, rright)
 *
 *   determine if a Mach message has expected reply form
 *
 */
#define is_machhdr(port, rright) (port == MACH_PORT_NULL || (rright & (MACH_PORT_TYPE_SEND_ONCE | MACH_PORT_TYPE_SEND)))

/*
 *  mnnetportcopy_out
 *
 *       put a port descriptor into a network header
 *       this goes in host byte order; receiver will use the
 *       archtype marker to get them into the destination byte order
 */
static void mnportcopy_out(msghdr, netport)
     char *msghdr;
     mnetport *netport;
{
  bcopy((char *)&netport->net_port_number,  msghdr, PORTID_NETLEN);
  bcopy((char *)&netport->net_port_rights, msghdr += PORTID_NETLEN,
	PORTRIGHT_NETLEN);
  bcopy((char *)&netport->receiver_host_addr, msghdr + PORTRIGHT_NETLEN,
	sizeof(IPhost));
}

/*
 * convert_mach_msg_to_netmach_msg
 *          (netmsg, xmsg, outmsg, remote_port, dest, msgtype)
 *
 *    creates outgoing message header and a network data msg structure
 *
 *    converts all transferred ports (except fast reply port),
 *      out-of-line data, etc. 
 * 
 */
static void
convert_mach_msg_to_netmach_msg (
  struct mach_msg_big_t  *msg,
  Msg                    *xkmsg,   /* NB msg *IS* the data in xkmsg */
  machnetipc_hdr         *outmsg,
  mnetport               *remote_port,
  IPhost		  dest,
  enum MACHIPCTYPE	  msgtype)
{
  mnetport         *repport, *remport;
  mach_port_right_t reply_right;

  xTrace0(machripcp, TR_FULL_TRACE, "convert_mach_msg_to_netmach_msg");

  outmsg->hdr.architecture_type = PERM1;
  outmsg->hdr.machnetipcmsg_type = MACHRIPC_MSG;

  if (msg->mmhdr.msgh_remote_port == MACH_PORT_NULL)
    reply_right = 0;
  else
    if (mach_port_type(mach_task_self(), msg->mmhdr.msgh_remote_port, &reply_right) !=
	KERN_SUCCESS) {
      xTrace1(machripcp, TR_ERRORS, "machripc convert_mach_msg: Could not get local port %x type",
	      msg->mmhdr.msgh_local_port);
      return;
    }
  reply_right &= ~MACH_PORT_TYPE_RECEIVE; /* can't transfer receive right here*/

  if (!is_machhdr( msg->mmhdr.msgh_remote_port, reply_right))
    {
      xTrace0(machripcp, TR_ERRORS, "machnetipc convert_mach_to_netmach: using inappropriate right; cannot handle this");
      xTrace5(machripcp, TR_DETAILED, "machripc convert parameters port %x number%x inline %x right %x send_once representation %x",
	      msg->mmhdr.msgh_remote_port,
	      msg->mmbody.msgt_number,
	      msg->mmbody.msgt_inline,
	      reply_right, MACH_PORT_TYPE_SEND_ONCE);
      return;
  }

  if (msgtype != MACHRIPC_RPC_REPLY_MSG) {
    mnportcopy_out(&outmsg->content.machmsg.transport_port, remote_port);
    mnportcopy_out(&outmsg->content.machmsg.reply_port, &Null_Netport);
    if (msgtype != MACHRIPC_RPC_REQUEST_MSG && /* requests have fast-sendonce*/
	(remport = quick_port_convert(&(msg->mmhdr.msgh_local_port)))
	== (mnetport *)NULL)
      {
	/* this must be an error; if this task knows the port, it must be
	   already be a netport */
	xTrace1(machripcp, TR_ERRORS, "machnetipc convert_mach_msg cannot find reply port descriptor %x", msg->mmhdr.msgh_remote_port);
	outmsg->content.machmsg.transport_port = *((mn_netport_t *)&Null_Netport);
	return;
      }
#ifdef PORTLOCKS
    /* don't need to lock fast send-once ports */
    if (readerLock(remport->rwlock_ptr) != XK_SUCCESS) {
      xTrace0(machripcp, TR_ERRORS, "machnetipc: convert_mach_msg: remote port died during message conversion");
      return;
    }
#endif PORTLOCKS
    outmsg->content.machmsg.transport_port = *((mn_netport_t *)remport);
  }
  outmsg->content.machmsg.transport_port.net_port_rights = 
    ONESEND(  outmsg->content.machmsg.transport_port.net_port_rights );
  outmsg->content.machmsg.netport_count = 0;
  xTrace1(machripcp, TR_FULL_TRACE, "machnetipc: convert_mach_msg length %d", msg->mmhdr.msgh_size);
  xTrace1(machripcp, TR_FULL_TRACE, "machnetipc: convert_mach_msg new length %d", msg->mmhdr.msgh_size);

  /* convert the reply port, favoring RPC-style messages */

  if (reply_right == MACH_PORT_TYPE_SEND_ONCE
      && msgtype != MACHRIPC_RPC_REPLY_MSG) /* your standard client request */
    {
      outmsg->hdr.machnetipcmsg_type = MACHRIPC_RPC_REQUEST_MSG;
      xTrace0(machripcp, TR_DETAILED, "machnetipc: convert msg to netmsg: RPC Request msg");
      /* the reply right should be filled in by caller, using fast so */
    }
  else {
    if ( msgtype == MACHRIPC_RPC_REPLY_MSG )
      {
	/* your typical server reply */
	/* reply port must be null for rpc_reply */
	xTrace0(machripcp, TR_DETAILED, "machnetipc: convert msg to netmsg: RPC Reply msg");
	outmsg->hdr.machnetipcmsg_type = MACHRIPC_RPC_REPLY_MSG;
	outmsg->content.machmsg.reply_port.net_port_number = 0;
	/* the transport port will be filled in by caller, using
	   fast send_once rights */
      }
    else {
      if ((repport = quick_port_convert(&(msg->mmhdr.msgh_remote_port)))
	  == (mnetport *)&Null_Netport)
	{
	  /* this is a new right */
	  mnetport mnport;

	xTrace2(machripcp, TR_FUNCTIONAL_TRACE, "machripc converted new reply port descriptor for port %x right %x", msg->mmhdr.msgh_remote_port, reply_right);
	if (msg->mmhdr.msgh_remote_port != MACH_PORT_NULL) {
	  mnport = convert_to_netport(msg->mmhdr.msgh_remote_port,
				      ONESEND(reply_right), dest, 0, 0);
	  outmsg->content.machmsg.reply_port = *((mn_netport_t *)&mnport);
	  outmsg->content.machmsg.reply_port.net_port_rights =
	    ONESEND(outmsg->content.machmsg.reply_port.net_port_rights);
	}
	else
	  outmsg->content.machmsg.reply_port = *((mn_netport_t *)&Null_Netport);
      }
    else {
      xTrace3(machripcp, TR_FUNCTIONAL_TRACE, "machnetipc: convert_msg_to_net: found existing reply port descriptor for port %x right %x desc right %x", msg->mmhdr.msgh_remote_port, reply_right, repport->net_port_rights);
      if (repport != &Null_Netport
#ifdef PORTLOCKS
	  && readerLock(repport->rwlock_ptr) == XK_SUCCESS
#endif PORTLOCKS
	  ) {
	if (!(repport->net_port_rights & reply_right)) {
	  xTrace0(machripcp, TR_FUNCTIONAL_TRACE, "machnetipc: convert_msg_to_net: needs to add new right to existing right old");
	  if ((repport->net_port_rights == MACH_PORT_TYPE_SEND_ONCE) ||
	      (reply_right == MACH_PORT_TYPE_SEND_ONCE)) {
	    xTrace0(machripcp, TR_ERRORS, "machnetipc: convert_msg_to_net: Cannot add send_once right");
	  }
	  else xTrace0(machripcp, TR_ERRORS, "machnetipc:convert_msg_to_net: right addition not supported");
	}
	portm_add_sender(repport, dest); /* updates the make_send_count */
	outmsg->content.machmsg.reply_port = *((mn_netport_t *)repport);
      }
      else {
	xTrace0(machripcp, TR_EVENTS, "machnetipc: convert_msg_to_net: null reply port or reply port died");
	repport = &Null_Netport;
      }
    }
    }
  }
  outmsg->content.machmsg.message_id = msgid_assign();

  if (msg->mmhdr.msgh_bits & MACH_MSGH_BITS_COMPLEX) /* if necessary, add ports, ooldata */
    xk_mach_msg_convert(msg, xkmsg, outmsg, dest);

    /* with fast send_once, caller does this */
  if ( msgtype != MACHRIPC_RPC_REPLY_MSG &&
    remport->net_port_rights == MACH_PORT_TYPE_SEND_ONCE ) {
    xTrace2(machripcp, TR_DETAILED, "machnetipc: convert_msg_to_net: Removing network send_once right for port number %x mach_port %x", remote_port->net_port_number, remote_port->real_send_port);
    /* remove from database; less interest in old sender
       and deallocate the remport structure */
    portm_port_remove_send_once(remote_port);
  }
  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: convert msg to netmsg exits");
}

/*
 *
 * machr_control
 *
 */
static int
machr_control(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
  xTrace0(machripcp, TR_FULL_TRACE, "machr_control called");
    xAssert(xIsProtocol(self));
    switch (opcode) {
      default:
	return xControl(xGetDown(self, MNI_TRANSPORT_DNUM), opcode, buf, len);
    }
}

/*
 *
 * machr_close
 *
 */
static xkern_return_t
machr_close(self)
    XObj self;
{
  xTrace0(machripcp, TR_FULL_TRACE, "machr_close called");
  /* xFree((char *)self); */
  return XK_SUCCESS;
}

/*
 *
 * machr_opendone
 *
 */
static xkern_return_t
machr_opendone(self, session, llp, session_type)
{
  xTrace0(machripcp, TR_FULL_TRACE, "machr_opendone called");
  return XK_SUCCESS;
}

/*
 *  Initialization utilities
 *
 *     getproc, machr_session_init
 *
 */

static void
getproc(XObj p)
{
    xAssert(p->type == Protocol);
    p->push = machr_push;
    p->open = machr_open;
    p->opendone = machr_opendone;
    p->openenable = machr_openenable;
    p->demux = machr_demux;
    p->calldemux = machr_calldemux;
    p->control = machr_control;
}

static void
machr_session_init(XObj p, Mach_ActiveId *state)
{
/*    xAssert(p->type == Protocol); */
    xTrace0(machripcp, TR_FULL_TRACE, "machr_session_init");
    p->push = machr_push;
    p->open = machr_open;
    p->close = machr_close;
    p->openenable = machr_openenable;
    p->demux = machr_demux;
    p->calldemux = machr_calldemux;
    p->control = machr_control;
    if (state == 0)
      (Mach_ActiveId *)(p->state) = TYPE_MALLOC(Mach_ActiveId);
    else (Mach_ActiveId *)(p->state) = state;
}

/*
 *  Utility Routines of various sorts
 *
 *
 */

static void
subtime(t1, t2, t3)
    XTime *t1, *t2, *t3;
{
    t3->sec = t2->sec - t1->sec;
    t3->usec = t2->usec - t1->usec;
    if (t3->usec < 0) {
	t3->usec += 1000000;
	t3->sec -= 1;
    }
}


