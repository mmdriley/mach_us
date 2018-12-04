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
 *	File:	zalloc.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 * HISTORY:
 * $Log:	zalloc.h,v $
 * Revision 1.5  94/07/21  17:49:21  mrt
 * 	Updated copyright
 * 
 * Revision 1.4  91/07/01  14:16:13  jms
 * 	Merge to new branch.
 * 	[91/05/29  10:50:36  roy]
 * 
 * Revision 1.3.2.1  90/11/05  18:47:41  roy
 * 	No change.
 * 
 * 
 * Revision 1.3.1.1  90/10/29  16:33:49  roy
 * 	Created from Mach 3 single-server source.
 * 
 * 
 *
 */

#ifndef	_ZALLOC_
#define	_ZALLOC_

/*
#include <uxkern/import_mach.h>
#include <sys/macro_help.h>
*/

#include <mach.h>
#include <cthreads.h>

/* #include "macro_help.h" */
/* Just include relevant macro_help.h stuff inline for now. */
#include <mach/boolean.h>

#ifdef	lint
boolean_t	NEVER;
boolean_t	ALWAYS;
#else	lint
#define		ALWAYS		TRUE
#endif	lint

#define		MACRO_BEGIN	do {
#define		MACRO_END	} while (NEVER)

#define		MACRO_RETURN	if (ALWAYS) return
/* End macro_help.h stuff */

/*
 *	A zone is a collection of fixed size blocks for which there
 *	is fast allocation/deallocation access.  Kernel routines can
 *	use zones to manage data structures dynamically, creating a zone
 *	for each type of data structure to be managed.
 *
 */

typedef struct zone {
	struct mutex	lock;		/* generic lock */
	int		count;		/* Number of elements used now */
	vm_offset_t	free_elements;
	vm_size_t	cur_size;	/* current memory utilization */
	vm_size_t	max_size;	/* how large can this zone grow */
	vm_size_t	elem_size;	/* size of an element */
	vm_size_t	alloc_size;	/* size used for more memory */
	boolean_t	doing_alloc;	/* is zone expanding now? */
	char		*zone_name;	/* a name for the zone */
	unsigned int
	/* boolean_t */	pageable :1,	/* zone pageable? */
	/* boolean_t */	sleepable :1,	/* sleep if empty? */
	/* boolean_t */ exhaustible :1;	/* merely return if empty? */
} *zone_t;

#define		ZONE_NULL	((zone_t) 0)

vm_offset_t	zalloc();
vm_offset_t	zget();
zone_t		zinit();
void		zfree();
void		zchange();

#define ADD_TO_ZONE(zone, element) \
	MACRO_BEGIN							\
		*((vm_offset_t *)(element)) = (zone)->free_elements;	\
		(zone)->free_elements = (vm_offset_t) (element);	\
		(zone)->count--;					\
	MACRO_END

#define REMOVE_FROM_ZONE(zone, ret, type)				\
	MACRO_BEGIN							\
	(ret) = (type) (zone)->free_elements;				\
	if ((ret) != (type) 0) {					\
		(zone)->count++;					\
		(zone)->free_elements = *((vm_offset_t *)(ret));	\
	}								\
	MACRO_END

#define ZFREE(zone, element)		\
	MACRO_BEGIN			\
	register zone_t	z = (zone);	\
					\
	mutex_lock(&z->lock);		\
	ADD_TO_ZONE(z, element);	\
	mutex_unlock(&z->lock);		\
	MACRO_END

#define	ZALLOC(zone, ret, type)			\
	MACRO_BEGIN				\
	register zone_t	z = (zone);		\
						\
	mutex_lock(&z->lock);			\
	REMOVE_FROM_ZONE(zone, ret, type);	\
	mutex_unlock(&z->lock);			\
	if ((ret) == (type)0)			\
		(ret) = (type)zalloc(z);	\
	MACRO_END

#define	ZGET(zone, ret, type)			\
	MACRO_BEGIN				\
	register zone_t	z = (zone);		\
						\
	mutex_lock(&z->lock);			\
	REMOVE_FROM_ZONE(zone, ret, type);	\
	mutex_unlock(&z->lock);			\
	MACRO_END

void		zcram();
void		zone_init();

#endif	_ZALLOC_
