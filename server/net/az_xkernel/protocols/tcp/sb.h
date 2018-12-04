/*
 * sb.h
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Modified for x-kernel v3.1	12/10/90
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 */

#ifndef sb_h
#define sb_h

#define TCP_BUFFER_SPACE (4 * 1024)
#define sbhiwat(sb) ((sb)->hiwat)

struct sb {
  struct sb_i *next, *prev;
  int hiwat;
  int len;
};

struct sb_i {
  struct sb_i *next, *prev;
  Msg m;
};

#define sblength(sb) ((sb)->len)
#define sbspace(sb)  (sbhiwat(sb)-sblength(sb))
#define sbinit(sb) { \
		       (sb)->next = (sb)->prev = (struct sb_i*)(sb); \
		       (sb)->len=0; \
		       (sb)->hiwat=TCP_BUFFER_SPACE; \
		   }

extern struct sb_i *sbifreelist;
#define sbinew(s) { if ((s) = sbifreelist) sbifreelist = (s)->next; else (s) = (struct sb_i *)xMalloc(sizeof(struct sb_i)); }
#define sbifree(s) { (s)->next = sbifreelist; sbifreelist = (s); }

#ifdef __STDC__

extern void	sbappend( struct sb *, Msg * );
extern void 	sbcollect( struct sb *, Msg *, int off, int len, int delete );
extern void 	sbflush( struct sb * );
extern void 	sbdrop( struct sb * , int len );
extern void 	sbdelete( struct sb * );

#else

extern void	sbappend();
extern void 	sbcollect();
extern void 	sbflush();
extern void 	sbdrop();
extern void 	sbdelete();

#endif __STDC__

#endif
