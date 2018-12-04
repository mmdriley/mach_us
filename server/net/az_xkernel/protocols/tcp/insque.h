/* insque.h
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
 * Modified for x-kernel v3.2
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:23:26 $
 */

struct x {
  struct x *next, *prev;
};

#define insque(a, b) { \
  ((struct x *)(a))->next = (struct x *)(b); \
  ((struct x *)(a))->prev = ((struct x *)(b))->prev; \
  ((struct x *)(b))->prev = (struct x *)(a); \
  ((struct x *)(a))->prev->next = (struct x *)(a); \
}

#define remque(A) { \
  ((struct x *)(A))->prev->next = ((struct x *)(A))->next; \
  ((struct x *)(A))->next->prev = ((struct x *)(A))->prev; \
}

