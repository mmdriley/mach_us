/*
 *
 * ssr-simple-test.c
 *
 * Copyright 1993  University of Arizona Regents
 *
 * ssr version for v3.2 by Hilarie Orman Fri Oct 25 14:51:08 1991
 * initial working version Mon Mar 30 17:22:44 1992
 * version with forwarding to third node Mon Aug 24 12:54:49 MST 1992
 * uses Mach nameserver, runs as standalone process Thu Dec 31 18:58:14 1992
 *
 */

/*********************************************************************
 *
 * tests the simple server registry
 *
 * one machine has a server register, another has a client connect to
 * server and receive a reply on a Mach port
 *
 *
 * The command line argument tells the test program which host to address
 * as the server.  The server will register locally for a service,
 * and the client will request a remote service.
 * A host can run both a client and a server.
 * 
 *********************************************************************
 */

#include <stdio.h>
#include <mach.h>
#include <mach/message.h>
#include <mach/mach_port.h>
#include <cthreads.h>
#ifdef USEXK
#include "xkernel.h"
#else
#include "unxk.h"
#include <netdb.h>
#include <sys/param.h>
#include <servers/netname.h>
#endif
#include "ssr.h"

/* use send_once rights for the postcard test */
#define TESTPOSTSO

#define TIME
#define TRIPS 10
int trips = TRIPS;

#ifdef TIME    
    XTime 	startTime, endTime, netTime;
#endif
static 	void	subtime(
#ifdef __STDC__
			XTime *t1, XTime *t2, XTime *result
#endif
			);


#define LOCAL_PART  1
#define REMOTE_PART 0
#define IP_NULL { 0, 0, 0, 0 };
IPhost Null_IPaddr = IP_NULL;

int tracessrtestp;

#define STATEPTR ((struct ssrt_state *)(myProtl->state))

char *SERVERNAME;
static XObj myProtl;

#ifdef USEXK
static void getproc(XObj);
#endif

static int gotone;
static int count;

#define TYPE_MALLOC(type) (type *)xMalloc(sizeof(type))

#define IPEQUAL(_a, _b) ((_a.a == _b.a) && (_a.b == _b.b) && \
			      (_a.c == _b.c) && (_a.d == _b.d))

#ifdef USEXK
extern mach_port_t server_registry_port;
#else
mach_port_right_t server_registry_port;
#endif
static mach_port_t        ns_port;

/* this task's request port */
static mach_port_t		ssr_test_port = 0;
/* this task's request port receive right */
static mach_port_right_t 	ssr_test_port_rcv_right = 0;
/* this task's request port send right */
static mach_port_right_t 	ssr_test_port_send_right = 0;

#ifdef USEXK
long relpronum; /* used in constructing a participant stack for open */
#endif

/* the contents of a "postcard" */
struct postcard_contents {
  char postcard[9];
  IPhost source_host;
  int sequence_num;
};

/* a full mach postcard */
struct mach_postcard_msg {
  mach_msg_header_t mmhdr;
  mach_msg_type_t   mmbody;
  enum ssr_operation_type op;
  struct postcard_contents contents;
};

/* a mach complex test msg
   to be kosher, each piece must be forced to a 4byte boundary,
   so that the mach headers can be aligned on receive operations
*/
struct mach_complex_msg {
  mach_msg_header_t mmhdr;
  struct {
    mach_msg_type_t   mmbody;
    enum ssr_operation_type op;
    struct postcard_contents contents;
  } part1;
  struct {
  mach_msg_type_t   mmbody2;
  struct postcard_contents contents2;
  } part2;
  struct {
  mach_msg_type_t   mmbody2;
  mach_port_t	    port;
  } part3;
};

/* another mach complex test msg
   to be kosher, each piece must be forced to a 4byte boundary,
   so that the mach headers can be aligned on receive operations
*/
struct mach_complex_ool_msg {
  mach_msg_header_t mmhdr;
  struct {
    mach_msg_type_t   mmbody;
    enum ssr_operation_type op;
    struct postcard_contents contents;
  } part1;
  struct {
  mach_msg_type_t   mmbody2;
  struct postcard_contents contents2;
  } part2;
  struct {
  mach_msg_type_t   mmbody2;  /* port */
  mach_port_t	    port;
  } part3;
  struct {
    mach_msg_type_t mmbody2;  /* ool data, bytes */
    vm_offset_t     datadr;
  } part4;
  struct {
    mach_msg_type_t mmbody2;  /* ool data, port */
    mach_port_t    *datadr;
  } part5;
  struct {
    mach_msg_type_t mmbody2;  /* ool data, ports */
    mach_port_t    *datadr;
  } part6;
};

/* a full mach postcard server request via server registry request: 
   mach hdr, ssr data, postcard server request  */
struct mach_server_postcard_msg {
  mach_msg_header_t mmhdr;
  mach_msg_type_t   mmbody;

  struct ssrdata ssrd;

  enum ssr_operation_type op;
  struct postcard_contents contents;
};

/* eine kleine staten */
struct ssrt_state {
    IPhost remote_host;
    IPhost local_ip_addr;
    IPhost hurray_host;
    mach_port_t  test_server_port;
    mach_port_t  ss_client_port;
    enum ssr_service_type service_id;
    struct mach_postcard_msg  savedMsg;
    char  *server_name;
    char   subtype;
    int    trips;
    int	   msg_size;
  };

struct server_args {
  int wait_time;
  mach_port_t rport;
  struct local_ssr_msg *msg;
  XObj self;
  };

typedef struct { int port, host; } SSRaddr;

/* forward */
static xkern_return_t init_state();
static void server_receive_msg();
static void server_got_msg(struct mach_postcard_msg *msg,
			   struct ssrt_state *state);
#ifdef USE_XK
static void server_listen(Event ev, struct server_args *args);
#else
static void server_listen(struct server_args *args);
#endif
static xkern_return_t isClient();
static xkern_return_t isServer();
#ifdef USEXK
static void getproc_client(XObj p);
static void getproc_server(XObj p);
#endif
static void client_listen();
static void ssr_make_msg_to_ssr_server(
     struct mach_server_postcard_msg *pmsg,
     IPhost host,
     enum ssr_operation_type operation,
     enum ssr_service_type service,
     enum ssr_operation_type server_operation,
     mach_port_t port);
static void client();
static void client_msg_receive(struct ssrt_state *state);
static void client_msg_receive_and_forward(struct ssrt_state *state);
static void client_complex_receive_and_send(struct ssrt_state *state);
static void client_msg_receive_and_forward_complex(struct ssrt_state *state);
static void client_very_complex_receive_and_send(struct ssrt_state *state);

#ifndef USEXK

int globalArgc;
char **globalArgv;
struct xobject myself;
xkern_return_t ssrtest_init(XObj);
int atoint(char*);

/*
 *   xkmach_check_status
 *
 *
 *
 */
static void
xkmach_check_status(mach_port_t port)
{
  mach_port_type_t pstat;
  kern_return_t ret;

  xTrace0(ssrtestp, TR_FULL_TRACE, "ssr tester: xkmach_check_status: entered");
  if ((ret = mach_port_type(mach_task_self(), port, &pstat))
      != KERN_SUCCESS) {
    xTrace2(ssrtestp, TR_ERRORS, "ssr tester: could not get port status for port %x ret %x",
	    port, ret);
	return;
  }
  xTrace2(ssrtestp, TR_ALWAYS, "ssr tester: port %x has type 0x%x", port, pstat);
}


