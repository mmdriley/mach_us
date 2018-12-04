/*
 * xkpm_client.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.13 $
 * $Date: 1993/02/01 22:27:30 $
 *
 * Test client for the X-kernel version of the portmapper,
 * by Rich Schroeppel.  It exercises the portmapper, and
 * uses the information to contact the Determinant Server.
 */

#include <rpc/rpc.h>
#include "xkernel.h"
#include "ip.h"
#include "udp.h"
/* #include "sunrpc.h" */
#include "xkpm.h"
#include "xkpm_test.h"


#ifdef __STDC__

void	client_to_pmap( Event, VOID * );
void	pmapcln_init_part2( Event, VOID * );

#else

void 	client_to_pmap();
void 	pmapcln_init_part2();

#endif


long tracepmapclnp;
XObj cln_xobj=0;
#define SUNRPC xGetDown(cln_xobj,0)
XObj cln_pmap_session=0, cln_svr_session=0;
extern long yabcopy();
long srvport;
void test_server();
IPhost IPServer; long dash_h=0;
extern long run_server, run_portmapper, portmapper_portnumber;
long run_client=1, client_test_server=1;

extern void xkxdr_long();
extern long xkxdr_long_decode();


void xkpm_options()
{ long i; char *arg, *host=0;
  run_client=0; client_test_server=0; run_server=0; run_portmapper=0;
  for (i=1; i<globalArgc; i++)
  { arg=globalArgv[i];
    if (!strcmp(arg,"-p")) run_portmapper=1;
    if (!strcmp(arg,"-c")) run_client=1;
    if (!strcmp(arg,"-t")) client_test_server=1;
    if (!strcmp(arg,"-s")) run_server=1;
    if (!strcmp(arg,"-h"))
      (dash_h=1,str2ipHost(&IPServer,host=globalArgv[i+1]));
    if (!strcmp(arg,"-n"))
      sscanf(globalArgv[i+1],"%d",&portmapper_portnumber); }
  printf("Parameters:  P=%d N=%d C=%d T=%d S=%d H=%s=%x\n",
	 run_portmapper, portmapper_portnumber, run_client,
	 client_test_server, run_server, host, IPServer); }

void pmapcln_init(self) XObj self;
{ xkpm_options();

  xTrace0(pmapclnp, TR_MAJOR_EVENTS, "pmt_client init");

  if (!run_client)
  { xTrace0(pmapclnp, TR_MAJOR_EVENTS, "not running client\n");
    return; }

  if (cln_xobj) Kabort("pmt_client: init called twice?");
  cln_xobj = self;
  /* schedule rest of startup, after pmap initialized */
  evSchedule(pmapcln_init_part2,0,5000000);

  xTrace0(pmapclnp, TR_MAJOR_EVENTS, "pmt_client init returning");
  return;
}

void pmapcln_init_part2(ev, arg) Event ev; VOID *arg;  /* arg is not used */
{ Part part[2];
  IPhost pmipaddr;
  long pmport=PMAP_PORT, pmprog=PMAP_PROG, pmvers=PMAP_VERS;

  xTrace0(pmapclnp, TR_MAJOR_EVENTS, "pmt_client init (part2)");

  /* get the portmapper port number from the command line */
  pmport=portmapper_portnumber;

  /* get the server machine IP address from the command line */
  /* if -h option unspecified, assume server machine same as client */
  if (!dash_h)
  { if (xControl(SUNRPC, GETMYHOST, (char *)&IPServer, sizeof(IPhost)) < 0)
    { printf("Can't find my IP addr\n"); return; }; };

  bcopy((char *) &IPServer, (char *) &pmipaddr, sizeof(IPhost));

  partInit(part, 1);
  partPush(part[0], (long *) &pmipaddr, sizeof(IPhost));
  partPush(part[0], (long *) &pmport, sizeof(long));
  partPush(part[0], (long *) &pmprog, sizeof(long));
  partPush(part[0], (long *) &pmvers, sizeof(long));

  cln_pmap_session = xOpen(cln_xobj, cln_xobj, SUNRPC, part);
  if (cln_pmap_session == ERR_XOBJ)
  { printf("Client can't open session to Portmapper.\n"); return; };
  /* xAssert(xIsSession(cln_pmap_session)); */

  evSchedule(client_to_pmap,cln_pmap_session,5000000);
  xTrace0(pmapclnp, TR_FULL_TRACE, "pmt client startup test scheduled");

  xTrace0(pmapclnp, TR_FULL_TRACE, "pmt client init (part2) returns OK");

  return;
}

