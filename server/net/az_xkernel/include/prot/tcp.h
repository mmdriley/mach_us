/* 
 * tcp.h
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:41:16 $
 */

#ifndef tcp_h
#define tcp_h


#define TCP_PUSH		(TCP_CTL*MAXOPS + 0)
#define TCP_GETSTATEINFO	(TCP_CTL*MAXOPS + 1)
#define TCP_DUMPSTATEINFO	(TCP_CTL*MAXOPS + 2)
#define TCP_GETFREEPROTNUM	(TCP_CTL*MAXOPS + 3)
#define TCP_RELEASEPROTNUM	(TCP_CTL*MAXOPS + 4)
#define TCP_SETRCVBUFSPACE	(TCP_CTL*MAXOPS + 5)	/* set rx buf space */
#define TCP_GETSNDBUFSPACE	(TCP_CTL*MAXOPS + 6)	/* get tx buf space */
#define TCP_SETRCVBUFSIZE	(TCP_CTL*MAXOPS + 7)	/* set rx buf size */
#define TCP_SETSNDBUFSIZE	(TCP_CTL*MAXOPS + 8)	/* set tx buf size */
#define TCP_SETOOBINLINE	(TCP_CTL*MAXOPS + 9)	/* set oob inlining */
#define TCP_GETOOBDATA		(TCP_CTL*MAXOPS + 10)	/* read the oob data */
#define TCP_OOBPUSH		(TCP_CTL*MAXOPS + 11)	/* send oob message */
#define TCP_OOBMODE		(TCP_CTL*MAXOPS + 12)	/* this is an upcall */

#  ifdef __STDC__

void	tcp_init( XObj );

#  endif


#endif
