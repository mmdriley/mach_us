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
 *	File:	queue.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Type definitions for generic queues.
 *
 */
/*
 * HISTORY
 * $Log:	queue.h,v $
 * Revision 1.5  94/07/21  16:38:45  mrt
 * 	Updated copyright
 * 
 * Revision 1.4  90/10/02  11:34:00  mbj
 * 	Track single server changes.
 * 	[90/10/01  15:20:08  mbj]
 * 
 * Revision 1.3.1.1  90/09/10  17:53:51  mbj
 * 	Paul Neves' pure kernel tty_server changes
 * 
 * Revision 2.2  89/09/08  11:26:11  dbg
 * 	Streamlined generic queue macros.
 * 	[89/08/17            dbg]
 * 
 * 19-Dec-88  David Golub (dbg) at Carnegie-Mellon University
 *	Added queue_remove_last; removed round_queue.  [Changes from
 *	mwyoung: 19 Dec 88.]
 *
 * 29-Nov-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed include of cputypes.h.  Added queue_iterate.
 *	Split into two groups the macros that operate directly on
 *	queues (or expect the queue chain to be first in a structure)
 *	and those that operate on generic structures (where the chain
 *	may be anywhere).
 *
 * 17-Jan-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added queue_enter_first, queue_last and queue_prev for use by
 *	the TCP netport code.
 *
 * 17-Mar-87  David Golub (dbg) at Carnegie-Mellon University
 *	Made queue package return queue_entry_t instead of vm_offset_t.
 *
 * 27-Feb-87  David Golub (dbg) at Carnegie-Mellon University
 *	Made remqueue a real routine on all machines.  Defined old
 *	Queue routines as macros that invoke new queue routines.
 *
 * 25-Feb-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Fixed non-KERNEL compilation.
 *
 * 24-Feb-87  David L. Black (dlb) at Carnegie-Mellon University
 *	MULTIMAX: remqueue is now a real routine, removed define.
 *	This is done to mask a compiler bug.
 *
 * 26-Dec-86  David Golub (dbg) at Carnegie-Mellon University
 *	Removed unused macros: qcast, initqueue, queueempty, qnext,
 *	qtop, qprev.  These are duplicated by other macros in the
 *	package.
 *	Added history message just below.
 *
 *  8-Dec-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Removed old mpqueue package and replaced with macros here.
 *
 * 24-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Eliminated unused include of <sys/types.h>.
 *
 * 25-Jul-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Removed "qpush" and "qpop".
 *
 * 24-Jul-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Documentation change.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed enhancements made during VM coding.
 *
 * 12-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ************************************************************************
 */

#ifndef	_QUEUE_
#define	_QUEUE_

#if	ALLQUEUES
#include <sys/lock.h>
#endif	ALLQUEUES

/*
 *	Queue of abstract objects.  Queue is maintained
 *	within that object.
 *
 *	Supports fast removal from within the queue.
 *
 *	How to declare a queue of elements of type "foo_t":
 *		In the "*foo_t" type, you must have a field of
 *		type "queue_chain_t" to hold together this queue.
 *		There may be more than one chain through a
 *		"foo_t", for use by different queues.
 *
 *		Declare the queue as a "queue_t" type.
 *
 *		Elements of the queue (of type "foo_t", that is)
 *		are referred to by reference, and cast to type
 *		"queue_entry_t" within this module.
 */

/*
 *	A generic doubly-linked list (queue).
 */

struct queue_entry {
	struct queue_entry	*next;		/* next element */
	struct queue_entry	*prev;		/* previous element */
};

typedef struct queue_entry	*queue_t;
typedef	struct queue_entry	queue_head_t;
typedef	struct queue_entry	queue_chain_t;
typedef	struct queue_entry	*queue_entry_t;

/*
 *	enqueue puts "elt" on the "queue".
 *	dequeue returns the first element in the "queue".
 *	remqueue removes the specified "elt" from the specified "queue".
 */

#define enqueue(queue,elt)	enqueue_tail(queue, elt)
#define	dequeue(queue)		dequeue_head(queue)

void		enqueue_head();
void		enqueue_tail();
queue_entry_t	dequeue_head();
queue_entry_t	dequeue_tail();
void		remqueue();

/*
 *	Macro:		queue_init
 *	Function:
 *		Initialize the given queue.
 *	Header:
 *		void queue_init(q)
 *			queue_t		q;	/* MODIFIED *\
 */