/*
 *  main program
 *
 *
 *  start with -l 5 for tracelevel 5, -t v for instance 'v'
 *
 *         test -s -l 25 -t p                       ; postcard server
 *         test -l 25 -t p -p192.12.69.99 -trips 25 ; postcard client
 *         test -t v -v192.12.69.99                 ; very complex client
 *
 */
void main(argc,argv)
int argc; char **argv;
{ int i;
  globalArgc = argc; globalArgv = argv;

  for (i=0; i < globalArgc-1; i++) {
    if (!strncmp(globalArgv[i], "-l", 2))
    { tracessrtestp = atoint(globalArgv[++i]);
      printf("SSRtest using trace level %d.\n", tracessrtestp);
      break;
    }
  }

  for (i=0; i < globalArgc-1; i++) {
    if (!strncmp(globalArgv[i], "-t", 2))
    { 
      myself.instName = (char *)malloc(SS_NAME_SIZE);
      *myself.instName = *globalArgv[++i];
      *(myself.instName+1) = '\0';
      printf("SSRtest using name %c\n", *myself.instName);
      break;
    }
  }
  if (!myself.instName) {
    xTrace0(ssrtestp, TR_ERRORS, "ssr test failed to specifiy service id");
    return;
  }

  ssrtest_init(&myself);
 }

void xError(char *emsg) { printf(emsg); }

/* from merge/mach3/pxk/time.c */
void xGetTime(XTime *where) { gettimeofday(where,0) ; }

#endif

/*
 * ssr_test_init()
 *
 * Initialization of the ssr_test server and client
 *
 * Someday should run through a set of registrations, requests,
 *  unregisters, etc., driven from some test vector
 */
xkern_return_t
ssrtest_init(XObj self)
{
  char arr[24];
  IPhost ipaddr, nametoip();
  struct hostent *hostEnt;
  kern_return_t  kr;
  
  xTrace1(ssrtestp, TR_FULL_TRACE, "ssr test init %x\n", self);
  /* 
    start a server to listen to local tasks
    and a client to make requests

    The server initializes itself completely during initialization;
    the client starts a thread that will initialize later.
   */

#ifndef USEXK
  while (!server_registry_port) {
    kr = netname_look_up(name_server_port, "",
			 "simple_server", &server_registry_port);
    
    if (kr != KERN_SUCCESS) {
      printf("mnipc test program: netname_look_up: %s\n",
	     (char *)mach_error_string(kr));
      printf("mnipc test program will sleep a bit\n");
      sleep(30);
      printf("mnipc test program will retry\n");
    }
  }

  xkmach_check_status(server_registry_port);

#endif ~USEXK

  init_state(self);
  xTrace2(ssrtestp, TR_MAJOR_EVENTS, "ssr test init %x completes for %s",
	  self, ((struct ssrt_state *)(self->state))->server_name);
  if ( isServer(self) == XK_SUCCESS && isClient(self) == XK_SUCCESS)
    return(XK_SUCCESS);
  return(XK_FAILURE);
}


#ifndef USEXK

struct hostent *gethostbyname(char *);

void get_host_ip_address(char *where)
{ char name[MAXHOSTNAMELEN];
  struct hostent *ans;
  int i;

  gethostname(name, MAXHOSTNAMELEN);
  ans = gethostbyname(name);
  for (i=0;i<sizeof(IPhost);i++) where[i] = ans->h_addr[i];
}

/*
 * xAsyncThread
 *
 * platform dependent asynchronous thread startup
 *
 */
static void
xAsyncThread(Pfv func, void *args, char *name)
{ cthread_t child;

  child = cthread_fork((cthread_fn_t)func, args);
  cthread_set_name(child, name);
  cthread_detach(child);
}

#endif ~USEXK


/*
 * init_state
 *
 * sets up the master port and the protocol state block
 *
 */
static xkern_return_t
init_state(self)
     XObj self;
{
  struct ssrt_state *state;
  XObj   myProtl = self;

    xTrace0(ssrtestp, TR_FULL_TRACE, "SSR init");

    self->state = 0;
    self->state = state = TYPE_MALLOC(struct ssrt_state);
    state->msg_size = sizeof(struct local_ssr_msg);
    if (self->instName && *(self->instName) == 'p') {
      state->service_id = POSTCARD;
      state->server_name = "POSTCARD";
      state->subtype = 0;
      xTrace0(ssrtestp, TR_MAJOR_EVENTS, "POSTCARD state init");
    }

    if (self->instName && *(self->instName) == 'h') {
      state->service_id = SS_HURRAY;
      state->server_name = "HURRAY";
      state->subtype = 0;
      xTrace0(ssrtestp, TR_MAJOR_EVENTS, "HURRAY state init");
    }

    if (self->instName && *(self->instName) == 'x') {
      state->service_id = SS_COMPLEX;
      state->server_name = "COMPLEX";
      state->subtype = 0;
      xTrace0(ssrtestp, TR_MAJOR_EVENTS, "COMPLEX state init");
    }

    if (self->instName && *(self->instName) == 'o') {
      state->service_id = SS_OOL;
      state->server_name = "OUTOFLINE";
      state->subtype = 0;
      xTrace0(ssrtestp, TR_MAJOR_EVENTS, "OUTOFLINE state init");
    }

    if (self->instName && *(self->instName) == 'v') {
      state->service_id = SS_MOVE_RECEIVE;
      state->server_name = "MOVE RECEIVE";
      state->subtype = 0;
      xTrace1(ssrtestp, TR_MAJOR_EVENTS, "%s state init", state->server_name);
    }

    xTrace2(ssrtestp, TR_FULL_TRACE, "ssr test id %d, instName %c",
	    state->service_id, *(self->instName));

#ifdef USEXK
    relpronum = relProtNum(myProtl, xGetDown(self, 0));
    xControl(xGetDown(self, 0), GETMYHOST, (char *)&(STATEPTR->local_ip_addr), sizeof(IPhost));
#else
    get_host_ip_address((char *)&(STATEPTR->local_ip_addr));
#endif
    return(XK_SUCCESS);
  }

/* 
 * isServer()
 *
 *   an entry point that registers with SSR for a service type
 */
