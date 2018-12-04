/*
 * dns.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:40:29 $
 */

#ifndef dns_h
#define dns_h

typedef struct resolve_result {
  char	  name[256];
  IPhost  addr[10];
} Result;

#endif
