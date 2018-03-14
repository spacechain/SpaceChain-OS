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
** 文   件   名: types.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __SYS_TYPES_H
#define __SYS_TYPES_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#include "stdint.h"

/*********************************************************************************************************
  statvfs
*********************************************************************************************************/

#ifndef __fsblkcnt_t_defined
typedef uint64_t fsblkcnt_t;
#define __fsblkcnt_t_defined 1
#endif

#ifndef __fsfilcnt_t_defined
typedef uint64_t fsfilcnt_t;
#define __fsfilcnt_t_defined 1
#endif

/*********************************************************************************************************
  linux 兼容
*********************************************************************************************************/

#ifndef __major_t_defined
typedef uint_t  major_t;
#define __major_t_defined 1
#endif

#ifndef __minor_t_defined
typedef uint_t  minor_t;
#define __minor_t_defined 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

LW_API dev_t    makedev(major_t  major, minor_t minor);
LW_API major_t  major(dev_t dev);
LW_API minor_t  minor(dev_t dev);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  id type
*********************************************************************************************************/

#ifndef __id_t_defined
typedef long        id_t;
#define __id_t_defined 1
#endif

/*********************************************************************************************************
  xsi ipc
*********************************************************************************************************/

#ifndef __key_t_defined
typedef uint64_t    key_t;
#define __key_t_defined 1
#endif

/*********************************************************************************************************
  BSD
*********************************************************************************************************/

#ifndef ___int8_t_defined
typedef int8_t      __int8_t;
typedef uint8_t     __uint8_t;
#define ___int8_t_defined 1
#endif

#ifndef ___int16_t_defined
typedef int16_t     __int16_t;
typedef uint16_t    __uint16_t;
#define ___int16_t_defined 1
#endif

#ifndef ___int32_t_defined
typedef int32_t     __int32_t;
typedef uint32_t    __uint32_t;
#define ___int32_t_defined 1
#endif

#if LW_CFG_CPU_WORD_LENGHT == 32
#ifndef ___int64_t_defined
typedef int64_t     __int64_t;
typedef uint64_t    __uint64_t;
#define ___int64_t_defined 1
#endif
#endif

#ifndef ___int_least8_t_defined
typedef int_least8_t      __int_least8_t;
typedef uint_least8_t     __uint_least8_t;
#define ___int_least8_t_defined 1
#endif

#ifndef ___int_least16_t_defined
typedef int_least16_t     __int_least16_t;
typedef uint_least16_t    __uint_least16_t;
#define ___int_least16_t_defined 1
#endif

#ifndef ___int_least32_t_defined
typedef int_least32_t     __int_least32_t;
typedef uint_least32_t    __uint_least32_t;
#define ___int_least32_t_defined 1
#endif

#if LW_CFG_CPU_WORD_LENGHT == 32
#ifndef ___int_least64_t_defined
typedef int_least64_t     __int_least64_t;
typedef uint_least64_t    __uint_least64_t;
#define ___int_least64_t_defined 1
#endif
#endif

/*********************************************************************************************************
  socket
*********************************************************************************************************/

#ifndef __socklen_t_defined
typedef uint_t     socklen_t;
#define __socklen_t_defined 1
#endif

/*********************************************************************************************************
  fix newlib _default_types.h
*********************************************************************************************************/

#undef __INT8_TYPE__
#undef __UINT8_TYPE__

#undef __INT16_TYPE__
#undef __UINT16_TYPE__

#undef __INT32_TYPE__
#undef __UINT32_TYPE__

#undef __INT64_TYPE__
#undef __UINT64_TYPE__

#undef __INT_LEAST8_TYPE__
#undef __UINT_LEAST8_TYPE__

#undef __INT_LEAST16_TYPE__
#undef __UINT_LEAST16_TYPE__

#undef __INT_LEAST32_TYPE__
#undef __UINT_LEAST32_TYPE__

#undef __INT_LEAST64_TYPE__
#undef __UINT_LEAST64_TYPE__

/*********************************************************************************************************
  wint_t & mbstate_t type
*********************************************************************************************************/

#ifndef _WINT_T
#define _WINT_T
#ifndef __WINT_TYPE__
#define __WINT_TYPE__ unsigned int
#endif
typedef __WINT_TYPE__ wint_t;
#endif                                                                  /* _WINT_T                      */

#ifndef __mbstate_t_defined
typedef struct {
    int  count;
    union {
        wint_t  wch;
        unsigned char wchb[4];
    } value;
} _mbstate_t;
#define __mbstate_t_defined 1
#endif

typedef _mbstate_t mbstate_t;

#endif                                                                  /*  __SYS_TYPES_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
