/* 
 * $RCSfile: vdrop.h,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:42:34 $
 */

#ifndef vdrop_h
#define vdrop_h
   
#  ifdef __STDC__

void	vdrop_init( XObj );

#  endif

#define VDROP_CTL	TMP2_CTL

#define VDROP_GETINTERVAL	(VDROP_CTL * MAXOPS + 0)
#define VDROP_SETINTERVAL	(VDROP_CTL * MAXOPS + 1)


#endif  ! vcache_h
