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
** 文   件   名: px_sched_param.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 26 日
**
** 描        述: posix 调度兼容库 struct sched_param 定义.

** BUG:
2013.09.17  考虑到兼容性 struct sched_param 结构加入 sched_pad 保留区, 同时避免对现有软件造成兼容性影响.
*********************************************************************************************************/

#ifndef __PX_SCHED_PARAM_H
#define __PX_SCHED_PARAM_H

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  sched priority convert with SylixOS
*********************************************************************************************************/

#define PX_PRIORITY_CONVERT(prio)   (LW_PRIO_LOWEST - (prio))

/*********************************************************************************************************
  sched policies
*********************************************************************************************************/

struct sched_param {
    int                 sched_priority;                                 /*  POSIX 调度优先级            */
                                                                        /*  SCHED_SPORADIC parameter    */
    int                 sched_ss_low_priority;                          /*  Low scheduling priority for */
                                                                        /*  sporadic server.            */
    struct timespec     sched_ss_repl_period;                           /*  Replenishment period for    */
                                                                        /*  sporadic server.            */
    struct timespec     sched_ss_init_budget;                           /*  Initial budget for sporadic */
                                                                        /*  server.                     */
    int                 sched_ss_max_repl;                              /*  Max pending replenishments  */
                                                                        /*  for sporadic server.        */
    ULONG               sched_pad[12];                                  /*  扩展保留                    */
};

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __PX_SCHED_PARAM_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
