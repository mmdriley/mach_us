/*
 * xkpm_server.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.14 $
 * $Date: 1993/02/01 22:27:53 $
 *
 * Test server for the X-kernel version of the portmapper,
 * by Rich Schroeppel.
 *
 * Determinant Server:
 *
 * This program calculates 2x2 determinants for integers, doubles,
 * and complexes.  The client sends a message containing the
 * procedure number (1 = (long) integer, 2 = double, 3 = complex)
 * and the four arguments.  This program calculates the determinant
 * and replies with a message containing the answer, of the same
 * numeric type.
 */

#include <rpc/rpc.h>
#include "xkernel.h"
#include "ip.h"
#include "udp.h"
#include "sunrpc.h"
#include "xkpm.h"
#include "xkpm_test.h"

#define SUNRPC xGetDown(srv_xobj,0)
long tracepmapsrvp;
void print_xdr();

void xkxdr_long();
long xkxdr_long_decode();
void xkxdr_long_array();
void xkxdr_double();
double xkxdr_double_decode();


long detint(a,b,c,d) long a,b,c,d; { return a*d-b*c; }

double detdouble(a,b,c,d) double a,b,c,d; { return a*d-b*c; }


/* The complex arithmetic takes the actual structures as arguments,
 * but returns a pointer to a (freshly allocated) answer.
 * Complex arithmetic must do freeing of intermediate structures.
 */

typedef struct {double re,im;} complex;

void print_z(z) complex z; { printf("  %f %+f i",z.re,z.im); }

complex *zdif(a,b) complex a,b;
{ complex *c; c = (complex *) xMalloc(sizeof(complex));
  c->re = a.re - b.re; c->im = a.im - b.im;
  /* printf("zdif: %f %+f\n", c->re, c->im); */
  return c; }

complex *ztimes(a,b) complex a,b;
{ complex *c; c = (complex *) xMalloc(sizeof(complex));
  c->re = a.re * b.re - a.im * b.im; c->im = a.im * b.re + a.re * b.im;
  /* printf("ztimes: %f %+f\n", c->re, c->im); */
  return c; }

complex *detz(a,b,c,d) complex a,b,c,d;
{ complex *ad,*bc,*det; ad=ztimes(a,d); bc=ztimes(b,c); det=zdif(*ad,*bc);
  xFree((char *)ad); xFree((char *)bc);
  /* printf("detz: %f %+f\n", det->re, det->im); */
  return det; }

void xkxdr_complex(xdrs,z) XKXDR *xdrs; complex *z;
{ xkxdr_double(xdrs,z->re); xkxdr_double(xdrs,z->im); }

void xkxdr_complex_decode(xdrs,z) XKXDR *xdrs; complex *z;
{ z->re = xkxdr_double_decode(xdrs); z->im = xkxdr_double_decode(xdrs); }


typedef struct {long a,b,c,d; double ad,bd,cd,dd; complex az,bz,cz,dz; }
  Det_args;

long* det_int(abcd) Det_args *abcd;
{ long *reply;
  reply = (long *) xMalloc(sizeof(long));
  *reply = detint(abcd->a,abcd->b,abcd->c,abcd->d);
  return reply; }

double* det_double(abcd) Det_args *abcd;
{ double *reply;
  reply = (double *) xMalloc(sizeof(double));
  *reply = detdouble(abcd->ad,abcd->bd,abcd->cd,abcd->dd);
  return reply; }

complex* det_z(abcd) Det_args *abcd;
{ return detz(abcd->az,abcd->bz,abcd->cz,abcd->dz); }


