/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they made and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
/*
 * File: $Source: /afs/cs.cmu.edu/project/mach/mach3/rcs/us/server/net/cmu_build/user/usx_iobuf_mgr.cc,v $
 *
 * Purpose: Manager for a set of I/O buffers objects
 *		-- extended for network service under the x-kernel.
 *
 * This implementation is far from optimal. It should try to use
 * a shared-memory pool for fast data transfer between address spaces.
 *
 * HISTORY:
 * $Log:	usx_iobuf_mgr.cc,v $
 * Revision 2.3  94/07/13  18:07:09  mrt
 * 	Updated copyright
 * 
 * Revision 2.2  94/01/11  18:10:36  jms
 * 	Massively revised/re-written with the introduction of common "usx_" logic
 * 	TCP and xkernel v3.2
 * 	[94/01/10  12:56:14  jms]
 * 
 * Revision 2.2  91/11/06  14:15:05  jms
 * 	Initial C++ revision.
 * 	[91/09/27  16:10:17  pjg]
 * 
 * Revision 2.2  91/05/05  19:31:16  dpj
 * 	Merged up to US39
 * 	[91/05/04  10:05:59  dpj]
 * 
 * 	Removed explicit user info in records (now implicit).
 * 	[91/04/28  10:51:49  dpj]
 * 
 * 	First version.
 * 	[91/02/25  10:49:38  dpj]
 * 
 */

#ifndef lint
char * usx_iobuf_mgr_rcsid = "$Header: usx_iobuf_mgr.cc,v 2.3 94/07/13 18:07:09 mrt Exp $";
#endif	lint

#include	<usx_iobuf_mgr_ifc.h>
#include	<usx_internal.h>

extern "C" {
#include	<base.h>
#include	<us_error.h>
#include	<io_types.h>
#include	<io_types2.h>
#include	"upi.h"
}

extern "C" {
  bool usx_append_xblock(char *ptr, long len, void *arg);
}

extern boolean_t usx_debug_counts;

/*
 * Space to be reserved for headers when allocating x-kernel messages,
 * to reduce the need for a second allocation later on.
 *
 * Must be a multiple of 4.
 */
#define	USX_HDR_OVERHEAD	100


/*
 * Internal header for blocks.
 *
 * The data portion of each block is actually a x-kernel msg.
 */
struct io_block_private {
	Msg			xmsg;
};
typedef struct io_block_internal {
	struct io_block_private	s_private;
	struct io_block		s_public;
} *io_block_internal_t;


#define BASE default_iobuf_mgr
DEFINE_LOCAL_CLASS(usx_iobuf_mgr)

/*
 * Methods.
 */

mach_error_t 
usx_iobuf_mgr::io_alloc_block(unsigned int		blksize,
			      io_block_t		*newblk)	/* OUT */
{
	io_block_internal_t	rblk;
	Msg			*xmsg;
	char			*buf;
	unsigned int		buf_size;
	xkern_return_t		xret;

	DEBUG1(usx_debug, (0, "usx_iobuf_mgr::usx_io_alloc_block\n"));

	rblk = (io_block_internal_t) malloc(sizeof(struct io_block_internal));
	if (rblk == NULL) {
		*newblk = NULL;
		return(US_RESOURCE_EXHAUSTED);
	}
	xmsg = &(rblk->s_private.xmsg);

	/*
	 * NOTE: the "data" space in an "io_block_t" is within an xmsg.
	 * That xmsg in the "io_block_internal" will contain only
	 * the data for this block.  It is later reference copied 
	 * (msgConstructCopy) and appended
	 * to make more complex xMsgs, but a reference is held by this
	 * xMsg to be freed when the io_block_t is deallocated.
	 * The "data" survives beyond that point till the last xKernel
	 * reference goes away.
	 */

	/* Get extra space to msgPush on lower level headers */
	buf_size = (blksize + 3) & ~0x3;
	msgConstructAllocate(xmsg, (buf_size + USX_HDR_OVERHEAD), &buf);

	/* Free lower level hdr space to the xkern system*/
	/* XXX is this a crime against the Msg interface? */
	xret = msgPopDiscard(xmsg, USX_HDR_OVERHEAD);
	if (xret == XK_FAILURE) {
		ERROR((0,
			"usx_iobuf_mgr.io_alloc_block failed: %s\n",
				convert_xkernel_error(x_errno)));
	}
	buf += USX_HDR_OVERHEAD;

	rblk->s_public.owner = (int) this;	/* XXX need reference ? */
	rblk->s_public.next = NULL;
	rblk->s_public.type = IOB_RAWDATA;
	rblk->s_public.min_offset = ((unsigned int) buf) -
					((unsigned int) &(rblk->s_public));
	rblk->s_public.max_offset = rblk->s_public.min_offset + buf_size;
	ioblk_clear(&(rblk->s_public));

	*newblk = &(rblk->s_public);

	return(ERR_SUCCESS);
}


