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
** 文   件   名: lib_limits.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 07 日
**
** 描        述: limits.h.
*********************************************************************************************************/

#ifndef __LIB_LIMITS_H
#define __LIB_LIMITS_H

#include "../SylixOS/kernel/include/k_kernel.h"

#ifndef CHAR_BIT
#define CHAR_BIT            __ARCH_CHAR_BIT
#endif

#ifndef CHAR_MAX
#define CHAR_MAX            __ARCH_CHAR_MAX
#endif

#ifndef CHAR_MIN
#define CHAR_MIN            __ARCH_CHAR_MIN
#endif

#ifndef SHRT_MAX
#define SHRT_MAX            __ARCH_SHRT_MAX
#endif

#ifndef SHRT_MIN
#define SHRT_MIN            __ARCH_SHRT_MIN
#endif

#ifndef INT_MAX
#define INT_MAX             __ARCH_INT_MAX
#endif

#ifndef INT_MIN
#define INT_MIN             __ARCH_INT_MIN
#endif

#ifndef LONG_MAX
#define LONG_MAX            __ARCH_LONG_MAX
#endif

#ifndef LONG_MIN
#define LONG_MIN            __ARCH_LONG_MIN
#endif

#ifndef SCHAR_MAX
#define SCHAR_MAX           __ARCH_SCHAR_MAX
#endif

#ifndef SCHAR_MIN
#define SCHAR_MIN           __ARCH_SCHAR_MIN
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX           __ARCH_UCHAR_MAX
#endif

#ifndef USHRT_MAX
#define USHRT_MAX           __ARCH_USHRT_MAX
#endif

#ifndef UINT_MAX
#define UINT_MAX            __ARCH_UINT_MAX
#endif

#ifndef ULONG_MAX
#define ULONG_MAX           __ARCH_ULONG_MAX
#endif

#ifndef QUAD_MIN
#define QUAD_MIN            (-QUAD_MAX - 1)
#endif

#ifndef QUAD_MAX
#define QUAD_MAX            9223372036854775807LL                       /*  7fff_ffff_ffff_ffff         */
#endif

#ifndef UQUAD_MAX
#define UQUAD_MAX           (2ULL * QUAD_MAX + 1)
#endif

#ifndef LLONG_MAX
#define LLONG_MAX           QUAD_MAX
#endif

#ifndef LLONG_MIN
#define LLONG_MIN           QUAD_MIN
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX          UQUAD_MAX
#endif

#ifndef SIZE_T_MAX
#define SIZE_T_MAX          ULONG_MAX
#endif

#endif                                                                  /*  __LIB_LIMITS_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
