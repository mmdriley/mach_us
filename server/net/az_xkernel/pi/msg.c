/*
 * msg.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.60 $
 * $Date: 1993/02/03 04:11:15 $
 */

/*
 *  The msg tool package supports tree structured message management.
 *
 *  The algorithms were developed by Peter Druschel in C++
 *
 */


#include "assert.h"
#include "xk_debug.h"
#include "platform.h"
#include "x_util.h"
#include "x_libc.h"

/* don't change the order of these includes; MSG_NEW_ALG is defined in
   the first one and needed by the second
*/
#include "msg.h"
#include "msg_internal.h"


#define ROUND4(len)  ((len + 3) & ~3)

int	tracemsg = 0;

#if defined(__STDC__) || defined(__GNUC__)

static void	makeContiguous( Msg * );
void msgDestroy( Msg * );
void msgJoin( Msg *, Msg *, Msg * );

#else

static void	makeContiguous();
void msgDestroy();
void msgJoin();

#endif

#ifdef __STDC__
#define __CONCAT(a,b) a##b
#else
#define __CONCAT(a,b) a/**/b
#endif


#ifdef MSG_STATISTICS
#  define RECORD_USAGE(prefix) { 		\
  __CONCAT(prefix,_used)++; 				\
  if ( __CONCAT(prefix,_used) > __CONCAT(prefix,_hiwat) ) {	\
      __CONCAT(prefix,_hiwat) = __CONCAT(prefix,_used);	\
  }						\
  msgSpace_used += __CONCAT(prefix,size); 		\
  if ( msgSpace_used > msgSpace_hiwat ) {	\
      msgSpace_hiwat = msgSpace_used;		\
  }						\
}

#  define RECORD_RELEASE(prefix) { 		\
  __CONCAT(prefix,_used)--; 				\
  msgSpace_used -= __CONCAT(prefix,size); 		\
}						\

#else ! MSG_STATISTICS

#define RECORD_USAGE(prefix)
#define RECORD_RELEASE(prefix)

#endif MSG_STATISTICS


/* Msg operations */

/* initializer; must be called before any Msg instances are created 	*/
/* returns 1 after successful intialization				*/

bool msgInit()
{
  xTrace0(msg, TR_FULL_TRACE, "msgInit");
#ifdef MSGCACHEING
  int i;

  mWindow = (char *)vmAllocate(MNodePairVSpace + MNodeBufVSpace +
			       MNodePageVSpace);
  if (!mWindow) return 0;

  mpr_freeStore = (MNodePair*)mWindow;
  mpr_numNodes = MNodePairVSpace / MNodePairSize;
  mpr_lastFreeNode = 0;
  for (i = 0; i < mpr_numNodes; i++) mpr_freeStore[i].type = t_MNodePair;

  mbf_freeStore = (MNodeBuf*)(mWindow + MNodePairVSpace);
  mbf_numNodes = MNodeBufVSpace / MNodeBufSize;
  mbf_lastFreeNode = 0;
  for (i = 0; i < mbf_numNodes; i++) mbf_freeStore[i].type = t_MNodeBuf;

  mpg_freeStore = (MNodePage*)(mWindow + MNodePairVSpace + MNodeBufVSpace);
  mpg_numNodes = MNodePageVSpace / MNodePageSize;
  mpg_lastFreeNode = 0;
  for (i = 0; i < mpg_numNodes; i++) mpg_freeStore[i].type = t_MNodePage;
#endif

#ifdef MSG_NEW_ALG
  /* init the dummy stack for empty messages */
  dummyStack.type = t_MNodeDummy;
  dummyStack.refCnt = 1;
  dummyStack.b.leaf.size = 0;
  dummyStack.b.leaf.data = NULL;
#endif

  return TRUE;
}

/*
 * Utility functions
 */

static unsigned atomicInc(where)
    unsigned long *where;
{
  return (*where)++;
}

static unsigned atomicDec(where)
    unsigned long *where;
{
  xTrace1(msg, TR_FULL_TRACE, "atomicDec %x", where);
  xTrace1(msg, TR_FULL_TRACE, "atomicDec %x", *where);
  return (*where)--;
}

static void incRef(this)
    MNode *this;
{
  xAssert(this != (MNode *)0 && this != (MNode *)-1);
  xAssert(this->type != t_MNodeJunk);
  xTrace2(msg, TR_MAJOR_EVENTS, "incRef %x refCnt %d", this, this->refCnt);
 (void) atomicInc(&this->refCnt);
}

#ifdef MSGCACHEING
#define FREEONE(prefix, this) \
  __CONCAT(prefix,_lastFreeNode) = (this) - __CONCAT(prefix,_freeStore);
#else
#define FREEONE(prefix, this) \
  xTrace1(msg, TR_MAJOR_EVENTS, "FREEONE dis= %x", this); \
  xFree((char *)this); \
  RECORD_RELEASE(prefix); \
  xTrace0(msg, TR_FULL_TRACE, "FREEONE done"); 
#endif


/* 
 * decRef  - decrement reference count and free message if necessary
 * 
 * If the reference count reaches 0, the node is freed by the following 
 *  three cases:
 * 1. Recursively follow the b.pair structure and free the top node
 * 2. Free the page
 * 3. Use a node-specific function to free the b.buf.data structure and
 *    free the node structure
 */
static void decRef(this)
    MNode *this;
{
  xTrace1(msg, TR_FULL_TRACE, "decRef %x", this);
  xAssert(this != (MNode *)0 && this != (MNode *)-1 && this->refCnt >= 1);
  xTrace5(msg, TR_MAJOR_EVENTS, "decRef %x refCnt %d addr %x arg addr %x type %d", this, this->refCnt, decRef, &this, this->type);

  if (atomicDec(&this->refCnt) == 1) {
    switch(this->type) {
    case t_MNodePair:
      decRef(this->b.pair.l.node);
      decRef(this->b.pair.r.node);
      FREEONE(mpr, this);
      break;
    case t_MNodePage:
      if ( this->b.page.size > MNodePageDataSize ) {
	  /* 
	   * Remove allocation for any extra pages attached to this node. 
	   */
	  msgSpace_used -= (this->b.page.size - MNodePageDataSize);
      }
      FREEONE(mpg, this);
      break;
    case t_MNodeUsrPage:
      xTrace2(msg, TR_FULL_TRACE, "decRef t_MNodeUsrPage %x %x", this->b.usrpage.bFree,
	      this->b.usrpage.data);
      this->b.usrpage.bFree(this->b.usrpage.data);
      FREEONE(mup, this);
      break;
    case t_MNodeBuf:
      xTrace2(msg, TR_FULL_TRACE, "decRef t_MNodeBuf %x %x", this->b.buf.bFree,
	      this->b.buf.data);
      this->b.buf.bFree(this->b.buf.data);
      FREEONE(mbf, this);
      break;
#ifdef MSG_NEW_ALG
    case t_MNodeDummy:
      break;
#endif MSG_NEW_ALG
    default:
      printf("xkpanic: decRef wrong type %x %d\n", this, this->type);
      xAssert(0);
    }
  }
  xTrace1(msg, TR_FULL_TRACE, "decRef done %x", this);
}

