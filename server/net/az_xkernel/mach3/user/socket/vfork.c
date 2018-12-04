/*
 * $RCSfile: vfork.c,v $
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/02 00:19:29 $
 * $Author: menze $
 *
 * $Log: vfork.c,v $
 * Revision 1.2  1993/02/02  00:19:29  menze
 * copyright change
 *
 * Revision 1.1  1992/07/22  18:39:58  davidm
 * Initial revision
 *
 */
#include "xsi_main.h"

int
vfork()
{
    /* approximate by using normal fork(): */
    return fork();
} /* vfork */

			/*** end of vfork.c ***/
