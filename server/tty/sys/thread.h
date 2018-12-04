/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they made and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
/*
 *	File:	thread.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	This file contains the structure definitions for threads.
 *
 * HISTORY
 * $Log:	thread.h,v $
 * Revision 1.3  94/07/21  16:40:03  mrt
 * 	Updated copyright
 * 
 * Revision 1.2  91/07/01  14:15:35  jms
 * 	Make "uarea" in a thread based structure instad of just one.
 * 	[91/06/25  13:27:38  jms]
 * 
 * Revision 1.1  88/11/16  23:14:21  mbj
 * Initial revision
 * 
 */

#ifndef	_THREAD_
#define	_THREAD_

#include <sys/task.h>

struct thread {
	/* Task information */
	us_task_t		task;		/* Task to which I belong */

	/* Blocking information */
	int		wait_event;	/* event we are waiting on */
	int		exit_code;	/* How to exit the kernel: */
/*
 *	Exit code [enumerated type]
 */
#define	THREAD_EXIT		0	/* exit kernel normally */
#define	THREAD_TERMINATE	1	/* terminate on exit from kernel */
#define	THREAD_HALT		2	/* halt on exit from kernel */
#define	THREAD_HALTED		3	/* confirmation - thread has halted */

	/* Compatibility garbage */
	struct u_address {
		struct uthread	*uthread;
		struct utask	*utask;
	} u_address;
	int		unix_lock;	/* bind to unix_master */
};

typedef struct thread *us_thread_t;
#define THREAD_NULL	((us_thread_t) 0)

#if	US

/* routines for finding/creating and freeing uarea like stuff */
extern us_thread_t tty_get_thread_info();
extern void tty_release_thread_info();

#define	thread_should_halt()	(current_thread()->exit_code != THREAD_EXIT)

/*
 *	Machine specific implementations of the current thread macro
 *	designate this by defining CURRENT_THREAD.
 */
#ifndef CURRENT_THREAD
#define	current_thread()	(tty_get_thread_info())
#endif	CURRENT_THREAD

#endif	US
#endif	_THREAD_
