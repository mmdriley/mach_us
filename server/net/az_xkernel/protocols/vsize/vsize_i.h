/* 
 * vsize_i.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:31:36 $
 */

/* 
 * Indices into protocol and session down vectors
 */
#define SPI	0	/* Short Packet Index */
#define LPI	1	/* Large Packet Index */

#define DEFAULT_CUTOFF	512

typedef struct {
    Map		activeMap;
    Map 	passiveMap;
} PSTATE;

typedef struct {
    int		cutoff;
} SSTATE;

typedef	XObj	PassiveId;
typedef XObj	ActiveId[2];
