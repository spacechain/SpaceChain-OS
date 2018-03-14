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
** 文   件   名: stdlib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>
#include <alloca.h>

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

extern size_t __mb_cur_max;                                             /*  需要C外部库支持             */
#define	MB_CUR_MAX	__mb_cur_max

__LW_RETU_FUNC_DECLARE(int, __locale_mb_cur_max, (void))                /*  需要C外部库支持             */

typedef lib_div_t   div_t;
typedef lib_ldiv_t  ldiv_t;
typedef lib_lldiv_t lldiv_t;

__LW_RETU_FUNC_DECLARE(void *, malloc, (size_t  stNBytes))
__LW_RETU_FUNC_DECLARE(void *, mallocalign, (size_t  stNBytes, size_t  stAlign))
__LW_RETU_FUNC_DECLARE(void *, aligned_malloc, (size_t  stNBytes, size_t  stAlign))
__LW_RETU_FUNC_DECLARE(void, aligned_free, (void *pvPtr))
__LW_RETU_FUNC_DECLARE(void *, memalign, (size_t  stAlign, size_t  stNBytes))
__LW_RETU_FUNC_DECLARE(void, free, (void *pvPtr))
__LW_RETU_FUNC_DECLARE(void *, calloc, (size_t  stNNum, size_t  stSize))
__LW_RETU_FUNC_DECLARE(void *, realloc, (void *pvPtr, size_t  stNewSize))
__LW_RETU_FUNC_DECLARE(void *, xmalloc, (size_t  stSize))
__LW_RETU_FUNC_DECLARE(void *, xmallocalign, (size_t  stNBytes, size_t  stAlign))
__LW_RETU_FUNC_DECLARE(void *, xmemalign, (size_t  stAlign, size_t  stNBytes))
__LW_RETU_FUNC_DECLARE(void *, xcalloc, (size_t  stNNum, size_t  stSize))
__LW_RETU_FUNC_DECLARE(void *, xrealloc, (void *pvPtr, size_t  stNewSize))
__LW_RETU_FUNC_DECLARE(int, posix_memalign, (void **memptr, size_t alignment, size_t size))

__LW_RETU_FUNC_DECLARE(int, rand, (void))
__LW_RETU_FUNC_DECLARE(int, rand_r, (uint_t *puiSeed))
__LW_RETU_FUNC_DECLARE(void, srand, (uint_t  uiSeed))
__LW_RETU_FUNC_DECLARE(void, srandom, (uint_t uiSeed))
__LW_RETU_FUNC_DECLARE(long, random, (void))

__LW_RETU_FUNC_DECLARE(void, lcong48, (unsigned short p[7]))
__LW_RETU_FUNC_DECLARE(double, erand48, (unsigned short xseed[3]))
__LW_RETU_FUNC_DECLARE(double, drand48, (void))
__LW_RETU_FUNC_DECLARE(long, lrand48, (void))
__LW_RETU_FUNC_DECLARE(long, mrand48, (void))
__LW_RETU_FUNC_DECLARE(long, nrand48, (unsigned short xseed[3]))
__LW_RETU_FUNC_DECLARE(long, jrand48, (unsigned short xseed[3]))
__LW_RETU_FUNC_DECLARE(unsigned short *, seed48, (unsigned short xseed[3]))
__LW_RETU_FUNC_DECLARE(void, srand48, (long seed))

__LW_RETU_FUNC_DECLARE(void, abort, (void))
__LW_RETU_FUNC_DECLARE(int, system, (const char *pcCmd))

__LW_RETU_FUNC_DECLARE(div_t, div, (int  numer, int  denom))
__LW_RETU_FUNC_DECLARE(ldiv_t, ldiv, (long  numer, long  denom))
__LW_RETU_FUNC_DECLARE(lldiv_t, lldiv, (int64_t  numer, int64_t  denom))

__LW_RETU_FUNC_DECLARE(void *, malloc_new, (size_t  stNBytes))

__LW_RETU_FUNC_DECLARE(long, strtol, (const char *nptr, char **endptr, register int base))
__LW_RETU_FUNC_DECLARE(unsigned long, strtoul, (const char *nptr, char **endptr, register int base))
__LW_RETU_FUNC_DECLARE(long long, strtoll, (const char *nptr, char **endptr, int base))
__LW_RETU_FUNC_DECLARE(unsigned long long, strtoull, (const char *nptr, char **endptr, int base))

__LW_RETU_FUNC_DECLARE(long double, strtold, (const char *nptr, char **endptr))
__LW_RETU_FUNC_DECLARE(double, strtod, (const char *nptr, char **endptr))
__LW_RETU_FUNC_DECLARE(float, strtof, (const char *nptr, char **endptr))
__LW_RETU_FUNC_DECLARE(double, atof, (const char *str))

__LW_RETU_FUNC_DECLARE(char *, itoa, (int value, char *string, int radix))
__LW_RETU_FUNC_DECLARE(int, atoi, (const char *nptr))
__LW_RETU_FUNC_DECLARE(long, atol, (const char *nptr))
__LW_RETU_FUNC_DECLARE(long long, atoll, (const char *nptr))

__LW_RETU_FUNC_DECLARE(int, abs, (int  i))
__LW_RETU_FUNC_DECLARE(long, labs, (long  l))
__LW_RETU_FUNC_DECLARE(long long, llabs, (long long ll))

__LW_RETU_FUNC_DECLARE(void, qsort, 
                      (void *base, size_t nel, size_t width, int (*compar)(const void *, const void *)))

__LW_RETU_FUNC_DECLARE(void *, bsearch, (const void *key, const void *base0, \
                                        size_t nmemb,  size_t size, \
                                        int (*compar)(const void *, const void *)))
                      
__LW_RETU_FUNC_DECLARE(int, getenv_r, (const char  *pcName, char  *pcBuffer, int  iLen))
__LW_RETU_FUNC_DECLARE(char *, getenv, (const char  *pcName))
__LW_RETU_FUNC_DECLARE(int, putenv, (char  *cString))
__LW_RETU_FUNC_DECLARE(int, setenv, (const char  *pcName, const char  *pcValue, int  iRewrite))
__LW_RETU_FUNC_DECLARE(int, unsetenv, (const char  *pcName))

/*********************************************************************************************************
  wchar
*********************************************************************************************************/

__LW_RETU_FUNC_DECLARE(size_t, mbstowcs, (wchar_t *dst, const char *src, size_t len))
__LW_RETU_FUNC_DECLARE(size_t, wcstombs, (char *dst, const wchar_t *src, size_t len))
__LW_RETU_FUNC_DECLARE(int, wctomb, (char *s, wchar_t wchar))
__LW_RETU_FUNC_DECLARE(int, mblen, (const char *s, size_t n))
__LW_RETU_FUNC_DECLARE(int, mbtowc, (wchar_t *pwc, const char *s, size_t n))

/*********************************************************************************************************
  以下函数需要 libcextern 支持
*********************************************************************************************************/

char	*cgetcap(char *, const char *, int);
int	     cgetclose(void);
int	     cgetent(char **, const char * const *, const char *);
int	     cgetfirst(char **, const char * const *);
int	     cgetmatch(const char *, const char *);
int	     cgetnext(char **, const char * const *);
int	     cgetnum(char *, const char *, long *);
int	     cgetset(const char *);
int	     cgetstr(char *, const char *, char **);
int	     cgetustr(char *, const char *, char **);
void	 csetexpandtc(int);

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __STDLIB_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