static xkern_return_t
isServer(self)
     XObj self;
{
  mach_msg_type_name_t returned_type;
  struct local_ssr_msg mach_msg;
  mach_port_t ssr_server_test_port_send_right;
  kern_return_t ret;
#ifdef USEXK
  Part whom[2];
#endif
  IPhost destination;
  XObj lower_session, myProtl;
  struct ssrt_state *state;
  struct server_args *sargs = TYPE_MALLOC(struct server_args);

  xTrace1(ssrtestp, TR_FULL_TRACE, "%s Server check: isServer", ((struct ssrt_state *)(self->state))->server_name);

  state = (struct ssrt_state *)self->state;
  state->trips = 0;

  if (globalArgc > 1 && globalArgv[1][0] == '-' && globalArgv[1][1] == 's')  {
    xTrace0(ssrtestp, TR_FULL_TRACE, "ssr_test_server init");
    if ( (ret = mach_port_allocate(mach_task_self(),
				   MACH_PORT_RIGHT_RECEIVE,
				   &(state->test_server_port))) != KERN_SUCCESS )
      { 
	xTrace1(ssrtestp, TR_ALWAYS, "ssr test: initial port allocation failed %d\n", ret);
	return(XK_FAILURE);
      }
    else {
      xTrace1(ssrtestp, TR_ALWAYS, "ssr test: initial port allocation got port %x", state->test_server_port);
#ifdef USEXK
      getproc_server(self);
      /* the passive open is subsumed by the Mach msg, but maybe it
	 should preserve the xkernel semantics and copy the participant list,
	 etc., in sort of an RPC-like way
	 */
      partInit(whom, 2);
      partPush(whom[LOCAL_PART], &relpronum); /* made up number */
      /*
	Don't really need to do an open, because all the
	communication will be via Mach ports.
	But the xOpen helps synchronize the two protocols, anyway
	Return from this should guarantee that the "well-known" SSR
	port is ready for use.
	It's well-known here because it's in the same address space,
	  but we should be using a port register function in the future.
	*/
      if ((lower_session = xOpen(self, self, xGetDown(self, 0), whom)) == ERR_XOBJ) {
	xTrace0(ssrtestp, TR_ALWAYS, "ssr test server can't open lower protocol");
	return XK_FAILURE;
      }
      xTrace0(ssrtestp, TR_EVENTS, "ssr test server done with xopen\n");
#endif
    
      /* register to be the postcard server */

      xTrace2(ssrtestp, TR_FULL_TRACE, "test server server_registry_port %x, want requests on port %x", server_registry_port, state->test_server_port);
      mach_msg.mmhdr.msgh_remote_port = server_registry_port;
      mach_msg.mmhdr.msgh_local_port = state->test_server_port;
      mach_msg.mmhdr.msgh_bits = 
	MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND);
      mach_msg.mmhdr.msgh_size = sizeof(mach_msg);
      mach_msg.mmbody.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
      mach_msg.mmbody.msgt_size = 32;  /* 32 bits */
      mach_msg.mmbody.msgt_number = 2; /* 2 items in ssrd relevant to server */
      mach_msg.mmbody.msgt_inline = 1;
      mach_msg.mmbody.msgt_longform = 0;
      mach_msg.mmbody.msgt_deallocate = 0;
      mach_msg.ssrd.operation = REGISTER;
      mach_msg.ssrd.service_id = state->service_id;
      xTrace0(ssrtestp, TR_FULL_TRACE, "ssr-test: server init will register");
      if ((ret = mach_msg_send(&mach_msg)) != MACH_MSG_SUCCESS) /* this might block  */
	{
	  xTrace3(ssrtestp, TR_ERRORS, "ssr test server register service %d send failed %x %d\n", state->service_id, ret, ret);
	}
      else {
	xTrace0(ssrtestp, TR_FULL_TRACE, "ssr test server will begin listen");
	sargs->self = self;
	sargs->wait_time = 0;
	sargs->rport = state->test_server_port;
	xTrace0(ssrtestp, TR_FULL_TRACE, "ssr test server will begin listen");
#ifdef USEXK
	evDetach(evSchedule((Pfv)server_listen, (void *)sargs, 0)); /* start new thread to listen for requests*/
#else
	xAsyncThread((Pfv)server_listen, (void *)sargs, (char *)0);
#endif
      }
      /* now this initial thread exits */
      xTrace0(ssrtestp, TR_EVENTS, "ssr-test: server init exits");
    }
  }
  return XK_SUCCESS;
}

#define MAXNUM(a,b) a>=b?a:b

/* 
 * server_listen
 *
 * a listening thread to do a "hard wait"
 * always entered with the master lock held
 *
 *
 */
static void
#ifdef USE_XK
server_listen(Event ev, struct server_args *args)
#else
server_listen(struct server_args *args)
#endif
  {
    cthread_t		child;
    struct mach_postcard_msg *msg;
    mach_port_t		rport = args->rport;
    mach_msg_return_t	ret;
    struct ssrt_state  *state = (struct ssrt_state *)(args->self->state);
    bool		fail = FALSE;
    int			size =  MAXNUM(state->msg_size,
				    sizeof(struct mach_postcard_msg));

    xTrace2(ssrtestp, TR_FULL_TRACE, "%s server beginning listen on port %x",
	    state->server_name, rport);
#ifdef USEXK
    xk_master_unlock();
#endif
    while (!fail) {
      msg = (struct mach_postcard_msg *)xMalloc(size);

      size =  MAXNUM(state->msg_size, sizeof(struct mach_postcard_msg));
      msg->mmhdr.msgh_size = size;
      msg->mmhdr.msgh_local_port = rport;
      xTrace3(ssrtestp, TR_EVENTS, "%s server msg_receive operation port %x, size %d",
	      state->server_name, rport, size);
      if ((ret = mach_msg_receive(msg)) == KERN_SUCCESS) {
	server_got_msg(msg, state);
      }
      else {
	fail = TRUE;
	xTrace4(ssrtestp, TR_ERRORS, "ssrtest server %s msg recv failed %d (0x%x) port %x",
		state->server_name, ret, ret, rport);
      }
    }
  }

/* 
 * server_got_msg()
 *
 * the function of the server is performed here.  It replies to the
 * incoming client message
 *
 * it should destroy the argument msg when done with it
 *
 */
static void
server_got_msg(struct mach_postcard_msg *msg, struct ssrt_state *state)
{
  struct mach_postcard_msg pmsg;
  mach_msg_return_t ret;
  int service;
  XObj self;

  service = state->service_id;

  xTrace1(ssrtestp, TR_FULL_TRACE, "%s Server received request", state->server_name);
  xTrace4(ssrtestp, TR_EVENTS, "%s Server received request type %d on port %x reply port %x",
	  state->server_name, msg->op, msg->mmhdr.msgh_local_port, msg->mmhdr.msgh_remote_port);

  /*
  /* the COMPLEX server just accepts messages and displays them
   */    
    switch (service) {
    case SS_COMPLEX:
    case SS_OOL:
    case SS_MOVE_RECEIVE:
    /* this is a problem for the second msg; it doesn't have an op field */
      state->trips++;
      switch (msg->op) {
      case ABSORB:
	printf("HURRAY! %s server finished\n", state->server_name);
	return;
      case REQUEST:
      case REQUEST_SEND_PORT:
      case REQUEST_SIMPLE_REPLY:
	/* note REQUEST for SS_COMPLEX will send a postcard */
	/* further listens will be prepared for complex messages */
	state->msg_size = (service!=SS_OOL) ?
	  sizeof(struct mach_complex_msg):
	  sizeof(struct mach_complex_ool_msg);
	if (service == SS_MOVE_RECEIVE && msg->op == REQUEST
	    && state->trips > 1) {
	  kern_return_t    ret;
	  mach_port_type_t ptype;
	  mach_port_t      rport = 
	    ((struct mach_complex_msg *)msg)->part3.port;

	  if ((ret = mach_port_type(mach_task_self(), rport, &ptype))
	      == KERN_SUCCESS) {
	    if (!(ptype & MACH_PORT_TYPE_RECEIVE))
	      printf("%s server did not get expected receive right.  got %x for port %x\n",
		     state->server_name, ptype, rport);
	    else {
	      printf("%s got receive right!\n", state->server_name);
	      printf("%s destroy return code %x\n", state->server_name,
		     mach_port_destroy(mach_task_self(), rport));
	    }
	  }
	  else printf("%s failed to get valid port %x, ret %x",
		 state->server_name, rport, ret);
	}
	break;
      default:
	printf("%s server got bad request %d\n", state->server_name, msg->op);
	return;
      }
      break;

  /*
   * the HURRAY server just accepts messages and prints HURRAY
   */    
    case SS_HURRAY:
      if (msg->op == ABSORB)
	printf("HURRAY! HURRAY server finished\n");
      else printf("HURRAY server got bad request %d\n", msg->op);
      return;
      
    case POSTCARD:
      if (msg->op != REQUEST 
	  && msg->op != REQUEST_SEND_PORT
	  && msg->op != REQUEST_SIMPLE_REPLY) {
	xTrace0(ssrtestp, TR_EVENTS, "POSTCARD Server not replying");
	return;
      }
      break;
    default:
      xTrace1(ssrtestp, TR_ALWAYS, "ssr server: unknown server %s finished",
	      state->server_name);
      return;
    }

  /*
   * the POSTCARD server and other servers send back a canned reply (the postcard)
   */

  pmsg.contents.sequence_num = msg->contents.sequence_num;
  pmsg.op = REPLY;
  strncpy(&pmsg.contents.postcard, "postcard", 8);
  pmsg.contents.postcard[8] = '\0';
  pmsg.contents.source_host = state->local_ip_addr;
  if (msg->mmhdr.msgh_remote_port == MACH_PORT_NULL) {
    xTrace1(ssrtestp, TR_DETAILED, "%s server: Message with NULL port received; ignoring",
	    state->server_name);
    return;
  }
  {
    mach_port_type_t ptype;

    if ((ret = mach_port_type(mach_task_self(), msg->mmhdr.msgh_remote_port, &ptype))
	== KERN_SUCCESS) {
      xTrace3(ssrtestp, TR_EVENTS, "ssr test server (%s) has type %x for port %x",
	      state->server_name, msg->mmhdr.msgh_remote_port, ptype);
    }
  }
    xTrace3(ssrtestp, TR_EVENTS, "ssr test server (%s) sending local port %x remote port %x",
	    state->server_name,
	    msg->mmhdr.msgh_local_port, msg->mmhdr.msgh_remote_port);
  pmsg.mmhdr.msgh_remote_port = msg->mmhdr.msgh_remote_port;
  if (msg->op != REQUEST_SIMPLE_REPLY)
    pmsg.mmhdr.msgh_local_port = state->test_server_port;
  else
    pmsg.mmhdr.msgh_local_port = MACH_PORT_NULL;
#ifdef TESTPOSTSO
  pmsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE,
					msg->op==REQUEST?
				      MACH_MSG_TYPE_MAKE_SEND_ONCE:
					msg->op==REQUEST_SIMPLE_REPLY?
					0:
					MACH_MSG_TYPE_MAKE_SEND);
