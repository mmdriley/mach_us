/*
 * udp_port.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.6 $
 * $Date: 1993/02/01 22:28:11 $
 */


typedef unsigned short UDPport;

#define NAME 			udp
#define PROT_NAME		"udp"
#define TRACE_VAR		udpp
#define MAX_PORT		0xffff
#define	FIRST_USER_PORT		0x100
#define PORT_MAP_SIZE		201

#include "port_mgr.h"
