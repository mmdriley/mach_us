/*
 * utils.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.20 $
 * $Date: 1993/02/02 00:05:37 $
 */

#include <stdio.h>

#include "xk_debug.h"
#include "x_libc.h"
#include "x_util.h"


void
Kabort(s)
    char *s;
{
    fprintf(stderr, "xkernel abort: %s\n", s);
    abort();
}

