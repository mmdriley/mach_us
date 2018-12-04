/*
 *
 * mach-msg-intr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/05 00:49:51 $
 */

/*
 *
 *  Interpreter for the Mach msg structure
 *   This is called only for messages that have the 'complex' bit set
 *
 *  Support for heterogeneous architectures is not included in this
 *  version.
 *
 */

#include <stdio.h>
#include <mach.h>
#include <mach/message.h>
#include <mach/notify.h>
#include <cthreads.h>
#include "xkernel.h"
#include "ip.h"
#include "rwlock.h"
#include "machripc_internal.h"

extern int tracemachripcp;
extern Map xkMsgIdMap;  /* port transfer completion indicator */

#define VMADDR_SIZE  4

extern Pfv
  real_conversion_func(enum SOURCE_BYTE_ARCH , int );

/* Null_Netport  definition shared with port_maint */
extern mnetport Null_Netport;  /* error return value */

typedef	long (*Pfl) ();

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

/* external  */
mnetport convert_to_netport(mach_port_right_t port, mach_port_type_t right, IPhost dest, msg_id_t msgid);

#define is_netrpc(msg, rright) ((msg.msgt_inline == 1) && (msg.msgt_number == 1) && (rright == MACH_PORT_TYPE_SEND_ONCE))

#define is_machrpc(msg, rright) ((msg->mmbody.msgt_inline == 1) && (msg->mmbody.msgt_number == 1) && (rright == MACH_PORT_TYPE_SEND_ONCE))


/* ***** end temporary decl's ***** */

#define PORT_CHUNK_SIZE (20 * sizeof(mn_netport_t))


#define MAX_MACH_DATA_TYPE   MACH_MSG_TYPE_STRING_C
int item_size_table[MAX_MACH_DATA_TYPE] = {
  1, 2, 4, 0, 0, 0, 0, 0, 1, 4, 0, 1 
  };

/*
#define MACH_MSG_TYPE_UNSTRUCTURED	0
#define MACH_MSG_TYPE_BIT		0
#define MACH_MSG_TYPE_BOOLEAN		0
#define MACH_MSG_TYPE_INTEGER_16	1
#define MACH_MSG_TYPE_INTEGER_32	2
#define MACH_MSG_TYPE_CHAR		8
#define MACH_MSG_TYPE_BYTE		9
#define MACH_MSG_TYPE_INTEGER_8		9
#define MACH_MSG_TYPE_REAL		10
#define MACH_MSG_TYPE_STRING		12
#define MACH_MSG_TYPE_STRING_C		12
*/

/*
 *  Message pop and push utilities
 *
 *   Forward declarations
 *
 */

struct datargs {
  enum SOURCE_BYTE_ARCH	arch_type;
  int		data_type;
  int	  	real_number_arch;
};

#ifdef LOCALMSGAPPEND
/*
 *  local message append function (if msgtool does not support it)
 *
 */
struct msgappendargs {
  Msg   msg;
  Msg  *cmsg;
  Msg   rcmsg;
  char *buffer;
  char *buffertail;
  long *blen;
};
#endif LOCALMSGAPPEND

#ifdef PORTLOCKS
/*
 *  push a port address onto a message; entirely local operation;
 *  this will not go out on the net.
 *
 */
static long
paddrcopy ( from, to, len, arg)
     char *from, *to;
     long len;
     VOID *arg;
{
  bcopy(from, to, sizeof(mnetport *));
  return sizeof(mnetport *);
}
/*
 *  push a port net number onto a message; entirely local operation;
 *  this will not go out on the net.
 *
 */
static long
pnumcopy ( from, to, len, arg)
     char *from, *to;
     long len;
     VOID *arg;
{
  bcopy(from, to, sizeof(mnportid_t *));
  return sizeof(mnetport *);
}
#endif PORTLOCKS

#ifdef __STDC__
static long port_pop_and_swap(
			      char *input,
			      mach_port_t *output,
			      long len,
			      enum SOURCE_BYTE_ARCH byte_type
			      );

static long data_pop_and_swap(
			      char *input,
			      char *output,
			      long  len,
			      struct datargs *arg
			      );

static long port_append_func(
     mnetport *input,
     char     *output,
     int       len,
     struct    datargs *arg
			     );

static long
pop_an_int(
     char *input,
     int  *output,
     long len,
     int  *arg()
	   );

static long
addr_pop(
     char *input,
     int  *output,
     long len,
     enum SOURCE_BYTE_ARCH arch_type
	   );

static long
shorthdr_andlie (
     char *input,
     unsigned int *output,
     long len,
     void *(*arg[])()
  );


static long
longhdr_pop (
     char *input,
     mach_msg_type_long_t  *output,
     long len,
     void *(*arg[])();
  );

static long
oolhdr_pop (
     char *input,
     mach_msg_type_t  *output,
     long len,
     enum SOURCE_BYTE_ARCH arch_type
  );

static long
justcopy (
     char *input,
     char *output,
     long len,
     void *direction
	  );

static long
justcount( 
     char *input,
     int  *output,
     long  len,
     void *arg
	  );

static long
justcopy_andlie (
     char *input,
     char  *output,
     long len,		 
     void *direction
	  );

static long
justcopylonghdr (
     char *input,
     int  *output,
     long len,
     void *arch_type
	  );

#ifdef LOCALMSGAPPEND
static void
msgAppendDone(
     struct msgappendargs *msgarg);
#endif LOCALMSGAPPPEND

