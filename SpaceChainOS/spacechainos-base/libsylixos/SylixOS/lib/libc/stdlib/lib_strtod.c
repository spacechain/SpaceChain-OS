/*$OpenBSD: strtod.c,v 1.22 2006/05/19 14:15:27 thib Exp $ */
/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991 by AT&T.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/* Please send bug reports to
        David M. Gay
        AT&T Bell Laboratories, Room 2C-463
        600 Mountain Avenue
        Murray Hill, NJ 07974-2070
        U.S.A.
        dmg@research.att.com or research!dmg
 */

/* strtod for IEEE-, VAX-, and IBM-arithmetic machines.
 *
 * This strtod returns a nearest machine number to the input decimal
 * string (or sets errno to ERANGE).  With IEEE arithmetic, ties are
 * broken by the IEEE round-even rule.  Otherwise ties are broken by
 * biased rounding (add half and chop).
 *
 * Inspired loosely by William D. Clinger's paper "How to Read Floating
 * Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
 *
 * Modifications:
 *
 *      1. We only require IEEE, IBM, or VAX double-precision
 *              arithmetic (not IEEE double-extended).
 *      2. We get by with floating-point arithmetic in a case that
 *              Clinger missed -- when we're computing d * 10^n
 *              for a small integer d and the integer n is not too
 *              much larger than 22 (the maximum integer k for which
 *              we can represent 10^k exactly), we may be able to
 *              compute (d*10^k) * 10^(e-k) with just one roundoff.
 *      3. Rather than a bit-at-a-time adjustment of the binary
 *              result in the hard case, we use floating-point
 *              arithmetic to determine the adjustment to within
 *              one bit; only in really hard cases do we need to
 *              compute a second residual.
 *      4. Because of 3., we don't need a large table of powers of 10
 *              for ten-to-e (just some small tables, e.g. of 10^k
 *              for 0 <= k <= 22).
 */

/*
 * #define IEEE_LITTLE_ENDIAN for IEEE-arithmetic machines where the least
 *      significant byte has the lowest address.
 * #define IEEE_BIG_ENDIAN for IEEE-arithmetic machines where the most
 *      significant byte has the lowest address.
 * #define Long int on machines with 32-bit ints and 64-bit longs.
 * #define Sudden_Underflow for IEEE-format machines without gradual
 *      underflow (i.e., that flush to zero on underflow).
 * #define IBM for IBM mainframe-style floating-point arithmetic.
 * #define VAX for VAX-style floating-point arithmetic.
 * #define Unsigned_Shifts if >> does treats its left operand as unsigned.
 * #define No_leftright to omit left-right logic in fast floating-point
 *      computation of dtoa.
 * #define Check_FLT_ROUNDS if FLT_ROUNDS can assume the values 2 or 3.
 * #define RND_PRODQUOT to use rnd_prod and rnd_quot (assembly routines
 *      that use extended-precision instructions to compute rounded
 *      products and quotients) with IBM.
 * #define ROUND_BIASED for IEEE-format with biased rounding.
 * #define Inaccurate_Divide for IEEE-format with correctly rounded
 *      products but inaccurate quotients, e.g., for Intel i860.
 * #define Just_16 to store 16 bits per 32-bit Long when doing high-precision
 *      integer arithmetic.  Whether this speeds things up or slows things
 *      down depends on the machine and the number being converted.
 * #define Bad_float_h if your system lacks a float.h or if it does not
 *      define some or all of DBL_DIG, DBL_MAX_10_EXP, DBL_MAX_EXP,
 *      FLT_RADIX, FLT_ROUNDS, and DBL_MAX.
 * #define MALLOC your_malloc, where your_malloc(n) acts like malloc(n)
 *      if memory is available and otherwise does something you deem
 *      appropriate.  If MALLOC is undefined, malloc will be invoked
 *      directly -- and assumed always to succeed.
 */

/**************************** HANHUI ***************************/
#include "SylixOS.h"