void client_send_test_message(session,whom,data,size,reply)
XObj session; char *whom, *data; long size; Msg *reply;
{ Msg msg;

  xAssert(xIsSession(session));
  if (size) msgConstructBuffer(&msg,data,size);
  else msgConstructEmpty(&msg);
  msgConstructEmpty(reply);
  xTrace4(pmapclnp, TR_FULL_TRACE,
	  "pmtclient sending message %d of length %d to %s with reply %d",
	  &msg, msgLen(&msg), whom, reply);

  if (xCall(session,&msg,reply) == XK_FAILURE)
  { xTrace1(pmapclnp,TR_ERRORS,"pmtclient: error sending message to %s",whom);}
  else { xTrace1(pmapclnp,TR_FULL_TRACE,"pmtclient sent message to %s",whom);};
  msgDestroy(&msg);
}

void xkxdr_long_array(name,address,length) XKXDR *name; long *address, length;
{ long i; for (i=0;i<length;i++) xkxdr_long(name,address[i]); }

#ifndef SUNXDR
union dbl2longs { double d; struct {long l1,l2;} l; };
#endif

void xkxdr_double(name,data) XKXDR *name; double data;
#ifdef SUNXDR
{ if (!xdr_double(name,&data))
    printf("xdr failed! name = %d, data = %f\n",name,data); }
#else
{ union dbl2longs bits;
  bits.d = data; xkxdr_long(name, bits.l.l1); xkxdr_long(name, bits.l.l2); }
#endif

void xkxdr_double_array(name,address,length)
XKXDR *name; double *address; long length;
{ long i; for (i=0;i<length;i++) xkxdr_double(name,address[i]); }

double xkxdr_double_decode(name) XKXDR *name;
#ifdef SUNXDR
{ double data;
  if (!xdr_double(name,&data))
    printf("xdr failed! name = %d, data = %f\n",name,data);
  return data; }
#else
{ union dbl2longs data;
  data.l.l1 = xkxdr_long_decode(name); data.l.l2 = xkxdr_long_decode(name);
  return data.d; }
#endif


#define TESTARG ((((127*256)+162)*256+163)*256+164)

