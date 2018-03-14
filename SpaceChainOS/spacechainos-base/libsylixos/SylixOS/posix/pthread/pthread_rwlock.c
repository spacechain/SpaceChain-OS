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
** 文   件   名: pthread_rwlock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 读写锁库.

** BUG:
2010.01.12  加锁时阻塞时, 只需将 pend 计数器一次自增一次即可.
2010.01.13  使用计数信号量充当同步锁, 每次状态的改变激活所有的线程, 他们将自己进行抢占.
2012.12.13  由于 SylixOS 支持进程资源回收, 这里开始支持静态初始化.
2013.05.01  If successful, the pthread_rwlockattr_*() and pthread_rwlock_*() functions shall return zero;
            otherwise, an error number shall be returned to indicate the error.
2016.04.13  加入 GJB7714 相关 API 支持.
2016.05.09  隐形初始化确保多线程安全.
2016.07.21  使用内核读写锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  宏
*********************************************************************************************************/
#define __PX_RWLOCKATTR_SHARED  1
#define __PX_RWLOCKATTR_WRITER  2
/*********************************************************************************************************
  初始化锁
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_ulPRWLockInitLock;
/*********************************************************************************************************
** 函数名称: _posixPRWLockInit
** 功能描述: 初始化 读写锁 环境.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _posixPRWLockInit (VOID)
{
    _G_ulPRWLockInitLock = API_SemaphoreMCreate("prwinit", LW_PRIO_DEF_CEILING, 
                                                LW_OPTION_INHERIT_PRIORITY | 
                                                LW_OPTION_WAIT_PRIORITY | 
                                                LW_OPTION_DELETE_SAFE, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __pthread_rwlock_init_invisible
** 功能描述: 读写锁隐形创建. (静态初始化)
** 输　入  : prwlock        句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static void  __pthread_rwlock_init_invisible (pthread_rwlock_t  *prwlock)
{
    if (prwlock) {
        if (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreMPend(_G_ulPRWLockInitLock, LW_OPTION_WAIT_INFINITE);
            if (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID) {
                pthread_rwlock_init(prwlock, LW_NULL);
            }
            API_SemaphoreMPost(_G_ulPRWLockInitLock);
        }
    }
}
/*********************************************************************************************************
** 函数名称: pthread_rwlockattr_init
** 功能描述: 初始化一个读写锁属性块.
** 输　入  : prwlockattr    属性块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_init (pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    *prwlockattr = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlockattr_destroy
** 功能描述: 销毁一个读写锁属性块.
** 输　入  : prwlockattr    属性块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_destroy (pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *prwlockattr = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlockattr_setpshared
** 功能描述: 设置一个读写锁属性块是否进程共享.
** 输　入  : prwlockattr    属性块
**           pshared        共享
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_setpshared (pthread_rwlockattr_t *prwlockattr, int  pshared)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pshared) {
        *prwlockattr |= __PX_RWLOCKATTR_SHARED;
    
    } else {
        *prwlockattr |= ~__PX_RWLOCKATTR_SHARED;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlockattr_getpshared
** 功能描述: 获取一个读写锁属性块是否进程共享.
** 输　入  : prwlockattr    属性块
**           pshared        共享(返回)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_getpshared (const pthread_rwlockattr_t *prwlockattr, int  *pshared)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (pshared) {
        if (*prwlockattr & __PX_RWLOCKATTR_SHARED) {
            *pshared = PTHREAD_PROCESS_SHARED;
        
        } else {
            *pshared = PTHREAD_PROCESS_PRIVATE;
        }
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlockattr_setkind_np
** 功能描述: 设置一个读写锁属性块读写优先权.
** 输　入  : prwlockattr    属性块
**           pref           优先权选项
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_setkind_np (pthread_rwlockattr_t *prwlockattr, int pref)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pref == PTHREAD_RWLOCK_PREFER_READER_NP) {
        *prwlockattr &= ~__PX_RWLOCKATTR_WRITER;
    
    } else if (pref == PTHREAD_RWLOCK_PREFER_WRITER_NP) {
        *prwlockattr |= __PX_RWLOCKATTR_WRITER;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlockattr_setkind_np
** 功能描述: 设置一个读写锁属性块读写优先权.
** 输　入  : prwlockattr    属性块
**           pref           优先权选项
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlockattr_getkind_np (const pthread_rwlockattr_t *prwlockattr, int *pref)
{
    if (prwlockattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pref) {
        if (*prwlockattr & __PX_RWLOCKATTR_WRITER) {
            *pref = PTHREAD_RWLOCK_PREFER_WRITER_NP;
        
        } else {
            *pref = PTHREAD_RWLOCK_PREFER_READER_NP;
        }
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlock_init
** 功能描述: 创建一个读写锁.
** 输　入  : prwlock        句柄
**           prwlockattr    属性块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_init (pthread_rwlock_t  *prwlock, const pthread_rwlockattr_t  *prwlockattr)
{
    ULONG   ulOption = LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE;
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (prwlockattr) {
        if (*prwlockattr & __PX_RWLOCKATTR_WRITER) {
            ulOption |= LW_OPTION_RW_PREFER_WRITER;
        }
    }
    
    prwlock->PRWLOCK_ulRwLock = API_SemaphoreRWCreate("prwrlock", ulOption, LW_NULL);
    if (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID) {
        errno = ENOSPC;
        return  (ENOSPC);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlock_destroy
** 功能描述: 销毁一个读写锁.
** 输　入  : prwlock        句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_destroy (pthread_rwlock_t  *prwlock)
{
    if ((prwlock == LW_NULL) || (prwlock->PRWLOCK_ulRwLock == LW_OBJECT_HANDLE_INVALID)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SemaphoreRWDelete(&prwlock->PRWLOCK_ulRwLock)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlock_rdlock
** 功能描述: 等待一个读写锁可读.
** 输　入  : prwlock        句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_rdlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    if (API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, 
                             LW_OPTION_WAIT_INFINITE)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlock_tryrdlock
** 功能描述: 非阻塞等待一个读写锁可读.
** 输　入  : prwlock        句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_tryrdlock (pthread_rwlock_t  *prwlock)
{
    ULONG    ulError;

    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulError = API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, LW_OPTION_NOT_WAIT);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = EBUSY;
        return  (EBUSY);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlock_timedrdlock
** 功能描述: 等待一个读写锁可读 (带有超时的阻塞).
** 输　入  : prwlock        句柄
**           abs_timeout    绝对超时时间
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_timedrdlock (pthread_rwlock_t *prwlock,
                                 const struct timespec *abs_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((abs_timeout == LW_NULL) || 
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);              /*  转换超时时间                */

    ulError = API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** 函数名称: pthread_rwlock_reltimedrdlock_np
