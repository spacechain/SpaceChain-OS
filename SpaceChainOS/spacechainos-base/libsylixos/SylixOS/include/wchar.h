/*	$NetBSD: wchar.h,v 1.27 2008/04/28 20:22:54 martin Exp $	*/

/*-
 * Copyright (c)1999 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <stddef.h>
#include "sys/types.h"
#include "sys/cdefs.h"
#include "limits.h"
#include "stdio.h" /* for FILE* */

#ifndef _WCHAR_ALL_TYPE_
#define _WCHAR_ALL_TYPE_
typedef void *wctrans_t;
typedef enum {
    WC_TYPE_INVALID = 0,
    WC_TYPE_ALNUM,
    WC_TYPE_ALPHA,
    WC_TYPE_BLANK,
    WC_TYPE_CNTRL,
    WC_TYPE_DIGIT,
    WC_TYPE_GRAPH,
    WC_TYPE_LOWER,
    WC_TYPE_PRINT,
    WC_TYPE_PUNCT,
    WC_TYPE_SPACE,
    WC_TYPE_UPPER,
    WC_TYPE_XDIGIT,
    WC_TYPE_MAX
} wctype_t;
#endif /* _WCHAR_ALL_TYPE_ */

#ifdef	_BSD_WCHAR_T_
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

#ifdef	_BSD_MBSTATE_T_
typedef	_BSD_MBSTATE_T_	mbstate_t;
#undef	_BSD_MBSTATE_T_
#endif

#ifdef	_BSD_WINT_T_
typedef	_BSD_WINT_T_	wint_t;
#undef	_BSD_WINT_T_
#endif

#ifdef __GNUC__
/* limits of wchar_t */
#define WCHAR_MIN       INT_MIN /* wchar_t */
#define WCHAR_MAX       INT_MAX /* wchar_t */

#else
#define WCHAR_MIN       SHRT_MIN /* wchar_t */
#define WCHAR_MAX       SHRT_MAX /* wchar_t */

#ifndef __restrict
#define __restrict
#endif /* __restrict */
#endif /* __GNUC__ */

/* limits of wint_t */
#define WINT_MIN        INT_MIN /* wint_t */
#define WINT_MAX        INT_MAX /* wint_t */

struct tm;

__BEGIN_DECLS
__BEGIN_NAMESPACE_STD
wint_t	btowc(int);
size_t	mbrlen(const char * __restrict, size_t, mbstate_t * __restrict);
size_t	mbrtowc(wchar_t * __restrict, const char * __restrict, size_t,
	    mbstate_t * __restrict);
int	mbsinit(const mbstate_t *);
size_t	mbsrtowcs(wchar_t * __restrict, const char ** __restrict, size_t,
	    mbstate_t * __restrict);
size_t mbstowcs(wchar_t *dst, const char *src, size_t len);
size_t	wcrtomb(char * __restrict, wchar_t, mbstate_t * __restrict);
wchar_t	*wcscat(wchar_t * __restrict, const wchar_t * __restrict);
wchar_t	*wcschr(const wchar_t *, wchar_t);
int	wcscmp(const wchar_t *, const wchar_t *);
int	wcscoll(const wchar_t *, const wchar_t *);
wchar_t	*wcscpy(wchar_t * __restrict, const wchar_t * __restrict);
size_t	wcscspn(const wchar_t *, const wchar_t *);
size_t	wcsftime(wchar_t * __restrict, size_t, const wchar_t * __restrict,
	    const struct tm * __restrict);
size_t	wcslen(const wchar_t *);
wchar_t	*wcsncat(wchar_t * __restrict, const wchar_t * __restrict, size_t);
int	wcsncmp(const wchar_t *, const wchar_t *, size_t);
wchar_t	*wcsncpy(wchar_t * __restrict , const wchar_t * __restrict, size_t);
wchar_t	*wcspbrk(const wchar_t *, const wchar_t *);
wchar_t	*wcsrchr(const wchar_t *, wchar_t);
size_t	wcsrtombs(char * __restrict, const wchar_t ** __restrict, size_t,
	    mbstate_t * __restrict);
