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
** 文   件   名: sched_rms.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 04 月 17 日
**
** 描        述: 高精度 RMS 调度器 (使用 LW_CFG_TIME_HIGH_RESOLUTION_EN 作为时间精度保证).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "../include/px_pthread.h"
#include "../include/px_sched_rms.h"                                    /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_POSIXEX_EN > 0
/*********************************************************************************************************
** 函数名称: sched_rms_init
** 功能描述: 初始化 RMS 调度器
** 输　入  : prms      RMS 调度器
**           thread    需要调用 RMS 的线程.
** 输　出  : 是否初始化成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_rms_init (sched_rms_t  *prms, pthread_t  thread)
{
    if (!prms) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_ThreadSetSchedParam(thread, LW_OPTION_SCHED_FIFO, 
                                LW_OPTION_RESPOND_IMMIEDIA)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
    
    lib_bzero(prms, sizeof(sched_rms_t));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sched_rms_destroy
** 功能描述: 删除 RMS 调度器
** 输　入  : prms      RMS 调度器
** 输　出  : 是否删除成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_rms_destroy (sched_rms_t  *prms)
{
    if (!prms) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    prms->PRMS_iStatus = PRMS_STATUS_INACTIVE;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sched_rms_period
** 功能描述: 删除 RMS 调度器
** 输　入  : prms      RMS 调度器
**           period    RMS 周期
** 输　出  : 0 表示正确
**           error == EINTR    表示被信号激活.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_rms_period (sched_rms_t  *prms, const struct timespec *period)
{
    struct timespec temp;
    struct timespec etime;
    
    if (!prms || !period) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    switch (prms->PRMS_iStatus) {
    
    case PRMS_STATUS_INACTIVE:
        lib_clock_gettime(CLOCK_MONOTONIC, &prms->PRMS_tsSave);         /*  获得当前时间                */
        prms->PRMS_iStatus = PRMS_STATUS_ACTIVE;
        return  (ERROR_NONE);
        
    case PRMS_STATUS_ACTIVE:
        lib_clock_gettime(CLOCK_MONOTONIC, &temp);
        __timespecSub2(&etime, &temp, &prms->PRMS_tsSave);
        if (__timespecLeftTime(period, &etime)) {                       /*  执行时间超过周期            */
            lib_clock_gettime(CLOCK_MONOTONIC, &prms->PRMS_tsSave);     /*  获得当前时间                */
            errno = EOVERFLOW;
            return  (PX_ERROR);
        }
        
        __timespecSub2(&temp, period, &etime);
        
        /*
         *  注意: 这里直接加上周期是为了让每次测算都是以一个固定周期律进行
         *        提高周期精度. (不使用 lib_clock_gettime())
         */
        __timespecAdd(&prms->PRMS_tsSave, period);                      /*  以确定周期运行              */
        return  (nanosleep(&temp, LW_NULL));
        
    default:
        errno = ENOTSUP;
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
