/*     
 * $RCSfile: xkpm.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.24 $
 * $Date: 1993/02/01 22:32:49 $
 */

/*
 * X-kernel version of the portmapper,
 * by Rich Schroeppel, based on older versions by Sean O'Malley,
 * Larry Petersen, & Norm Hutchinson.
 */

#include <rpc/rpc.h>
#include "xkernel.h"
#include "ip.h"
#include "udp.h"
#include "sunrpc.h"
#include "xkpm.h"
#define MAX_SUNRPC_MSG_SIZE  15000  /* a guess */

typedef struct {unsigned long prog, vers, prot;} PMAP_EXT_ID;
typedef struct {XObj session; Msg *msg;} Sess_Msg_Pair;

Map port_map, call_map;

void print_pmap();
void print_xdr();
void prpchdr();

static void pmapproc_null();
static long *pmapproc_set();
static long *pmapproc_unset();
static long *pmapproc_getport();
static long *pmapproc_dump();
static void pmapproc_callit();

static XObj pmap_xobj=0;
#define SUNRPC xGetDown(pmap_xobj,0)
#define PMAP pmap_xobj
IPhost pmap_ipaddr;
long tracepmapp;

static xkern_return_t pmap_opendone(), pmap_calldemux();
#if 0
static xkern_return_t pmap_demux();
#endif

void xkxdr_long(name,data) XKXDR *name; long data;
#ifdef SUNXDR
{ if (!xdr_long(name,&data))
    printf("xdr failed! name = %d, data = %d\n",name,data); }
#else
{ xIfTrace(pmapp,TR_FULL_TRACE)
  { printf("xkxdr_long:  name = %ld, *name = %ld, data = %lx, htonl(data) = %lx\n",
	   (long)name, (long)*name, (long)data, (long)htonl(data));
    print_xdr((long)(*name)-6,16); };
  *(long *)(*name) = htonl(data); *name += sizeof(long);
  xIfTrace(pmapp,TR_FULL_TRACE)
  { printf("xkxdr_long:  name = %d, *name = %d, data = %x, htonl(data) = %x\n",
	   name, *name, data, htonl(data));
    print_xdr((long)(*name)-6,16); };
}
#endif

long xkxdr_long_decode(name) XKXDR *name;
#ifdef SUNXDR
{ long data;
  if (!xdr_long(name,&data))
    printf("xdr failed! name = %d, data = %d\n",name,data);
  return data; }
#else
{ long val; val = ntohl(*(long *)(*name)); *name += sizeof(long); return val; }
#endif

void xkxdr_pmap_decode(xdrs,pmargs,needport)
XKXDR *xdrs; PMAP_ARGS *pmargs; int needport;
{ pmargs->prog = xkxdr_long_decode(xdrs);
  pmargs->vers = xkxdr_long_decode(xdrs);
  pmargs->prot = xkxdr_long_decode(xdrs);
  if (needport) pmargs->port = xkxdr_long_decode(xdrs); }

long run_portmapper=1, portmapper_portnumber=111;


void pmap_init(self) XObj self;
{ Part part[2];
  long myport=PMAP_PORT, myprog=PMAP_PROG, myvers=PMAP_VERS;
  static PMAP_ARGS pmargs = {PMAP_PROG,PMAP_VERS,IPPROTO_UDP,PMAP_PORT};

  if (!run_portmapper)
  { xError("not running pmap\n"); return; }
  myport=pmargs.port=portmapper_portnumber; /*hack the portmapper port number*/

  xTrace0(pmapp, TR_MAJOR_EVENTS, "pmap init");

  if (pmap_xobj)
  { xError("SUN Portmapper: can't instantiate twice\n"); return; };
  pmap_xobj = self;
  pmap_xobj->opendone = pmap_opendone;
  pmap_xobj->calldemux = pmap_calldemux;
/*  pmap_xobj->demux = pmap_demux;     waiting for CALLIT */

  port_map = mapCreate(100,sizeof(PMAP_EXT_ID));
  call_map = mapCreate(50,sizeof(XObj));

  if (xControl(SUNRPC, GETMYHOST, (char *)&pmap_ipaddr, sizeof(IPhost)) < 0)
  { xError("Portmapper: Can't find my IP addr\n"); return; }

  partInit(part, 1);
  partPush(part[0], (long *) &myport, sizeof(long));
  partPush(part[0], (long *) &myprog, sizeof(long));
  partPush(part[0], (long *) &myvers, sizeof(long));
  if (xOpenEnable(PMAP, PMAP, SUNRPC, part) == XK_FAILURE)
  { xError("pmap xOpenEnable failed"); return; }

  /* Only UDP is presently supported; might need
   * second copy of SUNRPC to support TCP also.
   * PMAP should register itself twice, once for each of TCP & UDP;
   * but, since we don't support TCP yet, only register once. */

  pmapproc_set(&pmargs);  /* register the portmapper */

  xTrace0(pmapp, TR_FULL_TRACE, "pmap init returns OK");
}