#else
  pmsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
					msg->op==REQUEST ?
				      MACH_MSG_TYPE_MAKE_SEND:
					msg->op==REQUEST_SIMPLE_REPLY ?
					0:
					MACH_MSG_TYPE_MAKE_SEND);
#endif TESTPOSTSO
  pmsg.mmhdr.msgh_size = sizeof(struct mach_postcard_msg);
  pmsg.mmhdr.msgh_kind = MACH_MSGH_KIND_NORMAL;
  pmsg.mmbody.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
  pmsg.mmbody.msgt_size = MNBYTESIZE;
/*   pmsg.mmbody.msgt_size = sizeof(struct postcard_contents); */
  pmsg.mmbody.msgt_number = sizeof(struct mach_postcard_msg)-sizeof(mach_msg_header_t);;
  pmsg.mmbody.msgt_inline = 1;
  if ((ret = mach_msg_send(&pmsg)) != MACH_MSG_SUCCESS)
    {
      xTrace2(ssrtestp, TR_ERRORS, "ssr test %s server_got_msg: send fails %x",
	      state->server_name, ret);
      if (ret == MACH_SEND_INVALID_DEST) {
	xTrace0(ssrtestp, TR_ERRORS, "ssr test server_got_msg: retrying with other send right");
	pmsg.mmhdr.msgh_bits =
#ifdef TESTPOSTSO
	  MACH_MSGH_BITS(
			 MACH_MSG_TYPE_COPY_SEND,
			 msg->op==REQUEST ? MACH_MSG_TYPE_MAKE_SEND_ONCE:
			 msg->op==REQUEST_SIMPLE_REPLY ? 0 :
			 MACH_MSG_TYPE_MAKE_SEND
			 );
#else
	  MACH_MSGH_BITS(
		       MACH_MSG_TYPE_MOVE_SEND_ONCE,
			 msg->op==REQUEST_SIMPLE_REPLY ? 0 :
			 MACH_MSG_TYPE_MAKE_SEND
		       );
#endif TESTPOSTSO
	if ((ret = mach_msg_send(&pmsg)) != MACH_MSG_SUCCESS) /* this might block! */
	  xTrace1(ssrtestp, TR_ERRORS, "ssr test server_got_msg: send fails again %x", ret);
      }
    }
  else {
    /* now this thread should exit.*/
    xTrace1(ssrtestp, TR_FULL_TRACE, "%s Server replied", state->server_name);
  }
  /* xFree((void *) msg); */
}

/*
 *
 *  atoint -- simple ascii to decimal integer conversion
 *
 */
int atoint(str) char *str; {
  int a = 0;
  
  while (*str >= '0' && *str <= '9') { a = 10*a + *str - '0'; str++; }
  return a;
}

#ifndef USEXK
/* lifted from merge/pi/hoststr.c */
xkern_return_t str2ipHost(IPhost *h, char *s)
{   int a, b, c, d;
    if ( sscanf(s, "%d.%d.%d.%d", &a, &b, &c, &d) < 4 ) return XK_FAILURE;
    h->a = a; h->b = b; h->c = c; h->d = d;
    return XK_SUCCESS;
}

char * ipHostStr(IPhost *h)
{   static char str[4][32];
    static int i=0;
    i = ++i % 4;
    if ( h == 0 ) return "<null>";
    sprintf(str[i], "%d.%d.%d.%d", h->a, h->b, h->c, h->d);
    return str[i];
}
/* end of stuff from hoststr.c */
#endif ~USEXK

/*
 * isClient()
 *
 *  will start a client for server indicated in graph.comp, if
 *  corresponding letter is used: "-p" for postcard, "-x" for complex,
 *  or "-h" for hurray, matching "ssrtest/p", etc.
 *
 *  will record addr in hurray_host if "-r" is given
 *  e.g. -r 192.12.69.98 -h 192.12.69.99 for the hurray server, which
 *    requires two host names
 *
 *  will repeat 10 times if "-trips 10" is given.
 *
 *  set trace-level with -l 5.
 *
 */
static xkern_return_t
isClient(XObj self)
{
  int i;
  Event ev;
  struct ssrt_state *state = (struct ssrt_state *)self->state;
  XObj myProtl = self;
  
  xTrace0(ssrtestp, TR_FULL_TRACE, "isClient");  
  
  if (!self->state && init_state() == XK_FAILURE) {
    xTrace0(ssrtestp, TR_MAJOR_EVENTS, "ssr test client cannot initialize");
    return(XK_FAILURE);
  }
  for (i=0; i < globalArgc; i++) {
    if (!strncmp(globalArgv[i], "-r", 2))
      {
	xTrace1(ssrtestp, TR_FULL_TRACE, "SSR Client init %s", globalArgv[i]+2);
	str2ipHost((&state->hurray_host), globalArgv[i] + 2);
	continue;
      }

    if ((!strncmp(globalArgv[i], "-p", 2) && state->service_id == POSTCARD)
	||
	(!strncmp(globalArgv[i], "-x", 2) && state->service_id == SS_COMPLEX)
	||
	(!strncmp(globalArgv[i], "-o", 2) && state->service_id == SS_OOL)
	||
	(!strncmp(globalArgv[i], "-v", 2) && state->service_id == SS_MOVE_RECEIVE)
	||
	(!strncmp(globalArgv[i], "-h", 2) && state->service_id == SS_HURRAY))
	{
	  xTrace1(ssrtestp, TR_FULL_TRACE, "SSR Client init %s", globalArgv[i]+2);
	  if (globalArgv[i][2])
	    str2ipHost((&state->remote_host), globalArgv[i] + 2);
	  else
	    str2ipHost((&state->remote_host), globalArgv[++i]);
	  xTrace0(ssrtestp, TR_FULL_TRACE, "ssr start client thread");  
#ifdef USEXK
	  ev = evSchedule(client, (void *)self, 0);
	  evDetach(ev);
#else
	  xAsyncThread(client, (void *)self, (char *)0);
#endif
	  continue;
	}
    
    if (!strncmp(globalArgv[i], "-trips", 6)) {
      if (i+1 < globalArgc) {
	trips = atoint(globalArgv[++i]);
	state->trips = trips;
      }
      printf("SSRtest client using %d trips\n", trips);
      continue;
    }
  }
  xTrace0(ssrtestp, TR_FULL_TRACE, "isClient done");  
  return(XK_SUCCESS);
}


