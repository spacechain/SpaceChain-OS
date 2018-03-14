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
** 文   件   名: pthread_cond.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 条件变量兼容库. (cond 从来不会发生 EINTR)

** BUG:
2012.12.13  由于 SylixOS 支持进程资源回收, 这里开始支持静态初始化.
2013.05.01  If successful, the pthread_cond_*() functions shall return zero; 
            otherwise, an error number shall be returned to indicate the error.
2014.07.04  加入时钟类型设置与获取.
2016.04.13  加入 GJB7714 相关 API 支持.
2016.05.09  隐形初始化确保多线程安全.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  初始化锁
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_ulPCondInitLock;
/*********************************************************************************************************
  mutex 隐形创建
*********************************************************************************************************/
extern void  __pthread_mutex_init_invisible(pthread_mutex_t  *pmutex);
/*********************************************************************************************************
** 函数名称: _posixPCondInit
** 功能描述: 初始化 PCOND 环境.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _posixPCondInit (VOID)
{
    _G_ulPCondInitLock = API_SemaphoreMCreate("pcondinit", LW_PRIO_DEF_CEILING, 
                                              LW_OPTION_INHERIT_PRIORITY | 
                                              LW_OPTION_WAIT_PRIORITY | 
                                              LW_OPTION_DELETE_SAFE, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __pthread_cond_init_invisible
** 功能描述: 条件变量隐形创建. (静态初始化)
** 输　入  : pcond         条件变量控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static void  __pthread_cond_init_invisible (pthread_cond_t  *pcond)
{
    if (pcond) {
        if (pcond->TCD_ulSignal == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreMPend(_G_ulPCondInitLock, LW_OPTION_WAIT_INFINITE);
            if (pcond->TCD_ulSignal == LW_OBJECT_HANDLE_INVALID) {
                pthread_cond_init(pcond, LW_NULL);
            }
            API_SemaphoreMPost(_G_ulPCondInitLock);
        }
    }
}
/*********************************************************************************************************
** 函数名称: pthread_condattr_init
** 功能描述: 初始化条件变量属性块.
** 输　入  : pcondattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_condattr_init (pthread_condattr_t  *pcondattr)
{
    return  ((int)API_ThreadCondAttrInit(pcondattr));
}
/*********************************************************************************************************
** 函数名称: pthread_condattr_destroy
** 功能描述: 销毁条件变量属性块.
** 输　入  : pcondattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_condattr_destroy (pthread_condattr_t  *pcondattr)
{
    return  ((int)API_ThreadCondAttrDestroy(pcondattr));
}
/*********************************************************************************************************
** 函数名称: pthread_condattr_setpshared
** 功能描述: 设置条件变量属性块是否为进程.
** 输　入  : pcondattr     属性
**           ishare        是否为进程共享
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_condattr_setpshared (pthread_condattr_t  *pcondattr, int  ishare)
{
    return  ((int)API_ThreadCondAttrSetPshared(pcondattr, ishare));
}
/*********************************************************************************************************
** 函数名称: pthread_condattr_getpshared
** 功能描述: 获得条件变量属性块是否为进程.
** 输　入  : pcondattr     属性
**           pishare       是否为进程共享
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_condattr_getpshared (const pthread_condattr_t  *pcondattr, int  *pishare)
{
    return  ((int)API_ThreadCondAttrGetPshared(pcondattr, pishare));
}
/*********************************************************************************************************
** 函数名称: pthread_condattr_setclock
** 功能描述: 设置 pthread 时钟类型.
** 输　入  : pcondattr     属性
**           clock_id      时钟类型 CLOCK_REALTIME only
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_condattr_setclock (pthread_condattr_t  *pcondattr, clockid_t  clock_id)
{
    if (!pcondattr) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (clock_id != CLOCK_REALTIME) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_condattr_getclock
** 功能描述: 获取 pthread 时钟类型.
** 输　入  : pcondattr     属性
**           pclock_id     时钟类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_condattr_getclock (const pthread_condattr_t  *pcondattr, clockid_t  *pclock_id)
{
    if (!pcondattr || !pclock_id) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pclock_id = CLOCK_REALTIME;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_cond_init
** 功能描述: 初始化条件变量.
** 输　入  : pcond         条件变量控制块
**           pcondattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cond_init (pthread_cond_t  *pcond, const pthread_condattr_t  *pcondattr)
{
    ULONG       ulAttr = LW_THREAD_PROCESS_SHARED;
    
    if (pcond == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pcondattr) {
        ulAttr = *pcondattr;
    }

    if (API_ThreadCondInit(pcond, ulAttr)) {                            /*  初始化条件变量              */
        return  (errno);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_cond_destroy