#define	queue_init(q)	((q)->next = (q)->prev = q)

/*
 *	Macro:		queue_first
 *	Function:
 *		Returns the first entry in the queue,
 *	Header:
 *		queue_entry_t queue_first(q)
 *			queue_t	q;		/* IN *\
 */
#define	queue_first(q)	((q)->next)

/*
 *	Macro:		queue_next
 *	Function:
 *		Returns the entry after an item in the queue.
 *	Header:
 *		queue_entry_t queue_next(qc)
 *			queue_t qc;
 */
#define	queue_next(qc)	((qc)->next)

/*
 *	Macro:		queue_last
 *	Function:
 *		Returns the last entry in the queue.
 *	Header:
 *		queue_entry_t queue_last(q)
 *			queue_t	q;		/* IN *\
 */
#define	queue_last(q)	((q)->prev)

/*
 *	Macro:		queue_prev
 *	Function:
 *		Returns the entry before an item in the queue.
 *	Header:
 *		queue_entry_t queue_prev(qc)
 *			queue_t qc;
 */
#define	queue_prev(qc)	((qc)->prev)

/*
 *	Macro:		queue_end
 *	Function:
 *		Tests whether a new entry is really the end of
 *		the queue.
 *	Header:
 *		boolean_t queue_end(q, qe)
 *			queue_t q;
 *			queue_entry_t qe;
 */
#define	queue_end(q, qe)	((q) == (qe))

/*
 *	Macro:		queue_empty
 *	Function:
 *		Tests whether a queue is empty.
 *	Header:
 *		boolean_t queue_empty(q)
 *			queue_t q;
 */
#define	queue_empty(q)		queue_end((q), queue_first(q))


/*----------------------------------------------------------------*/
/*
 * Macros that operate on generic structures.  The queue
 * chain may be at any location within the structure, and there
 * may be more than one chain.
 */

/*
 *	Macro:		queue_enter
 *	Function:
 *		Insert a new element at the tail of the queue.
 *	Header:
 *		void queue_enter(q, elt, type, field)
 *			queue_t q;
 *			<type> elt;
 *			<type> is what's in our queue
 *			<field> is the chain field in (*<type>)
 */
#define queue_enter(head, elt, type, field)			\
{ 								\
	register queue_entry_t prev;				\
								\
	prev = (head)->prev;					\
	if ((head) == prev) {					\
		(head)->next = (queue_entry_t) (elt);		\
	}							\
	else {							\
		((type)prev)->field.next = (queue_entry_t)(elt);\
	}							\
	(elt)->field.prev = prev;				\
	(elt)->field.next = head;				\
	(head)->prev = (queue_entry_t) elt;			\
}

/*
 *	Macro:		queue_enter_first
 *	Function:
 *		Insert a new element at the head of the queue.
 *	Header:
 *		void queue_enter_first(q, elt, type, field)
 *			queue_t q;
 *			<type> elt;
 *			<type> is what's in our queue
 *			<field> is the chain field in (*<type>)
 */
#define queue_enter_first(head, elt, type, field)		\
{ 								\
	register queue_entry_t next;				\
								\
	next = (head)->next;					\
	if ((head) == next) {					\
		(head)->prev = (queue_entry_t) (elt);		\
	}							\
	else {							\
		((type)next)->field.prev = (queue_entry_t)(elt);\
	}							\
	(elt)->field.next = next;				\
	(elt)->field.prev = head;				\
	(head)->next = (queue_entry_t) elt;			\
}

/*
 *	Macro:		queue_field [internal use only]
 *	Function:
 *		Find the queue_chain_t (or queue_t) for the
 *		given element (thing) in the given queue (head)
 */
#define	queue_field(head, thing, type, field)			\
		(((head) == (thing)) ? (head) : &((type)(thing))->field)

/*
 *	Macro:		queue_remove
 *	Function:
 *		Remove an arbitrary item from the queue.
 *	Header:
 *		void queue_remove(q, qe, type, field)
 *			arguments as in queue_enter
 */
#define	queue_remove(head, elt, type, field)			\
{								\
	register queue_entry_t	next, prev;			\
								\
	next = (elt)->field.next;				\
	prev = (elt)->field.prev;				\
								\
	if ((head) == next)					\
		(head)->prev = prev;				\
	else							\
		((type)next)->field.prev = prev;		\
								\
	if ((head) == prev)					\
		(head)->next = next;				\
	else							\
		((type)prev)->field.next = next;		\
}

