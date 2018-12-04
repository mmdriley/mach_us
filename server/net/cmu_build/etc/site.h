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
/*
 * This file is derived from the x-kernel distributed by the
 * University of Arizona. See the README file at the base of this
 * source subtree for details about distribution.
 *
 * The Mach 3 version of the x-kernel is substantially different from
 * the original UofA version. Please report bugs to mach@cs.cmu.edu,
 * and not directly to the x-kernel project.
 */
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/etc/site.h,v $
 *
 * 
 * Purpose:  Describe the CMU site for the xkernel.
 * 
 * HISTORY
 * $Log:	site.h,v $
 * Revision 2.3  94/07/13  17:36:31  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:09:01  jms
 * 	Initial Version
 * 	[94/01/09  20:17:11  jms]
 * 
 * Revision 2.2  90/10/29  18:06:34  dpj
 * 	Integration into master source tree
 * 	[90/10/21  22:52:19  dpj]
 * 
 * 	First working version.
 * 	[90/10/03  21:54:08  dpj]
 * 
 *
 */

#if	MACH3_MODS

/************************************************************************
/*
/* String giving the absolute pathname of the xkernel main directory
/*
/************************************************************************/
#define SITE_ROOT_DIRECTORY "/afs/cs.cmu.edu/project/mach-4/jms/us/src/us/pkg/xkernel/cmu_build"
#define PROTOCOL_TABLE "/afs/cs.cmu.edu/project/mach-4/jms/us/src/us/pkg/xkernel/az/etc/prottbl"

/************************************************************************
/*
/*   Site dependent servers: where PROFILE runs, where DNS runs, and
/*   where IP router runs
/*
/************************************************************************/

#define SITE_DEVELOP_HOST {128, 2, 222, 78} 	/* profile host    */
#define SITE_DNS_SRV_NAME "papaya.srv.cs.cmu.edu" /* DNS host (name) */
#define SITE_DNS_SRV_ADDR "128.2.222.199"		/* DNS host (addr) */
#define SITE_IP_GTW 	  { 128, 2, 217, 254 }	/* IP gateway      */

/************************************************************************
/*
/*   Name, IP address, and Ethernet address for the two processors
/*   (client and server) on which the x-kernel will typically be
/*   booted. These defaults can be overriden by the -s and -c options
/*   to the boot program.
/*
/************************************************************************/

#define SITE_SERVER_NAME "test1"		   /* server's name     */
#define SITE_SERVER_IP {127, 0, 1, 82}		   /* server's IP addr  */
#define SITE_SERVER_ETH {0x3832, 0x3330, 0x3530}   /* server's ETH addr */

#define SITE_CLIENT_NAME "test2"		   /* client's name     */
#define SITE_CLIENT_IP {127, 0, 2, 82}		   /* client's IP addr  */
#define SITE_CLIENT_ETH {0x3832, 0x3330, 0x3531}   /* client's ETH addr */

#else	MACH3_MODS
/* 
 * site.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1991  Arizona Board of Regents
 */

/************************************************************************
 *
 * String giving the absolute pathname of the xkernel main directory
 *
 ************************************************************************/

#define SITE_ROOT_DIRECTORY "/cs/xkernel.v3.2"

#define PROTOCOL_TABLE "/cs/xkernel.v3.2/etc/prottbl"

/************************************************************************
 *
 *   Site dependent servers: where DNS runs, where IP router runs, etc.
 *
 ************************************************************************/

#define SITE_IP_GTW 	  { 192, 12, 69, 1 }          /* IP gateway      */



/************************************************************************
 *
 *   Name, IP address, and Ethernet address for the two processors
 *   (client and server) on which the x-kernel will typically be
 *   booted. These defaults can be overriden by the -s and -c options
 *   to the boot program.
 *
 ************************************************************************/

#define SITE_SERVER_NAME "kadmos"		   /* server's name     */
#define SITE_SERVER_IP { 192, 12, 69, 49 }	   /* server's IP addr  */
#define SITE_SERVER_ETH { 0xC00C, 0x4558, 0x04d2 } /* simulated ETH addr */

#define SITE_CLIENT_NAME "lubalin"		   /* client's name     */
#define SITE_CLIENT_IP { 192, 12, 69, 45 }	   /* client's IP addr  */
#define SITE_CLIENT_ETH { 0xC00C, 0x4558, 0x2694 } /* simulated ETH addr */



#endif	MACH3_MODS
