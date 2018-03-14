/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <sys/cdefs.h>
#include <SylixOS.h>
#include <stdarg.h>
#include <inttypes.h>
#include <ctype.h>
#include <limits.h>

/*
 * lib_strtol
 */
__weak_reference(lib_strtol, strtol);

long lib_strtol (const char *nptr, char **endptr, register int base)
{
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = (int)(cutoff % (unsigned long)base);
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}

/*
 * lib_strtoul
 */
__weak_reference(lib_strtoul, strtoul);

unsigned long lib_strtoul (const char *nptr, char **endptr, register int base)
{
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (int)((unsigned long)ULONG_MAX % (unsigned long)base);
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULONG_MAX;
		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}

/*
 * lib_strtoll
 */
__weak_reference(lib_strtoll, strtoll);

long long lib_strtoll (const char *nptr, char **endptr, int base)
{
    return  (long long)lib_strtoimax(nptr, endptr, base);
}

/*
 * lib_strtoull
 */
__weak_reference(lib_strtoull, strtoull);

unsigned long long lib_strtoull (const char *nptr, char **endptr, int base)
{
    return  (unsigned long long)lib_strtoumax(nptr, endptr, base);
}

/*
 * lib_itoa
 */
__weak_reference(lib_itoa, itoa);

char *lib_itoa (int value, char *string, int radix)
{
    char tmp[33];
    char *tp = tmp;
    int i;
    unsigned v;
    int sign;
    char *sp;

    if (radix > 36 || radix <= 1)
    {
        errno = EDOM;
        return 0;
    }

    sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned)value;
    while (v || tp == tmp)
    {
        i = v % radix;
        v = v / radix;
        if (i < 10)
            *tp++ = (char)(i+'0');
        else
            *tp++ = (char)(i + 'a' - 10);
    }

    if (string == 0)
        string = (char *)lib_malloc((tp-tmp)+sign+1);
    sp = string;

    if (sign)
        *sp++ = '-';
    while (tp > tmp)
        *sp++ = *--tp;
    *sp = 0;
    
    return string;
}

/*
 * lib_atoi
 */
__weak_reference(lib_atoi, atoi);

int  lib_atoi (const char *nptr)
{
    return (int)lib_strtol(nptr, (char **)LW_NULL, 10);
}

/*
 * lib_atol
 */
__weak_reference(lib_atol, atol);

long  lib_atol (const char *nptr)
{
    return lib_strtol(nptr, (char **)LW_NULL, 10);
}

/*
 * lib_atoll
 */
__weak_reference(lib_atoll, atoll);

long long lib_atoll (const char *nptr)
{
    return lib_strtoll(nptr, (char **)LW_NULL, 10);
}

/*
 * lib_atof
 */
__weak_reference(lib_atof, atof);

double lib_atof (const char *str)
{
    return  (lib_strtod(str, (char **)LW_NULL));
}

/*
 * lib_strtof
 */
__weak_reference(lib_strtof, strtof);

float lib_strtof (const char *nptr, char **endptr)
{
    return  ((float)lib_strtod(nptr, endptr));
}

/*
 * end
 */
