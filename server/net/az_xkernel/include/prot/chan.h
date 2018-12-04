/* 
 * chan.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:41:57 $
 */

#ifndef chan_h
#define chan_h

enum {
    CHAN_ABORT_CALL = CHAN_CTL*MAXOPS,
    CHAN_SET_TIMEOUT,
    CHAN_GET_TIMEOUT,
    CHAN_SET_MAX_TIMEOUT,
    CHAN_GET_MAX_TIMEOUT
};
    
#ifdef __STDC__

void	chan_init( XObj );

#endif

#endif chan_h
