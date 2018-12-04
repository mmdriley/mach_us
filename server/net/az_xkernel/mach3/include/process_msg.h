/* 
 * process_msg.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/02 00:03:13 $
 */

#ifndef process_msg_h
#define process_msg_h

#include "list.h"
#include "msg.h"
#include <device/net_status.h>

#define ROUND4(len)   ((len + 3) & ~3)

typedef struct input_buffer {
    struct list_entry	q;
    char                hdr[NET_HDW_HDR_MAX];
    Msg                 upmsg;
    char                *data;  	/* will contain mach_hdrs for out-of-kernel */
    XObj		driverProtl;	/* The protocol currently using this buffer */
} InputBuffer;


/*
 * Packet filter declarations.
 */
struct mach_hdrs {
  mach_msg_header_t msg_hdr;
  mach_msg_type_t header_type;
  char header[NET_HDW_HDR_MAX];
  mach_msg_type_t packet_type;
  struct packet_header packet_header;
};


typedef struct {
    mutex_t	lock;
    condition_t	notEmpty;
    list_t	list;
} ProtectedQueue;

#define pqRemove(_pq, _buf)						\
{									\
    mutex_lock( (_pq).lock );						\
    while ( ((_buf) = (InputBuffer *)delist_head_strong((_pq).list))	\
	   == 0 ) {							\
	condition_wait( (_pq).notEmpty, (_pq).lock );			\
    }									\
    mutex_unlock( (_pq).lock );						\
}

#define pqAppend(_pq, _buf)			\
{						\
    mutex_lock( (_pq).lock );			\
    enlist( (_pq).list, (list_entry_t)(_buf) );	\
    condition_signal( (_pq).notEmpty);		\
    mutex_unlock( (_pq).lock );			\
}

extern ProtectedQueue	xkInQueue;
extern ProtectedQueue	xkFreeQueue;


#ifdef __STDC__

void			xkBufferPoolInit( void );
void			xkFillThreadPool( void );

#else

void			xkBufferPoolInit();
void			xkFillThreadPool();

#endif __STDC__

#endif ! process_msg_h