#define PageMalloc(prefixsize, pages)  xMalloc(prefixsize * pages );

#ifdef MSGCACHEING
#define FINDAFREEONE(prefix, this) { \
  int i, prevVal; \
  xAssert(__CONCAT(prefix,_freeStore) != 0); \
 \
  /* find a free node */ \
  /* this will not stop until a free node is found */ \
  /* thus, there is a potential for deadlock, here */ \
  for (i = __CONCAT(prefix,_lastFreeNode); ; i = (i + 1) % __CONCAT(prefix,_numNodes) { \
    if (__CONCAT(prefix,_freeStore)[i].refCnt == 0) { \
      /* seems to be free, try to grab it */ \
      prevVal = atomicInc(&(__CONCAT(prefix,_freeStore)[i].refCnt)); \
      if (prevVal == 0) { \
	/* we successfully grabbed it */ \
	__CONCAT(prefix,_lastFreeNode) = (i + 1) % __CONCAT(prefix,_numNodes); \
	(this) = & __CONCAT(prefix,_freeStore)[i]; \
	break; \
      } \
      else { \
	/* somebody else grabbed it before us */ \
	(void) atomicDec(&(__CONCAT(prefix,_freeStore)[i].refCnt)); \
      } \
    } \
  } \
}
#else
#define FINDAFREEONE(prefix, this, pages) { \
  this = (MNode *) PageMalloc(__CONCAT(prefix,size), pages); \
  if (this == (MNode *)0) { \
    Kabort("xkpanic: malloc failure in msgtool\n"); \
  } \
  this->refCnt = 1; \
  this->type = __CONCAT(prefix,type); \
  RECORD_USAGE(prefix);	\
  xTrace1(msg, TR_MAJOR_EVENTS, "FINDAFREEONE %x", this); \
}
#endif

/* creator/constructor */
static MNodePair *
newMNodePair(loff, llen, ln, roff, rlen, rn)
    long loff, llen, roff, rlen;
    MNode *ln, *rn;
{
  MNodePair *this;

  xTrace0(msg, TR_FULL_TRACE, "newMNodePair");
  FINDAFREEONE(mpr, this, 1);

  /* refCnt was set by the allocation above */
  this->b.pair.l.offset = loff;
  this->b.pair.l.length = llen;
  this->b.pair.l.node = ln;
  this->b.pair.r.offset = roff;
  this->b.pair.r.length = rlen;
  this->b.pair.r.node = rn;
  return this;
}

#if 0	/* currently unused */

/* creator/constructor */
static MNodeBuf *
newMNodeBuf(len, data, bfree)
    long len;
    char *data;
    void (*bfree)();
{
  MNodeBuf *this;

  xTrace0(msg, TR_FULL_TRACE, "newMNodeBuf");
  xAssert(len >= 0);
  FINDAFREEONE(mbf, this, 1);

  this->b.buf.size = len;
  this->b.buf.data = data;
  this->b.buf.bFree = bfree;
  return this;
}

#endif


/* creator/constructor */
static MNodePage *
newMNodePage(pages)
    int	pages;
{
  MNodePage *this;

  xTrace0(msg, TR_FULL_TRACE, "newMNodePage");

#ifdef MSG_STATISTICS
  if ( pages > 1 ) {
      /* 
       * FINDAFREEONE takes care of statistics for single page
       * allocations, but we need to add the difference for larger
       * nodes. 
       */
      msgSpace_used += (pages - 1) * PageSize;
  }
#endif MSG_STATISTICS

  FINDAFREEONE(mpg, this, pages);

  this->b.page.size = MNodePageDataSize + ((pages-1) * PageSize);
  this->b.page.data = this->b.page.buffer;

  return this;
}

/* creator/constructor */
static MNodePage *
newMNodeUsrPage(freefunc, buffer, size)
     void (* freefunc)();
     char *buffer;
     long size;
{
  MNodePage *this;

  xTrace0(msg, TR_FULL_TRACE, "newMNodeUsrPage");
  FINDAFREEONE(mup, this, 1);

  this->b.usrpage.size = size;
  this->b.usrpage.data = buffer;
  this->b.usrpage.bFree = freefunc;
  return this;
}


/* construct an empty Msg						*/
void msgConstructEmpty(this)
    Msg *this;
{
  xTrace0(msg, TR_FULL_TRACE, "msgConstructEmpty");

#ifdef MSG_NEW_ALG
  this->tree = this->stack = &dummyStack;
  incRef(this->tree);
  this->stackHeadPtr = this->stackTailPtr= this->headPtr = this->tailPtr = NULL;
  this->state.myStack = 0; /* so that push won't write */
  this->tailstate.myLastStack = 0;
  this->state.numNodes = 0;
  this->lastStack = 0;
#else
  this->tree = this->stack = newMNodePage(1);
  this->length = 0;
  this->offset = this->stack->b.leaf.size;
#endif MSG_NEW_ALG
  this->attr = 0;
}


/* construct a Msg with a copy of the content of the buffer buf with */
/* length len; this constructor causes data to be copied!! */
void msgConstructBuffer(this, buf, len)
    Msg *this;
    char *buf;
    long len;
{
  /* round up to nearest multiple of msg PageSize */
  int roundlen = ROUND4(len);

  xTrace0(msg, TR_FULL_TRACE, "msgConstructBuffer");

  xAssert(len > 0 ); /* use msgConstructEmpty instead */
  this->tree = this->stack = newMNodePage(PAGEROUND(roundlen));
#ifdef MSG_NEW_ALG
  this->stackHeadPtr = this->stack->b.leaf.data + this->stack->b.leaf.size - roundlen;
  this->headPtr = this->stack->b.leaf.data;
  this->stackTailPtr = this->tailPtr = this->stackHeadPtr + len;
  this->state.myStack = 1;
  this->state.numNodes = 1;
  this->lastStack = this->stack;
  this->lastStackTailPtr = this->stackTailPtr;
  this->tailstate.myLastStack = 1;
  /* copy in data */
  bcopy(buf, this->stackHeadPtr, len);
#else
  this->length = len;
  this->offset = this->stack->b.leaf.size - roundlen;
  bcopy(buf, this->stack->b.leaf.data + this->offset, len);
#endif MSG_NEW_ALG
  this->attr = 0;
}