** 功能描述: 销毁条件变量.
** 输　入  : pcond         条件变量控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cond_destroy (pthread_cond_t  *pcond)
{
    if ((pcond == LW_NULL) || (pcond->TCD_ulSignal == LW_OBJECT_HANDLE_INVALID)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_ThreadCondDestroy(pcond)) {                                 /*  销毁条件变量                */
        return  (errno);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_cond_signal
** 功能描述: 发送一个条件变量有效信号.
** 输　入  : pcond         条件变量控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cond_signal (pthread_cond_t  *pcond)
{
    if (pcond == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_cond_init_invisible(pcond);
    
    if (API_ThreadCondSignal(pcond)) {
        return  (errno);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_cond_broadcast
** 功能描述: 发送一个条件变量广播信号.
** 输　入  : pcond         条件变量控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cond_broadcast (pthread_cond_t  *pcond)
{
    if (pcond == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_cond_init_invisible(pcond);
    
    if (API_ThreadCondBroadcast(pcond)) {
        return  (errno);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_cond_wait
** 功能描述: 等待一个条件变量广播信号.
** 输　入  : pcond         条件变量控制块
**           pmutex        互斥信号量
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cond_wait (pthread_cond_t  *pcond, pthread_mutex_t  *pmutex)
{
    if ((pcond == LW_NULL) || (pmutex == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_cond_init_invisible(pcond);
    __pthread_mutex_init_invisible(pmutex);
    
    if (API_ThreadCondWait(pcond, pmutex->PMUTEX_ulMutex, LW_OPTION_WAIT_INFINITE)) {
        errno = EINVAL;
        return  (EINVAL);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_cond_timedwait
** 功能描述: 等待一个条件变量广播信号(带有超时).
** 输　入  : pcond         条件变量控制块
**           pmutex        互斥信号量
**           abs_timeout   超时时间 (注意: 这里是绝对时间, 即一个确定的历史时间例如: 2009.12.31 15:36:04)
                           笔者觉得这个不是很爽, 应该再加一个函数可以等待相对时间.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cond_timedwait (pthread_cond_t         *pcond, 
                             pthread_mutex_t        *pmutex,
                             const struct timespec  *abs_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((pcond == LW_NULL) || (pmutex == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((abs_timeout == LW_NULL) || 
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_cond_init_invisible(pcond);
    __pthread_mutex_init_invisible(pmutex);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);              /*  转换超时时间                */

    ulError = API_ThreadCondWait(pcond, pmutex->PMUTEX_ulMutex, ulTimeout);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (ETIMEDOUT);
        
    } else if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
        
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_cond_reltimedwait_np
** 功能描述: 等待一个条件变量广播信号(带有超时).
** 输　入  : pcond         条件变量控制块
**           pmutex        互斥信号量
**           rel_timeout   相对超时时间.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_cond_reltimedwait_np (pthread_cond_t         *pcond, 
                                   pthread_mutex_t        *pmutex,
                                   const struct timespec  *rel_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((pcond == LW_NULL) || (pmutex == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((rel_timeout == LW_NULL) || 
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_cond_init_invisible(pcond);
    __pthread_mutex_init_invisible(pmutex);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);               /*  转换超时时间                */
    
    ulError = API_ThreadCondWait(pcond, pmutex->PMUTEX_ulMutex, ulTimeout);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (ETIMEDOUT);
        
    } else if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
        
    } else {
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** 函数名称: pthread_cond_getinfo
** 功能描述: 获得条件变量信息
** 输　入  : pcond         条件变量控制块
**           info          条件变量信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_cond_getinfo (pthread_cond_t  *pcond, pthread_cond_info_t  *info)
{
    if (!pcond || !info) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    info->signal = pcond->TCD_ulSignal;
    info->mutex  = pcond->TCD_ulMutex;
    info->cnt    = pcond->TCD_ulCounter;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_cond_show
** 功能描述: 显示条件变量信息
** 输　入  : pcond         条件变量控制块
**           level         显示等级
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cond_show (pthread_cond_t  *pcond, int  level)
{
    if (!pcond) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((level != 0) && (level != 1)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {
        errno = ECALLEDINISR;
        return  (ECALLEDINISR);
    }
    
    if (pcond->TCD_ulSignal == LW_OBJECT_HANDLE_INVALID) {
        errno = EMNOTINITED;
        return  (EMNOTINITED);
    }
    
    printf("cond show >>\n\n");
    printf("cond signal handle : %lx\n", pcond->TCD_ulSignal);
    printf("cond mutex  handle : %lx\n", pcond->TCD_ulMutex);
    printf("cond counter       : %lu\n", pcond->TCD_ulCounter);
    
    if (level == 1) {
        printf("cond signel show >>\n\n");
        API_SemaphoreShow(pcond->TCD_ulSignal);
        
        if (pcond->TCD_ulMutex) {
            printf("cond mutex show >>\n\n");
            API_SemaphoreShow(pcond->TCD_ulMutex);
        }
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
