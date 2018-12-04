/* 
 * process_msg.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/02 00:03:20 $
 */

#ifndef process_msg_h
#define process_msg_h

#include "eth_host.h"
#include "list.h"
#include "msg.h"
#include <device/net_status.h>

#define ROUND4(len)   ((len + 3) & ~3)

typedef struct input_buffer {
    struct list_entry	q;	/* used by list routines; must be first */
    char                hdr[NET_HDW_HDR_MAX];
    Msg                 upmsg;
    char                *data;  	
    XObj		driverProtl;	/* The protocol currently using this buffer */
} InputBuffer;


struct threadIBlock	{
    struct list_entry	q;	/* used by list routines; must be first */
    thread_t		thread;	/* id of suspended thread */
    InputBuffer		*buf;
    int			xkThreadId;
};


extern struct list_head	xkThreadQ;
extern struct list_head	xkBufferPool;
extern struct list_head	xkIncomingData;
extern int		xkIncomingData_lock;


#ifdef __STDC__

void			xkBufferPoolInit( void );
void			xkFillThreadPool( void );

#else

void			xkBufferPoolInit();
void			xkFillThreadPool();

#endif __STDC__

#endif ! process_msg_h

