/*
 * tcp_port.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 22:25:45 $
 */


typedef unsigned short TCPport;

#define MAX_PORT		0xffff
#define MIN_PORT		0

#define NAME 			tcp
#define PROT_NAME		"tcp"
#define TRACE_VAR		tcpp
#define	FIRST_USER_PORT		0x100
#define PORT_MAP_SIZE		201

#include "port_mgr.h"


