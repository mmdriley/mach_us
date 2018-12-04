/*
 * vmux_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.2 $
 * $Date: 1993/02/01 22:33:27 $
 */

#define VMUX_MAX_INTERFACES 	10

#define VMUX_MAP_SZ		11

typedef struct {
    XObj	llpList[VMUX_MAX_INTERFACES+1];
    Map		passiveMap;
} PState;
