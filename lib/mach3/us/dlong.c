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
 * dlong.c
 *
 * Double length long arithmetic implementation.
 *
 * Michael B. Jones  --  3-Feb-1989
 *
 * Algorithms are based on Knuth's algorithms A, S, M and D from volume II,
 * section 4.3.
 */

/*
 * HISTORY:
 * $Log:	dlong.c,v $
 * Revision 2.3  94/07/08  18:13:45  mrt
 * 	Updated copyrights
 * 
 * Revision 2.2  90/07/26  12:41:34  dpj
 * 	First version
 * 	[90/07/24  14:45:35  dpj]
 * 
 * Revision 1.4  89/03/17  12:32:14  sanzi
 * 	Check into MOSERVER branch.
 * 	[89/02/27  17:09:16  mbj]
 * 
 * Revision 1.3  89/02/24  01:41:26  mbj
 * 	Removed obsolete C++ dlong output routine.
 * 
 * Revision 1.2  89/02/17  15:42:02  mbj
 * Change type names dlong -> dlong_t, dlong_digit -> dlong_digit_t and
 * dlong_ac -> dlong_ac_t as per prevailing naming conventions.
 * 
 * Revision 1.1  89/02/17  11:46:50  mbj
 * Initial revision
 * 
 * 13-Feb-89  Michael Jones (mbj) at Carnegie-Mellon University
 *	Converted all functions which returned dlongs to accept a *dlong
 *	result parameter, since the Vax and Sun C compilers return
 *	non-scalar values in static memory!
 */

#include "dlong.h"
#if c_plusplus || __cplusplus
#include "stream.h"
#endif c_plusplus || __cplusplus

/*
 * Helper macros and functions
 */

#define isneg(x) ((x).d[0] & (1 << (dlong_base_bits-1)))

#define dlong_gets_zero(x) (x).d[0] = (x).d[1] = (x).d[2] = (x).d[3] = 0

static dlong_cmp_digits
C_TYPED_ARGLIST((dlong_digit_t *l1, dlong_digit_t *l2, int n))
C_ARG_NAMES((l1, l2, n))
C_ARG_TYPES(dlong_digit_t *l1; dlong_digit_t *l2; int n;)
{
    if (l1 != l2)
	while (--n >= 0)
	    if (*l1++ != *l2++)
		return (l1[-1] > l2[-1] ? 1 : -1);
    return 0;
}

/*
 * Type conversion operations
 */

void int_to_dlong
C_TYPED_ARGLIST((dlong_t *result, int x))
C_ARG_NAMES((result, x))
C_ARG_TYPES(dlong_t *result; int x;)
{
    if (x < 0)
	result->d[0] = result->d[1] = ~0;
    else
	result->d[0] = result->d[1] = 0;
    result->d[2] = (x >> dlong_base_bits);
    result->d[3] = x;
}

void long_to_dlong
C_TYPED_ARGLIST((dlong_t *result, long x))
C_ARG_NAMES((result, x))
C_ARG_TYPES(dlong_t *result; long x;)
{
    if (x < 0)
	result->d[0] = result->d[1] = ~0;
    else
	result->d[0] = result->d[1] = 0;
    result->d[2] = (x >> dlong_base_bits);
    result->d[3] = x;
}

void u_int_to_dlong
C_TYPED_ARGLIST((dlong_t *result, unsigned int x))
C_ARG_NAMES((result, x))
C_ARG_TYPES(dlong_t *result; unsigned int x;)
{
    result->d[0] = result->d[1] = 0;
    result->d[2] = (x >> dlong_base_bits);
    result->d[3] = x;
}

void u_long_to_dlong
C_TYPED_ARGLIST((dlong_t *result, unsigned long x))
C_ARG_NAMES((result, x))
C_ARG_TYPES(dlong_t *result; unsigned long x;)
{
    result->d[0] = result->d[1] = 0;
    result->d[2] = (x >> dlong_base_bits);
    result->d[3] = x;
}

int dlong_to_int
C_TYPED_ARGLIST((dlong_t x))
C_ARG_NAMES((x))
C_ARG_TYPES(dlong_t x;)
{
    return (x.d[2] * dlong_base + x.d[3]);
}

long dlong_to_long
C_TYPED_ARGLIST((dlong_t x))
C_ARG_NAMES((x))
C_ARG_TYPES(dlong_t x;)
{
    return (x.d[2] * dlong_base + x.d[3]);
}

unsigned int dlong_to_u_int
C_TYPED_ARGLIST((dlong_t x))
C_ARG_NAMES((x))
C_ARG_TYPES(dlong_t x;)
{
    return (x.d[2] * dlong_base + x.d[3]);
}

