/* 
 * blast.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.7 $
 * $Date: 1993/02/01 22:41:00 $
 */

#ifndef blast_h
#define blast_h

#define BLAST_SETOUTSTANDINGMSGS (BLAST_CTL*MAXOPS + 0)
#define BLAST_GETOUTSTANDINGMSGS (BLAST_CTL*MAXOPS + 1)

#  ifdef __STDC__

void		blast_init( XObj );

#  endif

#endif blast_h
