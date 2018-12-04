/* 
 * netmask.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:40:11 $
 */

#ifndef netmask_h
#define netmask_h

/* 
 * Public interface to the 'netmask' module
 */


/* 
 * netMaskAdd -- add 'mask' to the table as the netmask for 'net'
 */
int	netMaskAdd(
#ifdef __STDC__
		   IPhost *net, IPhost *mask
#endif
		   );


/* 
 * netMaskInit -- initialize the netmask table.  Must be called before
 * any other netmask routines.
 */
void	netMaskInit(
#ifdef __STDC__
		    void
#endif
		    );

/* 
 * netMaskFind -- Set 'mask' to the netmask for 'host'.  If an
 * appropriate netmask is not in the table, a default netmask based on
 * the address class is used.
 */
void	netMaskFind(
#ifdef __STDC__
		    IPhost *mask, IPhost *host
#endif
		    );


/* 
 * netMaskDefault -- Set 'mask' to the default netmask for 'host'.
 * This differs from the result of netMaskFind only if subnets are
 * being used.
 */
void	netMaskDefault(
#ifdef __STDC__
		    IPhost *mask, IPhost *host
#endif
		    );


/* 
 * netMaskSubnetsEqual -- returns non-zero if the two hosts are on the
 * same subnet
 */
int	netMaskSubnetsEqual(
#ifdef __STDC__
			 IPhost *, IPhost *
#endif
			 );


/* 
 * netMaskNetsEqual -- returns non-zero if the two hosts are on the
 * same net
 */
int	netMaskNetsEqual(
#ifdef __STDC__
			 IPhost *, IPhost *
#endif
			 );


/* 
 * netMaskIsBroadcast -- returns non-zero if the host is a broadcast address. 
 * address for its subnet.  A broadcast address is defined as:
 *
 *	1) having all ones or all zeroes in its host component 
 *	2) having all ones or all zeroes in the host component using the
 *	   default network mask for that address (this catches
 *	   'full network broadcast addresses' in the presence of subnets.)
 *  or	3) having ones in the entire address
 */
int	netMaskIsBroadcast(
#ifdef __STDC__
			   IPhost *
#endif
			   );

extern IPhost	IP_LOCAL_BCAST;

#endif netmask_h