unsigned long dlong_to_u_long
C_TYPED_ARGLIST((dlong_t x))
C_ARG_NAMES((x))
C_ARG_TYPES(dlong_t x;)
{
    return (x.d[2] * dlong_base + x.d[3]);
}

/*
 * Actual math operation implementations
 */

void dlong_add
C_TYPED_ARGLIST((dlong_t *result, dlong_t x, dlong_t y))
C_ARG_NAMES((result, x, y))
C_ARG_TYPES(dlong_t *result; dlong_t x; dlong_t y;)
{
    dlong_t z;
    register dlong_ac_t ac;

    z.d[3] = ac = x.d[3] + y.d[3];
    z.d[2] = ac = x.d[2] + y.d[2] + (ac >> dlong_base_bits);
    z.d[1] = ac = x.d[1] + y.d[1] + (ac >> dlong_base_bits);
    z.d[0] = ac = x.d[0] + y.d[0] + (ac >> dlong_base_bits);

    *result = z;
}

void dlong_neg
C_TYPED_ARGLIST((dlong_t *result, dlong_t x))
C_ARG_NAMES((result, x))
C_ARG_TYPES(dlong_t *result; dlong_t x;)
{
    dlong_t z;
    register dlong_ac_t ac;

    z.d[3] = ac = - (dlong_ac_t) x.d[3];
    z.d[2] = ac = - (dlong_ac_t) x.d[2] - ((ac >> dlong_base_bits) ? 1 : 0);
    z.d[1] = ac = - (dlong_ac_t) x.d[1] - ((ac >> dlong_base_bits) ? 1 : 0);
    z.d[0] = ac = - (dlong_ac_t) x.d[0] - ((ac >> dlong_base_bits) ? 1 : 0);

    *result = z;
}

void dlong_sub
C_TYPED_ARGLIST((dlong_t *result, dlong_t x, dlong_t y))
C_ARG_NAMES((result, x, y))
C_ARG_TYPES(dlong_t *result; dlong_t x; dlong_t y;)
{
    dlong_t z;
    register dlong_ac_t ac;

    z.d[3] = ac = x.d[3] - y.d[3];
    z.d[2] = ac = x.d[2] - y.d[2] - ((ac >> dlong_base_bits) ? 1 : 0);
    z.d[1] = ac = x.d[1] - y.d[1] - ((ac >> dlong_base_bits) ? 1 : 0);
    z.d[0] = ac = x.d[0] - y.d[0] - ((ac >> dlong_base_bits) ? 1 : 0);

    *result = z;
}

void dlong_mul
C_TYPED_ARGLIST((dlong_t *result, dlong_t u, dlong_t v))
C_ARG_NAMES((result, u, v))
C_ARG_TYPES(dlong_t *result; dlong_t u; dlong_t v;)
{
    int negative = 0;
    dlong_t w;
    int j;

    /*
     * Work in positive numbers
     */
    if (isneg(u)) {
	negative = 1;
	dlong_neg(&u, u);
    }
    if (isneg(v)) {
	negative = 1 - negative;
	dlong_neg(&v, v);
    }

    dlong_gets_zero(w);

    for (j = 3; j >= 0; j--) {
	if (v.d[j] == 0) {	/* Zero multplier? */
	    /* w.d[j] = 0; -- Already true */
	}
	else {
	    int i, k, iplusj;
	    dlong_ac_t v_j = v.d[j];

	    for (i = 3, k = 0, iplusj = i + j -3; iplusj >= 0; i--, iplusj--)
	    {
		dlong_ac_t t = u.d[i] * v_j + w.d[iplusj] + k;
		w.d[iplusj] = t;
		k = t / dlong_base;
	    }
	    /* w.d[j] = k; -- skip since j<m */
	}
    }

    if (negative)
	dlong_neg(result, w);
    else
	*result = w;
}

int dlong_cmp
C_TYPED_ARGLIST((dlong_t x, dlong_t y))
C_ARG_NAMES((x, y))
C_ARG_TYPES(dlong_t x; dlong_t y;)
{
    register int j;

    if (((x.d[0] ^ y.d[0]) & (1 << (dlong_base_bits-1))) != 0) {
        /* Signs differ */
	if ((x.d[0] & (1 << (dlong_base_bits-1))) == 0)
	    return 1;	/* x positive */
	else
	    return -1;	/* x negative */
    }

    for (j = 0; j <= 3; j++) {
        if (x.d[j] != y.d[j])
	    return (x.d[j] > y.d[j] ? 1 : -1);
    }
    return 0;
}

