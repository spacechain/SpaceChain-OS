/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: px_pthread_np.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 05 月 04 日
**
** 描        述: pthread 扩展兼容库.
*********************************************************************************************************/

#ifndef __PX_PTHREAD_NP_H
#define __PX_PTHREAD_NP_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_POSIXEX_EN > 0)

#include "px_pthread.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  pthread NULL attr method (PTHREAD_NULL_ATTR_METHOD_USE_INHERIT is default method)
*********************************************************************************************************/
#define PTHREAD_NULL_ATTR_METHOD_USE_INHERIT        0                   /*  inherit stack size and prio */
#define PTHREAD_NULL_ATTR_METHOD_USE_DEFSETTING     1                   /*  use default attr            */

LW_API int  pthread_null_attr_method_np(int  method, int *old_method);

LW_API int  pthread_attr_get_np(pthread_t  thread, pthread_attr_t *pattr);
LW_API int  pthread_getattr_np(pthread_t thread, pthread_attr_t *pattr);

LW_API int  pthread_setname_np(pthread_t  thread, const char  *name);
LW_API int  pthread_getname_np(pthread_t  thread, char  *name, size_t len);

LW_API int  pthread_setaffinity_np(pthread_t  thread, size_t setsize, const cpu_set_t *set);
LW_API int  pthread_getaffinity_np(pthread_t  thread, size_t setsize, cpu_set_t *set);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIXEX_EN > 0       */
#endif                                                                  /*  __PX_PTHREAD_NP_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
