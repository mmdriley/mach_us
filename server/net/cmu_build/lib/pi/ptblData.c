/* 
 * Mach Operating System
 * Copyright (c) 1994 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they made and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
/********************
 *  
 *  Purpose:  Xkernel table used to define the "numbers" of the various
 * 		protocols supported. (See xkernel manual)
 *  
 * HISTORY: 
 * $Log:	ptblData.c,v $
 * Revision 2.3  94/07/28  16:54:03  mrt
 * 	Added copyright
 * 	[94/07/19            mrt]
 * 
 * Revision 2.2  94/01/11  18:09:20  jms
 * 	Fix all of the numbers to be the "real" numbers. not the xkernel test numbers.
 *	Copied from az_xkernel/util/ptbldump/ptblData.c
 * 	[94/01/09  21:04:26  jms]
 * 
 *********************/

static MapEntry ethMap[] = { 
	{ "ethtest", 12288 },
	{ "ip", 2048 },
	{ "arp", 2054 },
	{ "rarp", 32821 },
	{ "blast", 12289 },
	{ "vsize", 12290 },
	{ "chan", 12291 },
	{ 0, 0 }
};

static MapEntry ipMap[] = { 
	{ "iptest", 100 },
	{ "udp", 17 },
#ifdef USE_FAKE_TCP_ID
	{ "tcp", 201 },
#else
	{ "tcp", 6 },
#endif  MACH3_MULTI
	{ "icmp", 1 },
	{ "blast", 101 },
	{ "vsize", 203 },
	{ "chan", 102 },
	{ 0, 0 }
};

#if USE_UDP_MAP
static MapEntry udpMap[] = { 
	{ "pmap", 111 },
	{ 0, 0 }
};
#endif USE_UDP_MAP

static Entry	entries[] = {
	{ "eth", 1, ethMap },
	{ "ip", 2, ipMap },
	{ "arp", 3, 0 }, 
	{ "rarp", 4, 0 }, 
#if USE_UDP_MAP
	{ "udp", 5, udpMap }, 
#else
	{ "udp", 5, 0 }, 
#endif USE_UDP_MAP
#if USE_FAKE_TCP
	{ "tcp", 201, 0 }, 
#else
	{ "tcp", 6, 0 }, 
#endif USE_FAKE_TCP
	{ "icmp", 7, 0 }, 
	{ "blast", 8, 0 }, 
	{ "sunrpc", 9, 0 }, 
	{ "pmap", 10, 0 }, 
	{ "vsize", 11, 0 }, 
	{ "vaddr", 12, 0 }, 
	{ "chan", 13, 0 }, 
	{ "select", 14, 0 }, 
	{ "vchan", 15, 0 }, 
	{ "null", 16, 0 }, 
	{ "vnet", 17, 0 }, 
	{ "vmux", 18, 0 }, 

	{ "mselect", 20, 0 }, 
	{ "bid", 21, 0 }, 
	{ "bidctl", 22, 0 }, 
	{ "vcache", 23, 0 }, 
	{ "srx", 24, 0 }, 
	{ "rrx", 25, 0 }, 
	{ "vdrop", 26, 0 }, 


/*
 * driver anchor protocols
 */
	{ "simeth", 1001, 0 }, 	/* ETH simulator on UDP */
	{ "ethdrv", 1002, 0 }, 	/* Mach out-of-kernel ethernet driver */
	{ "xklance", 1003, 0 }, 	/* Mach Lance ethernet driver */

/*
 * API anchor protocols
 */
	{ "xksocket", 2001, 0 }, 
#if MACH3_MULTI
	{ "usudp", 2100, 0 }, 
	{ "ustcp", 2101, 0 }, 
#endif MACH3_MULTI

/* crypt protocols */
	{ "crypt", 3001, 0 }, 

/*
 * test protocols
 */
	{ "ethtest", 10001, 0 }, 
	{ "iptest", 10002, 0 }, 
	{ "udptest", 10005, 0 }, 
	{ "tcptest", 10006, 0 }, 
	{ "blasttest", 10008, 0 }, 
	{ "sunrpctest", 10009, 0 }, 
	{ "chantest", 10013, 0 }, 
	{ "xrpctest", 10014, 0 }, 
	{ "ethdrvtest", 10015, 0 }, 
	{ 0, 0, 0 }
};
