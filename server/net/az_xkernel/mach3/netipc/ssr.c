/*
 *
 * ssr-simple.c
 *
 * based on
 * x-kernel v3.2
 *
 * Copyright 1993  University of Arizona Regents
 *
 * ssr version for v3.2 by Hilarie Orman Fri Oct 25 14:51:08 1991
 * initial working version Mon Mar 30 17:22:06 1992
 *
 */

/*
 *
 * SSR - the simple server registry for Mach
 *
 * this allows the association of a network service with a port on a particular
 * machine (service, machine, port).
 * Servers and clients initially contact the SSR to transfer the port
 * from the server to the client.  The SSR port is "well-known" Mach port,
 * available to local clients.
 *
 *  host1: server-(well-known port)->SSR, registers (server_port, service_id)
 *  host2: client-(well-known port)->SSR, requests (host1, service_id), and
 *                                        registers reply_port
 *  host2: SSR composes ssr_request message and sends it to host1 via the
 *   machripc transport service.
 *  host1: SSR-(server_port)->server, transfers reply_port (receiver on host2)
 *  host1: server-(reply_port)->host2:client, transfers ordinary Mach message
 *
 *  The communication over the well-known SSR port is assumed to be local to
 *  a single machine, although there is no actual limitation.
 */

/*
 *  TBD: request no senders notification and use it to relay
 *       no more senders to holder of receive right, ...
 *
 *       keep track of make send count? how? we have passed the receive
 *       right along to a server that won't necessarily give it back
 *       that means that this module must always hold a send right, to
 *       prevent a no more senders notification?  and give it up when all
 *       outstanding network send rights are destroyed?
 */

/*
 *
 * Look up error codes in :
 *
 * /usr/mach3/mk/src/latest/kernel/mach/kern_return.h
 *
 */

#include <stdio.h>
#include <mach.h>
#include <mach/message.h>
#include <mach/mach_port.h>
#include <cthreads.h>
#include <servers/netname.h>
#include "xkernel.h"
#include "ssr_internal.h"
#define SSRMODULE
#include "ssr.h"

mach_port_right_t server_registry_port;

#define pass_open_ho(part) (xOpenEnable(myProtl, myProtl, xGetDown(myProtl, 0), part))

#define LOCAL_PART  1
#define REMOTE_PART 0

#define IP_NULL { 0, 0, 0, 0 };

int tracessrp;

static XObj myProtl;

static XTime starttime;

static char name_buffer[SS_NAME_SIZE];

#define IPEQUAL(_a, _b) ((_a.a == _b.a) && (_a.b == _b.b) && \
			      (_a.c == _b.c) && (_a.d == _b.d))

typedef	long (*Pfl) ();

#define STATEPTR   ((struct ssr_state *)(self->state))
#define STATEPTR_S(_selfptr) ((struct ssr_state *)(_selfptr->state))
#define TYPE_MALLOC(type) (type *)xMalloc(sizeof(type))

/* the simplest functions to use with msgPop and msgPush */
/* mycopypop() */
static long
mycopypop(char *to, char *from, long len, void *arg) {
  if ((int)len == len) {
    xTrace3(ssrp, TR_FULL_TRACE, "copy pop %x to %x len %d", from, to, len);
    bcopy(from, to, len);
    return(len);
  }
  else xTrace1(ssrp, TR_ERRORS, "Untenable len for copy function %ld", len);
  return((long)0);
}

/* mycopypop_andlie() */
/*    this is a message "peek" */
static long
mycopypop_andlie(char *to, char *from, long len, void *arg) {
  if ((int)len == len) {
    xTrace3(ssrp, TR_FULL_TRACE, "copy pop and lie %x to %x len %d", from, to, len);
    bcopy(from, to, len);
    return(0);
  }
  else xTrace1(ssrp, TR_ERRORS, "Untenable len for copy function %ld", len);
  return((long)0);
}

/* mycopypush() */
static long
mycopypush(char *from, char *to, long len, void *arg) {
  if ((int)len == len) {
    xTrace3(ssrp, TR_FULL_TRACE, "copy push %x to %x len %d", from, to, len);
    bcopy(from, to, len);
    return(len);
  }
  else xTrace1(ssrp, TR_ERRORS, "Untenable len for copy function %ld", len);
  return((long)0);
}

