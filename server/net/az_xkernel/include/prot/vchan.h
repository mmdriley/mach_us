/*
 * vchan.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:41:33 $
 */

#ifndef vchan_h
#define vchan_h

/*
 * controlsession OPCODES
 */
#define VCHAN_INCCONCURRENCY (VCHAN_CTL*MAXOPS + 0)
#define VCHAN_DECCONCURRENCY (VCHAN_CTL*MAXOPS + 1)


#  ifdef __STDC__

void	vchan_init( XObj );

#  endif

#endif