mach_error_t usx_iobuf_mgr::io_free_block(io_block_t		blk)
{
	io_block_internal_t	rblk;

	DEBUG1(usx_debug, (0, "usx_iobuf_mgr::usx_io_free_block\n"));

	if (blk->owner == 0) {
		return(US_OBJECT_DEAD);
	}
	if (blk->owner != (int) this) {
		return(((default_iobuf_mgr*)(blk->owner))->io_free_block(blk));
	}

	rblk = (io_block_internal_t) (((char *) blk) - 
					sizeof(struct io_block_private));

	/*
	 * XXX Should we check for non-null blk->next ?
	 */

	msgDestroy(&(rblk->s_private.xmsg));
	rblk->s_public.owner = 0;
	free(rblk);

	return(ERR_SUCCESS);
}

/*
 * adjust the size of the xmsg associated with a block to coorespond to the
 * actual size of the block.
 */
void
usx_trim_xmsg_in_block_to_xmsg(io_block_t blk, Msg **xmsg)
{
	io_block_internal_t	rblk;

	rblk = (io_block_internal_t) (((char *) blk) - 
			sizeof(struct io_block_private));
	*xmsg = &(rblk->s_private.xmsg);
	if (blk->max_offset > blk->end_offset) {
		msgTruncate(*xmsg,
		(blk->end_offset - blk->min_offset));
	}

	if (blk->min_offset < blk->start_offset) {
		msgPopDiscard(*xmsg,
		blk->start_offset - blk->min_offset);
	}
}

mach_error_t
usx_iobuf_mgr::usx_convert_block_to_xmsg(io_block_t blk, Msg* newxmsg)
{
	Msg		*xmsg_ref;

	DEBUG1(usx_debug, (0, "usx_iobuf_mgr::usx_convert_block_to_msg\n"));

	usx_trim_xmsg_in_block_to_xmsg(blk, &xmsg_ref);
	msgConstructCopy(newxmsg, xmsg_ref);
	return(ERR_SUCCESS);
}

/*
 * struct for calling msgForEach of x-kernel land with.  Data needed to
 * place an xblock into a io_record_t (or list of io_block_t) as an io_block_t.
 */
typedef struct usx_foreach_xblock {
	int		owner;		/* "this" of msgForEach caller */
	io_record_t	rec;		/* iobufmgr record to put xblock in 
					   NULL if building block list*/
	io_block_t	blk_lst;	/* if building one */
	io_block_t	last_blk;	/* last block in blocklist */
	Msg		*xmsg_ptr;
} * usx_foreach_xblock_t;

/*
 * Iterator to be used with the xkernel msgForEach facility in order to
 * take a xblock in a xmsg and put it onto the end of a io_record_t.
 * If no record is supplied, then a list of io_block_t is built.
 */
bool usx_append_xblock(char *ptr, long len, void *arg)
{
	io_block_internal_t	rblk;
	unsigned int		blksize;
	usx_foreach_xblock_t	msgForEach_arg =
					(usx_foreach_xblock_t)arg;
	Msg			*head;

	if (0 == len) {
		return(TRUE);
	}

	rblk = (io_block_internal_t) malloc(sizeof(struct io_block_internal));
	if (rblk == NULL) {
		return(FALSE);
	}

	blksize = (unsigned int)len;

	rblk->s_public.owner = msgForEach_arg->owner;
	rblk->s_public.next = NULL;
	rblk->s_public.type = IOB_RAWDATA;

	rblk->s_public.min_offset = ((unsigned int) ptr) -
					((unsigned int) &(rblk->s_public));

	rblk->s_public.max_offset = rblk->s_public.min_offset + blksize;
	rblk->s_public.start_offset = rblk->s_public.min_offset;
	rblk->s_public.end_offset = rblk->s_public.max_offset;
	DEBUG0(usx_debug_counts, (0, "usx_iobuf_mgr::usx_append_block: count %d\n", ioblk_cursize(&(rblk->s_public))));

	head = &(rblk->s_private.xmsg);
	msgConstructEmpty(head);
	msgChopOff(msgForEach_arg->xmsg_ptr, head, blksize);

	if (NULL != msgForEach_arg->rec) {
		iorec_append_blk(msgForEach_arg->rec,&(rblk->s_public));
	}
	else {
		if (NULL == msgForEach_arg->blk_lst) {
			msgForEach_arg->blk_lst = &(rblk->s_public);
			msgForEach_arg->last_blk = msgForEach_arg->blk_lst;
		}
		else {
			msgForEach_arg->last_blk->next = &(rblk->s_public);
			msgForEach_arg->last_blk = 
					msgForEach_arg->last_blk->next;
		}
	}
	return(TRUE);
}

