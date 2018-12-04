/* 
 * idmap_templ.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.17 $
 * $Date: 1993/02/01 23:56:09 $
 */

/*
 * We need the definitions of:
 *	KEYSIZE
 *	HASH
 *	COMPBYTES
 *	COPYBYTES
 */

static Bind
BINDNAME ( table, ext, intern )
    Map table;
    register VOID *ext;
    VOID *intern;
{
    Bind     elem_posn, new_elem, prev_elem, *table_posn;
    register char *o_ext;
    int		ind;
    
    ind = HASH((char *)ext, table->tableSize, table->keySize);
    table_posn = &table->table[ind];

    elem_posn = *table_posn;
    prev_elem = elem_posn;
    while (elem_posn != 0) {
	o_ext = elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    if (elem_posn->internalid == intern) {
		return(elem_posn);
	    } else {
		return(ERR_BIND);
	    }
	} else {
	    prev_elem = elem_posn;
	    elem_posn = elem_posn->next;
	}
    }
    
    /* Note: Memory allocated here that is not immediately freed  cjt */
    /*
     * Elements must be inserted at the end of the list rather than at the
     * beginning for the semantics mapForEach to work properly.
     */
    GETMAPELEM(table, new_elem);
    COPYBYTES((char *)new_elem->externalid, (char *)ext);
    new_elem->internalid = intern;
    new_elem->next = 0;
    table->cache = new_elem;
    if ( prev_elem == 0 ) {
	*table_posn = new_elem;
    } else {
	prev_elem->next = new_elem;
    }
    return new_elem;
}

static xkern_return_t
RESOLVENAME ( table, ext, resPtr )
    Map table;
    register VOID *ext;
    register VOID **resPtr;
{
    register Bind elem_posn;
    register char *o_ext;
    
    if (elem_posn = table->cache) {
	o_ext = elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    if ( resPtr ) {
		*resPtr = elem_posn->internalid;
	    }
	    return XK_SUCCESS;
	}
    }
    elem_posn = table->table[HASH(ext, table->tableSize, table->keySize)];
    while (elem_posn != 0) {
	o_ext = elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    table->cache = elem_posn;
	    if ( resPtr ) {
		*resPtr = elem_posn->internalid;
	    }
	    return XK_SUCCESS;
	} else {
	    elem_posn = elem_posn->next;
	}
    }
    return XK_FAILURE;
}


/* 
 * UNBIND -- remove an entry based on the key->value pair
 */
static xkern_return_t
UNBINDNAME ( table, ext )
    Map table;
    register VOID *ext;
{
    Bind     elem_posn, *prev_elem;
    register char *o_ext;
    
    prev_elem = &table->table[HASH((char *)ext, table->tableSize,
				   table->keySize)];
    elem_posn = *prev_elem;
    table->cache = 0;
    while (elem_posn != 0) {
	o_ext = (char *)elem_posn->externalid;
	if (COMPBYTES(o_ext, (char *)ext)) {
	    *prev_elem = elem_posn->next;
	    FREEIT(table, elem_posn);
	    return XK_SUCCESS;
	} 
	prev_elem = &(elem_posn->next);
	elem_posn = elem_posn->next;
    }
    return XK_FAILURE;
}


/* 
 * REMOVE -- remove an entry based on the binding
 */
static xkern_return_t
REMOVENAME ( table, bind )
    Map table;
    Bind bind;
{
    Bind     elem_posn, *prev_elem;
    
    prev_elem = &table->table[HASH((char *)bind->externalid, table->tableSize,
				   table->keySize)];
    elem_posn = *prev_elem;
    table->cache = 0;
    while (elem_posn != 0) {
	if ( elem_posn == bind ) {
	    *prev_elem = elem_posn->next;
	    FREEIT(table, elem_posn);
	    return XK_SUCCESS;
	} 
	prev_elem = &(elem_posn->next);
	elem_posn = elem_posn->next;
    }
    return XK_FAILURE;
}



#undef BINDNAME
#undef REMOVENAME
#undef RESOLVENAME
#undef UNBINDNAME
#undef FIRSTNAME
#undef KEYSIZE
#undef HASH
#undef COMPBYTES
#undef COPYBYTES