/* xdr a determinant argument list  */
/*
long xdr_det(xdrs,fmt,abcd)
XKXDR *xdrs; long fmt; Det_args *abcd;
{ switch (fmt)
  {case DET_LONG:
     return xdr_long(xdrs,&abcd->a) && xdr_long(xdrs,&abcd->b) &&
	    xdr_long(xdrs,&abcd->c) && xdr_long(xdrs,&abcd->d);
   case DET_DOUBLE:
     return xdr_double(xdrs,&abcd->ad) && xdr_double(xdrs,&abcd->bd) &&
	    xdr_double(xdrs,&abcd->cd) && xdr_double(xdrs,&abcd->dd);
   case DET_COMPLEX:
     return xdr_complex(xdrs,&abcd->az) && xdr_complex(xdrs,&abcd->bz) &&
	    xdr_complex(xdrs,&abcd->cz) && xdr_complex(xdrs,&abcd->dz); };
  return FALSE; }
*/

long xkxdr_det_decode(xdrs,len,fmt,abcd)
XKXDR *xdrs; long len, fmt; Det_args *abcd;
{ /* printf("entered xkxdr_det_decode\n"); */
  /* printf("xdrs %d  len %d  fmt %d  abcd %d\n",xdrs,len,fmt,abcd); */
  switch (fmt)  /* check length, and return false if trouble */
  {case DET_LONG:
     if (len < 4*sizeof(long)) return FALSE;
     abcd->a = xkxdr_long_decode(xdrs); abcd->b = xkxdr_long_decode(xdrs);
     abcd->c = xkxdr_long_decode(xdrs); abcd->d = xkxdr_long_decode(xdrs);
     break;
   case DET_DOUBLE:
     if (len < 4*sizeof(double)) return FALSE;
    abcd->ad = xkxdr_double_decode(xdrs); abcd->bd = xkxdr_double_decode(xdrs);
    abcd->cd = xkxdr_double_decode(xdrs); abcd->dd = xkxdr_double_decode(xdrs);
    break;
   case DET_COMPLEX:
     if (len < 4*sizeof(complex)) return FALSE;
    xkxdr_complex_decode(xdrs,&abcd->az); xkxdr_complex_decode(xdrs,&abcd->bz);
    xkxdr_complex_decode(xdrs,&abcd->cz); xkxdr_complex_decode(xdrs,&abcd->dz);
   };
  return TRUE; }


void print_det_args(fmt,abcd) long fmt; Det_args *abcd;
{ switch (fmt)
  {case DET_LONG:
   printf("det args = %d %d %d %d\n",abcd->a,abcd->b,abcd->c,abcd->d); break;
   case DET_DOUBLE:
   printf("det args = %f %f %f %f\n",abcd->ad,abcd->bd,abcd->cd,abcd->dd);
   break;
   case DET_COMPLEX:
   printf("det args ="); print_z(abcd->az); print_z(abcd->bz);
   print_z(abcd->cz); print_z(abcd->dz); printf("\n"); }; }



#ifdef __STDC__

void	server_to_pmap( Event, VOID * );
void	server_to_pmap2( Event, VOID * );

#else

void 	server_to_pmap();
void 	server_to_pmap2();

#endif


static XObj srv_xobj=0;
XObj svr_pmap_session=0;
extern long yabcopy();
static xkern_return_t det_calldemux();
extern long portmapper_portnumber;
long run_server=1;

static xkern_return_t det_opendone(self,s,part)
XObj s,self; Part part;
{ xTrace0(pmapsrvp,TR_FUNCTIONAL_TRACE,"--->det_opendone");
  return XK_SUCCESS;  /* always succeed */  }


/* tell sunrpc we are available for service
   open session to register with portmapper
   schedule registration & deregistration */
