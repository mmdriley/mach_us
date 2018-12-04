/* 
 * $RCSfile: vdrop_i.h,v $
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:38:02 $
 */


#define	VDROP_ACT_MAP_SZ	11
#define	VDROP_PAS_MAP_SZ	11
#define VDROP_MAX_INTERVAL	20


typedef struct {
    Map		activeMap;
    Map 	passiveMap;
} PState;

typedef struct {
    int		interval;
    int		count;
} SState;