mach_error_t 
usx_iobuf_mgr::usx_convert_xmsg_to_block_lst(Msg *xmsg, io_block_t *blk_lst)
{
	struct usx_foreach_xblock msgForEach_arg;

	DEBUG1(usx_debug, (0, "usx_iobuf_mgr::usx_convert_xmsg_to_block_lst\n"));

        msgForEach_arg.owner = (int)this;
        msgForEach_arg.rec = NULL;
	msgForEach_arg.blk_lst = NULL;
        msgForEach_arg.xmsg_ptr = xmsg;
	msgForEach(xmsg, usx_append_xblock, &msgForEach_arg);
	*blk_lst = msgForEach_arg.blk_lst;

	return(ERR_SUCCESS);
}

mach_error_t 
usx_iobuf_mgr::usx_convert_xmsg_to_record(Msg		*xmsg,
					  io_record_t	*newrec)	/* OUT */
{
	struct usx_foreach_xblock msgForEach_arg;

	mach_error_t		ret;

	DEBUG1(usx_debug, (0, "usx_iobuf_mgr::usx_convert_xmsg_to_record\n"));
	ret = io_alloc_record(0,newrec);
	if (ret != ERR_SUCCESS) {
		*newrec = NULL;
    		return(ret);
	}

        msgForEach_arg.owner = (int)this;
        msgForEach_arg.rec = *newrec;
	msgForEach_arg.blk_lst = NULL;
        msgForEach_arg.xmsg_ptr = xmsg;
	msgForEach(xmsg, usx_append_xblock, &msgForEach_arg);

	return(ERR_SUCCESS);
}

mach_error_t 
usx_iobuf_mgr::usx_convert_record_to_xmsg(io_record_t	rec,
				  Msg		*newxmsg)	/* OUT */
{
	mach_error_t		ret;
	io_block_t		curblk;
	io_block_t		nxtblk;
	boolean_t		have_xmsg = FALSE;
	Msg			curxmsg;
	Msg			*curxmsg_ref;
	io_block_internal_t	rblk;

	DEBUG1(usx_debug, (0, "usx_iobuf_mgr::usx_convert_record_to_xmsg\n"));

	curblk = rec->first_block;
	while (curblk != NULL) {
		nxtblk = curblk->next;
		if (curblk->owner == 0) {
			return(US_OBJECT_DEAD);
		}
		if (curblk->owner != (int) this) {
			if (! have_xmsg) {
				msgConstructEmpty(newxmsg);
				have_xmsg = TRUE;
			}
			/*
			 * XXX Should use the memory from the foreign block
			 * to construct the xmsg.
			 */
			msgConstructBuffer(&curxmsg, ioblk_start(curblk),
				ioblk_cursize(curblk));

			ret = ((default_iobuf_mgr*)(curblk->owner))->io_free_block(curblk);
			if (ret != ERR_SUCCESS) {
				ERROR((0,
		"usx_convert_record_to_xmsg.io_free_block failed: %s\n",
						mach_error_string(ret)));
			}
			msgJoin(newxmsg, newxmsg, &curxmsg);
			msgDestroy(&curxmsg);
		}
		else {
			usx_trim_xmsg_in_block_to_xmsg(curblk, &curxmsg_ref);
			if (! have_xmsg) {
				msgConstructCopy(newxmsg, curxmsg_ref);
				have_xmsg = TRUE;
			}
			else {
				msgJoin(newxmsg, newxmsg, curxmsg_ref);
				msgDestroy(curxmsg_ref);
			}

			curblk->owner = 0;
			free(rblk);
			
		}
		curblk = nxtblk;
	}

	rec->first_block = NULL;
	rec->last_block = NULL;
	ret = io_free_record(rec);

	if (ret != ERR_SUCCESS) {
		ERROR((0,
		"usx_convert_record_to_xmsg.io_free_block failed: %s\n",
						mach_error_string(ret)));
	}

	return(ERR_SUCCESS);
}
