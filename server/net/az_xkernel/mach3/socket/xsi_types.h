/*
 * $RCSfile: xsi_types.h,v $
 *
 * Copyright (c) 1992  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1992/08/15 01:16:26 $
 * $Author: davidm $
 *
 * $Log: xsi_types.h,v $
 * Revision 1.2  1992/08/15  01:16:26  davidm
 * OOB message transmission is now implemented *and* tested.
 * Support for SIGURG was added.
 *
 * Revision 1.1  1992/07/22  00:14:07  davidm
 * Initial revision
 *
 */
#ifndef xsi_types_h
#define xsi_types_h

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>

#define XSI_SERVER_NAME	"xKernelSocketInterfacePort"
#define XSI_MAX_MSG_SIZE 4096

typedef int		xsi_clientid_t;
typedef struct sockaddr	*xsi_sockaddr_t;
typedef fd_set		xsi_fdset_t[1];
typedef struct timeval	xsi_timeval_t[1];
typedef struct iovec	xsi_iovec_t[1];
typedef struct msghdr   xsi_msghdr_t[1];
typedef char		*xsi_varbuf_t;

typedef int		xsi_select_mask_t;
#define SELECT_READFDS		0x1
#define	SELECT_WRITEFDS		0x2
#define SELECT_EXCEPTFDS	0x4
#define SELECT_TIMEOUT		0x8

#endif /* xsi_types_h */