#if LW_KERN_FLOATING > 0

#include <math.h>
#include "limits.h"
#include "ctype.h"

#define LARGE_EXP   (INT_MAX / 100)

#define F0  1.25
#define F1  F0*F0
#define F2  F1*F1
#define F3  F2*F2
#define F4  F3*F3
#define F5  F4*F4
#define F6  F5*F5
#define F7  F6*F6
#define F8  F7*F7
#define F9  F8*F8
#define F10 F9*F9
#define F11 F10*F10
#define F12 F11*F11
#define F13 F12*F12
#define F14 F13*F13

static const double f125[] = {
    F0, F1, F2, F3, F4, F5, F6, F7, F8,
    F9, 1.0
};

#define F125MAX     (&f125[sizeof(f125) / sizeof(double) - 1])
#define DBL_DIG     16

/*
 * strtod
 */
double  lib_strtod (const char *nptr, char **endptr)
{
    int           c, neg, ndigits, iexp, iexp1, legal;
    double        val, adj_exp;
    const double  *fp;
    
    if (endptr) {
        *endptr = (char *)nptr;
    }
    
    while (lib_isspace(c = *nptr++));
    neg = 0;
    
    switch (c) {

    case '-':
        neg = 1;
        /* fall through */
    case '+':
        c = *nptr++;
        break;
    }
    
    legal = 0;
    ndigits = 0;
    iexp = 0;
    val = 0.0;
    
    while (lib_isdigit(c)) {    /* collect everything before . */
        if (ndigits > DBL_DIG + 2) {
            iexp++;
        
        } else if (c != '0' || ndigits != 0) {
            val = val*10 + (c-'0');
            ndigits++;
        }
        legal = 1;
        c = *nptr++;
    }
    
    if (c == '.') {
        c = *nptr++;
        while (lib_isdigit(c)) {    /* collect everything after . */
            if (ndigits <= DBL_DIG + 2) {
                if (c != '0' || ndigits != 0) {
                    val = val*10 + (c-'0');
                    ndigits++;
                }
                iexp--;
            }
            legal = 1;
            c = *nptr++;
        }
    }
    
    if (legal && endptr) {
        *endptr = (char *)nptr - 1;
    }
    
    legal = 0;
    if (neg) {
        val = -val;
    }
    iexp1 = 0;
    neg = 0;
    if (c == 'e' || c == 'E') {
        switch (c = *nptr++) {
        
        case '-':
            neg = 1;
            /* fall through */
        case '+':
            c = *nptr++;
            break;
        }
        while (lib_isdigit(c)) {    /* collect exponent */
            if (iexp1 < LARGE_EXP) {
                iexp1 = iexp1*10 + c - '0';
            }
            legal = 1;
            c = *nptr++;
        }
        if (neg) {
            iexp1 = -iexp1;
        }
        iexp += iexp1;
    }
    
    if (legal && endptr) {
        *endptr = (char *)nptr - 1;
    }
    
    /* We now have result = val * 10**iexp. Change that to
       val1 * 2**iexp1 => val * 1.25**iexp * 2**(iexp*3) */
       
    adj_exp = 1.;
    iexp1 = iexp;
    if (iexp1 < 0) {
        iexp1 = -iexp1;
    }
    fp = f125;
    while (iexp1) {    /* compensate mantissa */
        if (iexp1 & 1) {
            adj_exp *= *fp;
        }
        iexp1 >>= 1;
        if (fp < F125MAX) {
            fp++;
        }
    }
    
    if (iexp >= 0) {
        val *= adj_exp;
    } else {
        val /= adj_exp;
    }
    iexp *= 3;
    
    return  (ldexp(val,iexp));
}

/*
 * strtold
 */
long double  lib_strtold (const char *nptr, char **endptr)
{
    return  (long double)lib_strtod(nptr, endptr);
}

#endif  /*  LW_KERN_FLOATING > 0  */
/*
 * END
 */
