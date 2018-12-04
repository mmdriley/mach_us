/* 
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990,1989,1988,1987 Carnegie Mellon University
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
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * Purpose:  Taken from 'mach3/.../src/mk/kernel/kern/queue.[hc]' for use
 *		with the xkernel routines which use it.
 * 
 * HISTORY
 * $Log:	insque.c,v $
 * Revision 2.2  94/01/11  18:09:14  jms
 * 	Had a long life.
 * 	[94/01/09  20:56:15  jms]
 * 
 */

struct queue_entry {
        struct queue_entry      *next;          /* next element */
        struct queue_entry      *prev;          /* previous element */
};

typedef struct queue_entry      *queue_t;
typedef struct queue_entry      queue_head_t;
typedef struct queue_entry      queue_chain_t;
typedef struct queue_entry      *queue_entry_t;

int insque(entry, pred)
        register struct queue_entry *entry, *pred;
{
        entry->next = pred->next;
        entry->prev = pred;
        (pred->next)->prev = entry;
        pred->next = entry;
	return(0);
}

int remque(elt)
        register struct queue_entry *elt;
{
        (elt->next)->prev = elt->prev;
        (elt->prev)->next = elt->next;
        return((int)elt);
}
