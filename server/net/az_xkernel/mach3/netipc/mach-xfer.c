/*
 *
 * mach-xfer.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/02 02:12:22 $
 */

/*
 *
 *  Support for port transfers
 *
 *   The glue between the port transfer protocols and the
 *   port manager.
 *
 */

#include <stdio.h>
#include <mach.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <cthreads.h>
#include "xkernel.h"
#include "ip.h"
#include "udp.h"
#include "machripc_internal.h"
#include "machripc_xfer.h"
#include "bidctl.h"

extern traceportmaintp;

extern mnetport Null_Netport;  /* error return value */

char *mportNetRepStr(mportNetRep *mn) { return("portxxx"); }


static void
addHostRef( h )
    IPhost	*h;
{
  xTrace0(portmaintp, TR_FULL_TRACE, "portmaint: addHostRef");
  portm_register(*h);
}

void
removeSendRight( npd )
    mnetport	*npd;
{
    xTrace1(portmaintp, TR_EVENTS,
	    "portmaintp: remove send right for port %d",
	    npd->net_port_number);
}


void
removeReceiveRight( npd )
    mnetport	*npd;
{
    xTrace1(portmaintp, TR_EVENTS,
	    "portmaintp: remove receive right for port %d",
	    npd->net_port_number);
}



void
receiveRightDeallocated( npd )
    mnetport	*npd;
{
    xTrace1(portmaintp, TR_EVENTS,
	    "portmaintp: receiveRightDeallocated receives port death notification of for port %d",
	    npd->net_port_number);
}

void
receiverMoved( npd )
    mnetport	*npd;
{
    xTrace2(portmaintp, TR_EVENTS,
	    "portmaintp: receiverMoved notification for port %d (new rcvr %s)",
	    npd->net_port_number, ipHostStr(&npd->receiver_host_addr));
    addHostRef(&npd->receiver_host_addr);
    /* could signal something to forward the queue? */
/*
     semWait(npd->something);
*/    
}

void
unscramble ( int *number, enum SOURCE_BYTE_ARCH arch )
{
  if (arch != MN_ARCH_MARKER) {
    xTrace0(portmaintp, TR_ALWAYS, "portmaintp: unscramble: incompatible architectures");
  }
}

/*
 *  addNewSender
 *
 *     we hold a network receive right; a new sender node is added
 *     to the list
 *
 */
void addNewSender( mnetport *portd, IPhost sender_addr, int mscount )
{
  int mcount;

  xTrace0(portmaintp, TR_FULL_TRACE, "portmaint: addNewSender");

  xIfTrace(portmaintp, TR_FULL_TRACE) {
    if (!(portd->net_port_rights & MACH_PORT_TYPE_RECEIVE)) {
      xTrace0(portmaintp, TR_ERRORS, "portmaintp: addNewSender: did not have receive right");
      return;
    }
  }
  /* pre-existing right */
  if (mapResolve(portd->senders_map, &sender_addr, (VOID *)&mcount)
      == XK_SUCCESS)
    {
      portd->sender_count++;
      portm_register(sender_addr);
#ifdef MAKESENDCOUNT
      mapUnbind(portd->senders_map, &sender_addr);
      mapBind(portd->senders_map, &sender_addr, mcount++);
      portd->make_send_count++;
#endif MAKESENDCOUNT
      return;
    }
  /* new right */
  if (mapBind(portd->senders_map, &sender_addr, mscount) == ERR_BIND)
    return;
  /* must add new send reference count ... */
  portm_register(sender_addr);
  return;
}

/*
 * findNetPort
 *
 *         port transfer protocols call this to get the 
 *            netport structure.
 */
xkern_return_t findNetPort( mn_netport_t *nportd,
			   enum SOURCE_BYTE_ARCH arch_tag,
			   bool create_flag,
			   mnetport **npd)
{
  mach_port_type_t right = nportd->net_port_rights;
  IPhost receiver_addr = nportd->receiver_host_addr;

  xTrace0(portmaintp, TR_FULL_TRACE, "portmaint: findNetPort");

  if (quick_netport_lookup(nportd->net_port_number, npd) != XK_SUCCESS)
    {
      if (!create_flag) return XK_FAILURE;
      convert_netport_to_tmp_mach_port (nportd, right,
					receiver_addr, right);
      quick_netport_lookup(nportd->net_port_number, npd);
    }
  /* new scheme should have make send count in npd nportd structure */
  if (right == MACH_PORT_TYPE_SEND) {
    if (nportd->make_send_count > (*npd)->make_send_count)
      (*npd)->make_send_count = nportd->make_send_count;
  }
  return XK_SUCCESS;
}

#ifdef HDRROUTINES
/*
 *    mnetportLoad
 */
void mnetportLoad( char *src, mportNetRep *dst, 
		   enum SOURCE_BYTE_ARCH arch_tag)
{
  if (sizeof(int) != 4) {
    xTrace0(portmaintp, TR_ALWAYS, "portmaint: mnetportLoad: Cannot convert incoming int to local int");
    return;
  }
  if (arch_tag == MN_ARCH_MARKER) {
    bcopy(src, (char*)dst->net_port_number, sizeof(int));
    src += 4;
    bcopy(src, (char*)dst->net_port_rights, sizeof(int));
    src += 4;
    bcopy(src, (char*)dst->receiver_host_addr, sizeof(IPhost));
  }
  dst->net_port_number = unscramble((int *)&src, arch_tag);
  src += 4;
  dst->net_port_rights = unscramble((int *)&src, arch_tag);
  src +=4;
  dst->receiver_host_addr = unscramble((int *)&src, arch_tag);
}

/*
 *    mnetportStore
 */
void mnetportStore( char *src, mportNetRep *dst)
{
  if (sizeof(int) != 4) {
    xTrace0(portmaintp, TR_ALWAYS, "portmaint: mnetportStore: Cannot convert outgoing int to net int");
    return;
  }
  bcopy((char *)&src->net_port_number, dst, 4);
  dst += 4;
  bcopy((char *)&dst->net_port_rights, dst, 4);
  dst += 4;
  bcopy((char *)&dst->receiver_host_addr, dst, sizeof(IPhost));
}
#endif HDRROUTINES