static void
set_resend_port(unsigned int segtype,
     mnetport *portd,
     mach_port_t *msgptr);

static int
set_resend_port_type(unsigned int segtype);

static mach_port_right_t
set_resend_right_type(unsigned int segtype);

static mach_port_type_t
set_resend_outward_port_type(unsigned int segtype);

extern xkern_return_t quick_netport_lookup( unsigned int portid, mnetport **n);

extern void rrxTransferComplete ( IPhost , msg_id_t);

extern void srxTransferComplete ( IPhost , msg_id_t);
#else
static long port_pop_and_swap();
static long data_pop_and_swap();
static long port_append_func();
static long pop_an_int();
static long addr_pop();
static long shorthdr_andlie();
static long longhdr_pop();
static long oolhdr_pop();
static long justcopy();
static long justcount();
static long justcopy_andlie();
#ifdef LOCALMSGAPPEND
static void msgAppendDone();
#endif LOCALMSGAPPEND
static void set_resend_port_type();
static mach_port_type_t set_resend_outward_port_type();
static mach_port_right_t set_resend_port();
static int set_resend_right_type();
extern xkern_return_t quick_netport_lookup();
static long justcopylonghdr();
extern void rrxTransferComplete ( );
extern void srxTransferComplete ( );
#endif __STDC__


/*
 *   mnfree_func --- required free operation for msgAppend
 */
void mnfree_func(void *ptr)
{
  xTrace1(machripcp, TR_DETAILED, "machnetipc: mnfree_func called with %x",
	  ptr);
  xFree((char *) ptr);
}


/*
 * xk_netmach_msg_to_mach
 *
 *    unsafe stub for heterogeneous architecture support
 */
mach_msg_header_t *
xk_netmach_msg_to_mach(Msg			*netmsg,
		       mach_msg_header_t	*outmsg,
		       mach_port_right_t	local_right,
		       int			mach_netport_count,
		       int			mach_notline_count,
		       mn_arch_tag_t		arch_type)
{
  xTrace0(machripcp, TR_ERRORS, "machnetipc: xk_netmach_msg_to_mach called: heterogeneous architecture support is not provided in this release: you lose");
  return outmsg;
}

/*
 * xk_mach_msg_convert (msg, netmsg, nethdr, destination)
 *
 *    creates outgoing message body 
 *
 *    convert all transferred ports, out-of-line data, etc.
 * 
 * 
 */