void client_to_pmap(ev, arg) Event ev; VOID * arg;
{ Msg msgr;
  char buf[100], data[4], moredata[100], i, vbuf[100],
       buf7[100], buf7r[100], *dumpbuf=0, buf8[100];
  XKXDR xdrs, xdrr, xdrt, xdru, xdrv, xdrw, xdrx, xdry;
  static long lookup_test[]={PMAPPROC_GETPORT,999,3,IPPROTO_UDP,0},
	     lookup_test2[]={PMAPPROC_GETPORT,DET_PROG,DET_VERS,IPPROTO_UDP,0},
             /* an alternative, to lookup the status server */
             lookup_test3[]={PMAPPROC_GETPORT,100024,1,IPPROTO_UDP,0},
             idata[]={PMAPPROC_CALLIT,DET_PROG,DET_VERS,DET_LONG,100,25,34,7};
  long j, m6val, m7val, rlen, more = 0;

  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient entered");

  /* see if portmapper present -- ping test */
  client_send_test_message(cln_pmap_session,"pmap",0,0,&msgr);
  msgDestroy(&msgr);

  data[0] = data[1] = data[2] = data[3] = 0;
  client_send_test_message(cln_pmap_session,"pmap",data,4,&msgr);
  msgDestroy(&msgr);

  for (i=0;i<100;i++) moredata[i] = i*i+50;
  client_send_test_message(cln_pmap_session,"pmap",moredata,100,&msgr);
  msgDestroy(&msgr);

  xTrace1(pmapclnp, TR_FULL_TRACE, "pmtclient xdring test arg %d", TESTARG);
  xkxdr_init(xdrs,buf,4,XDR_ENCODE);
  xkxdr_long(&xdrs,TESTARG);
  client_send_test_message(cln_pmap_session,"pmap",buf,100,&msgr);
  msgDestroy(&msgr);

  xTrace5(pmapclnp, TR_FULL_TRACE, "pmtclient xdring test args %d %d %d %d %d",
	  lookup_test[0],lookup_test[1],lookup_test[2],lookup_test[3],
	  lookup_test[4] );
  xkxdr_init(xdrs,buf,100,XDR_ENCODE);
  xkxdr_long_array(&xdrs,lookup_test,5);
  client_send_test_message(cln_pmap_session,"pmap",buf,100,&msgr);
  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient unwinding reply");
  msgPop(&msgr,yabcopy,vbuf,4,0);
  msgDestroy(&msgr);
  xkxdr_init(xdrr,vbuf,4,XDR_DECODE);
  m6val = xkxdr_long_decode(&xdrr);
  xTrace1(pmapclnp, TR_FULL_TRACE, "pmtclient reply = %d", m6val);

  /* ask the portmapper to lookup the status server */
  xTrace5(pmapclnp, TR_FULL_TRACE, "pmtclient xdring test args %d %d %d %d %d",
	  lookup_test3[0],lookup_test3[1],lookup_test3[2],lookup_test3[3],
	  lookup_test3[4] );
  xkxdr_init(xdrt,buf7,100,XDR_ENCODE);
  xkxdr_long_array(&xdrt,lookup_test3,5);
  client_send_test_message(cln_pmap_session,"pmap",buf7,100,&msgr);
  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient unwinding reply");
  msgPop(&msgr,yabcopy,buf7r,4,0);
  msgDestroy(&msgr);
  xkxdr_init(xdru,buf7r,4,XDR_DECODE);
  m7val = xkxdr_long_decode(&xdru);
  xTrace1(pmapclnp, TR_FULL_TRACE, "pmtclient reply = %d", m7val);

  xTrace5(pmapclnp, TR_FULL_TRACE, "pmtclient xdring test args %d %d %d %d %d",
	  lookup_test2[0],lookup_test2[1],lookup_test2[2],lookup_test2[3],
	  lookup_test2[4] );
  xkxdr_init(xdrt,buf7,100,XDR_ENCODE);
  xkxdr_long_array(&xdrt,lookup_test2,5);
  client_send_test_message(cln_pmap_session,"pmap",buf7,100,&msgr);
  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient unwinding reply");
  msgPop(&msgr,yabcopy,buf7r,4,0);
  msgDestroy(&msgr);
  xkxdr_init(xdru,buf7r,4,XDR_DECODE);
  m7val = xkxdr_long_decode(&xdru);
  xTrace1(pmapclnp, TR_FULL_TRACE, "pmtclient reply = %d", m7val);

  srvport = m7val;
  if (client_test_server) test_server();
  else printf("client skipping test of server\n");

  xTrace1(pmapclnp, TR_FULL_TRACE,
	  "pmtclient dump test: xdring test arg %d", PMAPPROC_DUMP);
  xkxdr_init(xdrv,data,4,XDR_ENCODE);
  xkxdr_long(&xdrv,PMAPPROC_DUMP);
  client_send_test_message(cln_pmap_session,"pmap",data,4,&msgr);
  rlen = msgLen(&msgr);
  xTrace1(pmapclnp, TR_FULL_TRACE,
	  "pmtclient unwinding reply of length %d",rlen);
  if (rlen>0) { dumpbuf = xMalloc(rlen);
		msgPop(&msgr,yabcopy,dumpbuf,rlen,0); };
  msgDestroy(&msgr);
  xkxdr_init(xdrw,dumpbuf,rlen,XDR_DECODE);

  printf("Pmap Dump results:\n");
  for (j=0;                     /* single "=" below is correct */
       (j + sizeof(long) <= rlen) && (more = xkxdr_long_decode(&xdrw));
       j += 5*sizeof(long))
  { long v1,v2,v3,v4;
    v1 = xkxdr_long_decode(&xdrw); v2 = xkxdr_long_decode(&xdrw);
    v3 = xkxdr_long_decode(&xdrw); v4 = xkxdr_long_decode(&xdrw);
    printf("more = %d  prog = %d  vers = %d  prot = %d  port = %d\n",
	   more,v1,v2,v3,v4);}
  printf("more = %d\n", more);
  if (dumpbuf) xFree(dumpbuf);

  return;  /* skip testing of portmapper indirect call option */

#if 0
  xTrace0(pmapclnp, TR_FULL_TRACE, "testing indirect call to detsrv via pmap");
  xkxdr_init(xdrx,buf8,8*sizeof(long),XDR_ENCODE);
  xkxdr_long_array(&xdrx,idata,8);
  client_send_test_message(cln_pmap_session,"pmap",buf8,8*sizeof(long),&msgr);
  rlen = msgLen(&msgr);
  xTrace1(pmapclnp, TR_FULL_TRACE,
	  "pmtclient unwinding reply of length %d",rlen);
  if (rlen>=sizeof(long))
  { dumpbuf = xMalloc(rlen);
    msgPop(&msgr,yabcopy,dumpbuf,rlen,0);
    xkxdr_init(xdry,dumpbuf,rlen,XDR_DECODE);
    printf("Indirect Call value = %d\n", xkxdr_long_decode(&xdry)); }
  else printf("Indirect Call reply too short, rlen=%d\n",rlen);
  msgDestroy(&msgr);
#endif
}

