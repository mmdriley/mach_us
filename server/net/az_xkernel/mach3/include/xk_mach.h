/* 
 * xk_mach.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/02 00:03:27 $
 */

/* 
 * prototypes for un-prototyped mach functions
 */

#include <mach/error.h>

#ifdef __STDC__

#ifndef XKMACHKERNEL

void			mach_msg_destroy( mach_msg_header_t * );
mach_msg_return_t	mach_msg_receive( mach_msg_header_t * );
mach_msg_return_t	mach_msg_server(boolean_t(*)(), mach_msg_size_t,
					mach_port_t);
mach_port_t		task_by_pid( int );
kern_return_t 		task_priority( task_t, int, boolean_t );
kern_return_t 		thread_priority( task_t, int, boolean_t );
mach_port_t 		mach_thread_self( void );
void			quit( int, char *, ... );

char			*mach_error_string( mach_error_t );

int			cthread_kernel_limit( void );
void			cthread_set_kernel_limit( int );
void			cthread_wire( void );

#endif ! XKMACHKERNEL


#endif