void dlong_divrem
C_TYPED_ARGLIST((dlong_t u, dlong_t v, dlong_t *quotient, dlong_t *remainder))
C_ARG_NAMES((u, v, quotient, remainder))
C_ARG_TYPES(dlong_t u; dlong_t v; dlong_t *quotient; dlong_t *remainder;)
{
    extern void bcopy C_ARG_DECLS((char *src, char *dst, int length));

    int negquotient = 0, negremainder = 0;
    int m_n, uoffset;
    int n, voffset;
    int m;

    dlong_digit_t d;
    dlong_digit_t uv[5];
    dlong_digit_t k;
    dlong_digit_t vv[4];
    dlong_digit_t v1;
    dlong_digit_t v2;
    dlong_digit_t qv[5];
    int j;
    dlong_digit_t nv[5];
    int dl, vl;
    int borrow;
    int ul;
    dlong_t q, r;

    /*
     * Work with positive numbers
     */
    if (isneg(u)) {
	negremainder = 1;
	dlong_neg(&u, u);
    }

    if (isneg(v)) {
	negquotient = ! negremainder;	/* Quotient sign opposite remainder */
	dlong_neg(&v, v);
    } else {
	negquotient = negremainder;	/* Quotient sign same as remainder */
    }

#if DEBUG_DLONG
    cout << "\nDividing " << u << " by " << v << "\n";
#endif DEBUG_DLONG

    /*
     * Find number of non-zero digits
     */
    for (m_n = 4, uoffset = 0;
	 uoffset < 4 && u.d[uoffset] == 0;
	 m_n--, uoffset++)
	;

    for (n = 4, voffset = 0;
	 voffset < 4 && v.d[voffset] == 0;
	 n--, voffset++)
	;

    m = m_n - n;

#if DEBUG_DLONG
    cout << "    uoffset = " << uoffset << ", voffset = " << voffset << "\n";
    cout << "    m_n = " << m_n << ", n = " << n << ", m = " << m << "\n";
#endif DEBUG_DLONG

    if (n == 0) {			/* Dividing by zero? */
#if DEBUG_DLONG
	cout << "Division by zero!\n";
#endif DEBUG_DLONG
	*remainder = u;
	int_to_dlong(quotient, 1 / n);	/* Respectfully divide by 0 */
	return;
    }

    /*
     * Special case for both values representable as unsigned longs.
     */
    if (n <= 2 && m_n <= 2) {
	dlong_ac_t lu = (u.d[2] * dlong_base + u.d[3]);
	dlong_ac_t lv = (v.d[2] * dlong_base + v.d[3]);

	u_long_to_dlong(quotient, lu / lv);
	if (negquotient) dlong_neg(quotient, *quotient);
		
	u_long_to_dlong(remainder, lu % lv);
	if (negremainder) dlong_neg(remainder, *remainder);

	return;
    }

    /*
     * Special case for one digit divisor.
     */
    if (n == 1) {
	dlong_t q;
	dlong_ac_t prevu = 0;
	dlong_digit_t vi = v.d[3];
	int r;

	dlong_gets_zero(q);
	for (r = uoffset; r < m_n + uoffset; r++) {
	    dlong_ac_t t = u.d[r] + prevu * dlong_base;
	    dlong_digit_t tmpq = t / vi;
	    q.d[r] = tmpq;
	    prevu = t - vi * tmpq;
	}

	if (negquotient)
	    dlong_neg(quotient, q);
	else
	    *quotient = q;

	u_long_to_dlong(remainder, prevu);
	if (negremainder) dlong_neg(remainder, *remainder);

#if DEBUG_DLONG
	cout << "    One digit divisor:  q = " << q << ", prevu = " << prevu << "\n";
#endif DEBUG_DLONG
	return;
    }

    /*
     * If len(u) < len(v) then u < v.
     */
    if (m_n < n) {
	dlong_gets_zero(*quotient);	/* Won't divide even once */

	if (negremainder)
	    dlong_neg(remainder, u);
	else
	    *remainder = u;

#if DEBUG_DLONG
	cout << "    Zero by m_n < n\n";
#endif DEBUG_DLONG
	return;
    }

    /*
     * If len(u) = len(v) then maybe u < v or u = v.
     */
    if (m_n == n) {
	int cmp = dlong_cmp_digits(&u.d[uoffset], &v.d[voffset], m_n);
	if (cmp < 0) {	/* u < v */
	    dlong_gets_zero(*quotient);	/* Won't divide even once */

	    if (negremainder)
		dlong_neg(remainder, u);
	    else
		*remainder = u;

#if DEBUG_DLONG
	    cout << "    Zero by u < v\n";
#endif DEBUG_DLONG
	    return;
	}
	else if (cmp == 0) {	/* u == v */
	    if (negquotient) {
		quotient->d[0] = quotient->d[1] =
		quotient->d[2] = quotient->d[3] = ~0;	/* -1 */
	    } else {
		quotient->d[0] = quotient->d[1] = quotient->d[2] = 0;
		quotient->d[3] = 1;			/* 1 */
	    }

	    dlong_gets_zero(*remainder);
#if DEBUG_DLONG
	    cout << "    One by u == v\n";
#endif DEBUG_DLONG
	    return;
	}    	
    }

    /*
     * End of special cases.  Begin division in full generality.
     */
    
    /*
     * D1a -- normalize.
     *	    d = base / (v1+1);
     */
    d = dlong_base / (v.d[voffset] + 1);

#if DEBUG_DLONG
    cout << "    d = " << d << "\n";
#endif DEBUG_DLONG

    /*
     * D1b -- normalize.
     *	    u[0..m+n] = u[1..m+n] * d;
     */
    if (d == 1) {	/* uv = u */
	uv[0] = 0;
	bcopy((char *) &u.d[uoffset], (char *) &uv[1],
	      m_n * sizeof(dlong_digit_t));
    }
    else {		/* uv = u * d */
	int Oi, i;
	k = 0;
	for (Oi = m_n - 1 + uoffset, i = m_n;
	     i > 0; Oi--, i--)
	{
	    dlong_ac_t t = u.d[Oi] * d + k;
	    uv[i] = t;
	    k = t / dlong_base;
	}
	uv[0] = k;
    }
#if DEBUG_DLONG
    {
	cout << "    uv = ( ";
	for (int i = 0; i <= m_n; i++) {
	    cout << uv[i] << " ";
	}
	cout << ")\n";
    }
#endif DEBUG_DLONG
    /*
     * D1c -- normalize.
     *	    v[1..n] = v[1..n] * d;
     */
    if (d == 1) {	/* vv = v */
	bcopy((char *) &v.d[voffset], (char *) &vv[0],
	      n * sizeof(dlong_digit_t));
    }
    else {		/* vv = v * d */
	int Oi, i;
	k = 0;
	for (Oi = n - 1 + voffset, i = n - 1;
	     i >= 0; Oi--, i--)
	{
	    dlong_ac_t t = v.d[Oi] * d + k;
	    vv[i] = t;
	    k = t / dlong_base;
	}
    }
#if DEBUG_DLONG
    {
	cout << "    vv = ( ";
	for (int i = 0; i <= n-1; i++) {
	    cout << vv[i] << " ";
	}
	cout << ")\n";
    }
#endif DEBUG_DLONG
    /*
     * D2, D7 -- loop for j = 0 to m.
     */
    v1 = vv[0];
    v2 = vv[1];

    for (j = 0; j <= m; j++) {
	dlong_ac_t q_hat;
	dlong_ac_t u_j, u_j1, u_j2;

#if DEBUG_DLONG
	cout << "    j = " << j << " ...\n";
#endif DEBUG_DLONG

	/*
	 * D3 -- calculate q^.
	 *  if u[j] = v[1] then q^ = base-1
	 *		   else q^ = (u[j] * base + u[j+1]) / v[1];
	 *  while v[2] * q^ >
	 *	  (u[j] * base + u[j+1] - q^ * v[1]) * base + u[j+2]
	 *  do q^ = q^ - 1;
	 */

	if (uv[j] == v1)
	    q_hat = dlong_base - 1;
	else
	    q_hat = (uv[j] * dlong_base + uv[j+1]) / v1;

#if DEBUG_DLONG
	cout << "        q_hat = " << q_hat << "\n";
#endif DEBUG_DLONG

	u_j = uv[j];
	u_j1 = uv[j+1];
	u_j2 = uv[j+2];

	for ( ; ; q_hat--) {	/* While loop in D3 comment */
	    dlong_ac_t u_j_q_hat =
		(u_j * dlong_base + u_j1) - (q_hat * v1);
	    if (u_j_q_hat / dlong_base != 0)
		break;	/* v[2] * q^ not > result with 3 "digits" */
	    u_j_q_hat = u_j_q_hat * dlong_base + u_j2;
	    if ((v2 * q_hat) <= u_j_q_hat)
		break;	/* while test failed */
	}

#if DEBUG_DLONG
	cout << "        q_hat = " << q_hat << "\n";
#endif DEBUG_DLONG

	/*
	 * D4 -- multiply and subtract.
	 *	u[j..j+n] = u[j..j+n] - q^ * v[1..n];
	 */

	/* nv = q^ * v[1..n] */
	k = 0;
	for (dl = n, vl = n-1; vl >= 0; vl--, dl--) {
	    dlong_ac_t t = vv[vl] * q_hat + k;
	    nv[dl] = t;
	    k = t / dlong_base;
	}
	nv[0] = k;
#if DEBUG_DLONG
	{
	    cout << "        nv = ( ";
	    for (int i = 0; i <= n; i++) {
		cout << nv[i] << " ";
	    }
	    cout << ")\n";
	}
#endif DEBUG_DLONG
	/* u[j..j+n] = u[j..j+n] - nv[0..n] */
	borrow = 0;
	for (ul = j + n, dl = n; dl >= 0; dl--, ul--) {
	    dlong_ac_t t = uv[ul] - nv[dl] - borrow;
	    uv[ul] = t;
	    borrow = (t / dlong_base) ? 1 : 0;
	}
#if DEBUG_DLONG
	{
	    cout << "        uv[j..j+n] = ( ";
	    for (int i = j; i <= j + n; i++) {
		cout << uv[i] << " ";
	    }
	    cout << ")\n";
	}
#endif DEBUG_DLONG
	/*
	 * D5 -- test remainder.
	 *	q[j] = q^;
	 *	if u[j] < 0 then
	 * D6 -- add back.
	 *	    u[j..j+n] = u[j..j+n] + 0,v[1..n];
	 *	    q[j] = q[j] - 1;
	 */
	qv[j] = q_hat;

#if DEBUG_DLONG
	cout << "        qv[j] = " << qv[j] << "\n";
	cout << "        borrow = " << borrow << "\n";
#endif DEBUG_DLONG

	if (borrow) {
	    k = 0;
	    for (ul = j + n, vl = n - 1; vl >= 0; vl--, ul--) {
		dlong_ac_t t = uv[ul] + vv[vl] + k;
		uv[ul] = t;
		k = t / dlong_base;
	    }
	    uv[j] += k;
	    qv[j] -= 1;
#if DEBUG_DLONG
	    {
		cout << "            uv[j..j+n] = ( ";
		for (int i = j; i <= j + n; i++) {
		    cout << uv[i] << " ";
		}
		cout << ")\n";
	    }
	    cout << "            qv[j] = " << qv[j] << "\n";
#endif DEBUG_DLONG
	}
    }

    /*
     * D8 -- unnormalize.
     *	    q[0..m] is quotient
     *	    u[m+1..m+n] / d is remainder
     */
    dlong_gets_zero(q);
    bcopy((char *) &qv[0], (char *) &q.d[3-m], (m+1) * sizeof(dlong_digit_t));

#if DEBUG_DLONG
    cout << "    q = " << q << "\n";
#endif DEBUG_DLONG

    if (negquotient)
	dlong_neg(quotient, q);
    else
	*quotient = q;

    /* Divide u[m+1..m+n] by d */
    dlong_gets_zero(r);
    if (d == 1)
	bcopy((char *) &uv[m+1], (char *) &r.d[4-n],
	      n * sizeof(dlong_digit_t));	/* r[4-n..3] = uv[m+1..m+n] */
    else {
	dlong_ac_t prevu = 0;
	int rl;

	/* divide by digit d */
	for (rl = 4 - n, ul = m + 1; ul <= m_n; ul++, rl++) {
	    dlong_ac_t t = uv[ul] + prevu * dlong_base;
	    dlong_digit_t tmpq = t / d;
	    r.d[rl] = tmpq;
	    prevu = t - d * tmpq;
	}
    }

#if DEBUG_DLONG
    cout << "    r = " << r << "\n";
#endif DEBUG_DLONG

    if (negremainder)
	dlong_neg(remainder, r);
    else
	*remainder = r;
}

void dlong_div
C_TYPED_ARGLIST((dlong_t *result, dlong_t u, dlong_t v))
C_ARG_NAMES((result, u, v))
C_ARG_TYPES(dlong_t *result; dlong_t u; dlong_t v;)
{
    dlong_t quotient, remainder;
    dlong_divrem(u, v, result, &remainder);
}

void dlong_rem
C_TYPED_ARGLIST((dlong_t *result, dlong_t u, dlong_t v))
C_ARG_NAMES((result, u, v))
C_ARG_TYPES(dlong_t *result; dlong_t u; dlong_t v;)
{
    dlong_t quotient, remainder;
    dlong_divrem(u, v, &quotient, result);
}
