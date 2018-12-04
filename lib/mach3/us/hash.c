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
 *  hash.c
 *
 *  Hash table routines:
 *		This is an open hash table, with
 *		the hash chains implemented as doubly-linked lists.
 * HISTORY
 * $Log:	hash.c,v $
 * Revision 2.4  94/07/08  18:13:47  mrt
 * 	Updated copyrights
 * 
 * Revision 2.3  93/01/20  17:37:20  jms
 * 	Stop leaking hash entries upon hash_remove
 * 	[93/01/18  16:17:54  jms]
 * 
 * Revision 2.2  90/07/26  12:41:40  dpj
 * 	First version
 * 	[90/07/24  14:45:53  dpj]
 * 
 * Revision 1.3  89/03/17  12:32:50  sanzi
 * 	add hash_free().
 * 	[89/02/08  16:48:29  dorr]
 * 
 * Revision 1.2  88/10/17  10:50:37  dorr
 * add rick's changes to allow defaulting of compare
 * and lookup functions.
 * 
 * Revision 1.2  88/08/09  14:00:52  sanzi
 * Added creation date, copyright message.
 * 
 *  05-May-88  Richard Sanzi (sanzi) at Carnegie-Mellon University
 *	Created.
 */

#include <hash.h>

typedef int (*routine_t)();

/*
 * Create and return a hash table which uses the given
 * mapping function, with the given number of initial entries.
 *
 * All entries must be initialized.
 */

hash_table_t hash_init (hashfun,comparefun,size)
int (*hashfun)();
boolean_t (*comparefun)();
unsigned int size;
{
    int i;
    hash_table_t ht = (hash_table_t) malloc(sizeof(struct hash_table));

    ht->table = (hash_entry_t) malloc(size * sizeof(struct hash_entry));
    ht->length = size-1;
    ht->hashfun = hashfun;
    ht->hashcompare = comparefun;

    for (i=0;i<size;i++) {
	ht->table[i].next = ht->table[i].next = HASH_NULL;
	ht->table[i].key = HASH_INVALID;
    }
    return(ht);
}

void hash_free(ht)
	register hash_table_t ht;
{
	register int i;
	/*
	 * blast this suckah
	 */
	for (i=0;i<=ht->length;i++) {

		register hash_entry_t entry = ht->table[i].next;
		register hash_entry_t next;

		while (entry) {
			next = entry->next;
			free(entry);
			entry = next;
		}

	}

	free(ht->table);
	free(ht);
}

/*
 * enter the key,value pair into the hash table by first
 * calling (*ht->hashfun)(key), then inserting the entry into
 * the list.
 * return TRUE if successful, and FALSE if the entry exists.
 */

boolean_t hash_enter (ht,key,value)
register hash_table_t ht;
register hash_key_t key;
hash_value_t value;
{
    register int index;
    register hash_entry_t entry;
    routine_t hash_function = ht->hashfun;
    routine_t compare = ht->hashcompare;

    if (hash_function == (routine_t)0) {
	index = (key & ht->length);
    } else {
	index = (*hash_function)(key) % (ht->length+1);
    }

    entry = &ht->table[index];

    if (compare == (routine_t)0) {
	while (entry->next != HASH_NULL) {
   		entry = entry->next;	
		if (entry->key == key) return(FALSE);
	}
    } else {
	while (entry->next != HASH_NULL) {
   		entry = entry->next;	
		if ((*compare)(entry->key,key))  return(FALSE);
	}
    }

    entry->next = (hash_entry_t) malloc(sizeof(struct hash_entry));
    if (entry->next != HASH_NULL) {
	entry->next->prev = entry;
	entry->next->next = HASH_NULL;
    }
    entry = entry->next;

    entry->key = key;
    entry->value = value;
    return(TRUE);
}
/*
 * return the entry corresponding to the given key .
 * return HASH_NULL if no entry exists.
 */
 
hash_value_t hash_lookup (ht,key)
register hash_table_t ht;
register hash_key_t key;
{
    register int index;
    register hash_entry_t entry;
    routine_t hash_function = ht->hashfun;
    routine_t compare = ht->hashcompare;

    if (hash_function == (routine_t)0) {
	index = (key & ht->length);
    } else {
	index = (*hash_function)(key) % (ht->length+1);
    }

    entry = &ht->table[index];

    if (compare == (routine_t)0) {
	    while (entry->next != HASH_NULL) {
		entry = entry->next;	
		if (entry->key == key) return(entry->value);
	    }
    } else {
	    while (entry->next != HASH_NULL) {
		entry = entry->next;	
		if ((*compare)(entry->key,key))  return(entry->value);
	    }
    }

    return((hash_value_t) HASH_NULL);
}


/*
 * remove the given entry from the hash table.
 * return TRUE if successful, FALSE if the entry
 * does not exist.
 */

boolean_t hash_remove (ht,key)
hash_table_t ht;
hash_key_t key;
{
    register int index;
    register hash_entry_t entry;
    routine_t hash_function = ht->hashfun;
    routine_t compare = ht->hashcompare;

    if (hash_function == (routine_t)0) {
	index = (key & ht->length);
    } else {
	index = (*hash_function)(key) % (ht->length+1);
    }

    entry = &ht->table[index];

    if (compare == (routine_t)0) {
    	while (entry->next != HASH_NULL) {
		entry = entry->next;
		if (entry->key == key) {
			if (entry->prev != HASH_NULL) 
				entry->prev->next = entry->next;
			if (entry->next != HASH_NULL) 
				entry->next->prev = entry->prev;
			free(entry);
		    	return(TRUE);
		}
	}
    } else {
    	while (entry->next != HASH_NULL) {
		entry = entry->next;
		if ((*compare)(entry->key,key)) {
			if (entry->prev != HASH_NULL) 
				entry->prev->next = entry->next;
			if (entry->next != HASH_NULL) 
				entry->next->prev = entry->prev;
			free(entry);
		    	return(TRUE);
		}
	}
    }

    return(FALSE);
}

 
boolean_t hash_apply(ht,fun)
hash_table_t ht;
int (*fun)();
{
    hash_entry_t entry;
    int i;
    for (i = 0 ; i < ht->length+1 ; i++) {
	entry = ht->table[i].next;	
	while (entry != HASH_NULL) {
	    (*fun)(entry->value);
	    entry = entry->next;
	}
    }
    return(TRUE);
}
	    
    