/* forward */
static void ssr_process_user_request(struct local_ssr_msg *msg);
static void service_request_listen();
static void getproc(XObj p);
static void connect_to_machr();

/* 
 * ssr_init()
 *
 * contacts the transport protocol (machripc) and
 * starts a listening thread
 *
 */
kern_return_t
ssr_init(XObj self)
{
  char arr[24];
  IPhost ipaddr, nametoip();
  struct hostent *hostEnt;
  Part whom[1];
  struct { int port, host; } destination;
  mach_msg_type_name_t returned_type;
  kern_return_t ret;
  
  xTrace0(ssrp, TR_FULL_TRACE, "ssr registry server init");

/* 
 *    registry_service
 *
 * the registry service waits for local processes to contact it via
 * its "well-known" port to register to act as servers or to make
 * requests.
 * Register operations require no network operations;
 * requests must be put into ssr format and sent to destination host
 */

  self->state = (char *)((struct ssr_state *)
			 xMalloc(sizeof (struct ssr_state)));
  if (self->state == (struct ssr_state *)ERR_XMALLOC)
    Kabort("ssr malloc failure in init");

  /* later we will use mapBind and mapResolve to register/unregister */
  ((struct ssr_state *)(self->state))->service_map_hash =
    mapCreate(SS_MAP_SIZE, SS_NAME_SIZE);

  if ( (ret = mach_port_allocate(mach_task_self(),
			  MACH_PORT_RIGHT_RECEIVE,
			  &(STATEPTR->wk_port))) != KERN_SUCCESS)
    {
      xTrace1(ssrp, TR_ALWAYS, "port allocate failed code %d\n", ret);
      Kabort("ssr: could not allocate master port, exiting");
      _exit(0); /* and suspenders */
    }
  if ((ret = mach_port_extract_right(mach_task_self(),
				     STATEPTR->wk_port,
				     MACH_MSG_TYPE_MAKE_SEND,
				     &(STATEPTR->wk_port_send_right),
				     &returned_type))
      != KERN_SUCCESS)
    {
      xTrace1(ssrp, TR_ALWAYS, "port extract failed code %d\n", ret);
/*      Kabort("ssr: could not extract master port send right, exiting"); */
/*      _exit(0); */ /* and suspenders */
    }
  server_registry_port = STATEPTR->wk_port_send_right; /* this is global to all
					       instances of the protocol */
  xTrace1(ssrp, TR_FUNCTIONAL_TRACE, "the ssr registry port is %x", server_registry_port);

  STATEPTR->service_map = (mach_port_t *)xMalloc(SERVICE_MAX * sizeof(mach_port_t));

  /* set up the interface function pointers */
  myProtl = self;
  getproc(myProtl);
  ret = netname_check_in(name_server_port, "simple_server",
			mach_task_self(), server_registry_port);
  if (ret != KERN_SUCCESS)
    xTrace1(ssrp, TR_ERRORS, "ssr: netname_check_in: %s\n",
	    mach_error_string(ret));
  
  /* now we start a listening thread */
  service_request_listen();
  evDetach(evSchedule(connect_to_machr, (void *)0, 0));
  xTrace0(ssrp, TR_MAJOR_EVENTS, "ssr init completes");
  return(XK_SUCCESS);
}

struct user_request {
  struct user_request   *self_ptr;
  int			service_id;
  int			operation;
  IPhost		host_addr;
  mach_port_right_t	port;
};

/*
 * connect_to_machr
 *
 * Let machr know that SSR is here and listening
 *
 */
static void
connect_to_machr()
{
  Part dummy[1];

  xTrace0(ssrp, TR_MAJOR_EVENTS, "ssr: connect_to_machr");
  partInit(dummy, 1);
  if (pass_open_ho(dummy) == XK_FAILURE)
    { xTrace0(ssrp, TR_ERRORS, "ssr: connect_to_machr failed"); }
  else
    xTrace0(ssrp, TR_FULL_TRACE, "ssr: connect_to_machr succeeded");
}


/*
 * ssr_mach_msg_receive()
 *
 * the SSR performs a send/listen loop for the Mach msg reply
 *  this is a thread independent of the master xkernel control
 *
 */