/* globals for the client */

static int msg_num = 1001;
static struct mach_postcard_msg msg;

static int total = 0;

/* 
 * client
 *
 *
 * this activates a client request message for the postcard service
 * from a particular machine, and waits for the reply.
 *
 *  all clients send their first request here; subsequent
 *   behavior is controlled by client_listen()
 *
 */
static void
#ifdef USE_XK
client(Event ev, XObj self)
#else
client(XObj self)
#endif
{
    char *buf;
    int lenindex, len;
    struct mach_server_postcard_msg pmsg;
    kern_return_t ret;
    mach_msg_type_name_t returned_type;
#ifdef USEXK
    Msg msg;
#endif
    mach_msg_return_t retm;
    mach_port_t *ss_port;
    struct ssrt_state *state = self->state;
    int service;
    
    xTrace1(ssrtestp, TR_FULL_TRACE, "%s client starting", state->server_name);
    xTrace2(ssrtestp, TR_ALWAYS, "I am the ssrtest client %s, talking to <%s>\n",
	   state->server_name, ipHostStr(&state->remote_host));
    
#ifdef USEXK
    xk_master_unlock();
#endif
    ss_port = &state->ss_client_port;
    if ((ret = mach_port_allocate(mach_task_self(),
				  MACH_PORT_RIGHT_RECEIVE,
				  ss_port)) == KERN_SUCCESS)
	{
	  /* first message only is sent from here */
	  if (state->service_id == SS_HURRAY || state->service_id == SS_MOVE_RECEIVE)
	    service = POSTCARD;
	  else service = state->service_id;
	  ssr_make_msg_to_ssr_server(&pmsg, state->remote_host,
				     REQUEST,
				     service,
				     REQUEST_SEND_PORT,
				     *ss_port);
	  bzero((char *)&(state->savedMsg), sizeof(state->savedMsg));
	  state->savedMsg.mmhdr = pmsg.mmhdr;
	  state->savedMsg.mmbody = pmsg.mmbody;
	  
	  ++total;
	  xTrace2(ssrtestp, TR_MAJOR_EVENTS, "%s Sending client Mach request to server (%d) ...\n", state->server_name, total);
	  count = 0;
	  if ((retm = mach_msg_send(&pmsg)) == MACH_MSG_SUCCESS)
	    client_listen(state);
	  else
	    xTrace4(ssrtestp, TR_ERRORS, "%s client send failed port %x %x %d\n", state->server_name, pmsg.mmhdr.msgh_remote_port, retm, retm);
	  /* and current thread is done  */
	}
    else
      xTrace2(ssrtestp, TR_ERRORS, "%s Could not allocate reply port %d",
	      state->server_name, ret);
  }

/* 
 * client_listen
 *
 *    starts the continuation processing for a client
 *
 */
static void
client_listen(struct ssrt_state *state)
{
    cthread_t child;

    xTrace1(ssrtestp, TR_FULL_TRACE, "%s client beginning send/listen",
	    state->server_name);

    if (state->service_id == SS_COMPLEX)
      client_complex_receive_and_send(state);
    if (state->service_id == SS_OOL)
      client_very_complex_receive_and_send(state);
    if (state->service_id == SS_COMPLEX)
      client_complex_receive_and_send(state);
    else if (state->service_id == SS_MOVE_RECEIVE)
      client_msg_receive_and_forward_complex(state);
    else if (state->service_id == SS_HURRAY)
      client_msg_receive_and_forward(state);
    else
      client_msg_receive(state);
  }

/*
 * client_msg_receive
 *
 * the client performs a send/listen loop for the Mach msg reply
 *  it interacts with the POSTCARD server, which is like a PING-PONG service
 *  
 *  this does no xK operations and could be completely outside the
 *  master lock.
 *  
 */
static void
client_msg_receive(struct ssrt_state *state)
{
  mach_msg_return_t ret;
  struct mach_postcard_msg msg;

  state->test_server_port = MACH_PORT_NULL;
relisten:
  xTrace1(ssrtestp, TR_FULL_TRACE, "client_msg_receive starting for port %x", state->ss_client_port);
  msg.mmhdr.msgh_local_port = state->ss_client_port;
  msg.mmhdr.msgh_size = sizeof(struct mach_postcard_msg);
  if ( (ret = mach_msg_receive(&msg)) == MACH_MSG_SUCCESS)
    {
      if (total == 1) {
	printf("got a postcard \"%8s\" from %d.%d.%d.%d, type %d length %d seq %d on port %x\n",
	     &(msg.contents.postcard),
	     msg.contents.source_host.a,
	     msg.contents.source_host.b,
	     msg.contents.source_host.c,
	     msg.contents.source_host.d,
	     msg.op,
	     msg.mmhdr.msgh_size,
	     msg.contents.sequence_num,
	     state->ss_client_port);
	state->test_server_port = msg.mmhdr.msgh_remote_port;
#ifdef TIME
	xGetTime(&startTime);
#endif
      }
      if (total < trips) {
	state->savedMsg.contents.sequence_num = total++;
	state->savedMsg.op = REQUEST_SIMPLE_REPLY;
	if (total == 2)
#ifdef TESTPOSTSO
	  state->savedMsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
						    MACH_MSG_TYPE_MAKE_SEND_ONCE);
#else
	  state->savedMsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
						    MACH_MSG_TYPE_MAKE_SEND);
#endif
	  else
#ifdef TESTPOSTSO
	    state->savedMsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
						      MACH_MSG_TYPE_MAKE_SEND_ONCE);
#else
	    state->savedMsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
						      MACH_MSG_TYPE_MAKE_SEND);
#endif TESTPOSTSO
	state->savedMsg.mmhdr.msgh_remote_port = state->test_server_port;
	state->savedMsg.mmhdr.msgh_local_port = state->ss_client_port;
	xTrace3(ssrtestp, TR_EVENTS, "Sending %dth reply on port %x reply port %x", total, state->savedMsg.mmhdr.msgh_remote_port, state->ss_client_port);
      }
      else  {
	/* state->savedMsg.op = SHUTDOWN; */
	state->savedMsg.mmhdr.msgh_remote_port = MACH_PORT_NULL;
	printf("ssrtest client ending test");
	total = 0;
#ifdef TIME
	xGetTime(&endTime);
	subtime(&startTime, &endTime, &netTime);
	printf("\nlen = %4d, %d trips: %6d.%06d\n", 
	       sizeof(state->savedMsg), trips, netTime.sec, netTime.usec);
#endif
	return;
      }
      xTrace1(ssrtestp, TR_ALWAYS, "ssrtest client sending test message %d", total);
      if (state->savedMsg.mmhdr.msgh_remote_port == MACH_PORT_NULL ||
      (ret = mach_msg_send(&state->savedMsg)) != MACH_MSG_SUCCESS) {
	  printf("ssrtest client send failed %x (%d) on port %x; test over",
		 ret, ret, state->savedMsg.mmhdr.msgh_remote_port);
	  return;
	}
      else {
	goto relisten;
      }
    }
  else {
    printf("ssrtest error from machlisten %d %x; test over", ret, ret);
  }
}