void pmapsrv_init(self) XObj self;
{ Part part[2], pmpart[2];
  IPhost myipaddr;
  long myport=DET_PORT, myprog=DET_PROG, myvers=DET_VERS,
       pm_port=PMAP_PORT, pm_prog=PMAP_PROG, pm_vers=PMAP_VERS;

  printf("pmtserver_init\n");
  if (!run_server) { printf("not running server\n"); return; }
  pm_port=portmapper_portnumber; /* set the portmapper port number */

  xTrace0(pmapsrvp, TR_MAJOR_EVENTS, "pmt_server init");
  if (srv_xobj) Kabort("pmt_server: init called twice?");
  srv_xobj = self;

  if (xControl(SUNRPC, GETMYHOST, (char *)&myipaddr, sizeof myipaddr) < 0)
  { printf("Can't find my IP addr\n"); return; }

  /* tell sunrpc we're open for business */
  partInit(part, 1);
  partPush(part[0], (long *) &myport, sizeof(long));
  partPush(part[0], (long *) &myprog, sizeof(long));
  partPush(part[0], (long *) &myvers, sizeof(long));
  srv_xobj->opendone = det_opendone;
  srv_xobj->calldemux = det_calldemux;
  if (xOpenEnable(srv_xobj, srv_xobj, SUNRPC, part) == XK_FAILURE)
    Kabort("pmtserver xOpenEnable to SUNRPC failed");
  xTrace0(pmapsrvp, TR_FULL_TRACE, "pmt server init openenable ok");

  /* tell sunrpc we want to talk to the portmapper */
  partInit(pmpart, 1);
  partPush(pmpart[0], (long *) &myipaddr, sizeof(IPhost));
  partPush(pmpart[0], (long *) &pm_port, sizeof(long));
  partPush(pmpart[0], (long *) &pm_prog, sizeof(long));
  partPush(pmpart[0], (long *) &pm_vers, sizeof(long));
  svr_pmap_session = xOpen(srv_xobj, srv_xobj, SUNRPC, pmpart);
  if (svr_pmap_session == ERR_XOBJ)
    Kabort("pmtserver unable to open SUNRPC session to portmapper");

  /* schedule registration with portmapper, immediately after initialization */
  evSchedule(server_to_pmap,svr_pmap_session,0);
  /* schedule deregistration with portmapper, after 30 seconds */
  evSchedule(server_to_pmap2,svr_pmap_session,30*1000000);

  xTrace0(pmapsrvp, TR_FULL_TRACE, "pmt server init returns OK");
}

/* register the determinant server with the portmapper */
void server_to_pmap(ev, arg) Event ev; VOID * arg;
{ Msg msg, msgr; char buf[5*sizeof(long)];
  xkern_return_t val;
  XKXDR xdrs,xdrr;
  static long pmreg[] = {PMAPPROC_SET,DET_PROG,DET_VERS,IPPROTO_UDP,DET_PORT};
  long rlen, rval; char rbuf[sizeof(long)];

  xTrace5(pmapsrvp, TR_FULL_TRACE, "pmtserver xdring test args %d %d %d %d %d",
	  pmreg[0], pmreg[1], pmreg[2], pmreg[3], pmreg[4] );
  xkxdr_init(xdrs,buf,5*sizeof(long),XDR_ENCODE);
  xkxdr_long_array(&xdrs,pmreg,5);
  xIfTrace(pmapsrvp, TR_FULL_TRACE) print_xdr(buf,5*sizeof(long));
  msgConstructBuffer(&msg,buf,5*sizeof(long));
  msgConstructEmpty(&msgr);
  xTrace0(pmapsrvp, TR_FULL_TRACE, "pmtserver built registration message");
  xTrace1(pmapsrvp, TR_FULL_TRACE, "pmtserver msg length = %d", msgLen(&msg));
  val = xCall(svr_pmap_session,&msg,&msgr);
  xTrace1(pmapsrvp, TR_FULL_TRACE,
	  "pmtserver sent registration message; val = %d",val);
  if (val == XK_FAILURE)
    Kabort(
      "determinant server: error sending registration message to portmapper");
  if ((rlen = msgLen(&msgr))<sizeof(long))
  { printf("pmtserver: reply from portmapper is too short, %d",rlen);
    Kabort("determinant server: received too short reply from portmapper"); }
  msgPop(&msgr,yabcopy,rbuf,sizeof(long),0);
  xkxdr_init(xdrr,rbuf,sizeof(long),XDR_DECODE);
  rval = xkxdr_long_decode(&xdrr);
  xTrace1(pmapsrvp, TR_FULL_TRACE, "pmtserver: portmapper reply = %d", rval);
  if (!rval) Kabort("pmtserver: portmapper refused to register server");
  xTrace0(pmapsrvp, TR_FULL_TRACE, "pmtserver registered with portmapper");
  msgDestroy(&msg);
  msgDestroy(&msgr);
}

