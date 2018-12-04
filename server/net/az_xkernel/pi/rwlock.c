/*     
 * $RCSfile: rwlock.c,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 23:58:37 $
 */

/* 
 * Implementation of a readers-writers lock.
 *
 * This implementation favors writers:
 *	In a choice between granting the lock to queued readers and
 *	queued writers, the lock will always be granted to a writer.
 *
 *	When a writer is queued and the lock is held by readers, no
 *	new readers will be allowed in before the queued writer.
 *	
 * This implementation depends on running within the x-kernel monitor
 * (i.e., the x-kernel master lock must be held to execute any lock
 * operations.)
 */

#include "assert.h"
#include "process.h"
#include "xk_debug.h"
#include "x_libc.h"
#include "rwlock.h"

int	tracerwlock;


/* 
 * flags field 
 */
#define	DESTROYED 1


static void
destroy( l )
    ReadWriteLock	*l;
{
    if ( l->destroyFunc ) {
	l->destroyFunc(l, l->destFuncArg);
    }
}


void
rwLockDump( l )
    ReadWriteLock	*l;
{
    xTrace2(rwlock, TR_ALWAYS, "readers: %d active, %d queued",
	    l->activeReaders, l->queuedReaders);
    xTrace2(rwlock, TR_ALWAYS, "writers: %d active, %d queued",
	    l->activeWriter, l->queuedWriters);
}


xkern_return_t
readerLock( l )
	ReadWriteLock *l;
{
	xAssert( l->activeWriter <= 1 );
	xAssert( l->queuedReaders >= 0 && l->activeReaders >= 0);
	xAssert( l->queuedWriters >= 0 && l->activeWriter >= 0);
	if ( l->flags & DESTROYED ) {
	    return XK_FAILURE;
	}
	if ( l->activeWriter || l->queuedWriters > 0 ) {
		l->queuedReaders++;
		semWait(&l->readersSem);
		if ( l->flags & DESTROYED ) {
		    readerUnlock(l);
		    return XK_FAILURE;
		}
	} else {
		l->activeReaders++;
	}
	xAssert( l->activeWriter == 0 && l->activeReaders > 0 );
	return XK_SUCCESS;
}



void
readerUnlock( l )
	ReadWriteLock *l;
{
	xAssert( l->activeReaders > 0 && l->activeWriter == 0 );
	xAssert( l->queuedReaders >= 0 && l->activeReaders > 0);
	xAssert( l->queuedWriters >= 0 && l->activeWriter == 0);

	l->activeReaders--;
	if ( l->activeReaders == 0 ) {
		if ( l->queuedWriters > 0 ) {
			l->activeWriter = 1;
			l->queuedWriters--;
			semSignal(&l->writersSem);
		} else if ( l->flags & DESTROYED ) {
		       /* 
		        * If there are no queued writers, no readers
			* will have been queuing while this reader
			* held the lock.
		        */
		    	xAssert( l->queuedReaders == 0 );
		    	destroy(l);
		}
	}
}



xkern_return_t
writerLock( l )
	ReadWriteLock *l;
{
	xAssert( l->activeWriter <= 1 );
	xAssert( l->queuedReaders >= 0 && l->activeReaders >= 0);
	xAssert( l->queuedWriters >= 0 && l->activeWriter >= 0);
	if ( l->flags & DESTROYED ) {
	    return XK_FAILURE;
	}
	if ( l->activeReaders || l->activeWriter ) {
		l->queuedWriters++;
		semWait(&l->writersSem);
		if ( l->flags & DESTROYED ) {
		    writerUnlock(l);
		    return XK_FAILURE;
		}
	} else {
		l->activeWriter = 1;
	}
	xAssert( l->activeReaders == 0 && l->activeWriter == 1 );
	return XK_SUCCESS;
}


void
writerUnlock( l )
	ReadWriteLock *l;
{
	xAssert( l->activeReaders == 0 && l->activeWriter == 1 );
	xAssert( l->queuedReaders >= 0 );
	xAssert( l->queuedWriters >= 0 );

	l->activeWriter--;
	if ( l->queuedWriters > 0 ) {
		l->activeWriter++;
		l->queuedWriters--;
		semSignal(&l->writersSem);
	} else {
	    	if ( l->queuedReaders == 0 && l->flags & DESTROYED ) {
		    	destroy(l);
		} else {
		    	/*
		     	 * could also just signal one and have that
		     	 * thread signal another reader if there are
		    	 * no writers in the queue (gives even further
		    	 * preference to writers)		
		    	 */
		    	while ( l->queuedReaders ) {
				l->queuedReaders--;
				l->activeReaders++;
				semSignal(&l->readersSem);
		    	}
		} 
	} 
}


void
rwLockInit( l )
    ReadWriteLock	*l;
{
    bzero((char *)l, sizeof(ReadWriteLock));
    semInit(&l->readersSem, 0);
    semInit(&l->writersSem, 0);
}


void
rwLockDestroy( l, f, arg )
    ReadWriteLock	*l;
    RwlDestroyFunc	f;
    VOID		*arg;
{
    xTrace0(rwlock, TR_EVENTS, "rwLockDestroy called");
    l->destroyFunc = f;
    l->destFuncArg = arg;
    l->flags |= DESTROYED;
    if ( l->queuedReaders == 0 && l->queuedWriters == 0 &&
	 l->activeReaders == 0 && l->activeWriter == 0 ) {
	destroy(l);
    } 
}