/* construct a Msg that is a copy of another Msg			*/
void msgConstructCopy(this, another)
    Msg *this, *another;
{
  xTrace0(msg, TR_FULL_TRACE, "msgConstructCopy");
  incRef(another->tree);
  this->tree = another->tree;
  this->stack = another->stack;
#ifdef MSG_NEW_ALG
  this->headPtr = another->headPtr;
  this->tailPtr = another->tailPtr;
  this->stackHeadPtr = another->stackHeadPtr;
  this->stackTailPtr = another->stackTailPtr;
  this->state = another->state;
  this->state.myStack = 0;
  this->lastStack = another->lastStack;
  this->lastStackTailPtr = another->lastStackTailPtr;
  this->tailstate.myLastStack = 0;
#else
  this->offset = another->offset;
  this->length = another->length;
#endif MSG_NEW_ALG
  this->attr = another->attr;
}

/* construct a Msg that refers to a contig				*/
void msgConstructContig(this, contig)
    Msg *this;
    MContig *contig;
{
  xTrace0(msg, TR_FULL_TRACE, "msgConstructContig");
  xAssert(0);
  /* This implementation assumed contig was an MNode, not an Mcontig.  menze */
#ifdef MCONTIG
  incRef(contig);
  this->offset = 0;
  this->length = contig->b.leaf.size;
  this->tree = contig;
  this->stack = contig;
  this->attr = 0;
#endif
}

/* constructor... to this Msg an uninitialized contiguous piece of memory
 * of length len and return a pointer to it.
 * Question:  Should this worry about making sure that the left edge of the
 * buffer is long word aligned?
 */
void msgConstructAllocate(this, len, buf)
    Msg *this;
    long len;
    char **buf;
{
  /* round up to nearest multiple of msg PageSize */
  int roundlen = ROUND4(len);

  xTrace0(msg, TR_FULL_TRACE, "msgConstructAllocate");
  xAssert(len > 0);
  this->tree = this->stack = newMNodePage(PAGEROUND(roundlen));
#ifdef MSG_NEW_ALG
  this->stackHeadPtr = this->stack->b.leaf.data + this->stack->b.leaf.size - roundlen;
  this->headPtr = this->stack->b.leaf.data;
  this->stackTailPtr = this->tailPtr = this->stackHeadPtr + len;
  this->state.myStack = 1;
  this->state.numNodes = 1;
  *buf = this->stackHeadPtr;
  this->lastStack = this->tree;
  this->lastStackTailPtr = this->stackTailPtr;
  this->tailstate.myLastStack = 1;
#else
  this->length = len;
  this->offset = this->stack->b.leaf.size - roundlen;
  *buf = this->stack->b.leaf.data + this->offset;
#endif MSG_NEW_ALG
  this->attr = 0;
}


/*
 * msgConstructInplace()
 *
 *  make a data area into a message; the caller's routine is responsible
 *  for eventually freeing the data area, msgDestroy will only
 *  deallocate a small internal node buffer.  The caller must not
 *  directly deallocate the buffer; it must be a side-effect of
 *  msgDestroy.
 *
 */
void
msgConstructInplace(this, stack, length, freefunc)
     Msg  *this;
     char *stack;
     long  length;
     void  (*freefunc)();
{
  xTrace0(msg, TR_FULL_TRACE, "msgConstructInplace");

  xAssert(length > 0 );
  this->tree = this->stack = newMNodeUsrPage(freefunc, stack, length);
  this->headPtr = this->stackHeadPtr = stack;
  this->stackTailPtr = this->tailPtr = this->stackHeadPtr + length;
  this->state.myStack = 1;
  this->state.numNodes = 1;
  this->attr = 0;
  this->lastStack = this->tree;
  this->lastStackTailPtr = this->stackTailPtr;
  this->tailstate.myLastStack = 1;
}

/*
 * msgConstructAppend()
 *
 *  Allocate a buffer and reserve it for appending.
 *  The stack head is at the low address instead of the high.
 *  This will cause an overflow on a msgPush operation, but not
 *  on a msgAppend operation.
 *  Return to the user a pointer to the beginning of the buffer.
 *
 */
void
msgConstructAppend(this, totalsize, bufferptr)
     Msg   *this;
     long   totalsize;
     char **bufferptr;
{
  int roundlen = ROUND4(totalsize);

  xTrace0(msg, TR_FULL_TRACE, "msgConstructAppend");

  xAssert(totalsize > 0 );
  this->tree = this->stack = newMNodePage(PAGEROUND(roundlen));
  this->stackHeadPtr = this->stack->b.leaf.data + this->stack->b.leaf.size - roundlen;
  this->headPtr = this->stack->b.leaf.data;
  this->stackTailPtr = this->tailPtr = this->stackHeadPtr;
  this->state.myStack = 1;
  this->state.numNodes = 1;
  this->attr = 0;
  this->lastStack = this->stack;
  this->lastStackTailPtr = this->stackTailPtr;
  this->tailstate.myLastStack = 1;
  *bufferptr = this->stackTailPtr;
}

/*
 *  msgAppend
 *
 *   copy data to the end of the last stack, if there is room;
 *     otherwise, allocate a new buffer and use msgJoin.
 *   NB the user supplies the size of the new message buffer
 *
 */
