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
 *	File:	h/lock.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Locking primitives definitions
 *
 * HISTORY:
 * $Log:	lock.h,v $
 * Revision 1.6  94/07/21  17:49:26  mrt
 * 	Updated copyright
 * 
 * Revision 1.5  89/05/18  10:47:31  dorr
 * 	include file cataclysm
 * 
 * 25-May-88  Richard Sanzi (sanzi) at Carnegie-Mellon University
 *	Changed to use cthreads structures and routines.  Purged history.
 *
 *
 */

#ifndef	_LOCK_H_
#define	_LOCK_H_

#include <features.h>

#include <mach/boolean.h>

#include <cthreads.h>

/*
 *	The general lock structure.  Provides for multiple readers,
 *	upgrading from read to write, and sleeping until the lock
 *	can be gained.
 */

struct lock {
	/*	Only the "interlock" field is used for hardware exclusion;
	 *	other fields are modified with normal instructions after
	 *	acquiring the interlock bit.
	 */
	struct mutex	interlock;	/* Interlock for remaining fields    */
	boolean_t	want_write;	/* Writer is waiting, or locked      */
					/* for write 			     */
	boolean_t	want_upgrade;	/* Read-to-write upgrade waiting     */
	boolean_t	waiting;	/* Someone is sleeping on lock       */
	int		read_count;	/* Number of accepted readers        */
	cthread_t	thread;		/* thread that has the lock 	     */
	int		recursion_depth;/* Depth of recursion 		     */
	struct condition
			lock_wakeup;	/* condition variable for notifies */
};

typedef	struct mutex	simple_lock_data_t;
typedef struct mutex	*simple_lock_t;

typedef	struct lock	lock_data_t;
typedef	struct lock	*lock_t;

#define	simple_lock_init(l)	mutex_init(l)
#define simple_lock(l)		mutex_lock(l)
#define	simple_unlock(l)	mutex_unlock(l)

/* Sleep locks must work even if no multiprocessing */

void		lock_init();
void		lock_sleepable();
void		lock_write();
void		lock_read();
void		lock_done();
boolean_t	lock_read_to_write();
void		lock_write_to_read();
boolean_t	lock_try_write();
boolean_t	lock_try_read();
boolean_t	lock_try_read_to_write();

#define	lock_read_done(l)	lock_done(l)
#define	lock_write_done(l)	lock_done(l)

void		lock_set_recursive();
void		lock_clear_recursive();

#endif	_LOCK_H_
