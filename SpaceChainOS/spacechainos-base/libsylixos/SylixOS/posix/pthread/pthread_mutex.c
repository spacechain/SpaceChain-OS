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
** 文   件   名: pthread_mutex.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 互斥信号量兼容库. (mutex 从来不会发生 EINTR)

** BUG:
2010.01.13  初始化互斥量时, 如果没有指定属性, 则是用默认属性.
2011.05.30  pthread_mutex_init() 不再判断 pmutex 中互斥量的有效性.
2012.06.18  Higher numerical values for the priority represent higher priorities.
2012.12.08  加入 PTHREAD_MUTEX_NORMAL 和 PTHREAD_MUTEX_ERRORCHECK 的支持.
2012.12.13  由于 SylixOS 支持进程资源回收, 这里开始支持静态初始化.
2013.05.01  Upon successful completion, pthread_mutexattr_*() and pthread_mutex_*() shall return zero; 
            otherwise, an error number shall be returned to indicate the error.
2016.05.08  隐形初始化确保多线程安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
#include "../include/posixLib.h"                                        /*  posix 内部公共库            */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  默认属性
*********************************************************************************************************/
static const pthread_mutexattr_t    _G_pmutexattrDefault = {
        1,
        PTHREAD_MUTEX_DEFAULT,                                          /*  允许递归调用                */
        PTHREAD_MUTEX_CEILING,
        (LW_OPTION_INHERIT_PRIORITY | LW_OPTION_WAIT_PRIORITY)          /*  PTHREAD_PRIO_NONE           */
};
/*********************************************************************************************************
  初始化锁
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_ulPMutexInitLock;
/*********************************************************************************************************
** 函数名称: _posixPMutexInit
** 功能描述: 初始化 MUTEX 环境.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _posixPMutexInit (VOID)
{
    if (!_G_ulPMutexInitLock) {
        _G_ulPMutexInitLock = API_SemaphoreMCreate("pmutexinit", LW_PRIO_DEF_CEILING,
                                                   LW_OPTION_INHERIT_PRIORITY |
                                                   LW_OPTION_WAIT_PRIORITY |
                                                   LW_OPTION_DELETE_SAFE, LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __pthread_mutex_init_invisible
** 功能描述: 互斥量隐形创建. (静态初始化) (这里不是 static 因为 pthread cond 要使用)
** 输　入  : pmutex         句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  __pthread_mutex_init_invisible (pthread_mutex_t  *pmutex)
{
    if (pmutex) {
        if (pmutex->PMUTEX_ulMutex == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreMPend(_G_ulPMutexInitLock, LW_OPTION_WAIT_INFINITE);
            if (pmutex->PMUTEX_ulMutex == LW_OBJECT_HANDLE_INVALID) {
                pthread_mutex_init(pmutex, LW_NULL);
            }
            API_SemaphoreMPost(_G_ulPMutexInitLock);
        }
    }
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_init
** 功能描述: 初始化互斥量属性块.
** 输　入  : pmutexattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_init (pthread_mutexattr_t *pmutexattr)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pmutexattr = _G_pmutexattrDefault;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_destroy
** 功能描述: 销毁一个互斥量属性块.
** 输　入  : pmutexattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_destroy (pthread_mutexattr_t *pmutexattr)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pmutexattr->PMUTEXATTR_iIsEnable = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_setprioceiling
** 功能描述: 设置一个互斥量属性块的天花板优先级.
** 输　入  : pmutexattr     属性
**           prioceiling    优先级
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setprioceiling (pthread_mutexattr_t *pmutexattr, int  prioceiling)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((prioceiling < __PX_PRIORITY_MIN) ||
        (prioceiling > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pmutexattr->PMUTEXATTR_iPrioceiling = prioceiling;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getprioceiling
** 功能描述: 获得一个互斥量属性块的天花板优先级.
** 输　入  : pmutexattr     属性
**           prioceiling    优先级(返回)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getprioceiling (const pthread_mutexattr_t *pmutexattr, int  *prioceiling)
{
    if ((pmutexattr == LW_NULL) || (prioceiling == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *prioceiling = pmutexattr->PMUTEXATTR_iPrioceiling;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_setprotocol
** 功能描述: 设置一个互斥量属性块的算法类型.
** 输　入  : pmutexattr     属性
**           protocol       算法
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setprotocol (pthread_mutexattr_t *pmutexattr, int  protocol)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    switch (protocol) {
    
    case PTHREAD_PRIO_NONE:                                             /*  默认使用优先级继承算法      */
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_INHERIT_PRIORITY;
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_WAIT_PRIORITY;    /*  fifo                        */
        break;
    
    case PTHREAD_PRIO_INHERIT:
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_INHERIT_PRIORITY; /*  优先级继承算法              */
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_WAIT_PRIORITY;
        break;
        
    case PTHREAD_PRIO_PROTECT:
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_INHERIT_PRIORITY; /*  优先级天花板                */
        pmutexattr->PMUTEXATTR_ulOption |=  LW_OPTION_WAIT_PRIORITY;
        break;
        
    default:
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_getprotocol
** 功能描述: 获得一个互斥量属性块的算法类型.
** 输　入  : pmutexattr     属性
**           protocol       算法(返回)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getprotocol (const pthread_mutexattr_t *pmutexattr, int  *protocol)
{
    if ((pmutexattr == LW_NULL) || (protocol == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_INHERIT_PRIORITY) &&
        (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_WAIT_PRIORITY)) {
        *protocol = PTHREAD_PRIO_INHERIT;
    } else if (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_WAIT_PRIORITY) {
        *protocol = PTHREAD_PRIO_PROTECT;
    } else {
        *protocol = PTHREAD_PRIO_NONE;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_setpshared
** 功能描述: 设置一个互斥量属性块是否进程共享.
** 输　入  : pmutexattr     属性
**           pshared        共享
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setpshared (pthread_mutexattr_t *pmutexattr, int  pshared)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_getpshared
** 功能描述: 获取一个互斥量属性块是否进程共享.
** 输　入  : pmutexattr     属性
**           pshared        共享(返回)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getpshared (const pthread_mutexattr_t *pmutexattr, int  *pshared)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pshared) {
        *pshared = PTHREAD_PROCESS_PRIVATE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_settype
** 功能描述: 设置一个互斥量属性块类型.
** 输　入  : pmutexattr     属性
**           type           类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_settype (pthread_mutexattr_t *pmutexattr, int  type)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((type != PTHREAD_MUTEX_NORMAL) &&
        (type != PTHREAD_MUTEX_ERRORCHECK) &&
        (type != PTHREAD_MUTEX_RECURSIVE)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pmutexattr->PMUTEXATTR_iType = type;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_gettype
** 功能描述: 获得一个互斥量属性块类型.
** 输　入  : pmutexattr     属性
**           type           类型(返回)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_gettype (const pthread_mutexattr_t *pmutexattr, int  *type)
{
    if ((pmutexattr == LW_NULL) || (type == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *type = pmutexattr->PMUTEXATTR_iType;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_setwaitqtype
** 功能描述: 设置互斥量等待队列类型.
** 输　入  : pmutexattr     属性
**           waitq_type     等待队列类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setwaitqtype (pthread_mutexattr_t  *pmutexattr, int  waitq_type)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (waitq_type == PTHREAD_WAITQ_PRIO) {
        pmutexattr->PMUTEXATTR_ulOption |= LW_OPTION_WAIT_PRIORITY;
    
    } else if (waitq_type == PTHREAD_WAITQ_FIFO) {
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_WAIT_PRIORITY;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_getwaitqtype
** 功能描述: 获得互斥量等待队列类型.
** 输　入  : pmutexattr     属性
**           waitq_type     等待队列类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getwaitqtype (const pthread_mutexattr_t *pmutexattr, int *waitq_type)
{
    if ((pmutexattr == LW_NULL) || (waitq_type == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_WAIT_PRIORITY) {
        *waitq_type = PTHREAD_WAITQ_PRIO;
    
    } else {
        *waitq_type = PTHREAD_WAITQ_FIFO;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_setcancelsafe
** 功能描述: 设置互斥量安全类型.
** 输　入  : pmutexattr     属性
**           cancel_safe    安全类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_setcancelsafe (pthread_mutexattr_t  *pmutexattr, int  cancel_safe)
{
    if (pmutexattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (cancel_safe == PTHREAD_CANCEL_SAFE) {
        pmutexattr->PMUTEXATTR_ulOption |= LW_OPTION_DELETE_SAFE;
    
    } else if (cancel_safe == PTHREAD_CANCEL_UNSAFE) {
        pmutexattr->PMUTEXATTR_ulOption &= ~LW_OPTION_DELETE_SAFE;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutexattr_getcancelsafe
** 功能描述: 获得互斥量安全类型.
** 输　入  : pmutexattr     属性
**           cancel_safe    安全类型
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutexattr_getcancelsafe (const pthread_mutexattr_t *pmutexattr, int *cancel_safe)
{
    if ((pmutexattr == LW_NULL) || (cancel_safe == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_iIsEnable != 1) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr->PMUTEXATTR_ulOption & LW_OPTION_DELETE_SAFE) {
        *cancel_safe = PTHREAD_CANCEL_SAFE;
    
    } else {
        *cancel_safe = PTHREAD_CANCEL_UNSAFE;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_init
** 功能描述: 创建一个互斥量.
** 输　入  : pmutex         句柄
**           pmutexattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_init (pthread_mutex_t  *pmutex, const pthread_mutexattr_t *pmutexattr)
{
    pthread_mutexattr_t     mutexattr;
    UINT8                   ucPriority;

    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pmutexattr) {
        mutexattr = *pmutexattr;
    } else {
        mutexattr = _G_pmutexattrDefault;                               /*  使用默认属性                */
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(mutexattr.PMUTEXATTR_iPrioceiling);
    
    if (mutexattr.PMUTEXATTR_iType == PTHREAD_MUTEX_NORMAL) {
        mutexattr.PMUTEXATTR_ulOption |= LW_OPTION_NORMAL;              /*  不检查死锁 (不推荐)         */
    
    } else if (mutexattr.PMUTEXATTR_iType == PTHREAD_MUTEX_ERRORCHECK) {
        mutexattr.PMUTEXATTR_ulOption |= LW_OPTION_ERRORCHECK;          /*  检查死锁                    */
    }
    
    pmutex->PMUTEX_ulMutex = API_SemaphoreMCreate("pmutex", 
                                ucPriority,
                                mutexattr.PMUTEXATTR_ulOption,
                                LW_NULL);                               /*  创建互斥量                  */
    if (pmutex->PMUTEX_ulMutex == 0) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    pmutex->PMUTEX_iType = mutexattr.PMUTEXATTR_iType;                  /*  未来扩展使用                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_destroy
** 功能描述: 删除一个互斥量.
** 输　入  : pmutex         句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_destroy (pthread_mutex_t  *pmutex)
{
    if ((pmutex == LW_NULL) || (pmutex->PMUTEX_ulMutex == LW_OBJECT_HANDLE_INVALID)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SemaphoreMDelete(&pmutex->PMUTEX_ulMutex)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_lock
** 功能描述: lock 一个互斥量.
** 输　入  : pmutex         句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_lock (pthread_mutex_t  *pmutex)
{
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    if (API_SemaphoreMPend(pmutex->PMUTEX_ulMutex,
                           LW_OPTION_WAIT_INFINITE)) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_trylock
** 功能描述: trylock 一个互斥量.
** 输　入  : pmutex         句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_trylock (pthread_mutex_t  *pmutex)
{
    ULONG    ulError;
    
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    ulError = API_SemaphoreMPend(pmutex->PMUTEX_ulMutex, LW_OPTION_NOT_WAIT);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = EAGAIN;
        return  (EAGAIN);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_timedlock
** 功能描述: lock 一个互斥量(带有超时时间).
** 输　入  : pmutex         句柄
**           abs_timeout    超时时间
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_timedlock (pthread_mutex_t  *pmutex, const struct timespec *abs_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((abs_timeout == LW_NULL) || 
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);              /*  转换超时时间                */

    ulError = API_SemaphoreMPend(pmutex->PMUTEX_ulMutex, ulTimeout);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (ETIMEDOUT);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_reltimedlock_np
** 功能描述: lock 一个互斥量(带有超时时间).
** 输　入  : pmutex         句柄
**           rel_timeout    相对超时时间
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_mutex_reltimedlock_np (pthread_mutex_t  *pmutex, const struct timespec *rel_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((rel_timeout == LW_NULL) || 
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);               /*  转换超时时间                */
    
    ulError = API_SemaphoreMPend(pmutex->PMUTEX_ulMutex, ulTimeout);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = ETIMEDOUT;
        return  (ETIMEDOUT);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** 函数名称: pthread_mutex_unlock
** 功能描述: unlock 一个互斥量.
** 输　入  : pmutex         句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_unlock (pthread_mutex_t  *pmutex)
{
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    if (API_SemaphoreMPost(pmutex->PMUTEX_ulMutex)) {
        if (errno == ERROR_EVENT_NOT_OWN) {
            errno = EPERM;
        } else {
            errno = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_setprioceiling
** 功能描述: 设置一个互斥量优先级天花板.
** 输　入  : pmutex         句柄
**           prioceiling    优先级
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_setprioceiling (pthread_mutex_t  *pmutex, int  prioceiling)
{
             UINT16             usIndex;
    REGISTER PLW_CLASS_EVENT    pevent;
    
    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((prioceiling < __PX_PRIORITY_MIN) ||
        (prioceiling > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    usIndex = _ObjectGetIndex(pmutex->PMUTEX_ulMutex);
    
    if (!_ObjectClassOK(pmutex->PMUTEX_ulMutex, _OBJECT_SEM_M)) {       /*  类型是否正确                */
        errno = EINVAL;
        return  (EINVAL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  下标是否正确                */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    pevent->EVENT_ucCeilingPriority = (UINT8)PX_PRIORITY_CONVERT(prioceiling);
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_getprioceiling
** 功能描述: 获得一个互斥量优先级天花板.
** 输　入  : pmutex         句柄
**           prioceiling    优先级(返回)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_getprioceiling (pthread_mutex_t  *pmutex, int  *prioceiling)
{
             UINT16             usIndex;
    REGISTER PLW_CLASS_EVENT    pevent;
    
    if ((pmutex == LW_NULL) || (prioceiling == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    usIndex = _ObjectGetIndex(pmutex->PMUTEX_ulMutex);
    
    if (!_ObjectClassOK(pmutex->PMUTEX_ulMutex, _OBJECT_SEM_M)) {       /*  类型是否正确                */
        errno = EINVAL;
        return  (EINVAL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  下标是否正确                */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    *prioceiling = PX_PRIORITY_CONVERT(pevent->EVENT_ucCeilingPriority);
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_getinfo
** 功能描述: 获得互斥量信息.
** 输　入  : pmutex        互斥量句柄
**           info          互斥量信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_mutex_getinfo (pthread_mutex_t  *pmutex, pthread_mutex_info_t  *info)
{
             UINT16             usIndex;
    REGISTER PLW_CLASS_EVENT    pevent;
    
    if ((pmutex == LW_NULL) || (info == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    usIndex = _ObjectGetIndex(pmutex->PMUTEX_ulMutex);
    
    if (!_ObjectClassOK(pmutex->PMUTEX_ulMutex, _OBJECT_SEM_M)) {       /*  类型是否正确                */
        errno = EINVAL;
        return  (EINVAL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  下标是否正确                */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pevent = &_K_eventBuffer[usIndex];
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (pevent->EVENT_ulCounter) {
        info->value = 1;
    } else {
        info->value = 0;
    }
    
    if (pevent->EVENT_ulOption & LW_OPTION_INHERIT_PRIORITY) {
        info->inherit = 1;
    } else {
        info->inherit = 0;
    }
    
    info->prioceiling = PX_PRIORITY_CONVERT(pevent->EVENT_ucCeilingPriority);
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
        info->wait_type = PTHREAD_WAITQ_PRIO;
    } else {
        info->wait_type = PTHREAD_WAITQ_FIFO;
    }
    
    if (pevent->EVENT_ulOption & LW_OPTION_DELETE_SAFE) {
        info->cancel_type = PTHREAD_CANCEL_SAFE;
    } else {
        info->cancel_type = PTHREAD_CANCEL_UNSAFE;
    }
    
    info->blocknum = _EventWaitNum(EVENT_SEM_Q, pevent);
    
    if (info->value == 0) {
        info->ownner = ((PLW_CLASS_TCB)(pevent->EVENT_pvTcbOwn))->TCB_ulId;
    } else {
        info->ownner = LW_OBJECT_HANDLE_INVALID;
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_mutex_show
** 功能描述: 显示互斥量信息.
** 输　入  : pmutex        互斥量句柄
**           level         等级
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_mutex_show (pthread_mutex_t  *pmutex, int  level)
{
    (VOID)level;

    if (pmutex == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_mutex_init_invisible(pmutex);
    
    API_SemaphoreShow(pmutex->PMUTEX_ulMutex);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
