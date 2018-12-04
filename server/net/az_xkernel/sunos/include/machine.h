/* 
 * machine.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 23:53:17 $
 */

#ifndef machine_h
#define machine_h

#ifdef __STDC__

void	cancelSignalHandler( int );
void	init_clock( Pfv, long );
void	installSignalHandler( int, Pfi, VOID * );

#endif

#endif machine_h
