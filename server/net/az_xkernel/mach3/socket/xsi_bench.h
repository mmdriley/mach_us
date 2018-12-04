/*
 * $RCSfile: xsi_bench.h,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:09:39 $
 * $Author: menze $
 *
 * $Log: xsi_bench.h,v $
 * Revision 1.2  1993/02/02  00:09:39  menze
 * copyright change
 *
 * Revision 1.1  1992/12/01  22:16:49  menze
 * Initial revision
 *
 */
#ifndef xsi_bench_h
#define xsi_bench_h

#include "xsi.h"

extern bool xsi_i_am_server;
extern bool xsi_i_am_client;
extern int  xsi_fixed_prio;

extern void xsi_bench_process_options(void);
extern void xsi_benchmark(Event ev, void *arg);

#endif /* xsi_bench_h */
