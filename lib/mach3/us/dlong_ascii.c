/* 
 * Mach Operating System
 * Copyright (c) 1994,1993,1992,1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they made and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
/*
 * dlong_ascii.c
 *
 * Double length long arithmetic string conversion and string i/o routines.
 *
 * Michael B. Jones  --  22-Feb-1989
 */

/*
 * HISTORY:
 * $Log:	dlong_ascii.c,v $
 * Revision 2.3  94/07/08  18:13:46  mrt
 * 	Updated copyrights
 * 
 * Revision 2.2  90/07/26  12:41:37  dpj
 * 	First version
 * 	[90/07/24  14:45:45  dpj]
 * 
 * Revision 1.1  89/02/24  01:34:48  mbj
 * Initial revision
 * 
 */

#include "dlong.h"
#include <ctype.h>
#include <strings.h>

#if c_plusplus || __cplusplus
#include <stream.h>
#else
#include <stdio.h>
#endif c_plusplus || __cplusplus

static char dlong_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

void dlong_to_string
C_TYPED_ARGLIST((char *str, dlong_t x, int base))
C_ARG_NAMES((str, x, base))
C_ARG_TYPES(char *str; dlong_t x; int base;)
/*
 * Represent the dlong "x" as a number in base "base" in string "str".
 */
{
    dlong_t t, b, minus_b;
    dlong_t rem, div;
    register char *s = str;
    char digits[64];		/* Digits stored backwards */
    register char *dp = digits;

    INT_TO_DLONG(&b, (base >= 2 && base <= 36) ? base : 10);
    DLONG_NEG(&minus_b, b);

    /*
     * Work with a negative temporary since 2**63 is not representable as a
     * dlong_t, but -2**63 is.
     */
    if (DLONG_SGN(x) < 0) {
	*s++ = '-';
	t = x;
    } else {
	dlong_neg(&t, x);
    }

    while (dlong_cmp(t, minus_b) <= 0) {
	dlong_divrem(t, b, &div, &rem);
	*dp++ = dlong_digits[-dlong_to_int(rem)];
	t = div;
    }
    *s++ = dlong_digits[-dlong_to_int(t)];

    while (dp != digits) *s++ = *--dp;	/* Unreverse digits */
    *s++ = 0;	/* Null-terminate string */
}

char *string_to_dlong
C_TYPED_ARGLIST((dlong_t *x, char *str, int base))
C_ARG_NAMES((x, str, base))
C_ARG_TYPES(dlong_t *x; char *str; int base;)
/*
 * Convert a string to a dlong.  Return a pointer to next unread character
 * in the input string.
 */
{
    dlong_t t, d;
    int negative = 0;
    dlong_t b, minus_b;
    register char c;
    register char *s = str;
    register char *dx;

    /*
     * Work with a negative temporary so -2**63 is handled correctly.
     */

    INT_TO_DLONG(&b, base);
    DLONG_NEG(&minus_b, b);

    DLONG_ZERO(&t);

    while (isspace(*s)) s++;	/* Skip leading blankspace */
    if (*s == '-') {
	negative = 1;
	s++;
    }

    while (1) {
	c = *s;
	if isupper(c) c = tolower(c);
	if (!c || ! (dx = index(dlong_digits, c))) {
	    if (negative) {
		*x = t;			/* Already negated */
	    } else {
		DLONG_NEG(x, t);	/* Negate negative temporary */
	    }
	    return s;
	}
	int_to_dlong(&d, dx - dlong_digits);	/* Digit value */
	dlong_mul(&t, t, b);
	DLONG_SUB(&t, t, d);
	s++;
    }
}

#if c_plusplus || __cplusplus
#else c_plusplus || __cplusplus

int dlong_print
C_TYPED_ARGLIST((FILE *fp, dlong_t x, int base))
C_ARG_NAMES((fp, x, base))
C_ARG_TYPES(FILE *fp; dlong_t x; int base;)
/*
 * Print a dlong 
 */
{
    char representation[68];	/* Long enough even for base 2 */

    dlong_to_string(representation, x, base);
    return fprintf(fp, "%s", representation);
}
#endif c_plusplus || __cplusplus
