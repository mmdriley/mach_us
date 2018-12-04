/*
 * blast.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.35 $
 * $Date: 1993/02/01 22:21:00 $
 */

/*
 * Initialization and connection establishment / teardown routines
 */

#include "xkernel.h"
#include "blast.h"
#include "blast_internal.h"


#define checkPart(pxx, sxx, retVal) 					\
    if ( ! (pxx) || partLen(pxx) < 1 ) {				\
        xTrace1(blastp, TR_SOFT_ERROR, "%s -- bad participants", sxx); 	\
	return retVal;							\
    }


#ifdef __STDC__

static xkern_return_t	blastClose( XObj );
static void	sessnGetProc( XObj );
static void	protGetProc( XObj );

#else

static xkern_return_t	blastClose();
static void	sessnGetProc();
static void	protGetProc();

#endif __STDC__


int 		traceblastp=0;
BlastMask	blastFullMask[BLAST_MAX_FRAGS + 1];


void
blast_init(self)
    XObj self;
{
    Part 	part;
    PState 	*pstate;
    int 	i;
    XObj	llp;
    
    xTrace0(blastp, TR_GROSS_EVENTS, "BLAST init");
    xAssert(xIsProtocol(self));
    
    if ((llp = xGetDown(self,0)) == ERR_XOBJ) {
	xTrace0(blastp, TR_ERRORS, "blast couldn't access lower protocol");
	return;
    }
    protGetProc(self);
    pstate = (PState *)(self->state = xMalloc(sizeof(PState)));
    pstate->max_seq = 0;
    pstate->active_map = mapCreate(BLAST_ACTIVE_MAP_SZ, sizeof(ActiveID));
    pstate->passive_map = mapCreate(BLAST_PASSIVE_MAP_SZ, sizeof(PassiveID));
    pstate->mstateStack = stackCreate(BLAST_MSTATE_STACK_SZ);
    semInit(&pstate->outstanding_messages, OUTSTANDING_MESSAGES);
    semInit(&pstate->createSem, 1);
    pstate->max_outstanding_messages = OUTSTANDING_MESSAGES;
    for (i=1; i <= BLAST_MAX_FRAGS; i++) {
	BLAST_FULL_MASK(blastFullMask[i], i);
    }
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    xOpenEnable(self, self, llp, &part);
}


static long
getRelProtNum( hlp, llp, s )
    XObj	hlp, llp;
    char	*s;
{
    long	n;

    n = relProtNum(hlp, llp);
    if ( n == -1 ) {
	xTrace3(blastp, TR_SOFT_ERROR,
	       "%s: couldn't get prot num of %s relative to %s",
	       s, hlp->name, llp->name);
	return -1;
    }
    return n;
}


static XObj
blastOpen(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    XObj	s;
    XObj	lls;
    ActiveID 	active_id;
    PState 	*pstate;
    
    xTrace0(blastp, TR_MAJOR_EVENTS, "BLAST open");
    checkPart(p, "blastOpen", ERR_XOBJ);
    pstate = (PState *)self->state;
    semWait(&pstate->createSem);
    if ( (active_id.prot = getRelProtNum(hlpType, self, "blastOpen")) == -1 ) {
	return ERR_XOBJ;
    }
    if ( (lls = xOpen(self, self, xGetDown(self, 0), p)) == ERR_XOBJ ) {
	xTrace0(blastp, TR_MAJOR_EVENTS, "blast open: could not open lls");
	s = ERR_XOBJ;
    } else {
	xTrace0(blastp, TR_MAJOR_EVENTS, "blast_open successfully opened lls");
	active_id.lls = lls;
	xIfTrace(blastp, TR_MORE_EVENTS) {
	    blastShowActiveKey(&active_id, "blast_open");
	}
	/*
	 * is there an existing session?
	 */
	if ( mapResolve(pstate->active_map, &active_id, &s) == XK_FAILURE ) {
	    xTrace0(blastp, TR_MAJOR_EVENTS,
		    "blast_open creating new session");
	    s = blastCreateSessn(self, hlpRcv, hlpType, &active_id);
	    if ( s == ERR_XOBJ ) {
		xClose(lls);
	    }
	} else {
	    xTrace0(blastp, TR_MAJOR_EVENTS, "blast_open: session exists");
	    xClose(lls);
	}
    }
    semSignal(&pstate->createSem);
    return s;
}    


