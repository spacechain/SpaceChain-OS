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
** 文   件   名: sched.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: posix 调度兼容库.

** BUG:
2011.03.26  Higher numerical values for the priority represent higher priorities.
2014.07.04  调度参数第一个为进程 ID.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "../include/px_sched.h"                                        /*  已包含操作系统头文件        */
#include "../include/posixLib.h"                                        /*  posix 内部公共库            */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: sched_get_priority_max
** 功能描述: 获得调度器允许的最大优先级 (pthread 线程不能与 idle 同优先级!)
** 输　入  : iPolicy       调度策略 (目前无用)
** 输　出  : 最大优先级
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_get_priority_max (int  iPolicy)
{
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    return  (__PX_PRIORITY_MAX);                                        /*  254                         */
}
/*********************************************************************************************************
** 函数名称: sched_get_priority_min
** 功能描述: 获得调度器允许的最小优先级
** 输　入  : iPolicy       调度策略 (目前无用)
** 输　出  : 最小优先级
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_get_priority_min (int  iPolicy)
{
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (__PX_PRIORITY_MIN);                                        /*  1                           */
}
/*********************************************************************************************************
** 函数名称: sched_yield
** 功能描述: 将当前任务插入到同优先级调度器链表的最后, 主动让出一次 CPU 调度.
** 输　入  : NONE
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_yield (void)
{
    API_ThreadYield(API_ThreadIdSelf());
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sched_setparam
** 功能描述: 设置指定任务调度器参数
** 输　入  : pid           进程 / 线程 ID
**           pschedparam   调度器参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_setparam (pid_t  pid, const struct sched_param  *pschedparam)
{
    UINT8               ucPriority;
    ULONG               ulError;
    LW_OBJECT_HANDLE    ulThread;
    
    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
        (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    ulError = API_ThreadSetPriority(ulThread, ucPriority);
    if (ulError) {
        errno = ESRCH;
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: sched_getparam
** 功能描述: 获得指定任务调度器参数
** 输　入  : pid           进程 / 线程 ID
**           pschedparam   调度器参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_getparam (pid_t  pid, struct sched_param  *pschedparam)
{
    UINT8               ucPriority;
    ULONG               ulError;
    LW_OBJECT_HANDLE    ulThread;
    
    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    ulError = API_ThreadGetPriority(ulThread, &ucPriority);
    if (ulError) {
        errno = ESRCH;
        return  (PX_ERROR);
    } else {
        pschedparam->sched_priority = PX_PRIORITY_CONVERT(ucPriority);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: sched_setscheduler
** 功能描述: 设置指定任务调度器
** 输　入  : pid           进程 / 线程 ID
**           iPolicy       调度策略
**           pschedparam   调度器参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_setscheduler (pid_t                      pid, 
                         int                        iPolicy, 
                         const struct sched_param  *pschedparam)
{
    UINT8               ucPriority;
    UINT8               ucActivatedMode;
    LW_OBJECT_HANDLE    ulThread;

    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
        (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (API_ThreadGetSchedParam(ulThread,
                                LW_NULL,
                                &ucActivatedMode)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
    
    API_ThreadSetSchedParam(ulThread, (UINT8)iPolicy, ucActivatedMode);
    
    if (API_ThreadSetPriority(ulThread, ucPriority)) {
        errno = ESRCH;
        return  (PX_ERROR);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: sched_getscheduler
** 功能描述: 获得指定任务调度器
** 输　入  : pid           进程 / 线程 ID
** 输　出  : 调度策略
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_getscheduler (pid_t  pid)
{
    UINT8               ucPolicy;
    LW_OBJECT_HANDLE    ulThread;
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (API_ThreadGetSchedParam(ulThread,
                                &ucPolicy,
                                LW_NULL)) {
        errno = ESRCH;
        return  (PX_ERROR);
    } else {
        return  ((int)ucPolicy);
    }
}
/*********************************************************************************************************
** 函数名称: sched_rr_get_interval
** 功能描述: 获得指定任务调度器
** 输　入  : pid           进程 / 线程 ID
**           interval      current execution time limit.
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_rr_get_interval (pid_t  pid, struct timespec  *interval)
{
    UINT8               ucPolicy;
    UINT16              usCounter = 0;
    LW_OBJECT_HANDLE    ulThread;

    if (!interval) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

#if LW_CFG_MODULELOADER_EN > 0
    if (pid == 0) {
        pid =  getpid();
    }
    if (pid == 0) {
        ulThread = API_ThreadIdSelf();
    } else {
        ulThread = vprocMainThread(pid);
    }
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#else
    ulThread = (LW_OBJECT_HANDLE)pid;
    PX_ID_VERIFY(ulThread, LW_OBJECT_HANDLE);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (API_ThreadGetSchedParam(ulThread,
                                &ucPolicy,
                                LW_NULL)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
    if (ucPolicy != LW_OPTION_SCHED_RR) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_ThreadGetSliceEx(ulThread, LW_NULL, &usCounter)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
    
    __tickToTimespec((ULONG)usCounter, interval);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sched_setaffinity
** 功能描述: 设置进程调度的 CPU 集合
** 输　入  : pid           进程 / 线程 ID
**           setsize       CPU 集合大小
**           set           CPU 集合
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_setaffinity (pid_t pid, size_t setsize, const cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if (!setsize || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (vprocSetAffinity(pid, setsize, (PLW_CLASS_CPUSET)set)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sched_getaffinity
** 功能描述: 获取进程调度的 CPU 集合
** 输　入  : pid           进程 / 线程 ID
**           setsize       CPU 集合大小
**           set           CPU 集合
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  sched_getaffinity (pid_t pid, size_t setsize, cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if ((setsize < sizeof(cpu_set_t)) || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (vprocGetAffinity(pid, setsize, set)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sched_settimeslice
** 功能描述: 设置系统调度时间片
** 输　入  : ticks         时间片
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API  
int  sched_settimeslice (UINT32  ticks)
{
    INT             i;
    PLW_CLASS_TCB   ptcb;
    
    if (geteuid() != 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    if (ticks > 100) {
        errno = ERANGE;
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  获得 TCB 控制块             */
        if (ptcb == LW_NULL) {                                          /*  线程不存在                  */
            continue;
        }
        
        ptcb->TCB_usSchedSlice = (UINT16)ticks;
    }
    
    LW_SCHED_SLICE = (UINT16)ticks;
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sched_gettimeslice
** 功能描述: 获得系统调度时间片
** 输　入  : NONE
** 输　出  : 时间片
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
unsigned int  sched_gettimeslice (void)
{
    unsigned int  ticks;
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    ticks = (unsigned int)LW_SCHED_SLICE;
    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ticks);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