long yabcopy (to,from,len,xarg)  char *to,*from; long len; VOID *xarg;
{ bcopy(from,to,len); return len; }


static xkern_return_t pmap_calldemux(self,s,msg,msgr)
XObj self, s; Msg *msg, *msgr;
{ char *buf, buf2[100], *bufout;
  long len, len2, *results, pmproc; int popv=TRUE;
  PMAP_ARGS args;
  XKXDR xdrs, xdrr;

  xTrace0(pmapp,TR_FULL_TRACE,"pmap_calldemux: entered");

  /* externalize message so we can look at it */
  len = msgLen(msg);
  xTrace3(pmapp,TR_MAJOR_EVENTS,
	  "pmap_calldemux: msg = %d msgLen = %d  msgr = %d",msg,len,msgr);
  buf = (char *) (len ? xMalloc(len) : 0);
  xTrace0(pmapp,TR_FULL_TRACE,"pmap_calldemux: starting msgPop");
  if (buf) popv = msgPop(msg,yabcopy,buf,len,0);
  if (popv == FALSE || len < sizeof(long))
    { xError("Bad message to msgPop\n");
      xTrace2(pmapp,TR_ERRORS,
	      "bad message to msgPop: msg = %d  len = %d\n", msg, len);
      xFree(buf);
      return XK_FAILURE; }
  xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_calldemux: msg externalized");  

  /* authentication check would go here */

  /* the old way: extract the procedure from the call header */
  /* pmproc = ((SState *) s->state)->s_proc; */
  /* the new way: sunrpc sticks it on the front of the message, sans xdring */
  bcopy(buf,
	(char *)&pmproc,
	sizeof(long));
  /* next phrase is obsolete */
  /* xIfTrace(pmapp,TR_FULL_TRACE) {
       printf("s = %d  s->state = %d",s,s->state);
       prpchdr(((SState *) s->state)->hdr,"pmap call header"); }; */
  xTrace1(pmapp,TR_FULL_TRACE,"pmap_calldemux: procedure = %d",pmproc);
  if (pmproc < PMAPPROC_NULL || PMAPPROC_CALLIT < pmproc)
  { xTrace1(pmapp,TR_ERRORS,
	    "pmap_calldemux: PM procedure out of range: %d",pmproc);
    /* would be nice if there were a separate code for bad procedure number */
    xControl(s, SUNRPC_SVCGARBAGEARGS,0,0);
    xFree(buf);
    return XK_FAILURE; }

  /* if we are being asked to (un)register a server, check host is local */
  if (pmproc==PMAPPROC_SET || pmproc==PMAPPROC_UNSET)
  { IPhost srv_ipaddr;
    if (xControl(s, GETPEERHOST, (char *)&srv_ipaddr, sizeof(IPhost)) < 0)
    { xError(
	"PMAP: Can't find server IP address for (un)registration request.\n");
      if (buf) xFree(buf);
      return XK_FAILURE; }
    if (!IP_EQUAL(srv_ipaddr,pmap_ipaddr))
    { xError("PMAP: non-local attempt to (un)register with portmapper.\n");
      if (buf) xFree(buf);
      return XK_FAILURE; }; };

  /* decode arguments */
  xkxdr_init(xdrs,buf+sizeof(long),len-sizeof(long),XDR_DECODE);
  /* skip decode for NULL, DUMP, & CALLIT */
  if ( PMAPPROC_SET <= pmproc && pmproc <= PMAPPROC_GETPORT)
  { if (len<4*sizeof(long) || (pmproc==PMAPPROC_SET && len<5*sizeof(long)))
    { xTrace0(pmapp,TR_ERRORS,"pmap_calldemux: can't decode args");  
      xControl(s, SUNRPC_SVCGARBAGEARGS,0,0);
      if (buf) xFree(buf);
      return XK_FAILURE; }  
    xkxdr_pmap_decode(&xdrs,&args,pmproc==PMAPPROC_SET);
    xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_calldemux: args decoded ");
    xIfTrace(pmapp,TR_FULL_TRACE) { print_pmap(&args);}; };

  /* call procedure */
  results = (long *) 0;
  switch (pmproc)
  {case PMAPPROC_NULL:              pmapproc_null();         break;
   case PMAPPROC_SET:     results = pmapproc_set(&args);     break;
   case PMAPPROC_UNSET:   results = pmapproc_unset(&args);   break;
   case PMAPPROC_GETPORT: results = pmapproc_getport(&args); break;
   case PMAPPROC_DUMP:    results = pmapproc_dump(&len2);    break;
   /* CALLIT isn't implemented yet; ignore it for now */
   case PMAPPROC_CALLIT:  if (0) pmapproc_callit(msg,msgr,buf,len); break;
  }
  xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_calldemux: procedure called");

  /* xdr results */
  if (pmproc == PMAPPROC_DUMP) bufout=(char *)results; /* len2 already set */
  else { bufout=buf2;
    xkxdr_init(xdrr,buf2,100,XDR_ENCODE);
    len2 = 0;
    if (results)    /* skip xdr for NULL (and CALLIT, temporarily) */
    { xkxdr_long(&xdrr,*results); xFree((char *)results); len2=sizeof(long); }
       };
  xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_calldemux: results xdred");

  /* set authentication would go here */

  xIfTrace(pmapp,TR_FUNCTIONAL_TRACE)
  { printf("pmap_calldemux: len2 = %d buf2 = %d bufout = %d\n",
	   len2,buf2,bufout);
    print_xdr(bufout,len2); }
  if (len2) msgConstructBuffer(msgr,bufout,len2);  /* copy data into reply */
  xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_calldemux: msg internalized");

  if (buf) xFree(buf);
  if (pmproc == PMAPPROC_DUMP) xFree((char *)results);
  return XK_SUCCESS;
}


