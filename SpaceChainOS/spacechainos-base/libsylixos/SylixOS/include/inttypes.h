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
** 文   件   名: inttypes.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 07 日
**
** 描        述: 标准数据类型基本操作.
*********************************************************************************************************/

#ifndef __INTTYPES_H
#define __INTTYPES_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

#include "../SylixOS/lib/libc/inttypes/lib_inttypes.h"                  /*  此头文件默认没有加入        */

__LW_RETU_FUNC_DECLARE(intmax_t, imaxabs, (intmax_t j))
__LW_RETU_FUNC_DECLARE(imaxdiv_t, imaxdiv, (intmax_t numer, intmax_t denomer))
__LW_RETU_FUNC_DECLARE(intmax_t, strtoimax, (const char *nptr, char **endptr, int base))
__LW_RETU_FUNC_DECLARE(uintmax_t, strtoumax, (const char *nptr, char **endptr, int base))
__LW_RETU_FUNC_DECLARE(intmax_t, wcstoimax, (const wchar_t *nptr, wchar_t **endptr, int base))
__LW_RETU_FUNC_DECLARE(uintmax_t, wcstoumax, (const wchar_t *nptr, wchar_t **endptr, int base))

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __INTTYPES_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
