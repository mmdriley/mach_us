/*
 * $RCSfile: xsi_services.h,v $
 *
 * Copyright (c) 1992  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1992/08/15 01:16:26 $
 * $Author: davidm $
 *
 * $Log: xsi_services.h,v $
 * Revision 1.2  1992/08/15  01:16:26  davidm
 * OOB message transmission is now implemented *and* tested.
 * Support for SIGURG was added.
 *
 * Revision 1.1  1992/08/01  18:42:25  davidm
 * Initial revision
 *
 */
#ifndef xsi_services_h
#define xsi_services_h

#include "xksocket.h"

extern client_t *so_client_lookup(int client_id, boolean_t inhibit_bind);
extern socket_t *so_alloc_sock(void);
extern void	so_shutdown(socket_t *sd, int how);
extern void	so_new_session(socket_t *sd);

#endif /* xsi_services_h */
