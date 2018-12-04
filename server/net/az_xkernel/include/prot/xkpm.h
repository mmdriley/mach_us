/*
 * xkpm.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:41:38 $
 *
 * X-kernel version of the portmapper,
 * by Rich Schroeppel, based on older versions by Sean O'Malley,
 * Larry Petersen, & Norm Hutchinson.
 */

#ifndef XKPMAP_H
#define XKPMAP_H

#define PMAPPROC_NULL  0
#define PMAPPROC_SET  1
#define PMAPPROC_UNSET  2
#define PMAPPROC_GETPORT  3
#define PMAPPROC_DUMP  4
#define PMAPPROC_CALLIT  5

typedef struct {unsigned long prog, vers, prot, port;} PMAP_ARGS;

#define PMAP_PORT   111 
#define PMAP_PROG   100000
#define PMAP_VERS   2

#ifdef SUNXDR
#define XKXDR  XDR
#define xkxdr_init(name,address,length,direction) \
  {xdrmem_create(&name,address,length,direction);}
#else
typedef char* XKXDR;
#define XDR_ENCODE  1
#define XDR_DECODE  2
#define xkxdr_init(name,address,length,direction) \
  {name=address;}
#endif  /* SUNXDR */

#endif

/* end of xkpm.h */
