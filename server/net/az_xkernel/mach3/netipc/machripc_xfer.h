/*
 * machripc_xfer.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1991  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/01/14 19:05:30 $
 *
 */

#ifndef machripc_xfer_h
#define machripc_xfer_h


typedef int	NetPortNumber;
#define PORT_NUMBER_NETLEN	4
#define portNumberLoad(_p, _src)	bcopy((char *)(_src), (char *)(_p), \
					      PORT_NUMBER_NETLEN)
#define portNumberStore(_p, _dst)	bcopy((char *)(_p), (char *)(_dst), \
					      PORT_NUMBER_NETLEN)


#ifdef __STDC__

char *	portNumStr();

/* 
 * Set '*npd' to the netport descriptor for the given network mach port
 * representation.  The architecture tag is given to allow decoding of
 * the network representation.  If there is no appropriate local port
 * and 'create' is true, a new local port should be created.  If no
 * local port exists and 'create' is false, XK_FAILURE is returned.
 */
extern	xkern_return_t	findNetPort( mportNetRep *, mn_arch_tag_t,
				     bool create, mnetport **npd );

/* 
 * A new send right is being sent to the indicated host and we, as the
 * holder of the receive right are to add the host to the senders list.
 * If the host is not already on the senders list, we will initialize
 * the make_send_count to the indicated value.  
 */
extern	void		addNewSender( mnetport *, IPhost, int make_send_cnt );

/* 
 * A send right which was previously transferred via 'addSendRight' is
 * being revoked.
 */
extern	void		removeSendRight( mnetport * );

/* 
 * A receive right which was previously transferred via 'addReceiveRight' is
 * being revoked.
 */
extern	void		removeReceiveRight( mnetport * );

/* 
 * Undoes the effect of a previous 'addNewSender'.
 */
extern	void		removeSender( mnetport *, IPhost );

/* 
 * RRX has put the new receiver in the receiver_host_addr field of the
 * port.  This notification causes NetIPC to register BootId interest
 * with the new receiver, etc.
 *
 * If the local host was the old holder of the receive right for this
 * port, this call indicates the time when messages can be forwarded
 * to the new receiver (port information has been transferred to the
 * new receiver but all senders are still blocked.)
 */
extern void		receiverMoved( mnetport * );

/* 
 * Used by RRX to inform netIPC that the receive right has gone away
 * (in a way that netIPC wouldn't necessarily detect.)  
 */
extern void		receiveRightDeallocated( mnetport * );


#if 0

/* 
 * Loads 'dst' from the (unaligned) message buffer pointed to by 'src'
 */
extern void		mnetportLoad( char *src, mportNetRep *dst );

/* 
 * Stores 'src' into the (unaligned) message buffer pointed to by 'dst'
 */
extern void		mnetportStore( mportNetRep *src, char *dst );

#endif

#endif __STDC__


#  define mnetportLoad( _src, _dst )				\
		bcopy((_src), (char *)(_dst), MNETPORT_NETLEN)

#  define mnetportStore( _src, _dst )				\
		bcopy((char *)(_src), (_dst), MNETPORT_NETLEN)


#endif ! machripc_xfer_h
