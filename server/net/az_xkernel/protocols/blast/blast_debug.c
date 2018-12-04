/*
 * blast_debug.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.12 $
 * $Date: 1993/02/01 22:20:08 $
 */

#include "blast_internal.h"


void
blast_phdr(hdr)
    BLAST_HDR *hdr;
{
    xTrace1(blastp, TR_ALWAYS, "BLAST header: %s", blastOpStr(hdr->op));
    xTrace5(blastp, TR_ALWAYS,
	    " p: %d  sq: %d  f: %d  m: %s  l: %d",
	    hdr->prot_id, hdr->seq, hdr->num_frag,
	    blastShowMask(hdr->mask), hdr->len);
}


char *
blastOpStr(op)
    int op;
{  
    switch (op) {
      case BLAST_SEND: 		return "SEND";
      case BLAST_RETRANSMIT:	return "REXMIT";
      case BLAST_NACK:		return "NACK";
      default:			return "INVALID";
    }
}	


void
blastShowActiveKey(k, message)
    ActiveID *k;
    char *message;
{
    xTrace3(blastp, TR_ALWAYS, "%s active key:  lls = %x  prot = %d",
	   message, k->lls, k->prot);
}    
  

void
blastShowMstate(m, message)
    MSG_STATE *m;
    char *message;
{
    xTrace1(blastp, TR_ALWAYS, "%s mstate:", message);
    xTrace1(blastp, TR_ALWAYS, "   mask = %s", blastShowMask(m->mask));
    xTrace1(blastp, TR_ALWAYS, "oldMask = %s", blastShowMask(m->old_mask));
    blast_phdr(&m->hdr);
}


char *
blastShowMask( m )
    BlastMask	m;
{
    static char buf[2 * BLAST_MAX_FRAGS];
    char 	*b;
    int 	i;

    for ( b=buf, i = BLAST_MAX_FRAGS; i > 0; i--) {
	if (i && ! (i % 4)) {
	    *b++ = ' ';
	}
	*b++ = (BLAST_MASK_IS_BIT_SET(&m, i)) ? '1' : '0';
    }
    *b = 0;
    return buf;
}