/* unregister the determinant server with the portmapper */
void server_to_pmap2(ev, arg) Event ev; VOID * arg;
{ Msg msg, msgr; char buf[5*sizeof(long)];
  xkern_return_t val;
  XKXDR xdrs,xdrr;
  static long
    pmunreg[] = {PMAPPROC_UNSET,DET_PROG,DET_VERS+1,IPPROTO_UDP,DET_PORT};
  long rlen, rval; char rbuf[sizeof(long)];

  xTrace5(pmapsrvp, TR_FULL_TRACE, "pmtserver xdring test args %d %d %d %d %d",
	  pmunreg[0], pmunreg[1], pmunreg[2], pmunreg[3], pmunreg[4]);
  xkxdr_init(xdrs,buf,5*sizeof(long),XDR_ENCODE);
  xkxdr_long_array(&xdrs,pmunreg,5);
  msgConstructBuffer(&msg,buf,5*sizeof(long));
  msgConstructEmpty(&msgr);
  xTrace0(pmapsrvp, TR_FULL_TRACE, "pmtserver built deregistration message");
  xTrace1(pmapsrvp, TR_FULL_TRACE, "pmtserver msg length = %d", msgLen(&msg));
  val = xCall(svr_pmap_session,&msg,&msgr);
  xTrace1(pmapsrvp, TR_FULL_TRACE,
	  "pmtserver sent deregistration message; val = %d",val);
  if (val == XK_FAILURE)
  { printf(
     "determinant server: error sending deregistration message to portmapper");
    return;}
  if ((rlen = msgLen(&msgr))<sizeof(long))
  { printf("pmtserver: reply from portmapper is too short, %d",rlen);
    return; }
  msgPop(&msgr,yabcopy,rbuf,sizeof(long),0);
  xkxdr_init(xdrr,rbuf,sizeof(long),XDR_DECODE);
  rval = xkxdr_long_decode(&xdrr);
  xTrace1(pmapsrvp, TR_FULL_TRACE, "pmtserver: portmapper reply = %d", rval);
  if (!rval) { printf("pmtserver: portmapper refused to unregister server");
	       return; }
  xTrace0(pmapsrvp, TR_FULL_TRACE, "pmtserver unregistered with portmapper");
  msgDestroy(&msg);
  msgDestroy(&msgr);
}


