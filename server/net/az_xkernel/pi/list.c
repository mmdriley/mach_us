/*     
 * list.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 23:58:32 $
 */

#include "xk_debug.h"
#include "trace.h"
#include "list.h"
int tracelist;

/*
 *	Initialize a list
 *       
 */
void list_init(list)
	register struct list_head *list;
{
	xTrace1(list,TR_EVENTS,"list_init: %x",list);
	list->head = list->tail = 0;
}

/*
 *	Insert element at tail of list.
 *       It is safe for this to be interrupted by a head consumer.
 */
void enlist_tail(list,elt)
	register struct list_head *list;
	register list_entry_t	elt;
{
	xTrace4(list,TR_EVENTS,
		"enlist_tail: list %x  head %x  tail %x  elt %x",
		list,list->head,list->tail,elt);
        elt->next = 0;
	if (list->tail) list->tail->next = elt;
	list->tail = elt;
	if (!list->head) list->head = elt;
}

/*
 *	Remove and return element at head of list.
 *        It is safe for this to interrupt a tail supplier.
 */
list_entry_t delist_head(list)
	register struct list_head *list;
{
	register list_entry_t	elt;

	xTrace3(list,TR_EVENTS,"delist_head: list %x  head %x  tail %x",
		list,list->head,list->tail);
	/* consumer may not take from a zero or one element list */
	if ( !list->head || list->tail == list->head )
		return((list_entry_t)0);

	elt = list->head;
	list->head = elt->next;
	elt->next = 0;
	xTrace3(list,TR_EVENTS,"delist_head2: list %x  head %x  tail %x",
		list,list->head,list->tail);
	return(elt);
}

/*
 *	Remove and return element at head of list.
 *        This will remove the last element in the list.
 *        It is not safe unless it has sync'ed with the supplier
 */
list_entry_t delist_head_strong(list)
	register list_t	list;
{
	register list_entry_t	elt;

	xTrace3(list,TR_EVENTS,
		"delist_head_strong: list %x  head %x  tail %x",
		list,list->head,list->tail);
	if ( !list->head ) return((list_entry_t)0);

	elt = list->head;
	list->head = elt->next;
	if (!list->head) list->tail = 0;
	xTrace3(list,TR_EVENTS,
		"delist_head_strong2: list %x  head %x  tail %x",
		list,list->head,list->tail);
	return(elt);
}

/*
 *	Insert element at head of list.
 */
void enlist_head(list, elt)
	register list_t	list;
	register list_entry_t	elt;
{
	xTrace4(list,TR_EVENTS,
		"enlist_head: list %x  head %x  tail %x  elt %x",
		list,list->head,list->tail,elt);
	elt->next = list->head;
	list->head = elt;
}

/*
 *	Remove and return element at tail of list.
 *      Can't be done with singly-linked list.
 */
list_entry_t delist_tail(list)
	register list_t	list;
{
	return((list_entry_t)0);
}

