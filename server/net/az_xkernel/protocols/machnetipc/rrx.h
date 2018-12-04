/* 
 * rrx.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 22:34:29 $
 */
 
/* 
 * Mach Receive Right Transfer protocol (RRX)
 *
 * Interface description
 */


#ifndef rrx_h
#define rrx_h

#ifdef __STDC__

void		rrx_init( XObj );

/* 
 * Moves the receive rights in 'np' (a null-terminated array of
 * mnetport pointers) to host 'h' pending delivery of message 'id'.
 */
xkern_return_t	rrxMoveReceiveRights( IPhost h, MsgId id, mnetport **np );


/* 
 * Used to notify RRX that a message with ports transferred by RRX has
 * arrived.  
 */
void		rrxTransferComplete( IPhost , MsgId );


#else

void		rrx_init();
xkern_return_t	rrxMoveReceiveRights();
void		rrxTransferComplete();


#endif __STDC__

#endif  ! rrx_h