static xkern_return_t pmap_opendone(self,s,part)  XObj s,self; Part part;
{ xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"--->pmap_opendone"); return XK_SUCCESS; }


static void pmapproc_null() { }


static long *pmapproc_set(args) PMAP_ARGS *args;
{ PMAP_EXT_ID ext_id; long *reply;

  xTrace0(pmapp,TR_MAJOR_EVENTS,"pmap_set: entered\n");
  xIfTrace(pmapp,TR_MAJOR_EVENTS) print_pmap(args);
  ext_id.prog = args->prog;
  ext_id.vers = args->vers;
  ext_id.prot = args->prot;
  xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_set: args transferred\n");
  reply = (long *) xMalloc(sizeof(long));
  if (mapBind(port_map,&ext_id,args->port) == (Bind) -1 )
  { xTrace0(pmapp,TR_ERRORS,"pmap_set: bind fails\n");
    *reply = 0; return reply;}
  xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_set: bind succeeds\n");
  *reply = 1;
  return reply;
}

/* The effect of UNSET is simply to cancel the advertising.  Any existing
   communications between clients & servers is unmolested. */

typedef struct { long removed_any, removed_this_pass, prog, prot; } unset_comm;

static int pmap_unset_filter (mapkey, mapvalue, commarg)
PMAP_EXT_ID *mapkey; long mapvalue; unset_comm *commarg;
{ xTrace0(pmapp,TR_FULL_TRACE,"pmap_unset_filter: entered");
  if ((mapkey->prog == commarg->prog) && (mapkey->prot == commarg->prot))
  { xTrace3(pmapp,TR_EVENTS,
	    "pmap_unset_filter: unbinding program %d version %d protocol %d",
	    mapkey->prog,mapkey->vers,mapkey->prot);
    mapUnbind(port_map,mapkey);
    commarg->removed_any = commarg->removed_this_pass = TRUE; };
  return MFE_CONTINUE; }  

static long *pmapproc_unset(args)
PMAP_ARGS *args;
{ long *reply; unset_comm prog_prot;

  xTrace0(pmapp,TR_MAJOR_EVENTS,"pmap_unset: entered");
  reply = (long *) xMalloc(sizeof(long));
  *reply = 0;

  prog_prot.removed_any = FALSE;
  prog_prot.prog = args->prog; prog_prot.prot = args->prot;

/* UNSET is supposed to erase all versions of PROG x PROT -- rcs 13nov91 */

  do {xTrace0(pmapp,TR_EVENTS,"pmap_unset: cycling throught port map");
      prog_prot.removed_this_pass = FALSE;
      mapForEach(port_map, pmap_unset_filter, &prog_prot);}
    while (prog_prot.removed_this_pass);

  if (prog_prot.removed_any) *reply = 1;
  else xTrace0(pmapp,TR_ERRORS,"pmap_unset: no udp bind to unbind");

  return reply;
}


typedef struct { long prog, vers, prot, port; } getport_comm;

