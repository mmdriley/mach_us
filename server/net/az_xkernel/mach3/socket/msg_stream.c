/*
 * $RCSfile: msg_stream.c,v $
 *
 * Copyright (c) 1992  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1992/08/15 01:16:26 $
 * $Author: davidm $
 *
 * $Log: msg_stream.c,v $
 * Revision 1.2  1992/08/15  01:16:26  davidm
 * OOB message transmission is now implemented *and* tested.
 * Support for SIGURG was added.
 *
 * Revision 1.1  1992/08/01  18:42:25  davidm
 * Initial revision
 *
 */
#include <sys/socket.h>
#include "msg_stream.h"

struct varbuf {
    void *vb_buf;
    int   vb_len;
};


static bool
pop_segment(void *buf, long len, struct varbuf *vb)
{
    if (len > vb->vb_len) {
	len = vb->vb_len;
    } /* if */
    vb->vb_len -= len;
    bcopy(buf, vb->vb_buf, len);
    vb->vb_buf += len;
    return vb->vb_len > 0;
} /* pop_segment */


xkern_return_t
streamInit(msg_stream *ms, int hiwat)
{
    ms->ms_len = 0;
    ms->ms_hiwat = hiwat;
    ms->ms_head = 0;
    ms->ms_tail = 0;
    return XK_SUCCESS;
} /* streamInit */


xkern_return_t
streamAppend(msg_stream *ms, Msg *msg, struct sockaddr *from)
{
    struct msg_link *el;

    streamLen(ms) += msgLen(msg);

    el = (struct msg_link*) xMalloc(sizeof(struct msg_link));
    msgConstructCopy(&el->ml_msg, msg);
    bcopy((void*)from, (void*)&el->ml_from, sizeof(struct sockaddr));
    el->ml_prev = ms->ms_tail;
    el->ml_next = 0;
    if (ms->ms_tail) {
	ms->ms_tail->ml_next = el;
    } else {
	ms->ms_head = el;
    } /* if */
    ms->ms_tail = el;
    return XK_SUCCESS;
} /* streamAppend */


int
streamRemove(msg_stream *ms, void *buf, int len, int flags,
	     struct sockaddr *from, int *fromlen)
{
    Msg *m;
    int total;
    int to_copy;
    struct msg_link *ml;
    struct varbuf vb;

    if (!streamLen(ms)) {
	return 0;
    } /* if */

    if (flags & MSG_ATOMIC) {
	if (len > msgLen(&ms->ms_head->ml_msg)) {
	    len = msgLen(&ms->ms_head->ml_msg);
	} /* if */
    } else {
	if (len > streamLen(ms)) {
	    len = streamLen(ms);
	} /* if */
    } /* if */

    if (!(flags & MSG_PEEK)) {
	streamLen(ms) -= len;
    } /* if */
    total = len;

    ml = ms->ms_head;
    vb.vb_buf = buf;

    if (from) {
	bcopy((void*)&ml->ml_from, (void*)from, sizeof(struct sockaddr));
	*fromlen = sizeof(struct sockaddr);
    } /* if */

    while (len) {
	m = &ml->ml_msg;
	to_copy = msgLen(m);
	if (to_copy > len) {
	    to_copy = len;
	} /* if */
	vb.vb_len = to_copy;
	msgForEach(m, (bool (*)()) pop_segment, &vb);
	if (flags & MSG_PEEK) {
	    ml = ml->ml_next;
	} else {
	    msgPopDiscard(m, to_copy);
	    if (!msgLen(m) || (flags & MSG_ATOMIC)) {
		msgDestroy(m);
		ms->ms_head = ml->ml_next;
		if (ms->ms_head) {
		    ms->ms_head->ml_prev = 0;
		} else {
		    ms->ms_tail = 0;
		} /* if */
		xFree((void*) ml);
		ml = ms->ms_head;
	    } /* if */
	} /* if */
	len -= to_copy;
    } /* while */
    return total;
} /* streamRemove */


xkern_return_t
streamDestroy(msg_stream *ms)
{
    struct msg_link *ml;
    struct msg_link *garbage;

    ml = ms->ms_head;
    while (ml) {
	msgDestroy(&ml->ml_msg);
	garbage = ml;
	ml = ml->ml_next;
	xFree((void*) garbage);
    } /* while */
    streamLen(ms) = 0;
    ms->ms_head = ms->ms_tail = 0;
    return XK_SUCCESS;
} /* streamDestroy */

			/*** end of msg_stream.c ***/