/* static */ void
xk_mach_msg_convert (
  struct mach_msg_big_t  *msg,
  Msg			 *fulloutmsg,
  machnetipc_hdr	 *nethdr,
  IPhost		  dest)   
{
  Msg		 	 outdata_dict_msg;
  long		 	 msg_size;
  mach_msg_type_t	*msg_ptr;
  int			*data_item_number;
  int			 item_count;
  int			 port_item_number;
  mn_netport_t		*port_buffer;
  Msg			 port_dict_msg;
  Msg			 msg_body;
  int			 notinline = 0;
  bool			 made_portd = FALSE, made_datad = FALSE;
  msg_id_t		 msgid = nethdr->content.machmsg.message_id;
  int		         ports; /* port completion indicator */
#ifdef PORTLOCKS
  Msg                   *locked_ports = (Msg *)xMalloc(sizeof(Msg));
#endif PORTLOCKS

  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: xk_mach_msg_convert");

  msg_size = msg->mmhdr.msgh_size - sizeof(mach_msg_header_t);
  msg_ptr = (mach_msg_type_t *)((char *)msg + sizeof(mach_msg_header_t));
  data_item_number = 0;
  port_item_number = 0;

  while (msg_size > 0) {
    int		     hdr_size;
    int		     item_size;
    int		     item_byte_size;
    int		     item_name;
    mach_port_t     *port_ptr;
    mn_netport_t    *netport_ptr;
    int		     i;
    int		     port_count;
    char	    *item_ptr;
    int		     datainline;
    bool	     receive_right;

    hdr_size = sizeof(mach_msg_type_t);
    item_count = msg_ptr->msgt_number;
    item_size = msg_ptr->msgt_size;
    item_name = msg_ptr->msgt_name;
    datainline = msg_ptr->msgt_inline;
    receive_right = FALSE;

    if (msg_ptr->msgt_longform) {
      hdr_size = sizeof(mach_msg_type_long_t);
      item_count = msg_ptr->msgt_number;
      item_size = ((mach_msg_type_long_t *)msg_ptr)->msgtl_size;
      item_name = ((mach_msg_type_long_t *)msg_ptr)->msgtl_name;
    }
    if (!item_size || item_size != MNBYTESIZE)
      /* this is to check for older code that had bug */
      {
	if (item_size != 32) {
	  item_size *= MNBYTESIZE;
	  xTrace1(machripcp, TR_ERRORS, "machetnipc: msg_msg_convert: item size bad value %d", item_size);
	}
      }
    if (datainline)
      item_byte_size = ROUND4((item_size*item_count)/MNBYTESIZE);
    else
      item_byte_size = ROUND4(sizeof(vm_address_t));

    item_ptr = ((char *)msg_ptr) + hdr_size;
    if (!item_byte_size) continue;  /* zero size msg */
    
    if (item_byte_size > msg_size) {
      xTrace4(machripcp, TR_ERRORS, "machnetipc: mach_msg_convert: item size %d (%d %d) exceeds remaining msg length %d",
	      item_byte_size, item_count, item_size, msg_size);
      return;
    }
    
    xTrace5(machripcp, TR_DETAILED, "machnetipc: mach_msg_convert item_count %d item_size %d hdr_size %d msg_size %d msg_name 0x%x",
	    item_count, item_size, hdr_size, msg_size, item_name);
    xTrace1(machripcp, TR_DETAILED, "machnetipc: mach_msg_convert data inline %d", datainline);
    
    switch(item_name) {
      
    case MACH_MSG_TYPE_PORT_RECEIVE:
      goto process_port;
    case MACH_MSG_TYPE_COPY_SEND:
    case MACH_MSG_TYPE_MAKE_SEND:
    case MACH_MSG_TYPE_MAKE_SEND_ONCE:
      xTrace1(machripcp, TR_DETAILED, "machnetipc: data convert: should not have this name %x", item_name);
      goto process_port;
    case MACH_MSG_TYPE_PORT_SEND:
      /* might have triggered no_more_senders; might be new */
      goto process_port;
    case MACH_MSG_TYPE_PORT_SEND_ONCE:
      /* might be time for no_more_senders; might be new */
process_port:
      xTrace0(machripcp, TR_DETAILED, "machnetipc: xk_mach_to_netmach port item");
      if (datainline) {
	port_ptr = (mach_port_t *)(((char *)msg_ptr) + hdr_size);
	port_count = 1;
      }
      else {
	port_ptr = *((mach_port_t **)(((char *)msg_ptr) + hdr_size));
	xTrace1(machripcp, TR_DETAILED, "machnetipc: xk_mach_to_netmach: out of line port(s) at %x",
		port_ptr);
	port_count = item_count;
      }
      for (i=0; i<port_count; i++) {
	mnetport	port_desc;
	kern_return_t   kret;

	/* this will do any port transfer operations */
	{
#ifdef XK_DEBUG
	  extern int traceportmaintp;
	  int savetr = traceportmaintp;
	  int savemtr = tracemachripcp;

	  traceportmaintp = TR_FULL_TRACE;
	  tracemachripcp  = TR_FULL_TRACE;
#endif XK_DEBUG
	  xTrace5(machripcp, TR_DETAILED, "machnetipc: xk_mach_to_netmach: will convert port for msgid %d dest %d.%d.%d.%d",
		  msgid,
		  dest.a, dest.b, dest.c, dest.d);
	  xTrace2(machripcp, TR_DETAILED, "machnetipc: xk_mach_to_netmach: original port type %x resend port type %x",
		  item_name, set_resend_outward_port_type(item_name));
	  port_desc = convert_to_netport(*port_ptr,
					 set_resend_right_type(item_name),
					 dest, msgid);
#ifdef PORTLOCKS
	  /* save the descriptor address for later unlocking */
	  if (readerLock(&port_desc.rwlock) == XK_SUCCESS) {
	    /*	  msgPush(locked_ports, (Pfv)paddrcopy, &port_desc, sizeof(mnetport), 0);*/
	    msgPush(locked_ports, (Pfv)pnumcopy, &port_desc.net_port_number, sizeof(mnportid_t), 0);
	}
	  else {
	    xTrace0(machripcp, TR_SOFT_ERRORS, "machnetipc: xk_mach_convert: port died during lock");
	    port_desc = Null_Netport;
	  }
#endif PORTLOCKS
	  port_desc.net_port_rights = set_resend_right_type(item_name);
	  if (item_name != MACH_MSG_TYPE_PORT_RECEIVE &&
	      *port_ptr != MACH_PORT_NULL)
	    if ((kret = mach_port_deallocate(mach_task_self(), *port_ptr))
		!= KERN_SUCCESS)
	      {
		xTrace2(machripcp, TR_ERRORS, "machnetipc: xk_mach_convert: error deallocating port right %x, code %x",
			*port_ptr, kret);
	      }

#ifdef XK_DEBUG
	  traceportmaintp = savetr;
	  tracemachripcp  = savemtr;
#endif XK_DEBUG
	}
	if (made_portd == FALSE) {
	  msgConstructAppend(&port_dict_msg, PORT_CHUNK_SIZE, (char **)&port_buffer);
	  made_portd = TRUE;
	}
	msgAppend(&port_dict_msg, (MStoreFun)port_append_func,
		  (mn_netport_t *)&port_desc, sizeof(mn_netport_t),
		  (void *)0, PORT_CHUNK_SIZE);
	bcopy((char *)&port_item_number, ((char *)port_ptr), NETPORTINDEXSIZE);
	port_item_number++;
	port_ptr++;
      }
      if (datainline) break;
    default:
      xTrace0(machripcp, TR_DETAILED, "machnetipc: mach_msg_convert data item");
      if (!datainline) {
	Msg data_msg;
	vm_address_t *local_item_addr = (vm_address_t *)item_ptr;

	xTrace0(machripcp, TR_DETAILED, "machnetipc: xk_mach_msg_convert ool data item");
	if (made_datad == FALSE) {
	  msgConstructEmpty(&outdata_dict_msg);
	  made_datad = TRUE;
	}
	msgConstructInplace(&data_msg, item_ptr, item_size * item_count, (Pfv)mnfree_func);
	msgJoin(&outdata_dict_msg, &outdata_dict_msg, &data_msg);
	msgDestroy(&data_msg);
	bcopy((char *)&notinline, (char *)local_item_addr, NETDATAINDEXSIZE);
	notinline++;
      }
      break;
    }
    msg_size -= hdr_size + item_byte_size;
    (char *)msg_ptr  = ((char *)msg_ptr) + hdr_size + item_byte_size;
  }

  /* misuse the sequence num field to indicate whether or not
     a port transfer completion notice will be required          */
  if (mapResolve(xkMsgIdMap, &msgid, &ports) == XK_SUCCESS) {
      ((machnetipc_hdr *)nethdr)->content.machmsg.sequence_num = 1<<ports;
      mapUnbind(xkMsgIdMap, &msgid);
    }

  if (port_item_number ) {
    xTrace1(machripcp, TR_DETAILED, "machnetipc: convert_to_netmsg adds %d ports", port_item_number);
    ((machnetipc_hdr *)nethdr)->content.machmsg.netport_count = port_item_number;
    xIfTrace(machripcp, TR_DETAILED) {
      msgShow(&port_dict_msg);
      msgShow(fulloutmsg);
    }
    msgJoin(fulloutmsg, &port_dict_msg, fulloutmsg);
    msgDestroy(&port_dict_msg);
    xIfTrace(machripcp, TR_DETAILED) {
      msgShow(fulloutmsg);
    }
#ifdef PORTLOCKS
/*    msgSetAttr(fulloutmsg, 0, locked_ports, sizeof(locked_ports)); */
#endif PORTLOCKS
  }
  if (notinline) {
    xTrace0(machripcp, TR_DETAILED, "machnetipc: convert_to_netmsg adds data");
    msgJoin(fulloutmsg, fulloutmsg, &outdata_dict_msg);
    ((machnetipc_hdr *)nethdr)->content.machmsg.notinline_count = notinline;
  }
  /* mach_msg_destroy(msg); */
  xIfTrace(machripcp, TR_DETAILED) {
    printf("machnetipc: convert mach to netmach msg dump:\n");
    msgShow(fulloutmsg);
  }
  xTrace0(machripcp, TR_FULL_TRACE, "machripc: full convert to netmsg exits");
}