void
msgAppend(this, appendfunc, tail, tailLen, arg, newlength)
     Msg	*this;
     MStoreFun   appendfunc;
     VOID	*tail;
     VOID       *arg;
     long	 tailLen, newlength;
{
  char		*where;

  xTrace0(msg, TR_FULL_TRACE, "msgAppend");
  xAssert(tailLen >= 0 );

  if (this->tailstate.myLastStack &&
      this->lastStack &&
      ((this->lastStack == this->stack &&
	this->lastStackTailPtr >= this->stackTailPtr &&
	this->lastStackTailPtr + tailLen < this->lastStack->b.leaf.data + this->lastStack->b.leaf.size)
       ||
       (this->lastStackTailPtr + tailLen < this->lastStack->b.leaf.data + this->lastStack->b.leaf.size))) {
    /* cleanest case - there is room in the last stack */
      where = this->lastStackTailPtr;
      appendfunc(tail, where, tailLen, arg);    
      this->lastStackTailPtr += tailLen;
      this->stackTailPtr = this->lastStackTailPtr;
      this->tailPtr += tailLen;
    }
  else {
    /* this is more work; must get a new stack for the tail */
    Msg		newMsg;

    msgConstructAppend(&newMsg, newlength, &where);
    appendfunc(tail, where, tailLen, arg);    
    newMsg.stackTailPtr = (newMsg.tailPtr += tailLen);
    msgJoin(this, this, &newMsg);
    msgDestroy(&newMsg);
    return;
  }
}


/* destructor								*/
void msgDestroy(this)
    Msg *this;
{
  xTrace1(msg, TR_FULL_TRACE, "msgDestroy arg this: %x", this);
  decRef(this->tree);
  xTrace0(msg, TR_FULL_TRACE, "msgDestroy done");
}

/* assignment								*/
void msgAssign(this, another)
    Msg *this, *another;
{
  xTrace0(msg, TR_FULL_TRACE, "msgAssign");

  incRef(another->tree);
  decRef(this->tree);
  this->tree = another->tree;
  this->stack = another->stack;
#ifdef MSG_NEW_ALG
  this->headPtr = another->headPtr;
  this->tailPtr = another->tailPtr;
  this->stackHeadPtr = another->stackHeadPtr;
  this->stackTailPtr = another->stackTailPtr;
  this->state = another->state;
  this->state.myStack = (this == another) && this->state.myStack;
  this->tailstate.myLastStack = (this == another) && this->tailstate.myLastStack;
#else
  this->offset = another->offset;
  this->length = another->length;
#endif MSG_NEW_ALG
  this->attr = another->attr;
}


/* return the current length of the message				*/
long msgLen(this)
    Msg *this;
{
  xTrace0(msg, TR_FULL_TRACE, "msgLen");
#ifdef MSG_NEW_ALG  
  return this->tailPtr - this->stackHeadPtr;
#else
  return this->length;
#endif MSG_NEW_ALG  
}


/* truncate this Msg to length newLength				*/
void msgTruncate(this, newLength)
     Msg *this;
     long newLength;
{
#ifdef MSG_NEW_ALG  
  long delta;
#endif MSG_NEW_ALG  

  xTrace0(msg, TR_FULL_TRACE, "msgTruncate");
#ifdef MSG_NEW_ALG
  delta = this->tailPtr - this->stackHeadPtr - newLength;
  if (delta > 0)
    this->tailPtr -= delta; 
  if (this->lastStack == this->stack) {
    this->stackTailPtr = this->tailPtr;
    this->lastStackTailPtr = this->tailPtr;
  }
  else
    this->lastStack = 0;
#else
  if (newLength > 0 && this->length > newLength) this->length = newLength;
#endif MSG_NEW_ALG
}


/* remove a chunk of length len from the head of this Msg		*/
/* and assign it to head						*/
void msgChopOff(this, head, len)
     Msg *this, *head;
     long len;
{
#ifdef MSG_NEW_ALG
     long mlen = this->tailPtr - this->stackHeadPtr;
#endif MSG_NEW_ALG

  xTrace0(msg, TR_FULL_TRACE, "msgChopOff");

#ifdef MSG_NEW_ALG
  if (len < 0 || len > mlen) len = mlen;
#else
  if (len < 0 || len > this->length) len = this->length;
#endif MSG_NEW_ALG

  if (this != head) {
    incRef(this->tree);
    decRef(head->tree);
    head->stack = this->stack;
    head->tree = this->tree;

#ifdef MSG_NEW_ALG
    head->headPtr = this->headPtr;
    head->stackHeadPtr = this->stackHeadPtr;
    head->stackTailPtr = this->stackTailPtr;
    head->state = this->state;
    this->state.myStack = 0;  /*  giving stack away */
    this->tailstate.myLastStack = 0;  /*  giving stack away */

    /* pop off the header */
    this->stackHeadPtr += len;
  }
  /* truncate the header */
  head->tailPtr = head->stackHeadPtr + len;
#else
    head->offset = this->offset;
    /* pop the head of this Msg */
    this->length -= len;
    this->offset += len;
  }
  head->length = len;
#endif MSG_NEW_ALG
}