static int pmap_getport_filter (mapkey, mapvalue, commarg)
PMAP_EXT_ID *mapkey; long mapvalue; getport_comm *commarg;
{ xTrace0(pmapp,TR_FULL_TRACE,"pmap_getport_filter: entered\n");
  if ((mapkey->prog == commarg->prog) && (mapkey->prot == commarg->prot))
  { xTrace3(pmapp,TR_EVENTS,
	    "pmap_getport_filter: found program %d version %d protocol %d\n",
	    mapkey->prog,mapkey->vers,mapkey->prot);
    commarg->vers = mapkey->vers; commarg->port = mapvalue;
    return 0;};
  return MFE_CONTINUE; }  


static long * pmapproc_getport(args) PMAP_ARGS *args;
{ PMAP_EXT_ID ext_id; long *reply; getport_comm prog_prot;

  xTrace0(pmapp,TR_MAJOR_EVENTS,"pmap_getport: entered\n");
  xIfTrace(pmapp,TR_MAJOR_EVENTS) print_pmap(args);

  ext_id.prog = args->prog;
  ext_id.vers = args->vers;
  ext_id.prot = args->prot;

  reply = (long *) xMalloc(sizeof(long));
  if ( mapResolve(port_map, &ext_id, reply) == XK_FAILURE ) 
      /* failed: scan for another version of program */
  { xTrace3(pmapp,TR_ERRORS,
	 "pmap_getport: can't find exact program %d version %d protocol %d\n",
	    ext_id.prog, ext_id.vers, ext_id.prot);
    prog_prot.prog = ext_id.prog; prog_prot.prot = ext_id.prot;
    prog_prot.vers = -1; prog_prot.port = -1;
    mapForEach(port_map, pmap_getport_filter, &prog_prot);
    if (prog_prot.port == -1)
    { xTrace0(pmapp,TR_ERRORS,
	      "pmap_getport: can't find any version of program\n");}
    else { *reply = prog_prot.port;
	   xTrace1(pmapp,TR_ERRORS,
		   "pmap_getport: found version %d\n",prog_prot.vers);}; }
  xTrace1(pmapp,TR_MAJOR_EVENTS,"pmap_getport: port = %d\n",*reply);
  return reply; }

static long dump_space_used;

static int pmap_xdr_entry (mapkey, mapvalue, xdrs)
PMAP_EXT_ID *mapkey; long mapvalue; XKXDR *xdrs;
{ xkxdr_long(xdrs,TRUE);  /* this XDR code means "more follows" */
  xkxdr_long(xdrs,mapkey->prog);  xkxdr_long(xdrs,mapkey->vers);
  xkxdr_long(xdrs,mapkey->prot);  xkxdr_long(xdrs,mapvalue);
  dump_space_used += 5*sizeof(long);
  /* check free space */
  if (dump_space_used > MAX_SUNRPC_MSG_SIZE - sizeof(long)) return FALSE;
  return MFE_CONTINUE; }  


/* use mapForEach to write a  blurb about each entry in the portmap */
/* only fill up MAX_SUNRPC_MSG_SIZE bytes; forget any others */
/* reply is used differently: here it actually holds the xdr results */
/* store the length of the used part of the buffer into *alen */

static long *pmapproc_dump(alen) long *alen;
{ char *reply; XKXDR xdrs;

  xTrace0(pmapp,TR_MAJOR_EVENTS,"pmapproc_dump: entered\n");
  reply = (char *) xMalloc(MAX_SUNRPC_MSG_SIZE);
  dump_space_used = 0;
  xkxdr_init(xdrs,reply,MAX_SUNRPC_MSG_SIZE,XDR_ENCODE);
  mapForEach(port_map,pmap_xdr_entry,&xdrs);
  xkxdr_long(&xdrs,FALSE);  /* a note saying "no more stuff" */
  *alen = dump_space_used + sizeof(long);
  return (long *)reply; }

/* for callit */
/* client sends prog, vers, proc, args... */
/* garbage collection issue: how to return storage if we make a
   positive decision not to reply? */
/* need to pass in the reply message, and the remaining piece of the call
   message.
   this also comes up in "dump" routine.
   this is worse because reply must be handled in forwarding structure.
   what about freeing of call message? */
