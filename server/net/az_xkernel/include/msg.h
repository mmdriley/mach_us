/* 
 * msg.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.26 $
 * $Date: 1993/02/01 22:39:01 $
 */

#ifndef msg_h
#define msg_h

#include "msg_s.h"

#include "platform.h"


/*
 * the MContig structure
 * the memory descriptor structure
 */
typedef struct {
  /* size of contig */
  long   size;
  char*  addr;
 
  /* copy from a buffer into the contig at offset; 			*/
  /* return the number of bytes copied					*/
  long (*copyIn)(/* char* from, long offset, long size */);

  /* copy from the contig at offset to a buffer; 			*/
  /* return the number of bytes copied					*/
  long (*copyOut)(/* char* to, long offset, long size */);

} MContig;




/* 
 * types of user functions called in ForEach()
 */
#if defined(__STDC__) || defined(__GNUC__)
typedef bool (*XCharFun)( char *ptr, long len, void *arg );
typedef bool (*MContigFun)(MContig *contig, long offset, long len, void *arg); 

#else

typedef bool (*XCharFun)();
typedef bool (*MContigFun)();

#endif
/*
 * types of load and store functions for headers
 */

/* function to convert the header to network byte order and store it	
 * in a potentially unaligned buffer. 					
 * 'len' is the number of bytes to be read and 'arg' is an arbitrary
 * argument passed through msgPush
 */
typedef void (*MStoreFun)(
#if defined(__STDC__) || defined(__GNUC__)
			  void *hdr, char *des, long len, void *arg
#endif
			  );

/* function to load the header from a potentially unaligned buffer, 	
 * and convert it to machine byte order.
 * 'len' is the number of bytes to be read and 'arg' is an arbitrary
 * argument passed through msgPop
 */
typedef long (*MLoadFun)(
#if defined(__STDC__) || defined(__GNUC__)
			 void *hdr, char *src, long len, void *arg
#endif
			 );

#if defined(__STDC__) || defined(__GNUC__)

/* Msg operations */

/* initializer; must be called before any Msg instances are created 	*/
/* returns TRUE after successful intialization				*/
bool msgInit(void);

/************* constructors/destructors **************/
/* construct; an empty Msg */
void msgConstructEmpty(Msg* this);

/* construct a Msg with a copy of the content of the buffer buf with */
/* length len; this constructor causes data to be copied!! */
void msgConstructBuffer(Msg* this, char *buf, long len);

/* construct a Msg from another Msg */
void msgConstructCopy(Msg* this, Msg *another);

/* construct a Msg that refers to a contig */
void msgConstructContig(Msg* this, MContig* contig);

/* constructor... to this Msg an uninitialized contig */
/* of length len and return a pointer to the contig */
void msgConstructAllocate(Msg* this, long len, char **buf);

/* constructor... to this Msg an uninitialized contig */
/* of length len and return a pointer to the contig. */
/* this puts the stack pointer at a low address, for appending */
void msgConstructAppend(Msg* this, long len, char **buf);

/* constructor... make user data buffer part of the Msg. */
/* The freefunc will be called when the Msg is destroyed. */
void
msgConstructInplace(Msg  *this, char *stack, long  length, void  (*freefunc)());

/* destructor */
void msgDestroy(Msg* this);



/************* regular protocols call these **************/
/* assignment */
void msgAssign(Msg* this, Msg* m);

/* return the current length of the message */
long msgLen(Msg *this);

/* truncate this Msg to length newLength */
void msgTruncate(Msg* this, long newLength);

/* remove a chunk of length len from the head of this Msg */
/* and assign it to head; like old break */
void msgChopOff(Msg* this, Msg* head, long len);

/* assign to this Msg the concatenation of Msg1 and Msg2 */
void msgJoin(Msg* this, Msg* Msg1, Msg* Msg2);

/* push a header onto this Msg */
void msgPush(Msg* this, MStoreFun store, void *hdr, long hdrLen, void *arg);

/* pop a header from this Msg 
 * 'arg' is an arbitrary argument which will be passed to 'store'
 * returns TRUE after successful pop
 */
bool msgPop(Msg* this, MLoadFun load, void *hdr, long hdrLen, void *arg);

/* add a trailer to a message
 * 'arg' is an arbitrary argument which will be passed to 'store'.
 * if a new stack must allocated, it will of size newlength.
 */
void msgAppend(Msg* this, MStoreFun store, void *hdr, long hdrLen, void *arg, long newlength);

/* pop and discard an object of length len */
/* returns TRUE after successful pop */
bool msgPopDiscard(Msg *this, long len);

/* perform housecleaning to free unnecessary resources allocated to this msg */
void msgCleanUp(Msg *this);

/* Copy fragments of the message into a contiguous buffer.  Buffer must be */
/* long enough */
void msg2buf( Msg *this, char *buf );

#endif __STDC__


/*
 * associate an attribute with a message
 */
xkern_return_t	msgSetAttr(
#if defined(__STDC__) || defined(__GNUC__)
			   Msg *this, int name, VOID *attr, int len
#endif
			   );


/*
 * retrieve an attribute associated with a message
 */
VOID *		msgGetAttr(
#if defined(__STDC__) || defined(__GNUC__)
			  Msg *this, int name
#endif
			  );


/************* get to the data in the message **************/
/* for every contig in this Msg, invoke the function f with */
/* arguments const char *buf (=address of contig), long len */
/* (=length of contig), and void *arg (=user-supplied argument), */  
/* while f returns TRUE */
void msgForEach(
#if defined(__STDC__) || defined(__GNUC__)
		Msg *this, XCharFun f, void *arg
#endif
		);

/* for every contig in this Msg, invoke the function f with  */
/* arguments MContig *contig (=address of contig object), long offset, */
/* long length, and void *arg (=user-supplied argument), */
/* while f returns TRUE */
/* the memory described by contig may not be accessible!! */
void msgForEachContig(
#if defined(__STDC__) || defined(__GNUC__)
		      Msg *this, MContigFun f, void *arg
#endif
		      );



#if defined(__STDC__) || defined(__GNUC__)

/************* virtual memory funniness **************/
/* import a Msg from a different address space */
/* makes the content of the Msg foreign (contigs which may reside in */
/* a different address space) accessible in the current address space */
void msgImport(Msg* this, Msg* foreign);

/* lock the memory associated with the Msg in physical memory */
/* returns FALSE, if not all contigs of the Msg can be locked */
bool msgLock(Msg* this);

/* unlock the memory associated with the Msg */
/* returns FALSE, if not all contigs of the Msg can be unlocked */
bool msgUnlock(Msg* this);	


/************* debugging **************/
/* 
 * Display some textual representation of the message
 */
void msgShow( Msg *this );

/* 
 * Display statistics about message usage
 */
void msgStats( void );

#endif __STDC__

#endif msg_h
