/*
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * This license only applies to the source code demarcated by the page-wide 
 * line of asterisks.  The rest of the source code in this file may be
 * subject to a different open-source or proprietary software license.
 */

#include "stdio.h"

#if LW_KERN_FLOATING > 0

#include <math.h>
#include "floatio.h"

#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * roundcvt function
 */
static char *roundcvt (double  fract, int *exp, char *start, char *end, char ch, BOOL *pDoSign)
{
    double tmp;

    if (fract) {
        modf(fract * 10, &tmp);
       
    } else {
        tmp = to_digit(ch);
    }
    
    if (tmp > 4) {
        for (;; --end) {
            if (*end == '.') {
                --end;
            }
            
            if (++*end <= '9') {
                break;
            }
            
            *end = '0';
            if (end == start) {
                if (exp) {          /* e/E; increment exponent */
                    *end = '1';
                    ++*exp;
                
                } else {            /* f; add extra digit */
                    *--end = '1';
                    --start;
                }
                break;
            }
        }
    
    } else if (*pDoSign) { /* ``"%.3f", (double)-0.0004'' gives you a negative 0. */
        for (;; --end) {
            if (*end == '.') {
                --end;
            }

            if (*end != '0') {
                break;
            }

            if (end == start) {
                *pDoSign = FALSE;
            }
        }
    }

    return  (start);
}

/*
 * exponentcvt function
 */

static char *exponentcvt (char *p, int  exp, int  fmtch)
{
    char *t;
    char expbuf[MAXEXP];

    *p++ = fmtch;
    if (exp < 0) {
        exp = -exp;
        *p++ = '-';
    
    } else {
        *p++ = '+';
    }

    t = expbuf + MAXEXP;

    if (exp > 9) {
        do {
            *--t = to_char(exp % 10);
        } while ((exp /= 10) > 9);

        *--t = to_char(exp);

        for (; t < expbuf + MAXEXP; *p++ = *t++);
    
    } else {
        *p++ = '0';
        *p++ = to_char(exp);
    }

    return  (p);
}

/*
 * cvt function
 */
