/*
 * ethtest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.20 $
 * $Date: 1993/02/01 22:26:20 $
 */

/*
 * Ping-pong test of the ethernet protocol
 */

#include "xkernel.h"
#include "eth.h"

#define HOST_TYPE ETHhost
#define INIT_FUNC ethtest_init
#define TRACE_VAR ethtestp
#define PROT_STRING "eth"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0x0000, 0x0000, 0x0000 };
static HOST_TYPE ClientAddr = { 0x0000, 0x0000, 0x0000 };

#define TRIPS 100
#define TIMES 1
#define DELAY 3

/*
 * Define to do timing calculations
 */
#define TIME	 


static int lens[] = { 
  1, 200, 400, 600, 800, 1000, 1200
};


#define SAVE_SERVER_SESSN
#define STR2HOST	str2ethHost


#include "common_test.c"

static void
testInit()
{
}