/*
 * xk_ports_and_ool_convert
 *
 * Converts incoming message from compatible architecture to local form.
 * Convert all transferred ports, out-of-line data, etc.
 * The two header ports have been converted by the caller
 * The incoming message has had the type field and the two mnetport
 * structures popped off.
 * 
 */
mach_msg_header_t *
xk_ports_and_ool_convert(Msg			*netmsg,
			 mach_msg_header_t	*outmsg,
			 int			mach_netport_count,
			 int			mach_notinline_count,
			 msg_id_t		msgid,
			 IPhost			srchost,
			 int                    xfercompletion)
{
  long			xk_msg_length;
  int			mach_msg_length;
  mach_msg_header_t	*new_mach_msg;
  mach_msg_type_t	*msg_ptr;
  mach_port_type_t	remote_right;
  int			port_seg_count = 0;
  int			portcount = 0;
  int			outsegcount = 0;
  struct addrinfo {
    char		*index;
    int			 size;
    unsigned int	 type; } *addrs;
  mn_netport_t		*port_dict;
  mach_port_t		*local_port_dict;
  mach_msg_type_name_t   resend_method;

  xTrace0(machripcp, TR_FULL_TRACE, "xk_ports_and_ool_convert");

  xk_msg_length = msgLen(netmsg);

  port_seg_count = mach_netport_count;
  xTrace1(machripcp, TR_EVENTS, "machnetipc: ports and ool convert: port seg count %d", port_seg_count);
  xIfTrace(machripcp, TR_FULL_TRACE) {
    msgShow(netmsg);
  }

  if (port_seg_count) {
    int port_size = port_seg_count * sizeof(mn_netport_t);
    int i;

    port_dict = (mn_netport_t *)xMalloc(port_size);
    local_port_dict = (mach_port_t *)xMalloc(port_seg_count * sizeof(mach_port_t));
    msgPop(netmsg, (Pfl)justcopy, port_dict, port_size, (void *)1);
    xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: ports and ool convert: popped port dictionary");
    if (xfercompletion) {
      if (xfercompletion & 2) rrxTransferComplete(srchost, msgid);
      srxTransferComplete(srchost, msgid);
    }

    for (i=0; i<port_seg_count; i++) {
      xTrace2(machripcp, TR_FULL_TRACE, "machnetipc: ports and ool convert: port %x right %x",
	      port_dict[i].net_port_number,
	      port_dict[i].net_port_rights);
      local_port_dict[i] = 
	convert_netport_to_mach_port(&port_dict[i],
				     0 /* self */,
				     0 /* lower_session */,
				     0 /* reply_msg */,
				     msgid,
				     srchost);
    }
  }

  if (mach_notinline_count) {
    addrs = (struct addrinfo *)xMalloc(mach_notinline_count * sizeof(struct addrinfo));
  }

  /* get all the contiguous msg into one clump */
  msgPop(netmsg, (Pfl)justcopy_andlie, outmsg,
	 sizeof(mach_msg_header_t), (void *)1);
  if (outmsg->msgh_size > 0) {
    new_mach_msg = (mach_msg_header_t *)xMalloc(outmsg->msgh_size);
    msgPop(netmsg, (Pfl)justcopy, (char *)new_mach_msg,
	   outmsg->msgh_size, (void *)1);
    msg_ptr = (mach_msg_type_t *)(((char *)new_mach_msg)
				  + sizeof(mach_msg_header_t));
    mach_msg_length = outmsg->msgh_size - sizeof(mach_msg_header_t);
  }
  else {
    xTrace0(machripcp, TR_ERRORS, "machnetipc: ports_and_ool_convert: length 0");
    return 0;
  }

  xTrace1(machripcp, TR_EVENTS, "machnetipc: convert_ports_and_ool_data: length %d", mach_msg_length);
  if (xk_msg_length < sizeof(mach_msg_header_t)) {
    xTrace2(machripcp, TR_ERRORS, "machnetipc: convert_ports_and_ool_data: msg: bogus length %d dec(%x hex)", xk_msg_length, xk_msg_length);
    xk_msg_length = 0;
    return 0;
  }

  /* scan through once for all msginline conversions */
  while (mach_msg_length > 0)
    {
      long			item_size;
      /* what if conversions make the message longer?  should we scan
	 first to determine the length?  for now, assume length does
	 not change  */
	mach_msg_type_t		msginline_local;
	int			msginline;
        int			hdr_size;
	unsigned int		segtype;
	unsigned int		segtypesize;
        int			segcount;
        char			*outmsgdata;
        int			longform;
        mach_msg_type_t		*short_hdr;
	mach_msg_type_long_t	*long_hdr;

	if (msg_ptr->msgt_longform) {
	    long_hdr = (mach_msg_type_long_t *)msg_ptr;

	    item_size = long_hdr->msgtl_size;
	    segcount = long_hdr->msgtl_number;
	    /* this is the data block size in bytes */
	    item_size = ((MNBYTESIZE - 1) + (item_size * segcount))/MNBYTESIZE;
	    segtype = long_hdr->msgtl_name;
	    msginline  = ((mach_msg_type_t *)long_hdr)->msgt_inline;
	    hdr_size = MACHNETLONGHDRSIZE;
	    outmsgdata = ((char *)msg_ptr) + sizeof(mach_msg_type_long_t);
	    longform = 1;
	  }
	else
	  {
	    short_hdr = msg_ptr;

	    item_size = short_hdr->msgt_size;
	    segcount = short_hdr->msgt_number;
	    /* this is the data block size in bytes */
	    item_size = ((MNBYTESIZE - 1) + (item_size * segcount))/MNBYTESIZE;
	    segtype = short_hdr->msgt_name;
	    msginline = short_hdr->msgt_inline;
	    hdr_size = MACHNETTYPEHDRSIZE;
	    outmsgdata = ((char *)msg_ptr) + sizeof(mach_msg_type_t);
	    longform = 0;
	  }

      xTrace5(machripcp, TR_DETAILED, "machnetipc: ports and ool convert: size %d count %d name %x inline %d longform %d",
	      item_size, segcount, segtype, msginline, longform);

      if (item_size == 0) {
	(char *)msg_ptr += hdr_size;
	mach_msg_length -= hdr_size;
	xTrace0(machripcp, TR_DETAILED, "machnetipc: ports and ool convert: zero length segment");
	continue;
      }
      segtypesize = item_size_table[segtype];

      switch(segtype) {
	  mach_port_type_t port_send_type;

	case MACH_MSG_TYPE_COPY_SEND:
	case MACH_MSG_TYPE_MAKE_SEND:
	  xTrace4(machripcp, TR_DETAILED, "machnetipc: xk_ports_and_ool_convert: should not have this name 0x%x 0x%x 0x%x 0x%x", segtype, MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_PORT_SEND);
	  port_send_type = MACH_MSG_TYPE_PORT_SEND;
	  goto process_port;
	case MACH_MSG_TYPE_MAKE_SEND_ONCE:
	  xTrace1(machripcp, TR_DETAILED, "machnetipc: xk_ports_and_ool_convert: should not have this name 0x%x", segtype);
	  port_send_type = MACH_MSG_TYPE_PORT_SEND_ONCE;
	  goto process_port;
	  /* these are what I expect to see */
	case MACH_MSG_TYPE_PORT_RECEIVE:
	  port_send_type = MACH_MSG_TYPE_MOVE_RECEIVE;
	  goto process_port;
	case MACH_MSG_TYPE_PORT_SEND:
	  port_send_type = MACH_MSG_TYPE_MOVE_SEND;
	  goto process_port;
	case MACH_MSG_TYPE_PORT_SEND_ONCE:
	  port_send_type = MACH_MSG_TYPE_MOVE_SEND_ONCE;
process_port:	
	  xTrace1(machripcp, TR_EVENTS, "machnetipc: ports_and_ool_convert: port type %x", port_send_type);
	    if (longform)
	      ((mach_msg_type_long_t *)msg_ptr)->msgtl_name =
		port_send_type;
	    else
	      ((mach_msg_type_t *)msg_ptr)->msgt_name =
		port_send_type;

	  if (msginline) {
	    mach_port_t local_port;

	    while (segcount-- > 0) {
	      int		port_index;
	      mnetport		*portdesc;

	      port_index = *((int *)outmsgdata);
	      xTrace1(machripcp, TR_DETAILED, "machnetipc: ports and ool convert: port index %d", port_index);
	      local_port = MACH_PORT_NULL;
	      if (port_index >= 0 && port_index < port_seg_count
		&& quick_netport_lookup(port_dict[port_index].net_port_number,
					 &portdesc)
		    == XK_SUCCESS) {
		local_port = portdesc->real_receive_port;
	      }
	      *(mach_port_t *)outmsgdata = local_port;
	      xTrace2(machripcp, TR_DETAILED, "machnetipc: ports and ool convert: port is %x in msg at addr %x", local_port, outmsgdata);
	      outmsgdata += sizeof(mach_port_t);
	    }
	    ((char *)msg_ptr) += item_size;
	    mach_msg_length -= item_size;
	  }
	  else {
	      xTrace0(machripcp, TR_DETAILED, "machnetipc: xk_ports_and_ool_convert: ool port(s)");
	      goto processool;
	    }
	  break;
	default:
	  xTrace0(machripcp, TR_DETAILED, "machnetipc: ports and ool convert: data segment");
	  if (msginline) {
	    ((char *)msg_ptr) += item_size;
	    mach_msg_length -= item_size;
	  }
	  else
	    {
	      kern_return_t ret;

processool:
	      xTrace0(machripcp, TR_DETAILED, "machnetipc: xk_ports_and_ool_convert: ool data");
	      if ((ret=
		   vm_allocate(mach_task_self(),
			       (vm_address_t *)&addrs[outsegcount++].index,
			       item_size,
			       TRUE)) != KERN_SUCCESS)
		{
		  xTrace1(machripcp, TR_ERRORS, "machnetipc: xk_ports_and_ool_convert: cannot allocate space for incoming data or ports, code %x",
			  ret);
		  addrs[outsegcount-1].index = 0;
		}
	      *((vm_address_t **)msg_ptr) = 
		(vm_address_t *)addrs[outsegcount - 1].index;
	      /* should always set the deallocate bit */
	      addrs[outsegcount-1].size = item_size;
	      if (longform)
		((mach_msg_type_long_t *)msg_ptr)->msgtl_header.msgt_deallocate = 1;
	      else
		msg_ptr->msgt_deallocate = 1;
	      ((char *)msg_ptr) += sizeof(vm_address_t); /* NETADDRSIZE */
	      mach_msg_length -= sizeof(vm_address_t);
	    }
	  break;
	}
      ((char *)msg_ptr) += hdr_size;
      mach_msg_length -= hdr_size;
    }
  xTrace0(machripcp, TR_DETAILED, "machnetipc: xk_ports_and_ool_convert: at end of inline processing");

    /* the msg_ptr should be at the end of the message; now we just
       fill in the data at the previosly malloc'd addresses 
     */
  
  xIfTrace(machripcp, TR_ERRORS) {
    if (outsegcount != mach_notinline_count)
      printf("machnetipc: ports_and_ool_convert: out of line segment mismatch: %d %d\n",
	     outsegcount, mach_notinline_count);
  }

  outsegcount = 0;
  while (outsegcount < mach_notinline_count) {
    msgPop(netmsg, (Pfl)justcopy, addrs[outsegcount].index,
	   addrs[outsegcount].size, (void *)1);

    if ((addrs[outsegcount].type == MACH_PORT_TYPE_SEND) ||
	(addrs[outsegcount].type == MACH_PORT_TYPE_SEND_ONCE) ||
	(addrs[outsegcount].type == MACH_PORT_TYPE_RECEIVE)){

      int i;
      mach_port_t *port_ptr = (mach_port_t *)addrs[outsegcount].index;
      
      xTrace0(machripcp, TR_DETAILED, "machnetipc: ports and ool convert: ool ports");
      /* step through the popped off data; should increment by netsize */
      for (i=0; i<addrs[outsegcount].size; i++) {
	*port_ptr = local_port_dict[*((int *)port_ptr)];
	port_ptr++;
      }
    }
    outsegcount++;
  }
  xTrace0(machripcp, TR_EVENTS, "machnetipc: ports and ool data convert: normal return");
  return (mach_msg_header_t *)new_mach_msg;
}

