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
** 文   件   名: iconv.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 17 日
**
** 描        述: 多国语言编码转换, 需要外部库支持.
*********************************************************************************************************/

#ifndef __ICONV_H
#define __ICONV_H

#include <stddef.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(ICONV_T_DEFINED) && !defined(iconv_t) && !defined(__iconv_t_defined) 
typedef void *iconv_t;
#define ICONV_T_DEFINED
#define __iconv_t_defined
#endif                                                                  /*  ICONV_T_DEFINED             */

__BEGIN_NAMESPACE_STD

iconv_t iconv_open(const char *, const char *);
size_t  iconv(iconv_t, char **, size_t *, char **, size_t *);
int     iconv_close(iconv_t);

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __ICONV_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