/* assign to this Msg the concatenation of Msg1 and Msg2		*/
void msgJoin(this, msg1, msg2)
  Msg *this, *msg1, *msg2;
{
#ifdef MSG_NEW_ALG  
  long msg1Length = msg1->tailPtr - msg1->stackHeadPtr;
  long msg2Length = msg2->tailPtr - msg2->stackHeadPtr;
  long msg2Offset;
#endif MSG_NEW_ALG

  xTrace0(msg, TR_FULL_TRACE, "msgJoin");
#ifdef MSG_NEW_ALG
  if (msg1Length == 0) {
    /* result is just msg2 */
    msgAssign(this, msg2);
 
  } else if (msg2Length == 0) {
    /* result is just msg1 */
    msgAssign(this, msg1);

  } else {
    /* create a new pair node */
    /* increment refCnts      */
    incRef(msg1->tree);
    incRef(msg2->tree);
    /* cleanup my old binding */
    decRef(this->tree);
    /* create a new pair node */
    msg2Offset = msg2->stackHeadPtr - msg2->headPtr;

    this->tree = newMNodePair(0L, msg1->tailPtr - msg1->headPtr, msg1->tree,
			      msg2Offset, msg2Length, msg2->tree);
    xAssert(this->tree != 0);
    this->headPtr = msg1->headPtr;
    this->tailPtr = msg1->tailPtr + msg2Length;
    this->stack = msg1->stack;
    this->stackHeadPtr = msg1->stackHeadPtr;
    this->stackTailPtr = msg1->stackTailPtr;
    this->state.myStack = (this == msg1) && this->state.myStack;
    this->state.numNodes = msg1->state.numNodes + msg2->state.numNodes + 1;
    this->tailstate.myLastStack = msg2->tailstate.myLastStack;
    this->lastStack = msg2->lastStack;
    this->lastStackTailPtr = msg2->lastStackTailPtr;
#ifdef CONTROL_RESOURCES
   if (this->state.numNodes > numNodesHardLimit) makeContiguous();
#endif CONTROL_RESOURCES
  }
#else
  if (msg1->length == 0) {
    /* result is just msg2 */
    msgAssign(this, msg2);

  } else if (msg2->length == 0) {
    /* result is just msg1 */
    msgAssign(this, msg1);

  } else if (msg1->tree->type != t_MNodePair) {
    /* msg1 is contiguous, create a new pair node */
    /* increment refCnts */
    incRef(msg1->tree);
    incRef(msg2->tree);
    /* cleanup my old binding */
    decRef(this->tree);
    /* create a new pair node */
    this->tree = newMNodePair(0L, msg1->offset + msg1->length, msg1->stack,
			      msg2->offset, msg2->length, msg2->tree);
    xAssert(this->tree != 0);
    this->stack = msg1->stack;
    this->offset = msg1->offset;
    this->length = msg1->length + msg2->length;

  } else {
    /* msg1 is a tree */
    /* we have to be careful here not to free this Msg's old binding too */
    /* early */
    /* note that msg1 or msg2 could be this !! */

    /* create a pair node to combine msg1's data and msg2 */
    MNodePair *leftTree = (MNodePair *)msg1->tree;
    MNode *newPair = newMNodePair(leftTree->b.pair.r.offset,
				  leftTree->b.pair.r.length,
				  leftTree->b.pair.r.node,
				  msg2->offset, msg2->length, msg2->tree);
    MNode *myOldTree;

    xAssert(newPair != 0);
    /* increment refCnts */
    incRef(leftTree->b.pair.r.node);
    incRef(leftTree->b.pair.l.node);
    incRef(msg2->tree);
    /* save my old binding */
    myOldTree = this->tree;

    /* create a pair node to combine msg1's stack with the above node */
    this->tree =
      newMNodePair(0L, leftTree->b.pair.l.length, leftTree->b.pair.l.node,
		   0L, leftTree->b.pair.r.length + msg2->length, newPair);
    xAssert(this->tree != 0);
    this->stack = msg1->stack;
    this->offset = msg1->offset;
    this->length = msg1->length + msg2->length;

    /* now its safe to cleanup my old binding */
    decRef(myOldTree);
  }
#endif MSG_NEW_ALG
}

static char *msgPushOverflow(this, hdrLen)
    Msg *this;
    long hdrLen;
{
#ifdef MSG_NEW_ALG
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;
#endif MSG_NEW_ALG
  long roundlen = ROUND4(hdrLen);

  xTrace0(msg, TR_FULL_TRACE, "msgPushOverflow");

  /* create a new root node and a new stack */
  this->stack = newMNodePage(PAGEROUND(roundlen));
  xAssert(this->stack != 0);
#ifdef MSG_NEW_ALG
  this->tree =  newMNodePair(0L, this->stack->b.leaf.size, this->stack, offset, length, this->tree);
  xAssert(this->tree != 0);
  this->stackTailPtr = this->stack->b.leaf.data + this->stack->b.leaf.size;
  this->stackHeadPtr = this->stackTailPtr - hdrLen;
  this->headPtr = this->stack->b.leaf.data;
  this->tailPtr = this->stackTailPtr + length;
  this->state.myStack = 1;
  this->state.numNodes += 2;
  return this->stackHeadPtr;
#else
  this->tree = newMNodePair(0L, this->stack->b.leaf.size, this->stack,
			    this->offset, this->length, this->tree);
  xAssert(this->tree != 0);
  this->offset = this->stack->b.leaf.size - hdrLen;
  this->length += hdrLen;
  return this->stack->b.leaf.data + this->offset;
#endif MSG_NEW_ALG
}