** 功能描述: 等待一个读写锁可读 (带有超时的阻塞).
** 输　入  : prwlock        句柄
**           rel_timeout    相对超时时间
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_rwlock_reltimedrdlock_np (pthread_rwlock_t *prwlock,
                                       const struct timespec *rel_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;

    if ((rel_timeout == LW_NULL) || 
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);               /*  转换超时时间                */
    
    ulError = API_SemaphoreRWPendR(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** 函数名称: pthread_rwlock_wrlock
** 功能描述: 等待一个读写锁可写.
** 输　入  : prwlock        句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_wrlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    if (API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, 
                             LW_OPTION_WAIT_INFINITE)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlock_trywrlock
** 功能描述: 非阻塞等待一个读写锁可写.
** 输　入  : prwlock        句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_trywrlock (pthread_rwlock_t  *prwlock)
{
    ULONG   ulError;

    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);

    ulError = API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, LW_OPTION_NOT_WAIT);
    if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
        errno = EBUSY;
        return  (EBUSY);
    
    } else if (ulError) {
        if (errno != EDEADLK) {
            errno  = EINVAL;
        }
        return  (errno);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_rwlock_wrlock
** 功能描述: 等待一个读写锁可写 (带有超时的阻塞).
** 输　入  : prwlock        句柄
**           abs_timeout    绝对超时时间
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_timedwrlock (pthread_rwlock_t *prwlock,
                                 const struct timespec *abs_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((abs_timeout == LW_NULL) || 
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);              /*  转换超时时间                */

    ulError = API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** 函数名称: pthread_rwlock_reltimedwrlock_np
** 功能描述: 等待一个读写锁可写 (带有超时的阻塞).
** 输　入  : prwlock        句柄
**           rel_timeout    相对超时时间
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 当相距超过 ulong tick 范围时, 将自然产生溢出, 导致超时时间不准确.

                                           API 函数
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_rwlock_reltimedwrlock_np (pthread_rwlock_t *prwlock,
                                       const struct timespec *rel_timeout)
{
    ULONG   ulTimeout;
    ULONG   ulError;
    
    if ((rel_timeout == LW_NULL) || 
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    ulTimeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);               /*  转换超时时间                */
    
    ulError = API_SemaphoreRWPendW(prwlock->PRWLOCK_ulRwLock, ulTimeout);
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
** 函数名称: pthread_rwlock_unlock
** 功能描述: 释放一个读写锁.
** 输　入  : prwlock        句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_rwlock_unlock (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    if (API_SemaphoreRWPost(prwlock->PRWLOCK_ulRwLock)) {
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
** 函数名称: pthread_rwlock_getinfo
** 功能描述: 获得读写锁信息
** 输　入  : prwlock       读写锁控制块
**           info          读写锁信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_rwlock_getinfo (pthread_rwlock_t  *prwlock, pthread_rwlock_info_t  *info)
{
    ULONG   ulRWCount;
    ULONG   ulRPend;
    ULONG   ulWPend;

    if ((prwlock == LW_NULL) || (info == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthread_rwlock_init_invisible(prwlock);
    
    if (API_SemaphoreRWStatus(prwlock->PRWLOCK_ulRwLock,
                              &ulRWCount,
                              &ulRPend,
                              &ulWPend,
                              LW_NULL,
                              &info->owner)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    info->opcnt = (unsigned int)ulRWCount;
    info->rpend = (unsigned int)ulRPend;
    info->wpend = (unsigned int)ulWPend;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