static void
ssr_mach_msg_receive(struct local_ssr_msg *smsg)
{
  kern_return_t ret;
  cthread_t child;
  int *args;
  int error = 0;
  struct local_ssr_msg *nmsg;

  xTrace1(ssrp, TR_FULL_TRACE, "ssr_mach_msg_receive %x", smsg);

  while (smsg != (struct local_ssr_msg *)0 && !error) {
    bzero((char *)smsg, sizeof(struct local_ssr_msg));
    smsg->mmhdr.msgh_local_port = ((struct ssr_state *)(myProtl->state))->wk_port;
    smsg->mmhdr.msgh_size = sizeof(struct local_ssr_msg);
    if ((ret=mach_msg_receive(smsg)) == KERN_SUCCESS)
    {
      xTrace1(ssrp, TR_EVENTS, "SSR received %d", smsg->ssrd.operation);
      if (smsg->mmhdr.msgh_size == 0) continue;
      /* nmsg will be freed by ssr_process_user_request */
      xk_master_lock();
      nmsg = TYPE_MALLOC(struct local_ssr_msg);
      *nmsg = *smsg;
      ssr_process_user_request(nmsg);
      xk_master_unlock();
    }
    else {
      error = 1;
      xk_master_lock();
      xFree((char *)smsg);
      xTrace2(ssrp, TR_ALWAYS, "error from ssr mach msg recv %x (%d)", ret, ret);
      xk_master_unlock();
    }
  }
  xTrace0(ssrp, TR_ERRORS, "ssr mach msg recv terminates");
}

/*
 * ssr_process_user_request
 *
 * does the work of responding to user requests
 */
static void
ssr_process_user_request(struct local_ssr_msg *msg)
{
  mach_port_t reply_port = msg->mmhdr.msgh_remote_port;
  int service_id = msg->ssrd.service_id;
  int operation = msg->ssrd.operation;
  int size = msg->mmhdr.msgh_size;
  IPhost destination_host = msg->ssrd.destination_host;
  mach_port_right_t *smap = ((struct ssr_state *)(myProtl->state))->service_map;
  Msg  xkmsg;
  XObj ls;

  xTrace0(ssrp, TR_FULL_TRACE, "ssr_process_user_request");
  xTrace4(ssrp, TR_EVENTS, "ssr operation %d id %d size %d msg %x", operation,
	  service_id, size, msg);
  xTrace2(ssrp, TR_EVENTS, "ssr remote (reply) port %x, local port %x", reply_port, msg->mmhdr.msgh_local_port);

  bzero((char *)&xkmsg, sizeof(Msg));

  switch (operation) {
    case SHUTDOWN:
    /* used only during testing */
      Kabort("ssr shutdown command");
      break;
    case REGISTER:
      xTrace1(ssrp, TR_EVENTS, "ssr_process_user_request: registering port %x", reply_port);
      if (service_id >= SERVICE_MIN && service_id < SERVICE_MAX) {
	if (smap[service_id]) {
	  xTrace1(ssrp, TR_EVENTS, "service id re-registered %d", service_id);
	  mach_port_destroy(mach_task_self(), smap[service_id]);
	}
	smap[service_id] = reply_port;
      }
      else
	xTrace1(ssrp, TR_ERRORS, "service id out of range %d", service_id);
      break;
    case UNREGISTER:
      if (service_id >= SERVICE_MIN && service_id < SERVICE_MAX) {
	/* worry about threads using the port?? */
	if (smap[service_id])
	  mach_port_deallocate(mach_task_self(), smap[service_id]);
	smap[service_id] = 0;
      }
      else
	xTrace1(ssrp, TR_ERRORS, "service id out of range %d", service_id);
      break;
    case REQUEST:
      {
	Part machwhom[2];
	XObj machsession;

	xTrace4(ssrp, TR_FULL_TRACE, "ssr remote host %d.%d.%d.%d",
		destination_host.a, destination_host.b, destination_host.c,
		destination_host.d);
	xTrace1(ssrp, TR_FULL_TRACE, "ssr service id %d\n", msg->ssrd.service_id);
	msgConstructBuffer(&xkmsg, (char *)&(msg->ssrd), sizeof(struct ssrdata) + MAX_SSR_DATA);
	xTrace1(ssrp, TR_FULL_TRACE, "ssr msglen %d\n", msgLen(&xkmsg));
	partInit(machwhom, 2);
	{
	  long relpronum = relProtNum(myProtl, xGetDown(myProtl, 0));

	  partPush(machwhom[REMOTE_PART], &(msg->ssrd.destination_host), sizeof(IPhost));
	  partPush(machwhom[LOCAL_PART], &reply_port, sizeof(mach_port_t));
	  partPush(machwhom[LOCAL_PART], &relpronum, sizeof(long));
	  /* could push the service_id onto the remote part */
	  if ((machsession = xOpen(myProtl, myProtl, xGetDown(myProtl, 0), machwhom))
	      != ERR_XOBJ) {
	    xPush(machsession, &xkmsg);
	    xClose(machsession);
	  }
	  else {
	    xTrace0(ssrp, TR_ERRORS, "Open of machr session failed");
	  }
	  msgDestroy(&xkmsg);
	}
      }
      break;
    default: xTrace1(ssrp, TR_SOFT_ERROR, "bad client request type %d", operation);
      break;
    }
    xFree((void *)msg); /* allocated in ssr_msg_recv */
  }

