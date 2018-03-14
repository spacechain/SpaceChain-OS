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
** 文   件   名: px_pthread_attr.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 26 日
**
** 描        述: pthread 属性兼容库.

** BUG:
2013.09.17  为了提高未来的可扩展性, 这里增加 pthread_attr_t 大小, 但是成员进行重新排序, 避免对较早软件产生
            兼容性问题.
*********************************************************************************************************/

#ifndef __PX_PTHREAD_ATTR_H
#define __PX_PTHREAD_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#include "px_sched_param.h"                                             /*  调度参数                    */

/*********************************************************************************************************
  pthread 句柄
*********************************************************************************************************/

typedef LW_OBJECT_HANDLE    pthread_t;

/*********************************************************************************************************
  pthread attr
*********************************************************************************************************/

#define PTHREAD_CREATE_DETACHED             0
#define PTHREAD_CREATE_JOINABLE             1                           /*  default                     */

#define PTHREAD_INHERIT_SCHED               0
#define PTHREAD_EXPLICIT_SCHED              1                           /*  default                     */

#define PTHREAD_SCOPE_PROCESS               0
#define PTHREAD_SCOPE_SYSTEM                1                           /*  default                     */

#if (LW_CFG_GJB7714_EN > 0) && !defined(__SYLIXOS_POSIX)

typedef struct {
    char                   *name;                                       /*  名字                        */
    void                   *stackaddr;                                  /*  指定堆栈地址                */
    size_t                  stackguard;                                 /*  堆栈警戒区域大小            */
    size_t                  stacksize;                                  /*  堆栈大小                    */
    int                     schedpolicy;                                /*  调度策略                    */
    int                     inheritsched;                               /*  是否继承调度参数            */
    unsigned long           option;                                     /*  选项                        */
    struct sched_param      schedparam;                                 /*  调度参数                    */
    ULONG                   reservepad[8];                              /*  保留                        */
} pthread_attr_t;

#else

typedef struct {
    char                   *PTHREADATTR_pcName;                         /*  名字                        */
    void                   *PTHREADATTR_pvStackAddr;                    /*  指定堆栈地址                */
    size_t                  PTHREADATTR_stStackGuard;                   /*  堆栈警戒区域大小            */
    size_t                  PTHREADATTR_stStackByteSize;                /*  堆栈大小                    */
    int                     PTHREADATTR_iSchedPolicy;                   /*  调度策略                    */
    int                     PTHREADATTR_iInherit;                       /*  是否继承调度参数            */
    unsigned long           PTHREADATTR_ulOption;                       /*  选项                        */
    struct sched_param      PTHREADATTR_schedparam;                     /*  调度参数                    */
    ULONG                   PTHREADATTR_ulPad[8];                       /*  保留                        */
} pthread_attr_t;

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __PX_PTHREAD_ATTR_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
