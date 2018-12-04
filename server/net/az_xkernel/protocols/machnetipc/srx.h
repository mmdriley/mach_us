/* 
 * srx.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 22:34:24 $
 */
 
/* 
 * Mach Send Right Transfer protocol (SRX)
 */


#ifndef srx_h
#define srx_h


#ifdef __STDC__

/* 
 * Used to notify SRX that a message with ports transferred by SRX has
 * arrived.  
 */
void		srxTransferComplete( IPhost , MsgId );

/* 
 * Moves the send rights in 'np' (a null-terminated array of
 * mnetport pointers) to host 'h' pending delivery of message 'id'.
 */
xkern_return_t	srxMoveSendRights( IPhost h, MsgId id, mnetport **np );

#else

void		srxTransferComplete();
xkern_return_t	srxMoveSendRights();

#endif __STDC__

#endif  ! srx_h
