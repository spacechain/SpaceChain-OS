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
** 文   件   名: px_sched_rms.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 04 月 17 日
**
** 描        述: 高精度 RMS 调度器 (使用 LW_CFG_TIME_HIGH_RESOLUTION_EN 作为时间精度保证).
*********************************************************************************************************/

#ifndef __PX_SCHED_RMS_H
#define __PX_SCHED_RMS_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_POSIXEX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  RMS 结构
*********************************************************************************************************/

typedef struct {
    int              PRMS_iStatus;
    struct timespec  PRMS_tsSave;
    void            *PRMS_pvPad[16];
} sched_rms_t;

#ifdef __SYLIXOS_KERNEL
#define PRMS_STATUS_INACTIVE    0
#define PRMS_STATUS_ACTIVE      1
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API int      sched_rms_init(sched_rms_t  *prms, pthread_t  thread);
LW_API int      sched_rms_destroy(sched_rms_t  *prms);
LW_API int      sched_rms_period(sched_rms_t  *prms, const struct timespec *period);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIXEX_EN > 0       */
#endif                                                                  /*  __PX_SCHED_RMS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