int lib_cvtfloat (double  number, int  prec, BOOL  doAlt, int  fmtch, BOOL *pDoSign, char *startp, char *endp)
{
    char   *p;
    char   *t;
    double  fract;
    int     dotrim;
    int     expcnt;
    int     gformat;
    int     nonZeroInt = FALSE;
    double  integer;
    double  tmp;

    dotrim = expcnt = gformat = 0;

    if (number < 0) {
        number = -number;
        *pDoSign = TRUE;
    
    } else {
        *pDoSign = FALSE;
    }

    fract = modf(number, &integer);

    if (integer != (double)0.0) {
        nonZeroInt = TRUE;
    }

    t = ++startp;            /* get an extra slot for rounding. */

    /* get integer portion of number; put into the end of the buffer; the
     * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
     */

    for (p = endp - 1; integer; ++expcnt) {
        tmp  = modf(integer / 10, &integer);
        *p-- = to_char((int)((tmp + .01) * 10));
    }

    switch (fmtch) {
    
    case 'f':
        /* reverse integer into beginning of buffer */
        if (expcnt) {
            for (; ++p < endp; *t++ = *p);
        } else {
            *t++ = '0';
        }

        /* if precision required or alternate flag set, add in a
         * decimal point.
         */

        if (prec || doAlt) {
            *t++ = '.';
        }

        /* if requires more precision and some fraction left */

        if (fract) {
            if (prec) {
                do {
                    fract = modf(fract * 10, &tmp);
                    *t++  = to_char((int)tmp);
                } while (--prec && fract);
            }

            if (fract) {
                startp = roundcvt(fract, (int *)NULL, startp, t - 1,
                                  (char)0, pDoSign);
            }
        }

        for (; prec--; *t++ = '0');
        break;

    case 'e':
    case 'E':
eformat:    
        if (expcnt) {
            *t++ = *++p;

            if (prec || doAlt) {
                *t++ = '.';
            }

            /* if requires more precision and some integer left */

            for (; prec && ++p < endp; --prec) {
                *t++ = *p;
            }

            /* if done precision and more of the integer component,
             * round using it; adjust fract so we don't re-round
             * later.
             */

            if (!prec && ++p < endp) {
                fract  = 0;
                startp = roundcvt((double)0, &expcnt, startp, t - 1, *p,
                                  pDoSign);
            }

            --expcnt;    /* adjust expcnt for dig. in front of decimal */
        
        } else if (fract) { /* until first fractional digit, decrement exponent */
            /* adjust expcnt for digit in front of decimal */

            for (expcnt = -1;; --expcnt) 
            {
                fract = modf(fract * 10, &tmp);
                if (tmp) {
                    break;
                }
            }

            *t++ = to_char((int)tmp);

            if (prec || doAlt) {
                *t++ = '.';
            }
        
        } else {
            *t++ = '0';
            if (prec || doAlt) {
                *t++ = '.';
            }
        }

        /* if requires more precision and some fraction left */
        if (fract) {
            if (prec) {
                do {
                    fract = modf(fract * 10, &tmp);
                    *t++ = to_char((int)tmp);
                } while (--prec && fract);
            }

            if (fract) {
                startp = roundcvt(fract, &expcnt, startp, t - 1,(char)0,
                                  pDoSign);
            }
        }

        for (; prec--; *t++ = '0'); /* if requires more precision */
            
        /* unless alternate flag, trim any g/G format trailing 0's */

        if (gformat && !doAlt) {
            while (t > startp && *--t == '0');

            if (*t == '.') {
                --t;
            }
            ++t;
        }

        t = exponentcvt(t, expcnt, fmtch);
        break;

    case 'g':
    case 'G':
        /* a precision of 0 is treated as a precision of 1. */
        if (!prec) {
            ++prec;
        }

        /*
         * ``The style used depends on the value converted; style e
         * will be used only if the exponent resulting from the
         * conversion is less than -4 or greater than the precision.''
         *    -- ANSI X3J11
         */

        if (expcnt > prec || (!expcnt && fract && fract < .0001)) {
            /*
             * g/G format counts "significant digits, not digits of
             * precision; for the e/E format, this just causes an
             * off-by-one problem, i.e. g/G considers the digit
             * before the decimal point significant and e/E doesn't
             * count it as precision.
             */
            --prec;
            fmtch  -= 2;        /* G->E, g->e */
            gformat = 1;
            goto eformat;
        }

        /*
         * reverse integer into beginning of buffer,
         * note, decrement precision
         */
        if (expcnt) {
            for (; ++p < endp; *t++ = *p, --prec);
        } else {
            *t++ = '0';
        }
        
        /*
         * if precision required or alternate flag set, add in a
         * decimal point.  If no digits yet, add in leading 0.
         */

        if (prec || doAlt) {
            dotrim = 1;
            *t++ = '.';
        
        } else {
            dotrim = 0;
        }

        /* if requires more precision and some fraction left */

        if (fract) {
            if (prec) {
            /*
             * If there is a zero integer portion, so we can't count any
             * of the leading zeros as significant.  Roll 'em on out until
             * we get to the first non-zero one.
             */
                if (!nonZeroInt) {
                    do {
                        fract = modf(fract * 10, &tmp);
                        *t++ = to_char((int)tmp);
                    } while(!tmp);
                    prec--;
                }

            /*
             * Now add on a number of digits equal to our precision.
             */
                while (prec && fract) {
                    fract = modf(fract * 10, &tmp);
                    *t++ = to_char((int)tmp);
                    prec--;
                }
            }
            
            if (fract) {
                startp = roundcvt(fract, (int *)NULL, startp, t - 1,
                                  (char)0, pDoSign);
            }
        }

        /* alternate format, adds 0's for precision, else trim 0's */

        if (doAlt) {
            for (; prec--; *t++ = '0');
        
        } else if (dotrim) {
            while (t > startp && *--t == '0');
            if (*t != '.') {
                ++t;
            }
        }

    default :
        break;
    }

    return  (int)(t - startp);
}

#endif /* LW_KERN_FLOATING */
/*
 * end
 */
