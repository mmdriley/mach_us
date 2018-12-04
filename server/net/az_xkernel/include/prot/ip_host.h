/* 
 * ip_host.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/01 22:40:41 $
 */

#ifndef ip_host_h
#define ip_host_h

typedef struct iphost {
    u_char 	a,b,c,d;
} IPhost;

#define IP_EQUAL(a1,a2) ( (a1).a == (a2).a && (a1).b == (a2).b &&	\
			  (a1).c == (a2).c && (a1).d == (a2).d )

#define IP_AND( x, y, z ) {	\
	(x).a = (y).a & (z).a;	\
	(x).b = (y).b & (z).b;	\
	(x).c = (y).c & (z).c;	\
	(x).d = (y).d & (z).d;	\
      }

#define IP_OR( x, y, z ) {	\
	(x).a = (y).a | (z).a;	\
	(x).b = (y).b | (z).b;	\
	(x).c = (y).c | (z).c;	\
	(x).d = (y).d | (z).d;	\
      }

#define IP_COMP( x, y ) {	\
	(x).a = ~(y).a;		\
	(x).b = ~(y).b;		\
	(x).c = ~(y).c;		\
	(x).d = ~(y).d;		\
      }

#endif