XObj
blastCreateSessn( self, hlpRcv, hlpType, key )
    XObj self;
    XObj hlpRcv, hlpType;
    ActiveID *key;
{
    PState	*pstate;
    SState 	*state;
    BLAST_HDR	*hdr;
    XObj	s;

    pstate = (PState *)self->state;
    state = (SState *) xMalloc(sizeof(SState));
    bzero((char *)state, sizeof(SState));
    state->prot_id = key->prot;
    state->send_map = mapCreate(101, sizeof(BlastSeq));
    state->rec_map = mapCreate(101, sizeof(BlastSeq));
    /*
     * fill in header for messages that don't require fragmentation
     */
    hdr = &state->short_hdr;
    hdr->op = BLAST_SEND;
    hdr->prot_id = key->prot;
    hdr->seq = 0; /* protocol guarantees that 0 can never be a real seq */
    BLAST_MASK_CLEAR(hdr->mask);
    hdr->num_frag = 0;
    /*
     * Determine the maximum size of datagrams which this session
     * supports.  
     */
    if (xControl(key->lls, GETOPTPACKET, (char *)&state->fragmentSize,
		 sizeof(state->fragmentSize)) < 0) {
	xTrace0(blastp, TR_ERRORS,
		"Blast could not get opt packet size from lls");
	if (xControl(key->lls, GETMAXPACKET, (char *)&state->fragmentSize,
		     sizeof(state->fragmentSize)) < 0) {
	    xTrace0(blastp, TR_ERRORS,
		    "Blast could not get max packet size from lls");
	    xFree((char *)state);
	    return ERR_XOBJ;
	}
    }
    xTrace1(blastp, TR_MAJOR_EVENTS,
	    "fragment size (from llp): %d", state->fragmentSize);
    xTrace1(blastp, TR_MAJOR_EVENTS, "blast hdr len: %d", BLASTHLEN);
    state->fragmentSize -= BLASTHLEN;
    xTrace2(blastp, TR_MAJOR_EVENTS,
	    "Blast fragmenting into packets of size %d, max dgm: %d",
	    state->fragmentSize, state->fragmentSize * BLAST_MAX_FRAGS);
    /*
     * create session and bind to address
     */
    s = xCreateSessn(sessnGetProc, hlpRcv, hlpType, self, 1, &key->lls);
    s->state = (VOID *)state;
    state->self = s;
    s->binding = (Bind)mapBind(pstate->active_map, (char *)key, s);
    /*
     * just to be paranoid
     */
    if (s->binding == ERR_BIND) {
	xTrace0(blastp, TR_ERRORS, "blast_open: could not bind session");
	blastClose(s);
	return ERR_XOBJ;
    }
    xTrace1(blastp, TR_MAJOR_EVENTS, "blast_open returns %x", s);
    return s;
}


static xkern_return_t
blastOpenEnable(self, hlpRcv, hlpType, p)
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PassiveID 	key;
    PState 	*pstate;
    
    xTrace0(blastp, TR_MAJOR_EVENTS, "BLAST open enable");
    checkPart(p, "blastOpenEnable", XK_FAILURE);
    pstate = (PState *)self->state;
    if ( (key = getRelProtNum(hlpType, self)) == -1 ) {
	return XK_FAILURE;
    }
    xTrace1(blastp, TR_MAJOR_EVENTS,
	    "blast_openenable: ext_id.prot_id = %d", key);
    return defaultOpenEnable(pstate->passive_map, hlpRcv, hlpType, &key);
}


static xkern_return_t
blastOpenDisable( self, hlpRcv, hlpType, p )
    XObj self, hlpRcv, hlpType;
    Part *p;
{
    PassiveID   key;
    PState      *pstate;
    
    xTrace0(blastp, TR_MAJOR_EVENTS, "BLAST open disable");
    checkPart(p, "blastOpenDisable", XK_FAILURE);
    pstate = (PState *)self->state;
    if ( (key = getRelProtNum(hlpType, self)) == -1 ) {
	return XK_FAILURE;
    }
    return defaultOpenDisable(pstate->passive_map, hlpRcv, hlpType, &key);
}


/* 
 * Decrement the internal reference count and call blastClose if appropriate
 */
void
blastDecIrc( s )
    XObj	s;
{
    SState	*ss = (SState *)s->state;

    xAssert(ss->ircnt > 0);
    if ( --ss->ircnt == 0 && s->rcnt == 0 ) {
	blastClose(s);
    }
}


/*
 * blastClose: blast does no caching of sessions -- when a session's
 * reference count drops to zero it is destroyed.
 */
static xkern_return_t
blastClose(s)
    XObj s;
{
    SState *ss;
    PState	*ps;
    XObj	lls;
    
    xTrace1(blastp, TR_MAJOR_EVENTS, "blast_close of session %x", s);

    if (!s) return(XK_SUCCESS); 
    ss = (SState *)s->state;
    ps = (PState *)s->myprotl->state;

    xAssert(s->rcnt == 0);
    xTrace1(blastp, TR_EVENTS, "blast_close ircnt == %d", ss->ircnt);
    if ( ss->ircnt ) {
	return XK_SUCCESS;
    }
    if (s->binding) {
	mapRemoveBinding(ps->active_map,s->binding);
    }
    /* free blast state */
    if (ss->send_map) {
	blast_mapFlush(ss->send_map);
	mapClose(ss->send_map);
    }
    if (ss->rec_map) {
	blast_mapFlush(ss->rec_map);
	mapClose(ss->rec_map);
    }
    if ((lls = xGetDown(s, 0)) != ERR_XOBJ) {
	xClose(lls);
    }
    xDestroy(s);
    return XK_SUCCESS;
}

    	
MSG_STATE *
blastNewMstate(s)
    XObj s;
{
    PState	*ps;
    MSG_STATE	*mstate;
    
    ps = (PState *)xMyProtl(s)->state;
    mstate = (MSG_STATE *)stackPop(ps->mstateStack);
    if ( mstate == 0 ) {
	xTrace0(blastp, TR_MORE_EVENTS, "blast_pop: new_state created ");
	mstate = X_NEW(MSG_STATE);
    }
    bzero((char *)mstate, sizeof(MSG_STATE));
    /* 
     * Add a reference count for this message state
     */
    ((SState *)s->state)->ircnt++;
    xTrace1(blastp, TR_MORE_EVENTS, "blast receive returns %x", mstate);
    return mstate;
}


static void
sessnGetProc(s)
    XObj s;
{
    s->push = blastPush;
    s->control = blastControlSessn;
    s->close = blastClose;
    s->pop = blastPop;
}


static void
protGetProc(p)
    XObj p;
{
    p->control = blastControlProtl;
    p->open = blastOpen;
    p->openenable = blastOpenEnable;
    p->demux = blastDemux;
    p->opendisable = blastOpenDisable;
}

