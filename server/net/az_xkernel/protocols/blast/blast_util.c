/*
 * blast_util.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:20:44 $
 */

#include "blast_internal.h"
 
/* 
 * Returns the index of the first set bit in the mask
 */
int
blast_mask_to_i(mask)
    BlastMask	mask;
{
    int i;
    xTrace1(blastp, TR_FUNCTIONAL_TRACE, "mask_to_i: called mask =  %s",
	    blastShowMask(mask));
    
    for (i=1; i <= BLAST_MAX_FRAGS; i++) {
	if ( BLAST_MASK_IS_BIT_SET(&mask, 1) ) {
	    xTrace1(blastp, TR_DETAILED, "mask_to_i: returns i =  %d", i);
	    return i;
	}
	BLAST_MASK_SHIFT_RIGHT(mask, 1);
    }
    xTrace0(blastp, TR_DETAILED, "mask_to_i: returns 0");
    return 0;
}
    

void
blast_mapFlush(m)
    Map m;
{
  /* should remove all entries from map but. noop for now */
}