/* 
 * service_request_listen
 *
 * starts a listening thread - blocks without sledgehammer control
 * this is platform dependent, of course.
 *
 *  Have to make this take a larger message so we can piggy-back
 *   the first client request to the server
 *
 */
static void
service_request_listen()
{
    cthread_t child;
    struct local_ssr_msg *smsg = TYPE_MALLOC(struct local_ssr_msg);

    xTrace0(ssrp, TR_FULL_TRACE, "SSR beginning Mach port send/listen");

    child = cthread_fork((cthread_fn_t)ssr_mach_msg_receive, smsg);
    /* this thread will do a blocking receive, returning to
       sledgehammer control when the receive completes
     */
    cthread_set_name(child,"msg receive thread");
    cthread_detach(child);
    xTrace1(processcreation, TR_ERRORS, "SSR test created client thread_id: %d",
	    child);
    xTrace1(processswitch, TR_ERRORS, "SSR test created client  thread_id: %d", child);
}

/*
 * the null function
 */
static void
null(s,m,length)
int s, length;
char *m;
{
}

/* 
 *  SSR_DEMUX 
 *  
 *  Not much to do here; just push the message on up
 *  to the registered server, if any.
 *  We use mach_msg_send, because by now we should have
 *  a real mach msg embedded in the xkernel msg.
 *  The message contains a reply port and not much else; we
 *  will be transferring the port right to the registered server.
 *  
 */