/*
 * set_resend_port
 *
 *
 */
static void
set_resend_port(segtype, portd, msgptr)
     unsigned int segtype;  /* mach msg name type */
     mnetport *portd;
     mach_port_t *msgptr;
{
/*
  if (segtype==MACH_MSG_TYPE_PORT_SEND ||
      segtype==MACH_MSG_TYPE_PORT_SEND_ONCE)
    *msgptr = portd->real_send_port;
  else 
*/
    *msgptr = portd->real_receive_port;
}

/*
 * set_resend_port_type
 *
 *    Remote side will have received ports with a name
 *    field indicating the right; we now have to set a
 *    transfer type to deliver the right locally.
 *
 */
static int
set_resend_port_type(segtype)
     mach_port_type_t  segtype;  /* mach msg name type */
{

  switch ( segtype ) {
  case MACH_MSG_TYPE_PORT_SEND_ONCE:
    return MACH_MSG_TYPE_MAKE_SEND_ONCE;
  case MACH_MSG_TYPE_COPY_SEND:
    xTrace0(machripcp, TR_ERRORS, "machnetipc: set_resend_port: copying send right; not all semantics necessarily correct");
  case MACH_MSG_TYPE_PORT_SEND:
    return MACH_MSG_TYPE_MAKE_SEND;
  case MACH_MSG_TYPE_PORT_RECEIVE:
    return MACH_MSG_TYPE_MOVE_RECEIVE;
  }
  return segtype;
}

