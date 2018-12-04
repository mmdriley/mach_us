/*     
 * ip_frag.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:23:01 $
 */

/*
 * REASSEMBLY ROUTINES
 */



#include "xkernel.h"
#include "ip.h"
#include "ip_i.h"


#ifdef __STDC__

static void		displayFragTable( Fragtable * );
static void 		hole_create(Fraginfo *, int, int);

#else

static void		displayFragTable();
static void 		hole_create();

#endif __STDC__


xkern_return_t
ipReassemble(s, down_s, dg, hdr)
    XObj s;
    XObj down_s;
    Msg *dg;
    IPheader *hdr;
{
    PState *pstate;
    FragId fragid;
    Fragtable *fragtable;
    Fraginfo *fraginfo;
    Hole_ent *hole;
    u_short offset, len;
    xkern_return_t retVal;
    
    pstate = (PState *) s->myprotl->state;
    offset = FRAGOFFSET(hdr->frag)*8;
    len = hdr->dlen - GET_HLEN(hdr) * 4;
    xTrace3(ipp,4,"IP reassemble, seq=%d, off=%d, len=%d",
	    hdr->ident, offset, len);
    
    fragid.source = hdr->source;
    fragid.dest = hdr->dest;	/* might be multiple IP addresses for me! */
    fragid.prot = hdr->prot;
    fragid.pad  = 0;
    fragid.seqid = hdr->ident;
    
    if ( mapResolve(pstate->fragMap, &fragid, &fragtable) == XK_FAILURE ) {
	xTrace0(ipp,5,"IP reassemble, allocating new Fragtable");
	fragtable = (Fragtable *) xMalloc(sizeof(Fragtable));
	fragtable->binding = mapBind(pstate->fragMap, (char *)&fragid,
				     (int)fragtable );
	fragtable->listhead =  (Fraginfo *) xMalloc(sizeof(Fraginfo));
	fragtable->nholes = 1;
	fragtable->gcMark = FALSE;
	fraginfo = (Fraginfo *) xMalloc(sizeof(Fraginfo));
	fragtable->listhead->next = fragtable->listhead->prev = fraginfo;
	fraginfo->type = RHOLE;
	fraginfo->data.hole = hole = (Hole_ent *) xMalloc(sizeof(Hole_ent));
	fraginfo->next = fraginfo->prev = fragtable->listhead;
	hole->first = 0;
	hole->last = INFINITE_OFFSET;
    } else {
	xTrace1(ipp,5,"IP reassemble - found fragtable == %x",fragtable);
	fragtable->gcMark = FALSE;
    }
    
    xIfTrace(ipp, 7) {
	xTrace0(ipp, 7, "frag table before adding");
	displayFragTable(fragtable);
    }
    for( fraginfo = fragtable->listhead->next; fraginfo != fragtable->listhead;
	fraginfo = fraginfo->next ) {
	if ( fraginfo->type == RFRAG )
	  continue;
	hole = fraginfo->data.hole;
	if ( (offset < hole->last) && ((offset + len) > hole->first) ) {
	    xTrace0(ipp, 5, "IP reassemble, found hole for datagram");
	    xTrace2(ipp, 6, "hole->first: %d  hole->last: %d",
		    hole->first, hole->last);
	    /* check to see if frag overlaps previously received frags */
	    if ( offset < hole->first ) {
		xTrace0(ipp,5,"Truncating message from left");
		msgPopDiscard(dg, hole->first - offset);
		offset = hole->first;
	    }
	    if ( (offset + len) > hole->last ) {
		xTrace0(ipp,5,"Truncating message from right");
		/* msg_truncateright(dg,(hole->last - offset)); */
		msgTruncate(dg, hole->last - offset); 
		len = hole->last - offset;
	    }
	    /* now check to see if new hole(s) need to be made */
	    if ( ((offset + len) < hole->last) &&
		 (hdr->frag & MOREFRAGMENTS) ) {
		/* This hole is not created if this is the last fragment */
		xTrace0(ipp, 6, "Creating new hole above");
		hole_create(fraginfo,(offset+len),hole->last);
		fragtable->nholes++;
	    }
	    if ( offset > hole->first ) {
		xTrace0(ipp, 6, "Creating new hole below");
		hole_create(fraginfo->prev,hole->first,(offset));
		fragtable->nholes++;
	    }
	    /* update this fraginfo structure to be an RFRAG */
	    xFree((char *)fraginfo->data.hole);
	    fragtable->nholes--;
	    fraginfo->type = RFRAG;
	    /* fraginfo->data.frag = dg; */
	    msgConstructCopy(&fraginfo->data.frag, dg); 
	    break;
	} /* if found a hole */
    } /* for loop */
    xIfTrace(ipp, 7) {
	xTrace0(ipp, 7, "frag table after adding");
	displayFragTable(fragtable);
    }
    
    /* check to see if we're done */
    if ( fragtable->nholes == 0 ) {
	Msg fullMsg;
	
	Fraginfo *list;
	xTrace0(ipp,5,"IP reassemble : done collecting frags for datagram");
	/* msg_clear(fullMsg); */
	msgConstructEmpty(&fullMsg);
	for( list = fragtable->listhead->next;
	    list != fragtable->listhead; list = list->next ) {
	    if (list->type == RHOLE) {
		xTrace0(ipp, 1, "Hole in reassembly process...\n");
		continue;
	    }
	    msgJoin(&fullMsg, &fullMsg, &list->data.frag);
	}
	if (mapRemoveBinding(pstate->fragMap, fragtable->binding)
	    						== XK_FAILURE ) {
	    xTrace1(ipp, 2, "Error removing entry for seq %d", fragid.seqid);
	} else {
	    xTrace1(ipp, 5, "Successfully removed seq %d", fragid.seqid);
	}
	ipFreeFragtable(fragtable);
	xTrace1(ipp,4,"IP reassemble popping up message of length %d",
		msgLen(&fullMsg));
	retVal = ipMsgComplete(s, down_s, &fullMsg, hdr);
	msgDestroy(&fullMsg);
    } else {
	retVal = XK_SUCCESS;
    }
    return retVal;
}