/* push a header onto this Msg						*/
void msgPush(this, store, hdr, hdrLen, arg)
    Msg *this;
    MStoreFun store;
    VOID *hdr;
    long hdrLen;
    VOID *arg;
{
  char *where;

  xTrace0(msg, TR_FULL_TRACE, "msgPush");
#ifdef MSG_NEW_ALG
  if (this->state.myStack &&
      this->stackHeadPtr <= this->stackTailPtr &&
      this->stackHeadPtr - this->stack->b.leaf.data >= hdrLen) {
    this->stackHeadPtr -= hdrLen;
    where = this->stackHeadPtr;
#else
  if (
      /* not overflowing */
      this->offset >= hdrLen &&
      /* within stack */
      this->offset <= this->stack->b.leaf.size &&
      /* and stack is not shared */
      (this->tree->refCnt + this->stack->refCnt) == 2) {
    this->offset -= hdrLen;
    this->length += hdrLen;
    where = this->stack->b.leaf.data + this->offset;
#endif MSG_NEW_ALG
  } else {
    /* need a new stack */
    where = msgPushOverflow(this, hdrLen);
  }
    store(hdr, where, hdrLen, arg);
}


/*
 * helper function called during foreach()
 * to copy the stack in topUnderflow()
 */
struct arg {
  char *buf;
  long size;
};
static bool
s_copy(buf, len, arg)
    char *buf;
    long len;
    VOID *arg;
{
  struct arg *a = (struct arg *)arg;
  long chunk_size;
  int chunki;

  xTrace0(msg, TR_FULL_TRACE, "s_copy");
  chunk_size = (len < a->size) ? len : a->size;
  /* bcopy takes an int, so be careful */
  chunki = chunk_size;
  xAssert( (long) chunki == chunk_size);
  bcopy(buf, a->buf, chunki);
  a->buf += chunk_size;
  a->size -= chunk_size;
  return (a->size != 0);
}


#ifdef MSG_NEW_ALG
struct tu_farg {
  MNodeLeaf *leaf;
  long off;
  long len;
};

static bool
tu_leaf(leaf, off, len, arg)
    MNodeLeaf* leaf;
    long off, len;
    VOID *arg;
{
  struct tu_farg *a = (struct tu_farg *)arg;

  xTrace0(msg, TR_FULL_TRACE, "tu_leaf");
  a->leaf = leaf;
  a->off = off;
  a->len = len;
  return FALSE;
}
#endif MSG_NEW_ALG

/* 
 * msgTopUnderflow - restructure a message with short initial node
 *
 * returns a pointer to the beginning of the data
 *
 * When the first node of a message has too little data to satisfy a
 * msgPop request, the message is restructured to make the requested
 * data reside in one stack.
 *
 * This is an inefficient operation that should be rarely called.
 */

static char *msgTopUnderflow(this, hdrLen)
    Msg *this;
    long hdrLen;
{
  struct arg c_args;
  MNodePage *newStack;
  int roundlen = ROUND4(hdrLen);

  xTrace0(msg, TR_FULL_TRACE, "msgTopUnderflow");
  xTrace0(msg, TR_SOFT_ERROR, "msgTopUnderflow");

#ifdef MSG_NEW_ALG
  if (this->stackHeadPtr == this->stackTailPtr) {
    /* we are right at the boundary of the current stack */
    /* find the leftmost leaf node just to the right of the stack */
#ifdef TU_UNFOLD
    MNodePair *newNode = (MNodePair*)(this->tree);
    long newOff = this->stackHeadPtr - this->headPtr;
    long newLen;

    while (this->newNode->type == t_MNodePair) {
      if (newOff < newNode->l.length) {
	/* explore the left subtree */
	newOff += newNode->l.offset;
	newLen = newNode->l.length;
	newNode = (MNodePair*)newNode->l.node;
      }
      else {
	/* explore the right subtree */
	newOff = newOff - newNode->l.length + newNode->r.offset;
	newLen = newNode->r.length;
	newNode = (MNodePair*)newNode->r.node;
      }
    }
    /* found a leaf node */
    MNodeLeaf *newLeaf = (MNodeLeaf*)newNode;
    newLen -= newOff;

#else
    /* find the new stack using forEach */

    long newOff;
    long newLen;
    MNodeLeaf *newLeaf;
    struct tu_farg arg;

    msgForEach(this, tu_leaf, &arg);

    newOff = arg.off;
    newLen = arg.len;
    newLeaf = (MNodeLeaf*)arg.leaf;
 
#endif TU_UNFOLD

    /* printf("found a leaf node, newLen=%d\n", newLen); */
    /* check if the leaf node contains the entire requested head */
    if (newLen >= hdrLen) {
      long offset, length;

      /* just make the leaf node the new stack */
      this->stack = newLeaf;
      
      offset = this->stackHeadPtr - this->headPtr;
      length = this->tailPtr - this->stackHeadPtr;
      this->stackHeadPtr = this->stack->b.leaf.data + newOff;
      this->stackTailPtr = this->stackHeadPtr + newLen;
      this->headPtr = this->stackHeadPtr - offset;
      this->tailPtr = this->stackHeadPtr + length;
      this->state.myStack = 0; 
      return this->stackHeadPtr;
    }

    /* else we have to copy after all... */
  }
  xTrace0(msg, TR_SOFT_ERROR, "msgtopunderflow will copy");

#endif MSG_NEW_ALG
  /* create a new root node and a new stack */
  newStack = newMNodePage(PAGEROUND(roundlen));
  xAssert(newStack != (MNodePage *)0);

  /* fill the new stack with the requested head */
  c_args.buf = newStack->b.leaf.data + newStack->b.leaf.size - hdrLen;
  c_args.size = hdrLen;
  /* overflow checks handled by msgForEach */
  msgForEach(this, s_copy, &c_args);

  /* assign new message */
#ifdef MSG_NEW_ALG
  {
    long offset = this->stackHeadPtr - this->headPtr;
    long length = this->tailPtr - this->stackHeadPtr;

    this->tree = newMNodePair(0L, newStack->b.leaf.size, newStack,
			      offset + hdrLen, length - hdrLen, this->tree);
    xAssert(this->tree != 0);
    this->stack = newStack;
    this->stackTailPtr = this->stack->b.leaf.data + this->stack->b.leaf.size;
    this->stackHeadPtr = this->stackTailPtr - hdrLen;
    this->headPtr = this->stack->b.leaf.data;
    this->tailPtr = this->stackHeadPtr + length;
    this->state.myStack = 1;
    this->state.numNodes += 2;
    return this->stackHeadPtr;
  }
#else
  this->tree =
    newMNodePair(0L, newStack->b.leaf.size, newStack,
		 this->offset + hdrLen, this->length - hdrLen, this->tree);
  xAssert(this->tree != 0);
  this->stack = newStack;
  this->offset = newStack->b.leaf.size - hdrLen;
  return this->stack->b.leaf.data + this->offset;
#endif MSG_NEW_ALG
}


/* msgPop
 *
 * pop a header from this Msg
 * returns 1 after successful pop
 *   
 * calls msgTopUnderflow if necessary
 */
bool msgPop(this, load, hdr, hdrLen, arg)
    Msg *this;
    MLoadFun load;
    VOID *hdr;
    long hdrLen;
    VOID *arg;
{
  char *where;
  long actualLen;
#ifdef MSG_NEW_ALG
  long length = this->tailPtr - this->stackHeadPtr;
#endif MSG_NEW_ALG

  xTrace0(msg, TR_FULL_TRACE, "msgPop");
  xAssert(hdrLen>=0);
  if (!hdrLen) xTrace0(msg, TR_SOFT_ERROR, "msgPop zero length - useless");
#ifdef MSG_NEW_ALG
  if (hdrLen <= length) {
    /* Msg is long enough */
    if (this->stackHeadPtr + hdrLen <= this->stackTailPtr)
      {
	where = this->stackHeadPtr;
	actualLen = load(hdr, where, hdrLen, arg); 
	if (actualLen > 0) {  /* if a "peek", preserve ownerships */
	  this->stackHeadPtr += actualLen;
	  this->state.myStack = 0;
	  this->tailstate.myLastStack = 0;
	}
      }
    else {
      where = msgTopUnderflow(this, hdrLen);
      actualLen = load(hdr, where, hdrLen, arg); 
      this->stackHeadPtr += actualLen;
      this->state.myStack = 0;
      this->tailstate.myLastStack = 0;
    }
    xAssert(actualLen >= 0 && actualLen <= hdrLen);
    return TRUE;
  }
#else
  if (hdrLen <= this->length) {
    /* Msg is long enough */
    if (this->offset + hdrLen <= this->stack->b.leaf.size)
      where = this->stack->b.leaf.data + this->offset;
    else
      where = msgTopUnderflow(this, hdrLen);

    actualLen = load(hdr, where, hdrLen, arg);
    this->length -= actualLen;
    this->offset += actualLen;
    return TRUE;
  } else {
    return FALSE;
  }
#endif MSG_NEW_ALG
  return FALSE;
}


/* msgPopDiscard
 *
 * pop and discard an object of length len
 * returns TRUE after successful pop
 */
bool msgPopDiscard(this, len)
    Msg *this;
    long len;
{
#ifdef MSG_NEW_ALG  
  long length;
#endif MSG_NEW_ALG  

  xTrace0(msg, TR_FULL_TRACE, "msgPopDiscard");
  xAssert(len>=0);
  xIfTrace(msg, TR_SOFT_ERROR) {
    if (len==0)
      printf("msgPopDiscard of length 0; useless call");
  }
#ifdef MSG_NEW_ALG
  length = this->tailPtr - this->stackHeadPtr;
  if (len > length) len = length;
  this->stackHeadPtr += len;
  this->state.myStack = 0;
  this->tailstate.myLastStack = 0;
#else
  if (len > this->length) len = this->length;
  this->length -= len;
  this->offset += len;
#endif MSG_NEW_ALG
  return TRUE;
}



/* import a Msg from a different address space				*/
/* makes the content of the Msg foreign (contigs which may reside in	*/
/* a different address space) accessible in the current address space*/
/*ARGSUSED*/
void msgImport(this, foreign)
    Msg *this, *foreign;
{
  xTrace0(msg, TR_FULL_TRACE, "msgImport");
}


/* lock the memory associated with the Msg in physical memory		*/
/* returns 0, if not all contigs of the Msg can be locked		*/
/*ARGSUSED*/
bool msgLock(this)
    Msg *this;
{
  xTrace0(msg, TR_FULL_TRACE, "msgLock");
  return 1;
}


/* unlock the memory associated with the Msg				*/
/* returns 0, if not all contigs of the Msg can be unlocked		*/
/*ARGSUSED*/
bool msgUnlock(this)
    Msg *this;
{
  xTrace0(msg, TR_FULL_TRACE, "msgUnlock");
  return 1;
}


xkern_return_t
msgSetAttr( this, name, attr, len )
    Msg		*this;
    int		name;
    VOID	*attr;
    int		len;
{
    /* 
     * Only the default attribute (name == 0) is supported at this time. 
     */
    if ( name != 0 ) {
	xTrace1(msg, TR_SOFT_ERRORS,
		"msgSetAttr called with unsupported name %d", name);
	return XK_FAILURE;
    }
    this->attr = attr;
    return XK_SUCCESS;
}


VOID *
msgGetAttr( this, name )
    Msg		*this;
    int		name;
{
    /* 
     * Only the default attribute (name == 0) is supported at this time. 
     */
    if ( name != 0 ) {
	xTrace1(msg, TR_SOFT_ERRORS,
		"msgGetAttr called with unsupported name %d", name);
	return 0;
    }
    return this->attr;
}


/*
 *  Message Iteration Functions
 *
 *  Routines for processing the message data; hides the message tree
 *   structure and lengths of node buffers
 */

/*
 * Forward:
 */

static bool msgPairForEach( MNodePair *, long, long, XCharFun, VOID * );
static bool msgLeafForEach( MNodePair *, long, long, XCharFun, VOID * );

static bool msgInternalForEach(this, off, len, f, arg)
    MNode *this;
    long off, len;
    XCharFun f;
    VOID *arg;
{
  xTrace0(msg, TR_FULL_TRACE, "msgInternalForEach");
  switch(this->type) {
    case t_MNodePair:
      return msgPairForEach(this, off, len, f, arg);
    case t_MNodePage:
    case t_MNodeUsrPage:
    case t_MNodeBuf:
      return msgLeafForEach(this, off, len, f, arg);
    default:
      xAssert(0);
  }
  /*
   * To make the compiler happy
   */
  return FALSE;
}

static bool msgPairForEach(this, off, len, f, arg)
    MNodePair *this;
    long off, len;
    XCharFun f;
    VOID *arg;
{
  long chunk_size;
  struct nlink *k;

  xTrace0(msg, TR_FULL_TRACE, "msgPairForEach");
  /* left child */
  k = &this->b.pair.l;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    if (!msgInternalForEach(k->node, k->offset + off, chunk_size, f, arg))
      return 0;
    len -= chunk_size;
  }
  off = off - k->length + chunk_size;

  /* right child */
  k = &this->b.pair.r;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    if (!msgInternalForEach(k->node, k->offset + off, chunk_size, f, arg))
      return 0;
  }
  return 1;
}

