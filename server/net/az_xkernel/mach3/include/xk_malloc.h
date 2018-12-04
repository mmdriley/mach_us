/*
 * xk_malloc.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:03:06 $
 */


/* the memory throttle only works with the in-Mach-kernel version */

#ifdef XK_MEMORY_THROTTLE

#define MEG 1024*1024
#define XK_MEMORY_LIMIT 10*MEG

#ifndef IN_MALLOC_FILE
extern int memory_unused;
#endif IN_MALLOC_FILE

#define XK_MEMORY_REPORT_INTERVAL 128
static int report_count = 0;

#define XK_INCOMING_MEMORY_MARK ((XK_MEMORY_LIMIT) / 8)

#endif XK_MEMORY_THROTTLE

