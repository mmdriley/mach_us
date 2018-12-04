/*
 * $RCSfile: msg_stream.h,v $
 *
 * Copyright (c) 1992  Arizona Board of Regents
 *
 * $Revision: 1.1 $
 * $Date: 1992/08/01 18:42:25 $
 * $Author: davidm $
 *
 * $Log: msg_stream.h,v $
 * Revision 1.1  1992/08/01  18:42:25  davidm
 * Initial revision
 *
 */
#ifndef msg_stream_h
#define msg_stream_h

#include "xkernel.h"

#define MSG_ATOMIC	0x1000		/* don't cross message boundaries */

typedef struct msg_stream {
    int ms_len;		/* # of characters in stream */
    int ms_hiwat;	/* high water mark */
    struct msg_link {
	struct msg_link *ml_next;	/* next message */
	struct msg_link *ml_prev;	/* previous message */
	Msg		 ml_msg;	/* actual message */
	struct sockaddr	 ml_from;	/* originator of message */
    } *ms_head, *ms_tail;		/* head and tail of msg list */
} msg_stream;

xkern_return_t streamInit(msg_stream *ms, int hiwat);
xkern_return_t streamAppend(msg_stream *ms, Msg *msg, struct sockaddr *from);
int streamRemove(msg_stream *ms, void *buf, int len, int flags,
		 struct sockaddr *from, int *fromlen);
xkern_return_t streamDestroy(msg_stream *ms);

#define streamSetSize(ms,high)	(void)((ms)->ms_hiwat = (high))

#define streamGetSize(ms)	((ms)->ms_hiwat)
#define streamGetSpace(ms)	((ms)->ms_hiwat - (ms)->ms_len)
#define streamLen(ms)		((ms)->ms_len)
#define streamIsFlooded(ms)	((ms)->ms_len >= (ms)->ms_hiwat)

#endif /* msg_stream_h */