static bool msgLeafForEach(this, off, len, f, arg)
    MNode *this;
    long off, len;
    XCharFun f;
    VOID *arg;
{
  xTrace0(msg, TR_FULL_TRACE, "msgLeafForEach");
  xAssert(this->b.leaf.size - off >= len);
#ifdef MSG_NEW_ALG
  if (f==tu_leaf) return(tu_leaf(this, off, len, arg));
  else
#endif MSG_NEW_ALG
    return f(this->b.leaf.data + off, len, arg);
}

/* for every contig in this Msg, invoke the function f with 		*/
/* arguments const char *buf (=address of contig), long len 		*/
/* (=length of contig), and VOID * arg (=user-supplied argument),  	*/
/* while f returns TRUE							*/
void msgForEach(this, f, arg)
    Msg *this;
    XCharFun f;
    VOID *arg;
{
#ifdef MSG_NEW_ALG
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;
#endif MSG_NEW_ALG

  xTrace0(msg, TR_FULL_TRACE, "msgForEach");
#ifdef MSG_NEW_ALG
  (void) msgInternalForEach(this->tree, offset, length, f, arg);
#else
  (void) msgInternalForEach(this->tree, this->offset, this->length, f, arg);
#endif MSG_NEW_ALG
}


/*
 * Forward:
 */
static void	msgPairShow( MNode *, long, long, long );
static void	msgLeafShow( MNode *, long, long, long );

static char blanks[] = "                                ";

static void msgInternalShow(this, off, len, indent)
    MNode *this;
    long off, len, indent;
{
    xTrace0(msg, TR_FULL_TRACE, "msgInternalShow");
    switch(this->type) {
      case t_MNodePair:
	msgPairShow(this, off, len, indent);
	break;
      case t_MNodePage:
      case t_MNodeUsrPage:
      case t_MNodeBuf:
	msgLeafShow(this, off, len, indent);
	break;
      default:
	break;
    }
}

static void msgPairShow(this, off, len, indent)
    MNodePair *this;
    long off, len, indent;
{
  long chunk_size;
  struct nlink *k;

  xTrace0(msg, TR_FULL_TRACE, "msgPairShow");
  printf("%.*sPair: refCnt = %d off = %d len = %d\n", indent, blanks, 
	 this->refCnt, off, len);
  /* left child */
  k = &this->b.pair.l;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    msgInternalShow(k->node, k->offset + off, chunk_size, indent+2);
    len -= chunk_size;
  }
  off = off - k->length + chunk_size;

  /* right child */
  k = &this->b.pair.r;
  chunk_size = (off < k->length) ? k->length - off : 0;
  if (chunk_size > len) chunk_size  = len;
  if (chunk_size) {
    msgInternalShow(k->node, k->offset + off, chunk_size, indent+2);
  }
}

