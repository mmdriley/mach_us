/*
 * chan_debug.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 22:28:59 $
 */

#include "xkernel.h"
#include "chan_internal.h"
#include "bidctl.h"


/*
 * chanStatusStr - Print sequence status
 */
char *
chanStatusStr(stat)
    SEQ_STAT stat;
{
    switch (stat) {
      case old:		return "old";
      case current:	return "current";
      case new: 	return "new";
      default: 		return "UNKNOWN!!";
    }
}


/*
 * chanStateStr - Print state of server/client
 */
char *
chanStateStr(state)
    int state;
{
    switch (state) {
      case SVC_EXECUTE:	return "SVC_EXECUTE";
      case SVC_WAIT: 	return "SVC_WAIT";
      case SVC_IDLE: 	return "SVC_IDLE";
      case CLNT_FREE: 	return "CLNT_FREE";
      case CLNT_WAIT: 	return "CLNT_WAIT";
      case DISABLED: 	return "DISABLED";
      default:		return "UNKNOWN!!";
    }
}


#ifdef XK_DEBUG

static char *
flagStr( f )
    u_int	f;
{
    static char	s[80];

    s[0] = 0;
    if ( f & FROM_CLIENT ) {
	strcat(s, "FROM_CLIENT ");
    } else {
	strcat(s, "FROM_SERVER ");
    }
    if ( f & USER_MSG ) {
	strcat(s, "USER_MSG ");
    } 
    if ( f & ACK_REQUESTED ) {
	strcat(s, "ACK_REQUESTED ");
    }
    if ( f & NEGATIVE_ACK ) {
	strcat(s, "NAK ");
    }
    return s;
}

#endif XK_DEBUG


/*
 * pChanHdr
 */
void
pChanHdr(hdr)
    CHAN_HDR *hdr;
{
    xTrace1(chanp, TR_ALWAYS, "\t| CHAN header for channel %d:", hdr->chan);
    xTrace1(chanp, TR_ALWAYS, "\t|      flags:    %s", flagStr(hdr->flags));
    xTrace1(chanp, TR_ALWAYS, "\t|      seq: %d", hdr->seq);
    xTrace1(chanp, TR_ALWAYS, "\t|      prot_id: %d", hdr->prot_id);
    xTrace1(chanp, TR_ALWAYS, "\t|      len: %d", hdr->len);
}


void
chanDispKey( key )
    ActiveID	*key;
{
    xTrace3(chanp, TR_ALWAYS, "chan == %d, lls = %x, prot = %d",
	    key->chan, key->lls, key->prot_id);
}



