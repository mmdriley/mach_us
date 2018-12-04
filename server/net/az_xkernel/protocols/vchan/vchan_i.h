/*
 * vchan_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.3 $
 * $Date: 1993/02/01 22:32:37 $
 */

typedef struct {
    XObj 	*s;
    int 	size;
    int 	top;
    int		bottom;
    Semaphore 	available;
} SessnStack;


typedef struct {
    SessnStack 	stack;
    XObj	hlpType;
} SState;

typedef struct {
    IPhost	lHost, rHost;
    XObj	hlpType;
} ActiveKey;

typedef struct {
    Map	activeMap;
} PState;

#define ACTIVE_MAP_SIZE		101
#define DEFAULT_NUM_SESSNS	5