/*
 * set_resend_outward_port_type
 *
 *    message going to net needs standard port type
 */

static mach_port_type_t
set_resend_outward_port_type(segtype)
     mach_port_type_t  segtype;  /* mach msg name type */
{
  switch ( segtype ) {
  case MACH_MSG_TYPE_MOVE_SEND_ONCE:
  case MACH_MSG_TYPE_MAKE_SEND_ONCE:
    return MACH_MSG_TYPE_PORT_SEND_ONCE;
  case MACH_MSG_TYPE_COPY_SEND:
  case MACH_MSG_TYPE_MAKE_SEND:
    return MACH_MSG_TYPE_PORT_SEND;
  case MACH_MSG_TYPE_MOVE_RECEIVE:
    return MACH_MSG_TYPE_PORT_RECEIVE;
  }
  return segtype;
}

/*
 * set_resend_outward_port_type
 *
 *    message going to net needs standard port type
 */

static mach_port_right_t
set_resend_right_type(segtype)
     mach_port_type_t  segtype;  /* mach msg name type */
{
  switch ( segtype ) {
  case MACH_MSG_TYPE_MOVE_SEND_ONCE:
  case MACH_MSG_TYPE_MAKE_SEND_ONCE:
    return MACH_PORT_TYPE_SEND_ONCE;
  case MACH_MSG_TYPE_COPY_SEND:
  case MACH_MSG_TYPE_MAKE_SEND:
  case MACH_MSG_TYPE_MOVE_SEND:
    return MACH_PORT_TYPE_SEND;
  case MACH_MSG_TYPE_MOVE_RECEIVE:
    return MACH_PORT_TYPE_RECEIVE;
  }
  xTrace1(machripcp, TR_ERRORS, "machnetipc: set_resend_right_type: unknown seg type: %x", segtype);
  return segtype;
}

