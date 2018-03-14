/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_PANIC
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_INTTYPES
#define  __SYLIXOS_STDINT
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "locale.h"
#include "wchar.h"

/*
 * function define
 */
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
#endif /* __GNUC__ */

/*
 * float
 */

__LW_RETU_FUNC_DEFINE(int, isnan, (double  dX), (dX))
__LW_RETU_FUNC_DEFINE(int, isinf, (double  dX), (dX))

/*
 * stdlib
 */

__LW_RETU_FUNC_DEFINE(int, rand, (void), ())
__LW_RETU_FUNC_DEFINE(int, rand_r, (uint_t *puiSeed), (puiSeed))
__LW_VOID_FUNC_DEFINE(srand, (uint_t  uiSeed), (uiSeed))
__LW_VOID_FUNC_DEFINE(srandom, (uint_t uiSeed), (uiSeed))
__LW_RETU_FUNC_DEFINE(long, random, (void), ())

__LW_VOID_FUNC_DEFINE(lcong48, (unsigned short p[7]), (p))

__LW_RETU_FUNC_DEFINE(double, erand48, (unsigned short xseed[3]), (xseed))
__LW_RETU_FUNC_DEFINE(double, drand48, (void), ())

__LW_RETU_FUNC_DEFINE(long, lrand48, (void), ())
__LW_RETU_FUNC_DEFINE(long, mrand48, (void), ())
__LW_RETU_FUNC_DEFINE(long, nrand48, (unsigned short xseed[3]), (xseed))
__LW_RETU_FUNC_DEFINE(long, jrand48, (unsigned short xseed[3]), (xseed))
__LW_RETU_FUNC_DEFINE(unsigned short *, seed48, (unsigned short xseed[3]), (xseed))
__LW_VOID_FUNC_DEFINE(srand48, (long seed), (seed))

__LW_VOID_FUNC_DEFINE(qsort,
                      (void *base, size_t nel, size_t width, int (*compar)(const void *, const void *)), \
                      (base, nel, width, compar))

__LW_RETU_FUNC_DEFINE(void *, bsearch, (const void *key, const void *base0, \
                                        size_t nmemb,  size_t size, \
                                        int (*compar)(const void *, const void *)), \
                      (key, base0, nmemb, size, compar))


__LW_RETU_FUNC_DEFINE(long, strtol, (const char *nptr, char **endptr, register int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(unsigned long, strtoul, (const char *nptr, char **endptr, register int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(long long, strtoll, (const char *nptr, char **endptr, int base), \
                      (nptr, endptr, base))
__LW_RETU_FUNC_DEFINE(unsigned long long, strtoull, (const char *nptr, char **endptr, int base), \
                      (nptr, endptr, base))

__LW_RETU_FUNC_DEFINE(char *, itoa, (int value, char *string, int radix), (value, string, radix))
__LW_RETU_FUNC_DEFINE(int, atoi, (const char *nptr), (nptr))
__LW_RETU_FUNC_DEFINE(long, atol, (const char *nptr), (nptr))
__LW_RETU_FUNC_DEFINE(long long, atoll, (const char *nptr), (nptr))

__LW_RETU_FUNC_DEFINE(long double, strtold, (const char *nptr, char **endptr), (nptr, endptr))
__LW_RETU_FUNC_DEFINE(double, strtod, (const char *nptr, char **endptr), (nptr, endptr))
__LW_RETU_FUNC_DEFINE(float, strtof, (const char *nptr, char **endptr), (nptr, endptr))
__LW_RETU_FUNC_DEFINE(double, atof, (const char *str), (str))

__LW_RETU_FUNC_DEFINE(double, difftime, (time_t time1, time_t time2), (time1, time2))

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

__LW_RETU_FUNC_DEFINE(void *, malloc_new, (size_t  stNBytes), (stNBytes))

__LW_RETU_FUNC_DEFINE(char *, strdup, (const char *pcStr), (pcStr))
__LW_RETU_FUNC_DEFINE(char *, xstrdup, (const char *pcStr), (pcStr))

__LW_RETU_FUNC_DEFINE(char *, strndup, (const char *pcStr, size_t  stSize), (pcStr, stSize))
__LW_RETU_FUNC_DEFINE(char *, xstrndup, (const char *pcStr, size_t  stSize), (pcStr, stSize))

wchar_t *_wcsdup (const wchar_t *str)
{
    return  (wcsdup(str));
}

/*
 * end
 */