/*
 *	Macro:		queue_remove_first
 *	Function:
 *		Remove and return the entry at the head of
 *		the queue.
 *	Header:
 *		queue_remove_first(head, entry, type, field)
 *		entry is returned by reference
 */
#define	queue_remove_first(head, entry, type, field)		\
{								\
	register queue_entry_t	next;				\
								\
	(entry) = (type) ((head)->next);			\
	next = (entry)->field.next;				\
								\
	if ((head) == next)					\
		(head)->prev = (head);				\
	else							\
		((type)(next))->field.prev = (head);		\
	(head)->next = next;					\
}

/*
 *	Macro:		queue_remove_last
 *	Function:
 *		Remove and return the entry at the tail of
 *		the queue.
 *	Header:
 *		queue_remove_last(head, entry, type, field)
 *		entry is returned by reference
 */
#define	queue_remove_last(head, entry, type, field)		\
{								\
	register queue_entry_t	prev;				\
								\
	(entry) = (type) ((head)->prev);			\
	prev = (entry)->field.prev;				\
								\
	if ((head) == prev)					\
		(head)->next = (head);				\
	else							\
		((type)(prev))->field.next = (head);		\
	(head)->prev = prev;					\
}

/*
 *	Macro:		queue_assign
 */
#define	queue_assign(to, from, type, field)			\
{								\
	((type)((from)->prev))->field.next = (to);		\
	((type)((from)->next))->field.prev = (to);		\
	*to = *from;						\
}

/*
 *	Macro:		queue_iterate
 *	Function:
 *		iterate over each item in the queue.
 *		Generates a 'for' loop, setting elt to
 *		each item in turn (by reference).
 *	Header:
 *		queue_iterate(q, elt, type, field)
 *			queue_t q;
 *			<type> elt;
 *			<type> is what's in our queue
 *			<field> is the chain field in (*<type>)
 */
#define queue_iterate(head, elt, type, field)			\
	for ((elt) = (type) queue_first(head);			\
	     !queue_end((head), (queue_entry_t)(elt));		\
	     (elt) = (type) queue_next(&(elt)->field))


#if	ALLQUEUES

/*----------------------------------------------------------------*/
/*
 *	Define macros for queues with locks.
 */
struct mpqueue_head {
	struct queue_entry	head;		/* header for queue */
	struct slock		lock;		/* lock for queue */
};

typedef struct mpqueue_head	mpqueue_head_t;

#define	round_mpq(size)		(size)

#define mpqueue_init(q) \
	{ \
		queue_init(&(q)->head); \
		simple_lock_init(&(q)->lock); \
	}

#define mpenqueue_tail(q, elt) \
		simple_lock(&(q)->lock); \
		enqueue_tail(&(q)->head, elt); \
		simple_unlock(&(q)->lock);

#define mpdequeue_head(q, elt) \
		simple_lock(&(q)->lock); \
		if (queue_empty(&(q)->head)) \
			*(elt) = 0; \
		else \
			*(elt) = dequeue_head(&(q)->head); \
		simple_unlock(&(q)->lock);

/*
 *	Old queue stuff, will go away soon.
 */

/*
 **********************************************************************
 * HISTORY
 * 21-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD.  Changed to enQueue()/deQueue() names
 *	to avoid conflicts with 4.2 dequeue().
 *	[V1(1)]
 *
 * 06-Mar-80  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Created (V1.05).
 *
 **********************************************************************
 */

/*
 * General purpose structure to define circular queues.
 *  Both the queue header and the queue elements have this
 *  structure.
 */

struct Queue
{
    struct Queue * F;
    struct Queue * B;
};

#define	initQueue(q)	(queue_init((queue_t)(q)))
#define	enQueue(q,elt)	(enqueue_tail((queue_t)(q),(queue_entry_t)(elt)))
#define	deQueue(q)	((struct Queue *)dequeue_head((queue_t)(q)))
#define	remQueue(q,elt)	(remqueue((queue_t)(q),(queue_entry_t)(elt)))
#define	Queueempty(q)	(queue_empty((queue_t)(q)))

#endif	ALLQUEUES

#endif	_QUEUE_
