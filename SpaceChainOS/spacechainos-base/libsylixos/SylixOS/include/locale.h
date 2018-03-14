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
** 文   件   名: locale.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 05 月 30 日
**
** 描        述: 本地化设定/获取.这里没有实现任何功能, 只是为了兼容性需要而已, 
                 如果需要完整 locale 支持, 则需要外部 locale 库支持.
*********************************************************************************************************/

#ifndef __LOCALE_H
#define __LOCALE_H

#ifndef NULL
#include <stddef.h>
#endif                                                                  /*  NULL                        */

#include "sys/cdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

#include "../SylixOS/lib/libc/locale/lib_locale.h"

extern struct lconv *localeconv(void);
extern char *setlocale(int category, const char *locale);

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __LOCALE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
