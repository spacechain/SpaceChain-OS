/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "stdio.h"

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)

#if __STDC__
#include "stdarg.h"
#else
#include "varargs.h"
#endif

int
#if __STDC__
snprintf(char *str, size_t n, char const *fmt, ...)
#else
snprintf(str, n, fmt, va_alist)
	char *str;
	size_t n;
	char *fmt;
	va_dcl
#endif
{
	int ret;
	va_list ap;
	FILE f;
	char c;

	if ((int)n < 0) /* SylixOS support zero to measuring string length */
		return (EOF);
	if ((int)n == 0) {
	    n = 1;
	    str = &c;
	}
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	f._flags = __SWR | __SSTR;
	f._bf._base = f._p = (unsigned char *)str;
	f._bf._size = f._w = n - 1;
	ret = vfprintf(&f, fmt, ap);
	*f._p = 0;
	va_end(ap);
	return (ret);
}

/* buffer printf
 *
 * str    : buffer base address
 * n      : total buffer size
 * offset : current offset
 * fmt    : format string
 * ...    : var args
 * return : new offset
 */
size_t
#if __STDC__
bnprintf(char *str, size_t n, size_t offset, char const *fmt, ...)
#else
bnprintf(str, n, offset, fmt, va_alist)
    char *str;
	size_t n;
	size_t offset;
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
	FILE f;
    
    if (n == 0)
        return (0);
    if (offset >= (n - 1))
        return (offset);    /* buffer full */
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
    f._flags = __SWR | __SSTR;
	f._bf._base = f._p = (unsigned char *)str + offset;
	f._bf._size = f._w = n - offset - 1;
	vfprintf(&f, fmt, ap);
	*f._p = 0;
	offset += (f._p - f._bf._base);
	va_end(ap);
	return (offset);
}

#endif  /*  (LW_CFG_DEVICE_EN > 0)      */
        /*  (LW_CFG_FIO_LIB_EN > 0)     */