static void msgLeafShow(this, off, len, indent)
    MNode *this;
    long off, len, indent;
{
  int lim, i;
  char *c;
  
  xTrace0(msg, TR_FULL_TRACE, "msgLeafShow");
  xAssert(this->b.leaf.size - off >= len);
  printf("%.*sLeaf: %#x refCnt %d off %d len %d", indent, blanks, 
	 this->b.leaf.data, this->refCnt, off, len);
  /*
   * Display first 8 characters of data
   */
  printf("\tdata: ");
  lim = len < 8 ? len : 8;
  for (c=this->b.leaf.data+off, i=0; i < lim; c++, i++) {
    printf("%.2x ", *c & 0xff);
  }
  putchar('\n');
}

void msgShow(this)
    Msg *this;
{
#ifdef MSG_NEW_ALG  
  long offset = this->stackHeadPtr - this->headPtr;
  long length = this->tailPtr - this->stackHeadPtr;
#endif MSG_NEW_ALG

  xTrace0(msg, TR_FULL_TRACE, "msgShow");
#ifdef MSG_NEW_ALG  
  printf("Msg: Stack=%#x, stackHeadPtr=%#x, stackTailPtr=%#x, myStack=%d, myLastStack %d, numNodes=%d offset %ld length %ld\n",
	 this->stack, this->stackHeadPtr, this->stackTailPtr,
	 this->state.myStack, this->tailstate.myLastStack,
	 this->state.numNodes,
	 offset, length);
  if (length > 0)
    (void) msgInternalShow(this->tree, offset, length, 0L);
#else
  if (this->length > 0)
    (void) msgInternalShow(this->tree, this->offset, this->length, 0L);
#endif MSG_NEW_ALG
}


/* for every contig in this Msg, invoke the function f with 		*/
/* arguments MContig *contig (=address of contig object), long offset,	*/
/* long length, and VOID *arg (=user-supplied argument),  		*/
/* while f returns 1							*/
/* the memory described by contig may not be accessible!!		*/
/*ARGSUSED*/
void msgForEachContig(this, f, arg)
    Msg *this;
    MContigFun f;
    VOID *arg;
{
  xTrace0(msg, TR_FULL_TRACE, "msgForEachContig");
  xAssert(0);
}

/* copy this entire Msg into contiguous storage */
static void
makeContiguous(this)
     Msg *this;
{
#ifdef MSG_NEW_ALG
  long length;
  struct arg c_args;
  Msg empty, tmp;
#endif MSG_NEW_ALG

  xTrace0(msg, TR_FULL_TRACE, "makeContiguous");

#ifdef MSG_NEW_ALG
  length = this->tailPtr - this->stackHeadPtr;

  if (length == 0) {
    /* free the Msg's resources */
    msgConstructEmpty(&empty);
    msgAssign(this, &empty);
    return;
  }

  if (this->stack == this->tree) 
    /* this Msg is contiguous */
    return;

  /* create a new Msg with a contiguous buffer */
  msgConstructAllocate(&tmp, length, &c_args.buf);

  /* copy into the new Msg */
  c_args.size = length;
  msgForEach(this, s_copy, &c_args);

  /* assign the new Msg */
  msgAssign(this, &tmp);

#endif MSG_NEW_ALG
}

/* perform housecleaning to free unnecessary resources allocated to this msg */
void msgCleanUp(this)
     Msg *this;
{
#ifdef MSG_NEW_ALG
#ifdef undef
  long offset = this->stackHeadPtr - this->headPtr;
#endif
  long length = this->tailPtr - this->stackHeadPtr;

  if (length > 0) {
    if (this->state.numNodes > numNodesSoftLimit) makeContiguous(this);

#ifdef undef
    /* prune the tree */
    MNode *oldTree = tree;

    treePrune(offset, length, &offset, tree);
    
    if (tree != oldTree) {
      /* the root node was pruned */
      /* must change headPtr to reflect the new offset */
      this->headPtr = this->stackHeadPtr - offset;
    }

    /* if the stack was empty, it may have been deallocated */
    /* set it not-owned, so push won't try to write into it */
    this->state.myStack = 0;
    this->tailstate.myLastStack = 0;
#endif undef

  } else {
    /* this Msg is empty */
    /* free its resources */
    Msg empty;
    msgConstructEmpty(&empty);
    msgAssign(this, &empty);
  }
#endif MSG_NEW_ALG
}


static bool
frag2Buf( frag, len, bufPtr )
    char	*frag;
    long	len;
    VOID 	*bufPtr;
{
    xTrace3(msg, TR_FUNCTIONAL_TRACE, "frag2Buf copying %d bytes from %x to %x",
	    len, (int)frag, (int)(*(char **)bufPtr));
    bcopy(frag, *(char **)bufPtr, len);
    *(char **)bufPtr += len;
    return TRUE;
}


void
msg2buf( msg, buf )
    Msg		*msg;
    char	*buf;
{
    char	*ptr = buf;

    xTrace2(msg, TR_FUNCTIONAL_TRACE, "msg2Buf (%x, %x)", (int)msg, (int)buf);
    msgForEach(msg, frag2Buf, &ptr);
    xTrace0(msg, TR_FULL_TRACE, "msg2Buf returns");
}


void
msgStats()
{
    xTrace0(msg, TR_ALWAYS,
	    "x-kernel message statistics (current, high water):");
    xTrace2(msg, TR_ALWAYS, "    Pair nodes:      %6d, %6d",
	    mpr_used, mpr_hiwat);
    xTrace2(msg, TR_ALWAYS, "    Page nodes:      %6d, %6d",
	    mpg_used, mpg_hiwat);
    xTrace2(msg, TR_ALWAYS, "    User Page nodes: %6d, %6d",
	    mup_used, mup_hiwat);
    xTrace2(msg, TR_ALWAYS, "    Buffer nodes:    %6d, %6d",
	    mbf_used, mbf_hiwat);
    xTrace2(msg, TR_ALWAYS, "Total space used: %9d, %9d",
	    msgSpace_used, msgSpace_hiwat);
}