/* hole_create :
 *   insert a new hole frag after the given list with the given 
 *   first and last hole values
 */
static void
hole_create(fraglist, first, last)
    Fraginfo *fraglist;
    u_short first;
    u_short last;
{
    Fraginfo *newfrag;
    Hole_ent *newhole;
    
    xTrace2(ipp,5,"IP hole_create : creating new hole from %d to %d",
	    first,last);
    newfrag = (Fraginfo *) xMalloc(sizeof(Fraginfo));
    newfrag->type = RHOLE;
    newfrag->data.hole = newhole = (Hole_ent *) xMalloc(sizeof(Hole_ent));
    newfrag->prev = fraglist;
    newfrag->next = fraglist->next;
    fraglist->next->prev = newfrag;
    fraglist->next = newfrag;
    
    newhole->first = first;
    newhole->last = last;
}


void
ipFreeFragtable(fragtable)
    Fragtable *fragtable;
{
    Fraginfo *list, *next;
    
    for( list = fragtable->listhead->next;
	list != fragtable->listhead; list = next ) {
	next = list->next;
	if (list->type == RFRAG) {
	    msgDestroy(&list->data.frag);
	}
	xFree((char *)list);
    }
    xFree((char *)fragtable->listhead);
    xFree((char *)fragtable);
}



static void
displayFragTable(t)
    Fragtable *t;
{
    Fraginfo 	*info;

    xTrace2(ipp, 7, "Table has %d hole%c", t->nholes,
	    t->nholes == 1 ? ' ' : 's');
    for ( info = t->listhead->next; info != t->listhead; info = info->next ) {
	if ( info->type == RHOLE ) {
	    xTrace2(ipp, 7, "hole  first == %d  last == %d",
		    info->data.hole->first, info->data.hole->last);
	} else {
	    xTrace0(ipp, 7, "frag");
	}
    } 
}
