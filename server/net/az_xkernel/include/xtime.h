/*
 * xtime.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:39:19 $
 */

/* Same time structure as Unix */

#ifndef xtime_h
#define xtime_h

typedef struct {
  long sec;
  long usec;
} XTime;

#ifdef __STDC__

/* set *t to the current time of day */
void xGetTime(XTime *t);

#endif __STDC__


/* 
 * xAddtime -- Sets the XTime value _res to the sum of _t1 and _t2.
 * Assumes _t1 and _t2 are in standard time format (i.e., does not
 * check for integer overflow of the usec value.)
 */
#define xAddTime( _res, _t1, _t2 )			\
do {							\
    (_res)->sec = (_t1).sec + (_t2).sec;		\
    (_res)->usec = (_t1).usec + (_t2).usec;		\
    if ( (_res)->usec >= (1000 * 1000)) {		\
	(_res)->sec += (_res)->usec / (1000 * 1000);	\
	(_res)->usec %= (u_int)(1000 * 1000);		\
    }							\
} while (0)



/* 
 * xSubTime -- Sets _res to the difference of _t1 and _t2.  The resulting
 * value may be negative.
 */
#define xSubTime( _res, _t1, _t2 )		\
do {						\
    (_res)->sec = (_t1).sec - (_t2).sec;	\
    (_res)->usec = (_t1).usec - (_t2).usec;	\
    if ( (_res)->usec < 0 ) {			\
	(_res)->usec += (1000 * 1000);		\
	(_res)->sec -= 1;			\
    }						\
} while (0)



#endif ! xtime.h
