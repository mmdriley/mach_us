/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990 Carnegie Mellon University
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
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/pkg/malloc/lib/trace.c,v $
 *
 * Purpose: tracing functions for the malloc package.
 *
 * HISTORY
 * $Log:	trace.c,v $
 * Revision 2.4  94/07/14  16:03:45  mrt
 * 	Updated copyright
 * 
 * Revision 2.3  93/01/20  17:39:09  jms
 * 	Update to new_ipc.
 * 	Disable tracing by default
 * 	[93/01/18  17:25:24  jms]
 * 
 * Revision 2.2  90/10/29  17:34:12  dpj
 * 	Created.
 * 	[90/10/27  18:03:18  dpj]
 * 
 * 	First working version.
 * 	[90/10/21  21:35:11  dpj]
 * 
 *
 */

#ifndef lint
char * trace_rcsid = "$Header: trace.c,v 2.4 94/07/14 16:03:45 mrt Exp $";
#endif	lint

#include	<dll.h>
#include	<mach.h>
#include	<stdio.h>

#include	"malloc_backtrace.h"


/*
 * Depth for the stack traces.
 */
#ifndef	TRACE_DEPTH
#define	TRACE_DEPTH		5
#endif	TRACE_DEPTH

/*
 * Trace records.
 */
typedef struct malloc_trace_rec {
	dll_chain_t		chain;
	char			*event_label;
	int			req_nbytes;
	int			act_nbytes;
	char			*addr;
	char			*callers[TRACE_DEPTH];
} *malloc_trace_rec_t;

/*
 * Global variables.
 */
int				malloc_trace_buffer_output = 0;
int				malloc_trace_enabled = 0;
int				malloc_trace_lock = 0;
int				malloc_trace_inited = 0;
dll_head_t			malloc_trace_free_head;
dll_head_t			malloc_trace_active_head;
int				malloc_trace_free_count = 0;
int				malloc_trace_active_count = 0;


/*
 * Write one trace record to the default output stream.
 */
void malloc_trace_write(rec)
	malloc_trace_rec_t	rec;
{
	int			i;

	if (rec->req_nbytes == rec->act_nbytes)
		fprintf(stderr, "MALLOC_TRACE: %s of %ld at %ld\n",
			rec->event_label, (long) rec->req_nbytes,
			(long) rec->addr);
	else
		fprintf(stderr, "MALLOC_TRACE: %s of %ld gets %ld at %ld\n",
			rec->event_label, (long) rec->req_nbytes,
			(long) rec->act_nbytes, (long) rec->addr);

	for (i = 0; i < TRACE_DEPTH; i++) {
		if (rec->callers[i] == 0) break;
		fprintf(stderr, "MALLOC_TRACE: caller %8.8lx\n",
						(long) rec->callers[i]);
	}

	fprintf(stderr, "MALLOC_TRACE: \n");
}


/*
 * Expand the list of trace records.
 */
void malloc_trace_expand()
{
	kern_return_t		kr;
	char			*zone;
	int			num;
	malloc_trace_rec_t	rec;

	zone = 0;
	kr = vm_allocate(mach_task_self(),&zone,vm_page_size,TRUE);
	if (kr != KERN_SUCCESS) {
		mach3_spin_unlock(&malloc_trace_lock);
		mach_error("malloc_trace_expand.vm_allocate",kr);
		exit(1);
	}

	fprintf(stderr,"MALLOC_TRACE -- adding %d bytes of buffer space\n",
								vm_page_size);

	num = vm_page_size / sizeof(struct malloc_trace_rec);
	rec = (malloc_trace_rec_t) zone;

	mach3_spin_lock(&malloc_trace_lock);

	malloc_trace_free_count += num;

	for (; num > 0; num--, rec++) {
		dll_enter(&malloc_trace_free_head,rec,
						malloc_trace_rec_t,chain);
	}

	mach3_spin_unlock(&malloc_trace_lock);
}


/*
 * Clean the list of trace records.
 */
void malloc_trace_clean()
{
	malloc_trace_rec_t	rec;

	mach3_spin_lock(&malloc_trace_lock);

	fprintf(stderr,"MALLOC_TRACE -- cleaning %d records\n",
						malloc_trace_active_count);

	rec = (malloc_trace_rec_t) dll_first(&malloc_trace_active_head);
	while (! dll_end(&malloc_trace_active_head,(dll_entry_t) rec)) {
		dll_remove(&malloc_trace_active_head,rec,
						malloc_trace_rec_t,chain);
		malloc_trace_active_count--;
		dll_enter(&malloc_trace_free_head,rec,
						malloc_trace_rec_t,chain);
		malloc_trace_free_count++;
		rec = (malloc_trace_rec_t) dll_first(
						&malloc_trace_active_head);
	}

	mach3_spin_unlock(&malloc_trace_lock);
}


/*
 * Dump the list of trace records.
 */
void malloc_trace_dump()
{
	malloc_trace_rec_t	rec;

	mach3_spin_lock(&malloc_trace_lock);

	fprintf(stderr,"MALLOC_TRACE -- dumping %d records\n",
						malloc_trace_active_count);

	rec = (malloc_trace_rec_t) dll_first(&malloc_trace_active_head);
	while (! dll_end(&malloc_trace_active_head,(dll_entry_t) rec)) {
		malloc_trace_write(rec);
		rec = (malloc_trace_rec_t) dll_next((dll_entry_t)&rec->chain);
	}

	mach3_spin_unlock(&malloc_trace_lock);
}


/*
 * Enter one trace record.
 */
malloc_trace_enter(event_label,req_nbytes,act_nbytes,addr)
	char			*event_label;
	int			req_nbytes;
	int			act_nbytes;
	char			*addr;
{
	malloc_trace_rec_t	rec;
	struct malloc_trace_rec	rec_data;
	int			i;
	FRAMEPTR		frame;

	mach3_spin_lock(&malloc_trace_lock);

	if (malloc_trace_inited == 0) {
		malloc_trace_inited = 1;
		dll_init(&malloc_trace_free_head);
		dll_init(&malloc_trace_active_head);
	}

	if (malloc_trace_buffer_output) {
		while (dll_empty(&malloc_trace_free_head)) {
			mach3_spin_unlock(&malloc_trace_lock);
			malloc_trace_expand();
			mach3_spin_lock(&malloc_trace_lock);
		}
		dll_remove_first(&malloc_trace_free_head,rec,
						malloc_trace_rec_t,chain);
		malloc_trace_free_count--;
	} else {
		rec = &rec_data;
	}

	rec->event_label = event_label;
	rec->req_nbytes = req_nbytes;
	rec->act_nbytes = act_nbytes;
	rec->addr = addr;
	for (frame = NEXTFRAME(MYFRAME(event_label)), i = 0;
		frame && (i < TRACE_DEPTH);
		frame = NEXTFRAME(frame), i++) {
		rec->callers[i] = RET_ADDR(frame);
	}

	if (malloc_trace_buffer_output) {
		dll_enter(&malloc_trace_active_head,rec,
						malloc_trace_rec_t,chain);
		malloc_trace_active_count++;
	} else {
		malloc_trace_write(rec);
	}

	mach3_spin_unlock(&malloc_trace_lock);
}
