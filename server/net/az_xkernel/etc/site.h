/* 
 * site.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
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