/*
 *  data_pop_and_swap
 *
 *
 */
static long
data_pop_and_swap(input, output, len, arg)
     char *input;
     char *output;
     long  len;
     struct datargs *arg;
{
  Pfv		func;
  int		archin = arch_unpermute_index(arg->arch_type);

  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: data_pop_and_swap starts");

  switch (arg->data_type) {
/*  case MACH_MSG_TYPE_BOOLEAN:
  case MACH_MSG_TYPE_BIT:
*/
  case MACH_MSG_TYPE_UNSTRUCTURED:
    func = (Pfv) nop_32;
    break;
/*  case MACH_MSG_TYPE_BYTE:
  case MACH_MSG_TYPE_CHAR:
*/
  case MACH_MSG_TYPE_INTEGER_8:
    func = (Pfv)nop_8;
    break;
  case MACH_MSG_TYPE_INTEGER_16:
    func = (Pfv)unpermute_int16[archin];
    break;
  case MACH_MSG_TYPE_INTEGER_32:
    func = (Pfv)unpermute_int32[archin];
/*  case MACH_MSG_TYPE_STRING: */
  case MACH_MSG_TYPE_STRING_C:
    func = (Pfv)nop_8;
    break;
  case MACH_MSG_TYPE_REAL:
    func = (Pfv)real_conversion_func(arg->arch_type, arg->real_number_arch);
    break;
  default:
    xTrace1(machripcp, TR_ERRORS, "machnetipc: data_pop_and_swap: unknown msg type 0x%x", arg->data_type);
    len = 0;
  }
  while (len>0) {
    func(input, output);
    output += item_size_table[arg->data_type];
    input += item_size_table[arg->data_type];
    len--;
  }
  return len;
}


/*
 *  pop_an_int
 *
 *
 */
static long
pop_an_int(input, output, len, arg)
     char *input;
     int  *output;
     long len;
     int  *arg();
{
  if (len < item_size_table[MACH_MSG_TYPE_INTEGER_32])
    {
      xTrace1(machripcp, TR_ALWAYS, "machnetipc: data convert: not enough data to form an integer %d", len);
      return (0);
    }
  *output = (int)(arg(input));
  return item_size_table[MACH_MSG_TYPE_INTEGER_32];
}

/*
 *  addr_pop
 *
 *
 */
static long
addr_pop( inptr, outptr, len, arch_type)
     char *inptr;
     int  *outptr;
     long len;
     enum SOURCE_BYTE_ARCH arch_type;
{
  int          archin = arch_unpermute_index(arch_type);

/*  *output = unpermute_int32[archin](inptr); */ /* this line causes gcc to barf fatally */
  return VMADDR_SIZE;
}

/*
 *  port_pop_and_swap
 *
 *
 */
static long
port_pop_and_swap(inptr, outptr, len, arch_type)
     char *inptr;
     mach_port_t *outptr;
     long len;
     enum SOURCE_BYTE_ARCH arch_type;
{
  int          archin = arch_unpermute_index(arch_type);

  xTrace0(machripcp, TR_FULL_TRACE, "machnetipc: port_pop_and_swap starts");

  *outptr = quick_port_convert(unpermute_int32[archin](inptr));
  return(len);
}

/*
 *  port_append_func
 *
 *    move a brief description of the port to the outgoing message
 *
 */
static long
port_append_func(input, output, len, arg)
     mnetport *input;
     char     *output;
     int       len;
     struct datargs *arg;
{
  bcopy((char *)input, output, sizeof(mn_netport_t));
  return sizeof(mn_netport_t);
}

#ifdef LOCALMSGAPPEND
static void
msgAppendLocal(msgarg, func, data, length, newlength)
     struct msgappendargs *msgarg;
     Pfv func;
     void *data;
     long length;
     long newlength;
{
  char *fmsg;
  char *cmsg;
  char *buffer;
  long  blen;
  char *buffertail;

  fmsg = &msgarg->msg;
  cmsg =  msgarg->cmsg;
  buffer = msgarg->buffer;
  blen = msgarg->blen;
  buffertail = msgarg->buffertail;
  
  if (!buffer) {
    msgConstructAllocate(fmsg, newlength, &msgarg->buffer);
    msgarg->blen = newlength;
    msgarg->buffertail = msgarg->buffer;
    msgarg->cmsg = fmsg;
  }
  if (buffertail + length > buffer + blen) {
    msgTruncate(cmsg, buffertail - buffer);
    if (cmsg != fmsg) msgJoin(fmsg, fmsg, cmsg);
    msgConstructAllocate(&msgarg->rcmsg, newlength, &msgarg->buffer);
    msgarg->blen = newlength;
    msgarg->buffertail = msgarg->buffer;
    msgarg->cmsg = &msgarg->rcmsg;
    cmsg = msgarg->cmsg;
    buffer = msgarg->buffer;
  }
  bcopy(data, buffer, length);
}