/*
 *
 *  ssr_make_msg_to_ssr_server
 *
 *   a utility routine
 *
 */
void
ssr_make_msg_to_ssr_server(pmsg, host, operation, service_id, server_op, port)
     struct mach_server_postcard_msg *pmsg;
     IPhost host;
     enum ssr_operation_type operation, server_op;
     enum ssr_service_type service_id;
     mach_port_t port;
{
  kern_return_t ret;
  mach_msg_type_name_t returned_type;

      ret = mach_port_type(mach_task_self(), port, &returned_type);
      xTrace3(ssrtestp, TR_FULL_TRACE, "ssr test client port %x right %x %x",
	      port, returned_type, ret);
      ret = mach_port_type(mach_task_self(), server_registry_port, &returned_type);
      xTrace3(ssrtestp, TR_FULL_TRACE, "ssr test registry port %x type %x %x",
	      server_registry_port, returned_type, ret);
      pmsg->mmhdr.msgh_remote_port = server_registry_port;
      pmsg->mmhdr.msgh_local_port = port;
      pmsg->mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
#ifdef TESTPOSTSO
					     MACH_MSG_TYPE_MAKE_SEND_ONCE
#else
					     MACH_MSG_TYPE_MAKE_SEND
#endif TESTPOSTSO
					     );
      pmsg->mmhdr.msgh_size = sizeof(struct mach_server_postcard_msg);
      pmsg->mmbody.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
      pmsg->mmbody.msgt_size = MNBYTESIZE;
      pmsg->mmbody.msgt_number = sizeof(struct mach_server_postcard_msg) -
	                         sizeof(mach_msg_header_t);
      pmsg->mmbody.msgt_inline =     1;
      pmsg->mmbody.msgt_longform =   0;
      pmsg->mmbody.msgt_deallocate = 0;
      xTrace2(ssrtestp, TR_FULL_TRACE,
	      "ssrtest server_registry_port %x local port %x", server_registry_port, port);
      xTrace1(ssrtestp, TR_FULL_TRACE, "ssrtest msgh bits %x", pmsg->mmhdr.msgh_bits);
      pmsg->ssrd.service_id = service_id;
      pmsg->ssrd.operation = operation;
      pmsg->ssrd.sequence_num = msg_num;
      pmsg->ssrd.destination_host = host;
      pmsg->op = server_op;
/*      pmsg->contents undefined for requests */
}

/*
 * client_msg_receive_and_forward
 *
 * this is an alternative to client_msg_receive.
 *
 * the client performs a send/listen loop for the Mach msg reply
 *
 * This version requests a POSTCARD and then
 * forwards the POSTCARD server reply port to
 * the HURRAY server on another host, via the ssr_server.
 *
 * The result should be a postcard followed by a hurray message.
 * Three machines can be used for this to test port transfers
 *
 */
static void
client_msg_receive_and_forward(state)
     struct ssrt_state *state;
{
  mach_msg_return_t ret;
  struct mach_postcard_msg msg;
  struct mach_server_postcard_msg pmsg;
  IPhost second_host = state->hurray_host;

relisten:
  xTrace1(ssrtestp, TR_FULL_TRACE, "ssr-test-client: client_msg_receive_and_forward starting for port %x", state->ss_client_port);
  msg.mmhdr.msgh_local_port = state->ss_client_port;
  msg.mmhdr.msgh_size = sizeof(struct mach_postcard_msg);
  if ( (ret = mach_msg_receive(&msg)) == MACH_MSG_SUCCESS)
    {
      mach_port_t remote_port;

      printf("ssr test client got a postcard \"%8s\" from %d.%d.%d.%d, type %d length %d seq %d on port %x\n",
	     &(msg.contents.postcard),
	     msg.contents.source_host.a,
	     msg.contents.source_host.b,
	     msg.contents.source_host.c,
	     msg.contents.source_host.d,
	     msg.op,
	     msg.mmhdr.msgh_size,
	     msg.contents.sequence_num,
	     state->ss_client_port);

      /* the hurray service should print the message "HURRAY" */
      remote_port = msg.mmhdr.msgh_remote_port;
      ssr_make_msg_to_ssr_server(&pmsg, state->hurray_host, REQUEST, 
				 SS_HURRAY, ABSORB, state->ss_client_port);
      pmsg.mmhdr.msgh_local_port = remote_port;
      pmsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MOVE_SEND);
      
      if ((ret = mach_msg_send(&pmsg)) != MACH_MSG_SUCCESS) {
	xTrace4(ssrtestp, TR_ERRORS, "ssrtest client send failed %x (%d) on port %x reply %x",
		ret, ret, msg.mmhdr.msgh_remote_port,
		msg.mmhdr.msgh_local_port);
      }
      else {
	xTrace5(ssrtestp, TR_ALWAYS, "ssrtest forwarded POSTCARD reply to HURRAY server on host %d.%d.%d.%d using port %x",
		second_host.a, second_host.b,
		second_host.c, second_host.d,
		msg.mmhdr.msgh_remote_port);
	goto relisten;
      }
    }
  else {
    xTrace2(ssrtestp, TR_ERRORS, "error from machlisten %d %x", ret, ret);
  }
}

/*
 *  make_complex_ool_msg
 *
 *  just for putting together a multi-part message
 *
 */
static void
make_complex_ool_msg(cxmsg, name)
     struct mach_complex_ool_msg *cxmsg;
{
  xTrace1(ssrtestp, TR_FULL_TRACE, "ssrtest: make_complex_ool_msg entered for %s", name);

      cxmsg->mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
      xTrace1(ssrtestp, TR_FULL_TRACE, "ssrtest msgh bits %x", cxmsg->mmhdr.msgh_bits);
      cxmsg->mmhdr.msgh_size = sizeof(struct mach_complex_ool_msg);
      cxmsg->part1.op = ABSORB;

      cxmsg->part1.mmbody.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
      cxmsg->part1.mmbody.msgt_size = MNBYTESIZE;
      cxmsg->part1.mmbody.msgt_number = sizeof(cxmsg->part1) - sizeof(mach_msg_type_t);
      cxmsg->part1.mmbody.msgt_inline =     1;
      cxmsg->part1.mmbody.msgt_longform =   0;
      cxmsg->part1.mmbody.msgt_deallocate = 0;
      
      cxmsg->part2.mmbody2.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
      cxmsg->part2.mmbody2.msgt_size = MNBYTESIZE;
      cxmsg->part2.mmbody2.msgt_number = sizeof(cxmsg->part2)-sizeof(mach_msg_type_t);
      cxmsg->part2.mmbody2.msgt_inline =     1;
      cxmsg->part2.mmbody2.msgt_longform =   0;
      cxmsg->part2.mmbody2.msgt_deallocate = 0;
      
      printf("%s client moving right via %x\n",
	     name,
	     cxmsg->part3.mmbody2.msgt_name);
      cxmsg->part3.mmbody2.msgt_size = sizeof(mach_port_t)*MNBYTESIZE;
      cxmsg->part3.mmbody2.msgt_number =     1;
      cxmsg->part3.mmbody2.msgt_inline =     1;
      cxmsg->part3.mmbody2.msgt_longform =   0;
      cxmsg->part3.mmbody2.msgt_deallocate = 0;
      cxmsg->part3.mmbody2.msgt_name = MACH_MSG_TYPE_MOVE_RECEIVE;

