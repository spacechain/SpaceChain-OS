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
** 文   件   名: lib_stdlib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 库声明
*********************************************************************************************************/

#ifndef __LIB_STDLIB_H
#define __LIB_STDLIB_H

/*********************************************************************************************************
  rand
*********************************************************************************************************/

#ifndef RAND_MAX
#define RAND_MAX        __ARCH_INT_MAX
#endif                                                                  /* RAND_MAX                     */

/*********************************************************************************************************
  div struct
*********************************************************************************************************/

typedef struct {
    int     quot;                                                       /* Quotient.                    */
    int     rem;                                                        /* Remainder.                   */
} lib_div_t;

typedef struct {
    long    quot;                                                       /* Quotient.                    */
    long    rem;                                                        /* Remainder.                   */
} lib_ldiv_t;

typedef struct {
    INT64   quot;                                                       /* Quotient.                    */
    INT64   rem;                                                        /* Remainder.                   */
} lib_lldiv_t;

/*********************************************************************************************************
  LIB API
*********************************************************************************************************/

PVOID           lib_malloc(size_t  stNBytes);
PVOID           lib_mallocalign(size_t  stNBytes, size_t  stAlign);
PVOID           lib_memalign(size_t  stAlign, size_t  stNBytes);
VOID            lib_free(PVOID  pvPtr);
PVOID           lib_calloc(size_t  stNNum, size_t  stSize);
PVOID           lib_realloc(PVOID  pvPtr, size_t  stNewSize);
INT             lib_posix_memalign(PVOID *ppvMem, size_t  stAlign, size_t  stNBytes);

PVOID           lib_xmalloc(size_t  stSize);
PVOID           lib_xmallocalign(size_t  stNBytes, size_t  stAlign);
PVOID           lib_xmemalign(size_t  stAlign, size_t  stNBytes);
PVOID           lib_xcalloc(size_t  stNNum, size_t  stSize);
PVOID           lib_xrealloc(PVOID  pvPtr, size_t  stNewSize);

INT             lib_rand(VOID);
INT             lib_rand_r(uint_t *puiSeed);
VOID            lib_srand(uint_t  uiSeed);
VOID            lib_srandom(uint_t uiSeed);
LONG            lib_random(VOID);

void            lib_lcong48(unsigned short p[7]);
double          lib_erand48(unsigned short xseed[3]);
double          lib_drand48(void);
long            lib_lrand48(void);
long            lib_mrand48(void);
long            lib_nrand48(unsigned short xseed[3]);
long            lib_jrand48(unsigned short xseed[3]);
unsigned short *lib_seed48(unsigned short xseed[3]);
void            lib_srand48(long seed);

VOID            lib_abort(VOID);
INT             lib_system(CPCHAR  pcCmd);

lib_div_t       lib_div(int  numer, int  denom);
lib_ldiv_t      lib_ldiv(long  numer, long  denom);
lib_lldiv_t     lib_lldiv(INT64  numer, INT64  denom);

/*********************************************************************************************************
  C++ new 
*********************************************************************************************************/

PVOID           lib_malloc_new(size_t  stNBytes);

/*********************************************************************************************************
  BSD CLIB API
*********************************************************************************************************/

long            lib_strtol(const char *nptr, char **endptr, register int base);
unsigned long   lib_strtoul(const char *nptr, char **endptr, register int base);
long long       lib_strtoll(const char *nptr, char **endptr, int base);
unsigned 
long long       lib_strtoull(const char *nptr, char **endptr, int base);

long double     lib_strtold(const char *nptr, char **endptr);
double          lib_strtod(const char *nptr, char **endptr);
float           lib_strtof(const char *nptr, char **endptr);
double          lib_atof(const char *str);

char           *lib_itoa(int value, char *string, int radix);
int             lib_atoi(const char *nptr);
long            lib_atol(const char *nptr);
long long       lib_atoll(const char *nptr);

/*********************************************************************************************************
  abs
*********************************************************************************************************/

INT             lib_abs(INT  i);
LONG            lib_labs(LONG  l);
long long       lib_llabs(long long ll);

/*********************************************************************************************************
  sort
*********************************************************************************************************/

void            lib_qsort(void *base, size_t nel, size_t width,
                          int (*compar)(const void *, const void *));

/*********************************************************************************************************
  search
*********************************************************************************************************/

void           *lib_bsearch(const void *key, const void *base0,
                            size_t nmemb,  size_t size,
                            int (*compar)(const void *, const void *));

/*********************************************************************************************************
  environment variable
*********************************************************************************************************/

int             lib_getenv_r(const char  *pcName, char  *pcBuffer, int  iLen);
char           *lib_getenv(const char  *pcName);
int             lib_putenv(char  *cString);
int             lib_setenv(const char  *pcName, const char  *pcValue, int  iRewrite);
int             lib_unsetenv(const char  *pcName);

#endif                                                                  /*  __LIB_STDLIB_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