static void
msgAppendDone(msgarg)
     struct msgappendargs *msgarg;
{
  if (!msgarg->buffer) msgConstructEmpty(&msgarg->msg);
  else {
    msgTruncate(msgarg->cmsg, msgarg->buffertail - msgarg->buffer);
    msgJoin(&msgarg->msg, &msgarg->msg, msgarg->cmsg);
    msgarg->buffer = 0;
  }
}
#endif LOCALMSGAPPEND

static long
port_protocol_push_receive_right()
{
  return 0;
}

static long
port_protocol_pop_receive_right()
{
  return 0;
}

/*
 *  shorthdr_andlie
 *
 *   we rely on having the msgt_type be 4 bytes long, always
 *
 */
static long
shorthdr_andlie( input, output, len, argf)
     char *input;
     unsigned int *output;
     long  len;
     void  *(*argf[])();
{
  xTrace2(machripcp, TR_DETAILED, "shorthdr_andlie: %x %x", input, output);
  bcopy(input, (char *)output, len);
  *output = (unsigned int)(argf[MACH_MSG_TYPE_INTEGER_32]((int *)output));
  return 0;
}

/*
 * longhdr_pop
 *
 *
 */
static long
longhdr_pop( input, output, len, argf)
     char *input;
     mach_msg_type_long_t  *output;
     long  len;
     void  *(**argf());
{
  mach_msg_type_long_t *outstruct = output;
  char *first_input = input;
  unsigned int *fake = (unsigned int *)&(output->msgtl_header);

  xTrace2(machripcp, TR_DETAILED, "longhdr_pop: %x %x", input, output);

  bcopy(input, (char *)&output->msgtl_header, 4);
  *fake =
    (unsigned int)argf[MACH_MSG_TYPE_INTEGER_32](&output->msgtl_header);
  input += 4;
  bcopy(input, (char *)&outstruct->msgtl_name, 4);
  output->msgtl_name = (unsigned short)argf[MACH_MSG_TYPE_INTEGER_16](&output->msgtl_name);
  input += 2;
  bcopy(input, (char *)&outstruct->msgtl_size, 2);
  output->msgtl_size = (unsigned short)argf[MACH_MSG_TYPE_INTEGER_16](&output->msgtl_size);
  input += 2;
  bcopy(input, (char *)&outstruct->msgtl_number, 4);
  output->msgtl_number = (unsigned int)argf[MACH_MSG_TYPE_INTEGER_32](&output->msgtl_number);
  return input - first_input + 4; 
}

/*
 * oolhdr_pop
 *
 *
 */
static long
oolhdr_pop( input, output, len, arg)
     char *input;
     mach_msg_type_t *output;
     long  len;
     enum SOURCE_BYTE_ARCH arg;
{
  xTrace2(machripcp, TR_DETAILED, "oolhdr_pop: %x %x", input, output);

  bcopy(input, (char *)output, len);
  return len;
}

/*
 * justcopy
 *
 *
 */
static long
justcopy( input, output, len, direction)
     char *input;
     char  *output;
     long  len;
     void *direction;
{
  xTrace5(machripcp, TR_DETAILED, "justcopy: %x %x %ld (0x%lx) dir %d", input, output, len, len, direction);

  if (!direction) 
    bcopy(input, output, len);
  else
    bcopy(output, input, len);
  return len;
}

/*
 * justcount
 *
 *
 */
static long
justcount( input, output, len, arg)
     char *input;
     int  *output;
     long  len;
     void *arg;
{
  xTrace1(machripcp, TR_DETAILED, "justcount: %d", len);

  return len;
}

/*
 * justcopy_andlie
 *
 *
 */
static long
justcopy_andlie( input, output, len, direction)
     char *input;
     char  *output;
     long  len;
     void *direction;
{
  xTrace4(machripcp, TR_DETAILED, "justcopy_andlie: %x %x dir %d len %ld", input, output, direction, len);

  if (!direction)
    bcopy(input, output, len);
  else
    bcopy(output, input, len);
  return 0;
}

/*
 * justcopylonghdr
 *
 *   get a Mach message long segment hdr
 *
 *
 */
static long
justcopylonghdr(input, output, len, arg)
     char *input;
     int  *output;
     long  len;
     void *arg;
{
  mach_msg_type_long_t *outstruct = (mach_msg_type_long_t *)output;
  char *beg = input;

  xTrace2(machripcp, TR_DETAILED, "justcopylonghdr: %x %x", input, output);

  bcopy(input, (char *)&outstruct->msgtl_header, 4);
  input += 4;
  bcopy(input, (char *)&outstruct->msgtl_name, 4);
  input += 4;
  bcopy(input, (char *)&outstruct->msgtl_size, 2);
  input += 2;
  bcopy(input, (char *)&outstruct->msgtl_number, 4);
  return input - beg + 4;
}

