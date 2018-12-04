/* 
 * bidctl_id.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:36:34 $
 */


#include "xkernel.h"
#include "bidctl_i.h"

/* 
 * Determining local boot id's and query tags
 */


BootId
bidctlNewId()
{
    XTime	t;

    xGetTime(&t);
    return t.sec;
}


/* 
 * bidctlReqTag -- this should be unique for every request and should
 * overflow slowly enough to be sure it won't run into duplicates.  We
 * munge the time value in such away that the id changes every 256
 * usec and has an overflow time > 18 hours.
 */
BootId
bidctlReqTag()
{
    XTime	t;

    xGetTime(&t);
    return t.sec << 16 | ( (t.usec & 0x00ffff00) >> 8);
}
