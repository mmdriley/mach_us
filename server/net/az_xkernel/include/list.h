/*     
 * list.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:40:16 $
 */


/*
 *	A generic singly-linked list
 */

#ifndef list_h
#define list_h

struct list_entry {
	struct list_entry	*next;		/* next element */
};

struct list_head {
	struct list_entry	*head;		/* first element */
	struct list_entry	*tail;		/* last  element */
};

typedef struct list_head	*list_t;
typedef	struct list_entry	*list_entry_t;

/*
 *	enlist puts "elt" on the "list".
 *	delist returns the first element in the "list".
 */

#define enlist(list,elt)	enlist_tail(list, elt)
#define	delist(list)		delist_head(list)


#ifdef __STDC__

list_entry_t	delist_head( list_t );
list_entry_t	delist_head_strong( list_t );
list_entry_t	delist_tail( list_t );
void 		enlist_head( list_t, list_entry_t );
void 		enlist_tail( list_t, list_entry_t );
void		list_init( list_t );


#else

list_entry_t	delist_head();
list_entry_t	delist_head_strong();
list_entry_t	delist_tail();
void 		enlist_head();
void 		enlist_tail();
void		list_init();

#endif __STDC__

#endif list_h