void test_server()
{ Part part[2]; IPhost srvipaddr;
  long srvprog=DET_PROG, srvvers=DET_VERS, testval, testsb=8;
  static long dtesti[]={50,31,32,20};
  static double dtestd[]={50.0,31.0,32.0,20.0},
	        dtestz[]={50.0,-5.0,31.0,.3125,32.0,-27.5,20.0,20.5};
  double testvald, testdsb=8.0,
	 testvalr, testvali, testzrsb=101.90625, testzisb=1767.5;
  XKXDR xdrs, xdrr; char buf[100], vbuf[100]; Msg msgr; long i;

  bcopy((char *) &IPServer, (char *) &srvipaddr, sizeof(IPhost));

/* now actually do some testing */
/* open a session to the determinant server */

  partInit(part, 1);
  partPush(part[0], (long *) &srvipaddr, sizeof(IPhost));
  partPush(part[0], (long *) &srvport, sizeof(long));
  partPush(part[0], (long *) &srvprog, sizeof(long));
  partPush(part[0], (long *) &srvvers, sizeof(long));

  cln_svr_session = xOpen(cln_xobj, cln_xobj, SUNRPC, part);
  if (cln_svr_session == ERR_XOBJ)
  { printf("Can't open server session.\n"); return /*XK_FAILURE*/; };
  /* xAssert(xIsSession(cln_svr_session)); */

  /* ping the DET server */

  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient pinging the DET server");
  xkxdr_init(xdrs,buf,100,XDR_ENCODE);
  xkxdr_long(&xdrs,DET_NULL);
  client_send_test_message(cln_svr_session,"detsrv",buf,100,&msgr);
  xTrace1(pmapclnp, TR_FULL_TRACE, "pmtclient reply length %d", msgLen(&msgr));
  msgDestroy(&msgr);

  /* send a problem to the DET server */

  xTrace5(pmapclnp, TR_FULL_TRACE, "pmtclient xdring test args %d %d %d %d %d",
	  DET_LONG, dtesti[0], dtesti[1], dtesti[2], dtesti[3] );
  xkxdr_init(xdrs,buf,100,XDR_ENCODE);
  xkxdr_long(&xdrs,DET_LONG);
  xkxdr_long_array(&xdrs,dtesti,4);
  client_send_test_message(cln_svr_session,"detsrv",buf,100,&msgr);
  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient unwinding reply");
  msgPop(&msgr,yabcopy,vbuf,4,0);
  msgDestroy(&msgr);
  xkxdr_init(xdrr,vbuf,4,XDR_DECODE);
  testval = xkxdr_long_decode(&xdrr);
  xTrace1(pmapclnp, TR_FULL_TRACE, "pmtclient reply = %d", testval);
  if (testval != testsb)
    printf("pmtclient: error in determinant value.  Is %d, should be %d.\n",
	   testval, testsb);

  xTrace5(pmapclnp, TR_FULL_TRACE, "pmtclient xdring test args %d %f %f %f %f",
	  DET_DOUBLE, dtestd[0], dtestd[1], dtestd[2], dtestd[3] );
  xkxdr_init(xdrs,buf,100,XDR_ENCODE);
  xkxdr_long(&xdrs,DET_DOUBLE);
  xkxdr_double_array(&xdrs,dtestd,4);
  client_send_test_message(cln_svr_session,"detsrv",buf,100,&msgr);
  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient unwinding reply");
  msgPop(&msgr,yabcopy,vbuf,8,0);
  msgDestroy(&msgr);
  xkxdr_init(xdrr,vbuf,8,XDR_DECODE);
  testvald = xkxdr_double_decode(&xdrr);
  xTrace1(pmapclnp, TR_FULL_TRACE, "pmtclient reply = %f", testvald);
  if (testvald != testdsb)
    printf("pmtclient: error in determinant value.  Is %f, should be %f.\n",
	   testvald, testdsb);

  xIfTrace(pmapclnp, TR_FULL_TRACE) {
    printf("pmtclient xdring test args %d  %f %+fi  %f %+fi  %f %+fi  %f %+fi",
	   DET_COMPLEX, dtestz[0], dtestz[1], dtestz[2], dtestz[3],
	   dtestz[4], dtestz[5], dtestz[6], dtestz[7] );};
  xkxdr_init(xdrs,buf,100,XDR_ENCODE);
  xkxdr_long(&xdrs,DET_COMPLEX);
  xkxdr_double_array(&xdrs,dtestz,8);
  client_send_test_message(cln_svr_session,"detsrv",buf,100,&msgr);
  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient unwinding reply");
  msgPop(&msgr,yabcopy,vbuf,16,0);
  msgDestroy(&msgr);
  for (i=0;i<16;i++) printf(" %2x", vbuf[i]&0xff); printf("\n");
  xkxdr_init(xdrr,vbuf,16,XDR_DECODE);
  testvalr = xkxdr_double_decode(&xdrr);
  testvali = xkxdr_double_decode(&xdrr);
  printf("testvalr = %f\n", testvalr);
  printf("testvali = %f\n", testvali);
  xIfTrace(pmapclnp, TR_FULL_TRACE) {
    printf("pmtclient reply = %f %+fi\n", testvalr, testvali);};
  if ((testvalr != testzrsb) || (testvali != testzisb))
    printf(
     "pmtclient: error in determinant value.  Is %f+%fi, should be %f+%fi.\n",
     testvalr, testvali, testzrsb, testzisb);

  xTrace0(pmapclnp, TR_FULL_TRACE, "pmtclient: end of server tests");
 }


/* end of xkpm_client.c */

