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
** 文   件   名: lib_stdint.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 07 日
**
** 描        述: 标准数据类型定义.
*********************************************************************************************************/

#ifndef __LIB_STDINT_H
#define __LIB_STDINT_H

#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/lib/libc/limits/lib_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  least type (尽最大可能兼容)
*********************************************************************************************************/

#ifndef __int_least8_t_defined
typedef int8_t      int_least8_t;
typedef uint8_t     uint_least8_t;
#define __int_least8_t_defined 1
#endif

#ifndef __int_least16_t_defined
typedef int16_t     int_least16_t;
typedef uint16_t    uint_least16_t;
#define __int_least16_t_defined 1
#endif

#ifndef __int_least32_t_defined
typedef int32_t     int_least32_t;
typedef uint32_t    uint_least32_t;
#define __int_least32_t_defined 1
#endif

#ifndef __int_least64_t_defined
typedef int64_t     int_least64_t;
typedef uint64_t    uint_least64_t;
#define __int_least64_t_defined 1
#endif

/*********************************************************************************************************
  fast type (尽最大可能兼容)
*********************************************************************************************************/

#ifndef __int_fast8_t_defined
typedef int8_t      int_fast8_t;
typedef uint8_t     uint_fast8_t;
#define __int_fast8_t_defined 1
#endif

#ifndef __int_fast16_t_defined
typedef int16_t     int_fast16_t;
typedef uint16_t    uint_fast16_t;
#define __int_fast16_t_defined 1
#endif

#ifndef __int_fast32_t_defined
typedef int32_t     int_fast32_t;
typedef uint32_t    uint_fast32_t;
#define __int_fast32_t_defined 1
#endif

#ifndef __int_fast64_t_defined
typedef int64_t     int_fast64_t;
typedef uint64_t    uint_fast64_t;
#define __int_fast64_t_defined 1
#endif

/*********************************************************************************************************
  min max
*********************************************************************************************************/

#define INT8_MIN            -128
#define INT8_MAX            127
#define UINT8_MAX           255

#define INT_LEAST8_MIN      -128
#define INT_LEAST8_MAX      127
#define UINT_LEAST8_MAX     255

#define INT16_MIN           -32768
#define INT16_MAX           32767
#define UINT16_MAX          65535

#define INT_LEAST16_MIN     -32768
#define INT_LEAST16_MAX     32767
#define UINT_LEAST16_MAX    65535

#define INT32_MIN           (-2147483647-1)
#define INT32_MAX           2147483647
#define UINT32_MAX          4294967295U

#define INT_LEAST32_MIN     (-2147483647-1)
#define INT_LEAST32_MAX     2147483647
#define UINT_LEAST32_MAX    4294967295U

#define INT64_MIN           (-9223372036854775807L-1L)
#define INT64_MAX           9223372036854775807L
#define UINT64_MAX          18446744073709551615U

#define INT_LEAST64_MIN     (-9223372036854775807L-1L)
#define INT_LEAST64_MAX     9223372036854775807L
#define UINT_LEAST64_MAX    18446744073709551615U

#define INT_FAST8_MIN       INT8_MIN
#define INT_FAST8_MAX       INT8_MAX
#define UINT_FAST8_MAX      UINT8_MAX

#define INT_FAST16_MIN      INT16_MIN
#define INT_FAST16_MAX      INT16_MAX
#define UINT_FAST16_MAX     UINT16_MAX

#define INT_FAST32_MIN      INT32_MIN
#define INT_FAST32_MAX      INT32_MAX
#define UINT_FAST32_MAX     UINT32_MAX

#define INT_FAST64_MIN      INT64_MIN
#define INT_FAST64_MAX      INT64_MAX
#define UINT_FAST64_MAX     UINT64_MAX

/*********************************************************************************************************
  This must match size_t in stddef.h, currently long unsigned int
*********************************************************************************************************/

#ifndef SIZE_MIN
#define SIZE_MIN            0
#endif

#ifndef SIZE_MAX
#define SIZE_MAX            ULONG_MAX
#endif

#ifndef SSIZE_MIN
#define SSIZE_MIN           LONG_MIN
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX           LONG_MAX
#endif

/*********************************************************************************************************
  This must match sig_atomic_t in <signal.h> (currently int)
*********************************************************************************************************/

#define SIG_ATOMIC_MIN      INT_MIN
#define SIG_ATOMIC_MAX      INT_MAX

#define INTPTR_MIN          INT32_MIN
#define INTPTR_MAX          INT32_MAX
#define UINTPTR_MAX         UINT32_MAX

/*********************************************************************************************************
  This must match ptrdiff_t  in <stddef.h> (currently long int)
*********************************************************************************************************/

#define PTRDIFF_MIN         LONG_MIN
#define PTRDIFF_MAX         LONG_MAX

/*********************************************************************************************************
  Macros for minimum-width integer constant expressions 
*********************************************************************************************************/

#define INT8_C(x)           x
#define UINT8_C(x)          x##U

#define INT16_C(x)          x
#define UINT16_C(x)         x##U

#define INT32_C(x)          x##L
#define UINT32_C(x)         x##UL

#define INT64_C(x)          x##LL
#define UINT64_C(x)         x##ULL

/*********************************************************************************************************
  intmax_t (尽最大可能兼容 newlib use long long as intmax_t)
  sylixos must has 64bits int
*********************************************************************************************************/

typedef int64_t             intmax_t;
typedef uint64_t            uintmax_t;
#define INTMAX_C(x)         x##LL
#define UINTMAX_C(x)        x##ULL

#define INTMAX_MIN          INT64_MIN
#define INTMAX_MAX          INT64_MAX
#define UINTMAX_MAX         UINT64_MAX

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __LIB_STDINT_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
