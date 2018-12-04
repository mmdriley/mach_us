/*
 * chantest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.8 $
 * $Date: 1993/02/01 22:26:54 $
 */

/*
 * Ping-pong test of CHAN
 */

#include "xkernel.h"
#include "ip_host.h"

/*
 * These definitions describe the lower protocol
 */
#define HOST_TYPE IPhost
#define INIT_FUNC chantest_init
#define TRACE_VAR chantestp
#define PROT_STRING "chan"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

#define TRIPS 10
#define TIMES 1
#define DELAY 3
/*
 * Define to do timing calculations
 */
#define TIME
#define SAVE_SERVER_SESSN
#define RPCTEST
/* #define CUSTOM_ASSIGN */


static int lens[] = { 
  1000, 2000, 4000, 8000, 16000
};


#include "common_test.c"

static void
testInit()
{
}
