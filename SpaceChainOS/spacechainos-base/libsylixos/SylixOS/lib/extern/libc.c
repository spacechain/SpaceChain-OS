/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: libc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 08 月 15 日
**
** 描        述: c库导出函数.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_PANIC
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_INTTYPES
#define  __SYLIXOS_STDINT
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "locale.h"
/*********************************************************************************************************
  function define
*********************************************************************************************************/

#define __LW_RETU_FUNC_DEFINE(ret, name, declare, arg)   \
        ret name declare    \
        {   \
            return  (lib_##name arg );   \
        }   
        
#define __LW_VOID_FUNC_DEFINE(name, declare, arg)   \
        void name declare   \
        {   \
            lib_##name arg; \
        }   

#ifdef __GNUC__
#define __LW_NRET_FUNC_DEFINE(name, declare, arg)   \
        void name declare   \
        {   \
            lib_##name arg; \
            __builtin_unreachable(); \
        }   
#else
#define __LW_NRET_FUNC_DEFINE   __LW_VOID_FUNC_DEFINE
#endif                                                                  /*  __GNUC__                    */

/*********************************************************************************************************
  float
*********************************************************************************************************/
#if LW_KERN_FLOATING > 0

__LW_RETU_FUNC_DEFINE(int, isnan, (double  dX), (dX))
__LW_RETU_FUNC_DEFINE(int, isinf, (double  dX), (dX))

#endif                                                                  /*  LW_KERN_FLOATING            */
/*********************************************************************************************************
  ctype
*********************************************************************************************************/

__LW_RETU_FUNC_DEFINE(int, isalnum, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isalpha, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, iscntrl, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isdigit, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isgraph, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, islower, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isprint, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, ispunct, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isspace, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isupper, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isxdigit, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isascii, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, toascii, (int c), (c))
__LW_RETU_FUNC_DEFINE(int, isblank, (int c), (c))

/*********************************************************************************************************
  locale
*********************************************************************************************************/

__LW_RETU_FUNC_DEFINE(struct lconv *, localeconv, (void), ())
__LW_RETU_FUNC_DEFINE(char *, setlocale, (int category, const char *locale), (category, locale))

/*********************************************************************************************************
  assert
*********************************************************************************************************/

__LW_VOID_FUNC_DEFINE(assert, (int expression), (expression))

void __assert_func (const char *file, int line, const char *func, const char *failedexpr)
{
    fprintf(stderr,
            "__assert_func() assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
            failedexpr, file, line,
            func ? ", function: " : "", func ? func : "");
            
    panic("");
}

/*********************************************************************************************************
  stdlib
*********************************************************************************************************/

int brk (void *addr)
{
    errno = ENOSYS;
    return  (PX_ERROR);
}

void *sbrk (intptr_t incr)
{
    errno = ENOSYS;
    return  (LW_NULL);
}

__LW_RETU_FUNC_DEFINE(void *, malloc, (size_t  stNBytes), (stNBytes))
__LW_RETU_FUNC_DEFINE(void *, mallocalign, (size_t  stNBytes, size_t  stAlign), (stNBytes, stAlign))
__LW_RETU_FUNC_DEFINE(void *, memalign, (size_t  stAlign, size_t  stNBytes), (stAlign, stNBytes))
__LW_VOID_FUNC_DEFINE(free, (void *pvPtr), (pvPtr))
__LW_RETU_FUNC_DEFINE(void *, calloc, (size_t  stNNum, size_t  stSize), (stNNum, stSize))
__LW_RETU_FUNC_DEFINE(void *, realloc, (void *pvPtr, size_t  stNewSize), (pvPtr, stNewSize))
__LW_RETU_FUNC_DEFINE(void *, xmalloc, (size_t  stSize), (stSize))
__LW_RETU_FUNC_DEFINE(void *, xmallocalign, (size_t  stNBytes, size_t  stAlign), (stNBytes, stAlign))
__LW_RETU_FUNC_DEFINE(void *, xmemalign, (size_t  stAlign, size_t  stNBytes), (stAlign, stNBytes))
__LW_RETU_FUNC_DEFINE(void *, xcalloc, (size_t  stNNum, size_t  stSize), (stNNum, stSize))
__LW_RETU_FUNC_DEFINE(void *, xrealloc, (void *pvPtr, size_t  stNewSize), (pvPtr, stNewSize))
__LW_RETU_FUNC_DEFINE(int, posix_memalign, (void **memptr, size_t alignment, size_t size), (memptr, alignment, size))

void *aligned_malloc (size_t  bytes, size_t alignment)
{
    return  (mallocalign(bytes, alignment));
}

void aligned_free (void *p)
{
    free(p);
}

__LW_RETU_FUNC_DEFINE(int, rand, (void), ())
__LW_RETU_FUNC_DEFINE(int, rand_r, (uint_t *puiSeed), (puiSeed))
__LW_VOID_FUNC_DEFINE(srand, (uint_t  uiSeed), (uiSeed))
__LW_VOID_FUNC_DEFINE(srandom, (uint_t uiSeed), (uiSeed))
__LW_RETU_FUNC_DEFINE(long, random, (void), ())

__LW_VOID_FUNC_DEFINE(lcong48, (unsigned short p[7]), (p))

#if LW_KERN_FLOATING > 0
__LW_RETU_FUNC_DEFINE(double, erand48, (unsigned short xseed[3]), (xseed))
__LW_RETU_FUNC_DEFINE(double, drand48, (void), ())
#endif

__LW_RETU_FUNC_DEFINE(long, lrand48, (void), ())
__LW_RETU_FUNC_DEFINE(long, mrand48, (void), ())
__LW_RETU_FUNC_DEFINE(long, nrand48, (unsigned short xseed[3]), (xseed))
__LW_RETU_FUNC_DEFINE(long, jrand48, (unsigned short xseed[3]), (xseed))
__LW_RETU_FUNC_DEFINE(unsigned short *, seed48, (unsigned short xseed[3]), (xseed))
__LW_VOID_FUNC_DEFINE(srand48, (long seed), (seed))

__LW_NRET_FUNC_DEFINE(abort, (void), ())
__LW_RETU_FUNC_DEFINE(int, system, (const char *pcCmd), (pcCmd))

__LW_RETU_FUNC_DEFINE(lib_div_t, div, (int  numer, int  denom), (numer, denom))
__LW_RETU_FUNC_DEFINE(lib_ldiv_t, ldiv, (long  numer, long  denom), (numer, denom))
__LW_RETU_FUNC_DEFINE(lib_lldiv_t, lldiv, (int64_t  numer, int64_t  denom), (numer, denom))

__LW_RETU_FUNC_DEFINE(void *, malloc_new, (size_t  stNBytes), (stNBytes))

__LW_RETU_FUNC_DEFINE(long, strtol, (const char *nptr, char **endptr, register int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(unsigned long, strtoul, (const char *nptr, char **endptr, register int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(long long, strtoll, (const char *nptr, char **endptr, int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(unsigned long long, strtoull, (const char *nptr, char **endptr, int base), \
                      (nptr, endptr, base))

#if LW_KERN_FLOATING > 0
__LW_RETU_FUNC_DEFINE(long double, strtold, (const char *nptr, char **endptr), (nptr, endptr))
__LW_RETU_FUNC_DEFINE(double, strtod, (const char *nptr, char **endptr), (nptr, endptr))
__LW_RETU_FUNC_DEFINE(float, strtof, (const char *nptr, char **endptr), (nptr, endptr))
__LW_RETU_FUNC_DEFINE(double, atof, (const char *str), (str))
#endif                                                                  /*  LW_KERN_FLOATING            */

__LW_RETU_FUNC_DEFINE(char *, itoa, (int value, char *string, int radix), (value, string, radix))
__LW_RETU_FUNC_DEFINE(int, atoi, (const char *nptr), (nptr))
__LW_RETU_FUNC_DEFINE(long, atol, (const char *nptr), (nptr))
__LW_RETU_FUNC_DEFINE(long long, atoll, (const char *nptr), (nptr))

#if !defined(LW_CFG_CPU_ARCH_C6X)                                       /*  c6x comipler builtin abs fun*/
__LW_RETU_FUNC_DEFINE(int, abs, (int  i), (i))
__LW_RETU_FUNC_DEFINE(long, labs, (long  l), (l))
__LW_RETU_FUNC_DEFINE(long long, llabs, (long long ll), (ll))
#endif                                                                  /*  !LW_CFG_CPU_ARCH_C6X        */

__LW_VOID_FUNC_DEFINE(qsort, 
                      (void *base, size_t nel, size_t width, int (*compar)(const void *, const void *)), \
                      (base, nel, width, compar))

__LW_RETU_FUNC_DEFINE(void *, bsearch, (const void *key, const void *base0, \
                                        size_t nmemb,  size_t size, \
                                        int (*compar)(const void *, const void *)), \
                      (key, base0, nmemb, size, compar))
                      
__LW_RETU_FUNC_DEFINE(int, getenv_r, (const char  *pcName, char  *pcBuffer, int  iLen), \
                      (pcName, pcBuffer, iLen))
__LW_RETU_FUNC_DEFINE(char *, getenv, (const char  *pcName), (pcName))
__LW_RETU_FUNC_DEFINE(int, putenv, (char  *cString), (cString))
__LW_RETU_FUNC_DEFINE(int, setenv, (const char  *pcName, const char  *pcValue, int  iRewrite),\
                      (pcName, pcValue, iRewrite))
__LW_RETU_FUNC_DEFINE(int, unsetenv, (const char  *pcName), (pcName))

/*********************************************************************************************************
  time
*********************************************************************************************************/

__LW_RETU_FUNC_DEFINE(time_t, timelocal, (time_t *time), (time))
__LW_RETU_FUNC_DEFINE(char *, ctime, (const time_t *time), (time))
__LW_RETU_FUNC_DEFINE(char *, ctime_r, (const time_t *time, char *pcBuffer), (time, pcBuffer))

__LW_RETU_FUNC_DEFINE(char *, asctime_r, (const struct tm *ptm, char *pcBuffer), (ptm, pcBuffer))
__LW_RETU_FUNC_DEFINE(char *, asctime, (const struct tm *ptm), (ptm))

__LW_RETU_FUNC_DEFINE(struct tm *, gmtime_r, (const time_t *time, struct tm *ptmBuffer), (time, ptmBuffer))
__LW_RETU_FUNC_DEFINE(struct tm *, gmtime, (const time_t *time), (time))

__LW_RETU_FUNC_DEFINE(struct tm *, localtime_r, (const time_t *time, struct tm *ptmBuffer), (time, ptmBuffer))
__LW_RETU_FUNC_DEFINE(struct tm *, localtime, (const time_t *time), (time))

__LW_RETU_FUNC_DEFINE(time_t, mktime, (struct tm *ptm), (ptm))
__LW_RETU_FUNC_DEFINE(time_t, timegm, (struct tm *ptm), (ptm))

#if LW_KERN_FLOATING > 0
__LW_RETU_FUNC_DEFINE(double, difftime, (time_t time1, time_t time2), (time1, time2))
#endif

__LW_RETU_FUNC_DEFINE(clock_t, clock, (void), ())
__LW_RETU_FUNC_DEFINE(int, clock_getcpuclockid, (pid_t pid, clockid_t *clock_id), (pid, clock_id))
__LW_RETU_FUNC_DEFINE(int, clock_getres, (clockid_t  clockid, struct timespec *res), (clockid, res))
__LW_RETU_FUNC_DEFINE(int, clock_gettime, (clockid_t  clockid, struct timespec  *tv), (clockid, tv))
__LW_RETU_FUNC_DEFINE(int, clock_settime, (clockid_t  clockid, const struct timespec  *tv), (clockid, tv))
__LW_RETU_FUNC_DEFINE(int, clock_nanosleep, (clockid_t  clockid, int  iFlags, 
                           const struct timespec  *rqtp, struct timespec  *rmtp), 
                           (clockid, iFlags, rqtp, rmtp))

__LW_RETU_FUNC_DEFINE(int, gettimeofday, (struct timeval *tv, struct timezone *tz), (tv, tz))
__LW_RETU_FUNC_DEFINE(int, settimeofday, (const struct timeval *tv, const struct timezone *tz), (tv, tz))

__LW_VOID_FUNC_DEFINE(tzset, (void), ())

__LW_RETU_FUNC_DEFINE(hrtime_t, gethrtime, (void), ())
__LW_RETU_FUNC_DEFINE(hrtime_t, gethrvtime, (void), ())

/*********************************************************************************************************
  string
*********************************************************************************************************/

__LW_RETU_FUNC_DEFINE(int, ffs, (int valu), (valu))

__LW_RETU_FUNC_DEFINE(char *, rindex, (const char *pcString, int iC), (pcString, iC))
__LW_RETU_FUNC_DEFINE(char *, index, (const char *pcString, int iC), (pcString, iC))
__LW_RETU_FUNC_DEFINE(char *, strchrnul, (const char *pcString, int iC), (pcString, iC))
__LW_RETU_FUNC_DEFINE(char *, strrchr, (const char *pcString, int iC), (pcString, iC))
__LW_RETU_FUNC_DEFINE(char *, strchr, (const char *pcString, int iC), (pcString, iC))

__LW_RETU_FUNC_DEFINE(char *, strcat, (char *pcDest, const char *pcSrc), (pcDest, pcSrc))
__LW_RETU_FUNC_DEFINE(char *, strncat, (char *pcDest, const char *pcSrc, size_t stN), (pcDest, pcSrc, stN))
__LW_RETU_FUNC_DEFINE(size_t, strlcat, (char *pcDest, const char *pcSrc, size_t stN), (pcDest, pcSrc, stN))

__LW_RETU_FUNC_DEFINE(int, strcmp, (const char *pcStr1, const char *pcStr2), (pcStr1, pcStr2))
__LW_RETU_FUNC_DEFINE(int, strncmp, (const char *pcStr1, const char *pcStr2, size_t stLen), \
                      (pcStr1, pcStr2, stLen))

__LW_RETU_FUNC_DEFINE(int, strcasecmp, (const char *pcStr1, const char *pcStr2), (pcStr1, pcStr2))
__LW_RETU_FUNC_DEFINE(int, strncasecmp, (const char *pcStr1, const char *pcStr2, size_t  stLen), \
                      (pcStr1, pcStr2, stLen))

__LW_RETU_FUNC_DEFINE(char *, stpcpy, (char *pcDest, const char *pcSrc), (pcDest, pcSrc))
__LW_RETU_FUNC_DEFINE(char *, stpncpy, (char *pcDest, const char *pcSrc, size_t stN), (pcDest, pcSrc, stN))

__LW_RETU_FUNC_DEFINE(char *, strcpy, (char *pcDest, const char *pcSrc), (pcDest, pcSrc))
__LW_RETU_FUNC_DEFINE(char *, strncpy, (char *pcDest, const char *pcSrc, size_t stN), (pcDest, pcSrc, stN))
__LW_RETU_FUNC_DEFINE(size_t, strlcpy, (char *pcDest, const char *pcSrc, size_t stN), (pcDest, pcSrc, stN))
__LW_VOID_FUNC_DEFINE(bcopy, (const void *pvSrc, void *pvDest, size_t stN), (pvSrc, pvDest, stN))

__LW_RETU_FUNC_DEFINE(size_t, strlen, (const char *pcStr), (pcStr))
__LW_RETU_FUNC_DEFINE(size_t, strnlen, (const char *pcStr, size_t stN), (pcStr, stN))

__LW_RETU_FUNC_DEFINE(char *, strdup, (const char *pcStr), (pcStr))
__LW_RETU_FUNC_DEFINE(char *, xstrdup, (const char *pcStr), (pcStr))

__LW_RETU_FUNC_DEFINE(char *, strndup, (const char *pcStr, size_t  stSize), (pcStr, stSize))
__LW_RETU_FUNC_DEFINE(char *, xstrndup, (const char *pcStr, size_t  stSize), (pcStr, stSize))

__LW_RETU_FUNC_DEFINE(int, memcmp, (const void *pvMem1, const void *pvMem2, size_t stCount), \
                      (pvMem1, pvMem2, stCount))
                      
__LW_RETU_FUNC_DEFINE(int, bcmp, (const void *pvMem1, const void *pvMem2, size_t stCount), \
                      (pvMem1, pvMem2, stCount))
__LW_RETU_FUNC_DEFINE(void *, memcpy, (void *pvDest, const void *pvSrc, size_t stCount), \
                      (pvDest, pvSrc, stCount))
__LW_RETU_FUNC_DEFINE(void *, mempcpy, (void *pvDest, const void *pvSrc, size_t stCount), \
                      (pvDest, pvSrc, stCount))
__LW_RETU_FUNC_DEFINE(void *, memset, (void *pvDest, int iC, size_t stCount), \
                      (pvDest, iC, stCount))
__LW_VOID_FUNC_DEFINE(bzero, (void *pvStr, size_t stCount), (pvStr, stCount))
__LW_RETU_FUNC_DEFINE(void *, memchr, (const void *pvBuf, int c, size_t stCnt), \
                      (pvBuf, c, stCnt))
__LW_RETU_FUNC_DEFINE(void *, memrchr, (const void *pvBuf, int c, size_t stCnt), \
                      (pvBuf, c, stCnt))
__LW_RETU_FUNC_DEFINE(char *, strnset, (char *pcStr, int iVal, size_t stCount), (pcStr, iVal, stCount))

__LW_RETU_FUNC_DEFINE(int, tolower, (int  iC), (iC))
__LW_RETU_FUNC_DEFINE(int, toupper, (int  iC), (iC))

__LW_RETU_FUNC_DEFINE(int, strerror_r, (int  iNum, char *pcBuffer, size_t stLen), \
                      (iNum, pcBuffer, stLen))
__LW_RETU_FUNC_DEFINE(char *, strerror, (int  iNum), (iNum))
__LW_RETU_FUNC_DEFINE(char *, strsignal, (int  iSigNo), (iSigNo))
__LW_RETU_FUNC_DEFINE(char *, strstr, (const char *cpcS1, const char *cpcS2), (cpcS1, cpcS2))
__LW_RETU_FUNC_DEFINE(size_t, strcspn, (const char *cpcS1, const char *cpcS2), (cpcS1, cpcS2))
__LW_RETU_FUNC_DEFINE(char *, strpbrk, (const char *cpcS1, const char *cpcS2), (cpcS1, cpcS2))

__LW_RETU_FUNC_DEFINE(size_t, strftime, (char *s, size_t maxsize, const char *format, const struct tm *t),
                      (s, maxsize, format, t))
__LW_RETU_FUNC_DEFINE(char *, strptime, (const char *buf, const char *fmt, struct tm *tm),
                      (buf, fmt, tm))
__LW_RETU_FUNC_DEFINE(int, stricmp, (const char *s1, const char *s2), (s1, s2))
__LW_RETU_FUNC_DEFINE(size_t, strspn, (const char *s1, const char *s2), (s1, s2))
__LW_RETU_FUNC_DEFINE(char *, strtok, (char *s, const char *delim), (s, delim))
__LW_RETU_FUNC_DEFINE(char *, strtok_r, (char *s, const char *delim, char **last), (s, delim, last))

__LW_RETU_FUNC_DEFINE(int, strcoll, (const char *pcStr1, const char *pcStr2), (pcStr1, pcStr2))
__LW_RETU_FUNC_DEFINE(size_t, strxfrm, (char *s1, const char *s2, size_t n), (s1, s2, n))

__LW_VOID_FUNC_DEFINE(swab, (const void *from, void *to, size_t len), (from, to, len))
__LW_RETU_FUNC_DEFINE(char *, strsep, (char **s, const char *ct), (s, ct))

void *memmove (void *pvDest, const void *pvSrc, size_t stCount)
{
    return  (lib_memcpy(pvDest, pvSrc, stCount));
}

/*********************************************************************************************************
  inttypes.h
*********************************************************************************************************/

__LW_RETU_FUNC_DEFINE(intmax_t, imaxabs, (intmax_t j), (j))
__LW_RETU_FUNC_DEFINE(imaxdiv_t, imaxdiv, (intmax_t numer, intmax_t denomer), (numer, denomer))
__LW_RETU_FUNC_DEFINE(intmax_t, strtoimax, (const char *nptr, char **endptr, int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(uintmax_t, strtoumax, (const char *nptr, char **endptr, int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(intmax_t, wcstoimax, (const wchar_t *nptr, wchar_t **endptr, int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(uintmax_t, wcstoumax, (const wchar_t *nptr, wchar_t **endptr, int base), \
                      (nptr, endptr, base))

/*********************************************************************************************************
  END
*********************************************************************************************************/
