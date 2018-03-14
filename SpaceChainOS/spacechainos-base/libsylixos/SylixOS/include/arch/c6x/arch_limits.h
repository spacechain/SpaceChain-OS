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
** 文   件   名: arch_limits.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 02 月 28 日
**
** 描        述: TI C6X DSP limits 相关.
*********************************************************************************************************/

#ifndef __C6X_ARCH_LIMITS_H
#define __C6X_ARCH_LIMITS_H

#ifndef __ARCH_CHAR_BIT
#define __ARCH_CHAR_BIT            8
#endif

#ifndef __ARCH_CHAR_MAX
#define __ARCH_CHAR_MAX            127
#endif

#ifndef __ARCH_CHAR_MIN
#define __ARCH_CHAR_MIN            (-127-1)
#endif

#ifndef __ARCH_SHRT_MAX
#define __ARCH_SHRT_MAX            32767
#endif

#ifndef __ARCH_SHRT_MIN
#define __ARCH_SHRT_MIN            (-32767-1)
#endif

#ifndef __ARCH_INT_MAX
#define __ARCH_INT_MAX             2147483647
#endif

#ifndef __ARCH_INT_MIN
#define __ARCH_INT_MIN             (-2147483647-1)
#endif

#ifndef __ARCH_LONG_MAX
#define __ARCH_LONG_MAX            2147483647
#endif

#ifndef __ARCH_LONG_MIN
#define __ARCH_LONG_MIN            (-2147483647-1)
#endif

#ifndef __ARCH_SCHAR_MAX
#define __ARCH_SCHAR_MAX           127
#endif

#ifndef __ARCH_SCHAR_MIN
#define __ARCH_SCHAR_MIN           (-127-1)
#endif

#ifndef __ARCH_UCHAR_MAX
#define __ARCH_UCHAR_MAX           255
#endif

#ifndef __ARCH_USHRT_MAX
#define __ARCH_USHRT_MAX           65535
#endif

#ifndef __ARCH_UINT_MAX
#ifdef  __STDC__
#define __ARCH_UINT_MAX            4294967295u
#else
#define __ARCH_UINT_MAX            4294967295
#endif
#endif

#ifndef __ARCH_ULONG_MAX
#ifdef  __STDC__
#define __ARCH_ULONG_MAX           4294967295u
#else
#define __ARCH_ULONG_MAX           4294967295
#endif
#endif

#endif                                                                  /*  __C6X_ARCH_LIMITS_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