static xkern_return_t
ssr_demux(XObj self_protocol, XObj lls, Msg *machdg)
{
    struct server_msg mmsg;
    struct ssrdata regdata;
    mach_port_right_t server_port;
    mach_msg_return_t ret;
    mach_port_type_t  local_port_type;
    unsigned short    extraction_type = MACH_MSG_TYPE_MOVE_SEND_ONCE;
    kern_return_t kret;
    
    xTrace0(ssrp, TR_MAJOR_EVENTS, "ssr demux");
    /* message is: reply_port, ssr data, user data
     * pop off the mach port and the ssr data, check it,
     * send the user data to the server as a simple mach message,
     *  using the incoming_port from machdg as the mach msg reply port
     */
    msgPop(machdg, (Pfl)mycopypop, (void *)&mmsg.mmhdr.msgh_local_port, sizeof (mach_port_right_t), (void *)0);

   if ((kret=mach_port_type(mach_task_self(), mmsg.mmhdr.msgh_local_port,
			   &local_port_type))
       != KERN_SUCCESS)
     {
       xTrace2(ssrp, TR_ERRORS, "ssr_demux: could not get port type for port %x failure code %x hex", mmsg.mmhdr.msgh_local_port, kret);
     }
   else xTrace2(ssrp, TR_DETAILED, "machripc: netmach_to_actualmach: got port right of type %x for reply port %x", local_port_type, mmsg.mmhdr.msgh_local_port);
   if (local_port_type & MACH_PORT_TYPE_RECEIVE)
     extraction_type = MACH_MSG_TYPE_MAKE_SEND;

    msgPop(machdg, (Pfl)mycopypop, (void *)&regdata, sizeof(struct ssrdata), 0);
    if (regdata.service_id < SERVICE_MAX &&
	regdata.service_id >= SERVICE_MIN) {
      if ( server_port = STATEPTR_S(self_protocol)->service_map[regdata.service_id])
	  {
	    xTrace5(ssrp, TR_EVENTS, "ssr demux: found service %d registered for port %x; reply port %x msglength %ld asking for %d bytes",
		    regdata.service_id,
		    server_port,
		    mmsg.mmhdr.msgh_local_port,
		    msgLen(machdg),
		    sizeof(struct ssrdata));
	    xIfTrace(ssrp, TR_DETAILED) { msgShow(machdg); }
	    if ( regdata.operation != REQUEST ) {
	      xTrace0(ssrp, TR_ERRORS, "ssr_demux receives non-request; forcing to request");
	    }
	    /* local_port was set by msgPop above */
	    mmsg.mmhdr.msgh_remote_port = server_port;
	    /* may need to compute "desired type" */
	    mmsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
						  extraction_type);
	    mmsg.mmhdr.msgh_size = sizeof(struct local_ssr_msg);
	    mmsg.mmhdr.msgh_kind = MACH_MSGH_KIND_NORMAL;
	    mmsg.mmbody.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
	    mmsg.mmbody.msgt_size = sizeof(struct server_msg);
	    mmsg.mmbody.msgt_number = 1;
	    mmsg.mmbody.msgt_inline = 1;
	    msgPop(machdg, (Pfl)mycopypop, (void *)&mmsg.server_data, msgLen(machdg), 0);
	    /* make sure we got a send right ... */
	    xk_master_unlock();
	    if ((ret = mach_msg_send(&mmsg)) != MACH_MSG_SUCCESS) {
	      xTrace3(ssrp, TR_ERRORS, "ssr mach msg send for port %x failed %x %d", mmsg.mmhdr.msgh_remote_port, ret, ret);
	      xk_master_lock();
	      return(XK_FAILURE);
	    }
	    xk_master_lock();
	    return(XK_SUCCESS);
	  }
      else
	/* should send back failure msg */
	xTrace1(ssrp, TR_EVENTS, "ssr demux failed to find service %d registered",
		regdata.service_id);
    }
    else
      xTrace1(ssrp, TR_SOFT_ERROR, "ssr demux service_id out of range %d\n",
	      regdata.service_id);
    return(XK_FAILURE);
}

/*
 * ssr_opendone()
 *
 * record the machripc session
 *
 */
static xkern_return_t
ssr_opendone(XObj self, XObj lls, XObj llp, XObj orig_type)
{
  /* machripc is the only lower protocol that we can talk to */
  STATEPTR->session = lls;
  return(XK_SUCCESS);
}

/*
 * ssr_open()
 *
 * synchronize with ssr-test
 *
 */
static XObj
ssr_open(XObj self, XObj hlp, XObj hlptype, Part *participants)
{
  long pr = *(long *)partPop(participants[LOCAL_PART]);

  /* ssr-test is the only upper protocol that we can talk to */
  xTrace0(ssrp, TR_FULL_TRACE, "ssr_open OK");
  xTrace1(ssrp, TR_MORE_EVENTS, "ssr_open %d", pr);
  return((XObj)XK_SUCCESS);
}

/*
 * ssr_openenable()
 *
 * synchronize with any ssr test procedures
 *
 */
static xkern_return_t
ssr_openenable(XObj self, XObj hlp, XObj hlptype, Part *participants)
{
  /* ssr-test is the only upper protocol that we can talk to */
  xTrace0(ssrp, TR_FULL_TRACE, "ssr_openenable OK");
  return(XK_SUCCESS);
}

/*
 * ssr_control
 */
int
ssr_control(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    xAssert(xIsProtocol(self));
    switch (opcode) {
      default:
	return xControl(xGetDown(self, 0), opcode, buf, len);
    }
}


static void
subtime(XTime *t1, XTime *t2, XTime *t3)
{
    t3->sec = t2->sec - t1->sec;
    t3->usec = t2->usec - t1->usec;
    if (t3->usec < 0) {
	t3->usec += 1000000;
	t3->sec -= 1;
    }
}

static void
getproc(XObj p)
{
    xAssert(p->type == Protocol);
    p->open = ssr_open;
    p->opendone = ssr_opendone;
    p->openenable = ssr_openenable;
    p->demux = ssr_demux;
    p->control = ssr_control;
}