      printf("%s client moving ool data\n", name);
      cxmsg->part4.mmbody2.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
      cxmsg->part4.mmbody2.msgt_size = MNBYTESIZE;
      cxmsg->part4.mmbody2.msgt_number =     0; /* up to 2048 */
      cxmsg->part4.mmbody2.msgt_inline =     0;
      cxmsg->part4.mmbody2.msgt_longform =   0;
      cxmsg->part4.mmbody2.msgt_deallocate = 0;

      printf("%s client moving ool port\n", name);
      cxmsg->part5.mmbody2.msgt_name = MACH_MSG_TYPE_MAKE_SEND;
      cxmsg->part5.mmbody2.msgt_size = sizeof(mach_port_t)*MNBYTESIZE;
      cxmsg->part5.mmbody2.msgt_number =     1;
      cxmsg->part5.mmbody2.msgt_inline =     0;
      cxmsg->part5.mmbody2.msgt_longform =   0;
      cxmsg->part5.mmbody2.msgt_deallocate = 0;

      printf("%s client moving ool ports\n", name);
      cxmsg->part6.mmbody2.msgt_name = MACH_MSG_TYPE_MOVE_SEND;
      cxmsg->part6.mmbody2.msgt_size = sizeof(mach_port_t)*MNBYTESIZE;
      cxmsg->part6.mmbody2.msgt_number =     0;
      cxmsg->part6.mmbody2.msgt_inline =     0;
      cxmsg->part6.mmbody2.msgt_longform =   0;
      cxmsg->part6.mmbody2.msgt_deallocate = 0;
    }

/*
 *  make_complex_msg
 *
 *  just for putting together a multi-part message
 *
 */
static void
make_complex_msg(cxmsg) struct mach_complex_msg *cxmsg;
{
  xTrace0(ssrtestp, TR_FULL_TRACE, "ssrtest: make_complex_msg entered");

      cxmsg->mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
      xTrace1(ssrtestp, TR_FULL_TRACE, "ssrtest msgh bits %x", cxmsg->mmhdr.msgh_bits);
      cxmsg->mmhdr.msgh_size = sizeof(struct mach_complex_msg);
      cxmsg->part1.op = ABSORB;

      cxmsg->part1.mmbody.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
      cxmsg->part1.mmbody.msgt_size = MNBYTESIZE;
      cxmsg->part1.mmbody.msgt_number = sizeof(cxmsg->part1) - sizeof(mach_msg_type_t);
      cxmsg->part1.mmbody.msgt_inline =     1;
      cxmsg->part1.mmbody.msgt_longform =   0;
      cxmsg->part1.mmbody.msgt_deallocate = 0;
      
      cxmsg->part2.mmbody2.msgt_name = MACH_MSG_TYPE_UNSTRUCTURED;
      cxmsg->part2.mmbody2.msgt_size = MNBYTESIZE;
      cxmsg->part2.mmbody2.msgt_number = sizeof(cxmsg->part2)-sizeof(mach_msg_type_t);
      cxmsg->part2.mmbody2.msgt_inline =     1;
      cxmsg->part2.mmbody2.msgt_longform =   0;
      cxmsg->part2.mmbody2.msgt_deallocate = 0;
      
      cxmsg->part3.mmbody2.msgt_name = MACH_MSG_TYPE_MOVE_RECEIVE;
      printf("COMPLEX client moving right via %x", cxmsg->part3.mmbody2.msgt_name);
      cxmsg->part3.mmbody2.msgt_size = sizeof(mach_port_t)*MNBYTESIZE;
      cxmsg->part3.mmbody2.msgt_number =     1;
      cxmsg->part3.mmbody2.msgt_inline =     1;
      cxmsg->part3.mmbody2.msgt_longform =   0;
      cxmsg->part3.mmbody2.msgt_deallocate = 0;
    }

/*
 * client_complex_receive_and_send
 *
 * this is an alternative to client_msg_receive.
 *
 * This version requests a POSTCARD from the COMPLEX server and then
 * sends a complex message to that server.
 *
 * The result should be a postcard followed by a hurray message.
 * Three machines can be used for this to test port transfers.
 * Two machines can test out-of-line data and message type conversions.
 *
 */
static void
client_complex_receive_and_send(state)
     struct ssrt_state *state;
{
  mach_msg_return_t		ret;
  struct mach_postcard_msg	msg;
  struct mach_server_postcard_msg pmsg;
  struct mach_complex_msg	cxmsg;

relisten:
  xTrace1(ssrtestp, TR_FULL_TRACE, "ssr-test-client: client_msg_receive_and_forward starting for port %x", state->ss_client_port);
  msg.mmhdr.msgh_local_port = state->ss_client_port;
  msg.mmhdr.msgh_size = sizeof(struct mach_postcard_msg);
  if ( (ret = mach_msg_receive(&msg)) == MACH_MSG_SUCCESS)
    {
      printf("ssr test complex client got a postcard \"%8s\" from %d.%d.%d.%d, type %d length %d seq %d on port %x\n",
	     &(msg.contents.postcard),
	     msg.contents.source_host.a,
	     msg.contents.source_host.b,
	     msg.contents.source_host.c,
	     msg.contents.source_host.d,
	     msg.op,
	     msg.mmhdr.msgh_size,
	     msg.contents.sequence_num,
	     state->ss_client_port);
      make_complex_msg(&cxmsg);
      mach_port_allocate(mach_task_self(),
			 MACH_PORT_RIGHT_RECEIVE,
			 &cxmsg.part3.port);
      cxmsg.mmhdr.msgh_remote_port = msg.mmhdr.msgh_remote_port;
      cxmsg.mmhdr.msgh_local_port = state->ss_client_port;
      cxmsg.part3.mmbody2.msgt_name = MACH_MSG_TYPE_MOVE_SEND;
      if ((ret = mach_msg_send(&cxmsg)) != MACH_MSG_SUCCESS) {
	xTrace3(ssrtestp, TR_ERRORS, "ssrtest client send failed %x (%d) on port %x", ret, ret, msg.mmhdr.msgh_remote_port); }
      else {
	xTrace2(ssrtestp, TR_ALWAYS, "ssrtest sent complex msg to COMPLEX server using port %x size %d",
		msg.mmhdr.msgh_remote_port, sizeof(struct mach_complex_msg));
      }
    }
  else {
    xTrace2(ssrtestp, TR_ERRORS, "ssrtest: complex msg recv %d(0x%x)", ret, ret);
  }
}

/*
 * client_very_complex_receive_and_send
 *
 * this is an alternative to client_msg_receive.
 *
 * This version requests a POSTCARD from the COMPLEX server and then
 * sends a complex message to that server.
 *
 * The result should be a postcard followed by a hurray message.
 *
 */
