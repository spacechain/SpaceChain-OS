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
** 文   件   名: strings.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 11 月 11 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __STRINGS_H
#define __STRINGS_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

__LW_RETU_FUNC_DECLARE(int, bcmp, (const void *pvMem1, const void *pvMem2, size_t stCount))
__LW_RETU_FUNC_DECLARE(void, bcopy, (const void *pvSrc, void *pvDest, size_t stN))
__LW_RETU_FUNC_DECLARE(void, bzero, (void *pvStr, size_t stCount))
__LW_RETU_FUNC_DECLARE(int, ffs, (int valu))
__LW_RETU_FUNC_DECLARE(char *, rindex, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(char *, index, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(int, strcasecmp, (const char *pcStr1, const char *pcStr2))
__LW_RETU_FUNC_DECLARE(int, strncasecmp, (const char *pcStr1, const char *pcStr2, size_t  stLen))

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __STRINGS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
