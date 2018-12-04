/*
 * port_mgr.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.9 $
 * $Date: 1993/02/01 22:30:17 $
 */

/* 
 * This file defines the prototypes for the port management functions.
 * A header file with these prototypes can be formed by defining the macro
 *	NAME -- token to prepend to routine names
 * and then including this file.
 */


#ifdef __STDC__
#define XPASTE(X,Y) X##Y
#define PASTE(X,Y) XPASTE(X,Y)

/* 
 * Initializes the port map.  Must be called before any other routines. 
 */
void	PASTE(NAME, PortMapInit) ( void );

/* 
 * Attempts to get a free port >= FIRST_USER_PORT, placing the
 * new port in *port.  Returns 0 if successful, non-zero if not.
 */
int	PASTE(NAME, GetFreePort) ( long * );

/* 
 * Increases the reference count of the port.  The port does not have
 * to have been previously acquired.
 */
int	PASTE(NAME, DuplicatePort) ( long );

/* 
 * Decreases the reference count on a port previously acquired through
 * DuplicatePort() or GetFreePort().
 */
void	PASTE(NAME, ReleasePort) ( long );

#endif