size_t	wcsspn(const wchar_t *, const wchar_t *);
wchar_t	*wcsstr(const wchar_t *, const wchar_t *);
wchar_t *wcstok(wchar_t * __restrict, const wchar_t * __restrict,
		     wchar_t ** __restrict);
size_t	wcsxfrm(wchar_t *, const wchar_t *, size_t);
wchar_t	*wcswcs(const wchar_t *, const wchar_t *);
wchar_t	*wmemchr(const wchar_t *, wchar_t, size_t);
int	wmemcmp(const wchar_t *, const wchar_t *, size_t);
wchar_t	*wmemcpy(wchar_t * __restrict, const wchar_t * __restrict, size_t);
wchar_t	*wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t	*wmemset(wchar_t *, wchar_t, size_t);

size_t	wcslcat(wchar_t *, const wchar_t *, size_t);
size_t	wcslcpy(wchar_t *, const wchar_t *, size_t);
int	wcswidth(const wchar_t *, size_t);
int	wctob(wint_t);
int	wcwidth(wchar_t);

unsigned long int wcstoul(const wchar_t * __restrict,
	wchar_t ** __restrict, int);
long int wcstol(const wchar_t * __restrict,
	wchar_t ** __restrict, int);
double wcstod(const wchar_t * __restrict, wchar_t ** __restrict);

float wcstof(const wchar_t * __restrict, wchar_t ** __restrict);
long double wcstold(const wchar_t * __restrict, wchar_t ** __restrict);

/* LONGLONG */
long long int wcstoll(const wchar_t * __restrict,
	wchar_t ** __restrict, int);
/* LONGLONG */
unsigned long long int wcstoull(const wchar_t * __restrict,
	wchar_t ** __restrict, int);

wint_t ungetwc(wint_t, FILE *);
wint_t fgetwc(FILE *);
wchar_t *fgetws(wchar_t * __restrict, int, FILE * __restrict);
wint_t getwc(FILE *);
wint_t getwchar(void);
wint_t fputwc(wchar_t, FILE *);
int fputws(const wchar_t * __restrict, FILE * __restrict);
wint_t putwc(wchar_t, FILE *);
wint_t putwchar(wchar_t);

int fwide(FILE *, int);

wchar_t	*fgetwln(FILE * __restrict, size_t * __restrict);
int fwprintf(FILE * __restrict, const wchar_t * __restrict, ...);
int fwscanf(FILE * __restrict, const wchar_t * __restrict, ...);
int swprintf(wchar_t * __restrict, size_t n, const wchar_t * __restrict, ...);
int swscanf(const wchar_t * __restrict, const wchar_t * __restrict, ...);
int vfwprintf(FILE * __restrict, const wchar_t * __restrict, _BSD_VA_LIST_);
int vswprintf(wchar_t * __restrict, size_t, const wchar_t * __restrict,
    _BSD_VA_LIST_);
int vwprintf(const wchar_t * __restrict, _BSD_VA_LIST_);
int wprintf(const wchar_t * __restrict, ...);
int wscanf(const wchar_t * __restrict, ...);
int vfwscanf(FILE * __restrict, const wchar_t * __restrict, _BSD_VA_LIST_);
int vswscanf(const wchar_t * __restrict, const wchar_t * __restrict,
    _BSD_VA_LIST_);
int vwscanf(const wchar_t * __restrict, _BSD_VA_LIST_);

wchar_t *wcsdup (const wchar_t *);
int wcsncasecmp (const wchar_t *, const wchar_t *, size_t);
int wcscasecmp(const wchar_t *, const wchar_t *);
__END_NAMESPACE_STD
__END_DECLS

#ifndef WEOF
#define	WEOF 	((wint_t)-1)
#endif

#ifndef SYLIXOS
#define getwc(f) fgetwc(f)
#define getwchar() getwc(stdin)
#define putwc(wc, f) fputwc((wc), (f))
#define putwchar(wc) putwc((wc), stdout)
#else
wint_t  getwc(FILE *stream);
wint_t  getwchar(void);
wint_t  putwc(wchar_t wc, FILE *stream);
wint_t  putwchar(wchar_t wc);
#endif /* SYLIXOS */

#endif /* !_WCHAR_H_ */
