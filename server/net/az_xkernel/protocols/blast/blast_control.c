/*
 * blast_control.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.15 $
 * $Date: 1993/02/01 22:20:13 $
 */

#include "xkernel.h"
#include "blast_internal.h"

int
blastControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    PState	*pstate;
    int 	new_size;
    int		diff;
    
    xTrace1(blastp, TR_EVENTS, "blast_controlprotl, opcode: %d", opcode);
    pstate = (PState *)self->state;
    
    switch (opcode) {
      case BLAST_SETOUTSTANDINGMSGS:
	/* Set the number of outstanding messages to be the integer passed
         * in buf.
         */
	checkLen(len, sizeof(int));
	new_size = *(int *)buf;
	xTrace1(blastp, TR_MORE_EVENTS,
		"set outstanding messages to %d", new_size);
	if (new_size <= 0)
	    return -1;
	diff = new_size - pstate->max_outstanding_messages;
	for (; diff < 0; diff++) 
	    semWait(&pstate->outstanding_messages);
	for (; diff > 0; diff--) 
	    semSignal(&pstate->outstanding_messages);
	pstate->max_outstanding_messages = new_size;
	return 0;
	
      case BLAST_GETOUTSTANDINGMSGS:
	/* Return the current number of allowed outstanding messages */
	checkLen(len, sizeof(int));
	*(int *)buf = pstate->max_outstanding_messages;
	return 0;
	
      default:
        return xControl(xGetDown(self, 0), opcode, buf, len);
    } 
}
  

int
blastControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
    SState	*state;
    
    xTrace1(blastp, TR_EVENTS, "in blast_control with session=%x", s); 
    state = (SState *) s->state;
    switch (opcode) {
	
	/* free storage associated with ticket returned by push */
      case FREERESOURCES:
	checkLen(len, sizeof(int));
	xTrace1(blastp, TR_MORE_EVENTS, "blast killticket called with id %d",
		*(int *)buf);
        return blast_freeSendSeq(state, *(int *)buf);
	
      case GETMYPROTO:
      case GETPEERPROTO:
	checkLen(len, sizeof(long));
	*(long *)buf = state->prot_id;
	return sizeof(long);
	
      case GETOPTPACKET:
	checkLen(len, sizeof(int));
	*(int *)buf = state->fragmentSize;
	return sizeof(int);

      case GETMAXPACKET:
	checkLen(len, sizeof(int));
	*(int *)buf = state->fragmentSize * BLAST_MAX_FRAGS;
	return sizeof(int);

      default:
        return xControl(xGetDown(s, 0), opcode, buf, len);
    }
}