static xkern_return_t det_calldemux(self,s,msg,msgr)
XObj self, s; Msg *msg, *msgr;
{ char *buf, buf2[100]; int popv=TRUE;
  long len, len2; VOID *results;
  Det_args args;
  long det_proc; XKXDR xdrs, xdrr;

  xTrace3(pmapsrvp,TR_MAJOR_EVENTS,
	"det_calldemux: msg = %d msgLen = %d  msgr = %d",msg,msgLen(msg),msgr);
  xAssert(xIsSession(s));
  xTrace0(pmapsrvp,TR_FULL_TRACE,"det_calldemux: checked session(s)");

  /* externalize message */
  len = msgLen(msg);
  buf = (char *) (len ? xMalloc(len) : 0);
  xTrace0(pmapsrvp,TR_FULL_TRACE,"det_calldemux: starting msgPop");
  if (buf) popv = msgPop(msg,yabcopy,buf,len,0);
  if (popv == FALSE || len < sizeof(long))
  { printf("bad message to msgPop: msg = %d  len = %d\n", msg, len);
    goto fail_exit; }
  xTrace0(pmapsrvp,TR_FUNCTIONAL_TRACE,"det_calldemux: msg externalized");  

  /* authentication check would go here */

  /* the old way: extract procedure from SState structure */
  /* det_proc =  ((SState *) s->state)->s_proc; */
  /* the new way: sunrpc sticks it on the front of the message, sans xdring */
  bcopy(buf,(char *)&det_proc,sizeof(long));
  xTrace1(pmapsrvp,TR_FULL_TRACE,
	  "det_calldemux: extracted procedure.   proc: %d", det_proc);
  if ( det_proc < 0 || 3 < det_proc )
  { xTrace1(pmapsrvp,TR_ERRORS,
	    "det_calldemux: bad procedure number: %d", det_proc);
    /* really should have separate error for bad procedure */
    xControl(s, SUNRPC_SVCGARBAGEARGS,0,0);
    goto fail_exit; }

  /* decode arguments */
  if (det_proc) /* skip decode for procedure 0, ping */
  { xTrace0(pmapsrvp,TR_FUNCTIONAL_TRACE,"det_calldemux: decoding args");
    xkxdr_init(xdrs,buf+sizeof(long),len-sizeof(long),XDR_DECODE);
    if (xkxdr_det_decode(&xdrs,len-sizeof(long),det_proc,&args) == FALSE)
    { xTrace0(pmapsrvp,TR_ERRORS,"det_calldemux: can't decode args");  
      xControl(s, SUNRPC_SVCGARBAGEARGS,0,0);
      goto fail_exit;  }
    xTrace0(pmapsrvp,TR_FUNCTIONAL_TRACE,"det_calldemux: args decoded ");
    xIfTrace(pmapsrvp,TR_FULL_TRACE) { print_det_args(det_proc,&args);}; };

  /* call procedure */
  results = 0;
  switch (det_proc)
  {case DET_NULL:                                   break;
   case DET_LONG:     results = (VOID *)det_int(&args);     break;
   case DET_DOUBLE:   results = (VOID *)det_double(&args);  break;
   case DET_COMPLEX:  results = (VOID *)det_z(&args);       break; }
  xTrace0(pmapsrvp,TR_FUNCTIONAL_TRACE,"det_calldemux: procedure called");
  xIfTrace(pmapsrvp,TR_FULL_TRACE)
  { printf("result = ");
    switch (det_proc)
    {case DET_NULL:    printf("%d", results);            break;
     case DET_LONG:    printf("%d",*(long *) results);   break;
     case DET_DOUBLE:  printf("%f",*(double *) results); break;
     case DET_COMPLEX: print_z(*(complex *) results);    break; }
    printf("\n"); };

  /* xdr results */
  xkxdr_init(xdrr,buf2,100,XDR_ENCODE);
  len2 = 0;  
  if (results)   /* skip xdr for procedure 0 */
  { switch (det_proc)
    { case DET_LONG:    xkxdr_long(   &xdrr,*(long *)   results); break;
      case DET_DOUBLE:  xkxdr_double( &xdrr,*(double *) results); break;
      case DET_COMPLEX: xkxdr_complex(&xdrr, (complex *)results); break; }
    xFree(results);
#ifdef SUNXDR
    len2 = XDR_GETPOS(&xdrr);
#else
    len2 = xdrr-buf2;  /* calculate length if !SUNXDR */
#endif
  }
  xTrace0(pmapsrvp,TR_FUNCTIONAL_TRACE,"det_calldemux: results xdred  ");

  /* set authentication would go here */

  /* internalize message */
  xIfTrace(pmapsrvp,TR_FUNCTIONAL_TRACE)
  { printf("server_demux: len2 = %d buf2 = %d\n",len2,buf2);
    print_xdr(buf2,len2); }
  if (len2) msgConstructBuffer(msgr,buf2,len2);
  xTrace0(pmapsrvp,TR_FUNCTIONAL_TRACE,"det_calldemux: msg internalized");

  /* reply to call */
  if (buf) xFree(buf);
  return XK_SUCCESS;

fail_exit:
  if (buf) xFree(buf);
  return XK_FAILURE;
}


/* end of xkpm_server.c */