static void
client_very_complex_receive_and_send(state)
     struct ssrt_state *state;
{
  mach_msg_return_t		ret;
  struct mach_postcard_msg	msg;
  struct mach_server_postcard_msg pmsg;
  struct mach_complex_ool_msg	cxmsg;
  static char			static_data[4096];
  static mach_port_t		static_ports[10];

relisten:
  xTrace1(ssrtestp, TR_FULL_TRACE, "ssr-test-client: client_ver_complex_receive_and_send starting for port %x", state->ss_client_port);
  msg.mmhdr.msgh_local_port = state->ss_client_port;
  msg.mmhdr.msgh_size = sizeof(struct mach_postcard_msg);
  if ( (ret = mach_msg_receive(&msg)) == MACH_MSG_SUCCESS)
    {
      int i;

      printf("ssr test very complex client got a postcard \"%8s\" from %d.%d.%d.%d, type %d length %d seq %d on port %x\n",
	     &(msg.contents.postcard),
	     msg.contents.source_host.a,
	     msg.contents.source_host.b,
	     msg.contents.source_host.c,
	     msg.contents.source_host.d,
	     msg.op,
	     msg.mmhdr.msgh_size,
	     msg.contents.sequence_num,
	     state->ss_client_port);
      bzero((char *)&cxmsg, sizeof(cxmsg));
      make_complex_ool_msg(&cxmsg, state->server_name);
      mach_port_allocate(mach_task_self(),
			 MACH_PORT_RIGHT_RECEIVE,
			 &cxmsg.part3.port);
      cxmsg.mmhdr.msgh_remote_port = msg.mmhdr.msgh_remote_port;
      cxmsg.mmhdr.msgh_local_port = state->ss_client_port;
      cxmsg.part3.mmbody2.msgt_name = MACH_MSG_TYPE_MAKE_SEND;
      cxmsg.part4.datadr = (vm_offset_t)static_data;
      cxmsg.part4.mmbody2.msgt_number = 1024;
      cxmsg.part5.datadr = &state->ss_client_port;
      cxmsg.part5.mmbody2.msgt_number = 1;
      cxmsg.part6.datadr = (vm_address_t)static_ports;
      cxmsg.part6.mmbody2.msgt_number = 10;
      for (i=0; i<10; i++) static_ports[i] = MACH_PORT_NULL;
      msg.mmhdr.msgh_local_port = MACH_PORT_NULL;
      
      if ((ret = mach_msg_send(&cxmsg)) != MACH_MSG_SUCCESS) {
	xTrace4(ssrtestp, TR_ERRORS, "ssrtest client %s send failed %x (%d) on port %x",
		state->server_name,
		ret, ret, msg.mmhdr.msgh_remote_port); }
      else {
	xTrace3(ssrtestp, TR_ALWAYS, "ssrtest sent complex msg to %s server using port %x size %d",
		state->server_name,
		msg.mmhdr.msgh_remote_port, sizeof(struct mach_complex_msg));
      }
    }
  else {
    xTrace2(ssrtestp, TR_ERRORS, "ssrtest: very complex msg recv %d(0x%x)", ret, ret);
  }
}

/*
 * client_msg_receive_and_forward_complex
 *
 * this is an alternative to client_msg_receive.
 *
 * This version requests a POSTCARD server on one machine and
 * gives a send right on a newly allocated port to that machine.
 * Then it moves the receive right to a HURRAY server on a third machine.
 *
 * Three machines can be used for this to test port transfers
 *
 */
static void
client_msg_receive_and_forward_complex(state)
     struct ssrt_state *state;
{
  mach_msg_return_t ret;
  struct mach_postcard_msg msg;
  struct mach_server_postcard_msg pmsg;
  struct mach_server_postcard_msg *send_msg;
  IPhost second_host = state->hurray_host;
  struct mach_complex_msg cxmsg;
  int    teststate = 0;

relisten:
  xTrace1(ssrtestp, TR_FULL_TRACE, "ssr-test-client: client_msg_receive_and_forward_complex starting for port %x", state->ss_client_port);
  msg.mmhdr.msgh_local_port = state->ss_client_port;
  msg.mmhdr.msgh_size = sizeof(struct mach_postcard_msg);
  if ( teststate == 1 || (ret = mach_msg_receive(&msg)) == MACH_MSG_SUCCESS )
    {
      mach_port_t remote_port;

      if (teststate == 0)
	printf("ssr test client got a postcard \"%8s\" from %d.%d.%d.%d, type %d length %d seq %d on port %x\n",
	       &(msg.contents.postcard),
	       msg.contents.source_host.a,
	       msg.contents.source_host.b,
	       msg.contents.source_host.c,
	       msg.contents.source_host.d,
	       msg.op,
	       msg.mmhdr.msgh_size,
	       msg.contents.sequence_num,
	       state->ss_client_port);

      if (!state->hurray_host.a) {
	printf("No second host specified (use -r); test finished");
	return;
      }

      if (teststate ==0) {
	/* this distributes a send right to a new port */
	mach_port_allocate(mach_task_self(),
			   MACH_PORT_RIGHT_RECEIVE,
			   &cxmsg.part3.port);
	ssr_make_msg_to_ssr_server(&pmsg, state->remote_host, REQUEST, 
				   POSTCARD, REQUEST_SEND_PORT,
				   cxmsg.part3.port);
	pmsg.mmhdr.msgh_local_port = cxmsg.part3.port;
	pmsg.mmhdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_MAKE_SEND);
	send_msg = &pmsg;
	/* we won't wait for a reply; we aren't listening on that port
	   but machipc will see the reply after the receive right is
	   transferred (machipc will hold it as proxy) and it starts a
	   listener; it must forward the queue to the new receiver(!)
	 */
      }
      if (teststate == 1) {
	/* get port to SS_MOVE_RECEIVE server (stored in hurray host field)  */
	ssr_make_msg_to_ssr_server(&pmsg, state->hurray_host, REQUEST, 
				   SS_MOVE_RECEIVE, REQUEST_SEND_PORT,
				   state->ss_client_port);
	send_msg = &pmsg;
      }
      if (teststate == 2) {
	/* send a complex message, including a receive right, to the SS_MOVE_RECEIVE server */

	make_complex_msg(&cxmsg);
	cxmsg.part1.op = REQUEST;
	cxmsg.mmhdr.msgh_local_port = state->ss_client_port;
	/* heads right back to the SS_MOVE_RECEIVE server */
	cxmsg.mmhdr.msgh_remote_port = msg.mmhdr.msgh_remote_port;
	send_msg = (struct mach_server_postcard_msg *) &cxmsg;
      }
      if (teststate > 2) {
	printf("ssrtest complex test over");
	return;
      }

      teststate++;

      if ((ret = mach_msg_send(send_msg)) != MACH_MSG_SUCCESS) {
	xTrace4(ssrtestp, TR_ERRORS, "ssrtest client send failed %x (%d) on port %x reply %x",
		ret, ret, send_msg->mmhdr.msgh_remote_port,
		send_msg->mmhdr.msgh_local_port);
      }
      else {
	if (teststate == 1 || teststate == 2) {
	  printf("ssrtest %s sent POSTCARD request to server on host %d.%d.%d.%d using port %x state %d",
		 state->server_name,
		 teststate==1 ? state->remote_host.a : state->hurray_host.a,
		 teststate==1 ? state->remote_host.b : state->hurray_host.b,
		 teststate==1 ? state->remote_host.c : state->hurray_host.c,
		 teststate==1 ? state->remote_host.d : state->hurray_host.d,
		 send_msg->mmhdr.msgh_remote_port,
		 teststate);
	}
	else {
	  printf("ssrtest %s sent request to MOVE RECEIVE server on host %d.%d.%d.%d using port %x",
		 state->server_name,
		 second_host.a, second_host.b,
		 second_host.c, second_host.d,
		 send_msg->mmhdr.msgh_remote_port);
	}
	if (teststate > 2) {
	  printf("ssrtest %s test over", state->server_name);
	  return;
	}
	else goto relisten;
      }
    }
  else {
    xTrace2(ssrtestp, TR_ERRORS, "error from machlisten %d %x", ret, ret);
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

#ifdef USEXK
static void
getproc_client(XObj p)
{
    xAssert(p->type == Protocol);
}


static void
getproc_server(XObj p)
{
    xAssert(p->type == Protocol);
}
#endif

#ifndef USEXK
void	xTraceLock( ) { ; } ;
void	xTraceUnlock( ) { ; } ;
void	xTraceInit( ) { ; } ;
#endif

