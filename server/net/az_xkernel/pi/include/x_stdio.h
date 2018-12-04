/* 
 * x_stdio.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 23:57:06 $
 */

#ifndef x_stdio_h
#define x_stdio_h

#ifndef XKMACHKERNEL

#include <stdio.h>

#ifdef __STDC__

int	fclose( FILE * );
int	fflush( FILE * );
int	fprintf( FILE *, char *, ... );
int	fscanf( FILE *, char *, ... );
int	printf( char *, ... );
int	sscanf( char *, char *, ... );
void	setbuf( FILE *, char * );

#else 

int	fclose();
int	fflush();
int	fprintf();
int	fscanf();
int	printf();
int	sscanf();
void	setbuf();

#endif

int	_flsbuf();
int	_filbuf();

#endif ! XKMACHKERNEL

#endif ! x_stdio_h