static void pmapproc_callit(msg,msgr,buf,len)
Msg *msg, *msgr; char *buf; long len;
{ PMAP_EXT_ID ext_id; XKXDR xdrs; Part part[2]; long prog, vers, port; XObj s;
  Sess_Msg_Pair *smp;

  xTrace0(pmapp,TR_FULL_TRACE,"entering pmapproc_callit");
  xkxdr_init(xdrs,buf,8,XDR_DECODE);
  /* get prog,vers */
  /* check length */
  ext_id.prog = prog = xkxdr_long_decode(&xdrs);
  ext_id.vers = vers = xkxdr_long_decode(&xdrs);
  ext_id.prot = IPPROTO_UDP;
  
  /*  lookup
      ...-> fail */
  if ( mapResolve(port_map,&ext_id, &port) == XK_FAILURE ) {
      return; /* just forget it, but add trace message */
  }
  xTrace1(pmapp,TR_FULL_TRACE,"callit mapresolve worked.  port = %d",port);

  /* get xid?
     open session to server */

  partInit(part, 1);
  partPush(part[0], (long *) &pmap_ipaddr, sizeof(IPhost));
  partPush(part[0], (long *) &port, sizeof(long));
  partPush(part[0], (long *) &prog, sizeof(long));
  partPush(part[0], (long *) &vers, sizeof(long));

  s = xOpen(PMAP, PMAP, SUNRPC, part);
  /* better error message; make conditional on trace? */
  if (s == ERR_XOBJ)
  { xError("Portmapper can't open session to server."); return; };
  /* xAssert(xIsSession(s)); */
  xTrace0(pmapp,TR_FULL_TRACE,"open session to server worked");

  /*  index on session?  map */
  /*  store msgr associated with s */
  smp = (Sess_Msg_Pair *) xMalloc(sizeof(Sess_Msg_Pair));
  smp->session = s; smp->msg = msgr;

  /* should smp have the &? */
  if (mapBind(call_map,&smp,msgr) == (Bind) -1 )
  { xTrace0(pmapp,TR_ERRORS,"pmap_callit: bind fails");
    /* need to fail out gracefully */ };
  xTrace0(pmapp,TR_FUNCTIONAL_TRACE,"pmap_callit: bind succeeds");

  /*  call server with remainder of message */
  if (len>=2*sizeof(long))
    /* copy data into forwarded message */
    msgConstructBuffer(msg,buf+2*sizeof(long),len-2*sizeof(long));
  else msgConstructEmpty(msg);
  xTrace0(pmapp,TR_FULL_TRACE,"built message; shipping it ...");

  xPush(s,msg);  /* send on remainder of message! */
  xTrace0(pmapp,TR_FULL_TRACE,"shipped it; returning from callit");

  /* should I xFree(buf)? */
  return;  /* don't want return value */
  /*   ...? reply returns here?  unhuh -- need demux! */
}

#if 0
/* if Sunrpc call fails, need to cleanup call_map entry */
/* demux should only be used for callit replies; maybe it should
   defend itself from others, or handle them? */
static xkern_return_t pmap_demux(self,lls,msg) XObj self,lls; Msg *msg;
{ Sess_Msg_Pair *smp; Msg *msgr;

  xTrace0(pmapp,TR_FULL_TRACE,"entered pmap_demux");
  if ( mapResolve(call_map, &lls, &smp) == XK_FAILURE ) {  /* is &lls right? */
      xTrace0(pmapp,TR_FULL_TRACE,"map resolve for session lookup failed");
      return XK_SUCCESS;
  }
  xTrace0(pmapp,TR_FULL_TRACE,"map resolve for session lookup succeeded");
  msgr = smp->msg;
  /*  copy operation msg to msgr */
  /* looks like I need to save the original session, too. */

  xTrace0(pmapp,TR_FULL_TRACE,"pushing message for reply to client");
  xPush(smp->session,msgr);
  xTrace0(pmapp,TR_FULL_TRACE,
	  "pushed message for reply to client; freeing junk");
  xClose(lls); msgDestroy(msg); mapUnbind(call_map,lls); xFree(smp);/*cleanup*/
  xTrace0(pmapp,TR_FULL_TRACE,"exiting pmap_demux");
  return XK_SUCCESS;
}

/* need to add timeout code for cleaning up failed rpc calls.
   might just record time, and discard an oldie when map fills up */
/* or maybe reply returns here ... ? */
/* look up session
   close session
     if reply is "failed", drop it.  ?GC client reply?
   forward reply to client */
#endif

void print_pmap(hdr) PMAP_ARGS *hdr;
{ printf("pmap-prog = %d, pmap-vers = %d, pmap-prot = %d, pmap-port = %d\n",
	 hdr->prog, hdr->vers, hdr->prot, hdr->port); }


void print_xdr(a,b) char *a; long b;
{ long i;
  for (i=0;i<b;i++)
  { printf(" %2x", *(a+i) & 0xff); if (i%16==15) printf("\n");};
  printf("\n"); }

/* end of xkpm.c */


