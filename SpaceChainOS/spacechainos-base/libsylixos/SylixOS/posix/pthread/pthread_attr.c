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
** 文   件   名: pthread_attr.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 属性兼容库.

** BUG:
2012.09.11  创建线程是不必强行指定是否使用 FPU, 则应由操作系统自行判断.
2013.05.01  Upon successful completion, pthread_attr_*() shall return a value of 0; 
            otherwise, an error number shall be returned to indicate the error.
2013.05.03  pthread_attr_init() 堆栈大小设置为 0, 表示继承线程创建者堆栈大小.
2013.05.04  加入一些常用的 UNIX 扩展接口.
2013.09.17  加入对堆栈警戒区的支持.
2016.04.12  加入 GJB7714 相关 API 支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_POSIX
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
#include "../include/posixLib.h"                                        /*  posix 内部公共库            */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#include "limits.h"
/*********************************************************************************************************
** 函数名称: pthread_attr_init
** 功能描述: 初始化线程属性块.
** 输　入  : pattr         需要初始化的 attr 指针.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_init (pthread_attr_t  *pattr)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pcName          = "pthread";
    pattr->PTHREADATTR_pvStackAddr     = LW_NULL;                       /*  自动分配堆栈                */
    pattr->PTHREADATTR_stStackGuard    = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    pattr->PTHREADATTR_stStackByteSize = 0;                             /*  0 表示继承创建者优先级      */
    pattr->PTHREADATTR_iSchedPolicy    = LW_OPTION_SCHED_RR;            /*  调度策略                    */
    pattr->PTHREADATTR_iInherit        = PTHREAD_EXPLICIT_SCHED;        /*  继承性                      */
    pattr->PTHREADATTR_ulOption        = LW_OPTION_THREAD_STK_CHK;      /*  SylixOS 线程创建选项        */
    pattr->PTHREADATTR_schedparam.sched_priority = PX_PRIORITY_CONVERT(LW_PRIO_NORMAL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_destroy
** 功能描述: 销毁一个线程属性块.
** 输　入  : pattr         需要销毁的 attr 指针.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_destroy (pthread_attr_t  *pattr)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_ulOption = 0ul;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setstack
** 功能描述: 设置堆栈的相关参数.
** 输　入  : pattr         需要设置的 attr 指针.
**           pvStackAddr   堆栈地址
**           stSize        堆栈大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstack (pthread_attr_t *pattr, void *pvStackAddr, size_t stSize)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (_StackSizeCheck(stSize) || (stSize < PTHREAD_STACK_MIN)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pvStackAddr     = pvStackAddr;
    pattr->PTHREADATTR_stStackByteSize = stSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getstack
** 功能描述: 获得堆栈的相关参数.
** 输　入  : pattr         需要获取的 attr 指针.
**           ppvStackAddr  堆栈地址
**           pstSize       堆栈大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getstack (const pthread_attr_t *pattr, void **ppvStackAddr, size_t *pstSize)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (ppvStackAddr) {
        *ppvStackAddr = pattr->PTHREADATTR_pvStackAddr;
    }
    
    if (pstSize) {
        *pstSize = pattr->PTHREADATTR_stStackByteSize;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setguardsize
** 功能描述: 设置一个线程属性块的堆栈警戒区大小.
** 输　入  : pattr         需要设置的 attr 指针.
**           stGuard       堆栈警戒区大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setguardsize (pthread_attr_t  *pattr, size_t  stGuard)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_stStackGuard = stGuard;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getguardsize
** 功能描述: 获取一个线程属性块的堆栈警戒区大小.
** 输　入  : pattr         需要获取的 attr 指针.
**           pstSize       获取堆栈警戒区大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getguardsize (pthread_attr_t  *pattr, size_t  *pstGuard)
{
    if ((pattr == LW_NULL) || (pstGuard == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pstGuard = pattr->PTHREADATTR_stStackGuard;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setstacksize
** 功能描述: 设置一个线程属性块的堆栈大小.
** 输　入  : pattr         需要设置的 attr 指针.
**           stSize        堆栈大小
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstacksize (pthread_attr_t  *pattr, size_t  stSize)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (stSize) {
        if (_StackSizeCheck(stSize) || (stSize < PTHREAD_STACK_MIN)) {
            errno = EINVAL;
            return  (EINVAL);
        }
        
        if (stSize < LW_CFG_PTHREAD_DEFAULT_STK_SIZE) {
            stSize = LW_CFG_PTHREAD_DEFAULT_STK_SIZE;
        }
    }
    
    pattr->PTHREADATTR_stStackByteSize = stSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getstacksize
** 功能描述: 获取一个线程属性块的堆栈大小.
** 输　入  : pattr         需要获取的 attr 指针.
**           pstSize       堆栈大小保存地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getstacksize (const pthread_attr_t  *pattr, size_t  *pstSize)
{
    if ((pattr == LW_NULL) || (pstSize == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pstSize = pattr->PTHREADATTR_stStackByteSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setstackaddr
** 功能描述: 指定一个线程属性块的堆栈地址.
** 输　入  : pattr         需要设置的 attr 指针.
**           pvStackAddr   堆栈地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstackaddr (pthread_attr_t  *pattr, void  *pvStackAddr)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pvStackAddr = pvStackAddr;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getstackaddr
** 功能描述: 获取一个线程属性块的堆栈地址.
** 输　入  : pattr         需要获取的 attr 指针.
**           ppvStackAddr  保存堆栈地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getstackaddr (const pthread_attr_t  *pattr, void  **ppvStackAddr)
{
    if ((pattr == LW_NULL) || (ppvStackAddr == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *ppvStackAddr = pattr->PTHREADATTR_pvStackAddr;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setdetachstate
** 功能描述: 设置一个线程属性块的 detach 状态.
** 输　入  : pattr         需要设置的 attr 指针.
**           iDetachState  detach 状态
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setdetachstate (pthread_attr_t  *pattr, int  iDetachState)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (iDetachState == PTHREAD_CREATE_DETACHED) {
        pattr->PTHREADATTR_ulOption |= LW_OPTION_THREAD_DETACHED;
    
    } else if (iDetachState == PTHREAD_CREATE_JOINABLE) {
        pattr->PTHREADATTR_ulOption &= ~LW_OPTION_THREAD_DETACHED;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getdetachstate
** 功能描述: 获取一个线程属性块的 detach 状态.
** 输　入  : pattr         需要获取的 attr 指针.
**           piDetachState 保存 detach 状态
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getdetachstate (const pthread_attr_t  *pattr, int  *piDetachState)
{
    if ((pattr == LW_NULL) || (piDetachState == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pattr->PTHREADATTR_ulOption & LW_OPTION_THREAD_DETACHED) {
        *piDetachState = PTHREAD_CREATE_DETACHED;
    
    } else {
        *piDetachState = PTHREAD_CREATE_JOINABLE;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setschedpolicy
** 功能描述: 设置一个线程属性块的调度策略.
** 输　入  : pattr         需要设置的 attr 指针.
**           iPolicy       调度策略
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setschedpolicy (pthread_attr_t  *pattr, int  iPolicy)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = ENOTSUP;
        return  (ENOTSUP);
    }
    
    pattr->PTHREADATTR_iSchedPolicy = iPolicy;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getschedpolicy
** 功能描述: 获取一个线程属性块的调度策略.
** 输　入  : pattr         需要获取的 attr 指针.
**           piPolicy      调度策略
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getschedpolicy (const pthread_attr_t  *pattr, int  *piPolicy)
{
    if ((pattr == LW_NULL) || (piPolicy == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *piPolicy = pattr->PTHREADATTR_iSchedPolicy;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setschedparam
** 功能描述: 设置一个线程属性块的调度器属性.
** 输　入  : pattr         需要设置的 attr 指针.
**           pschedparam   调度器参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setschedparam (pthread_attr_t            *pattr, 
                                 const struct sched_param  *pschedparam)
{
    if ((pattr == LW_NULL) || (pschedparam == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_schedparam = *pschedparam;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getschedparam
** 功能描述: 获得一个线程属性块的调度器属性.
** 输　入  : pattr         需要获得的 attr 指针.
**           pschedparam   调度器参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getschedparam (const pthread_attr_t      *pattr, 
                                 struct sched_param  *pschedparam)
{
    if ((pattr == LW_NULL) || (pschedparam == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *pschedparam = pattr->PTHREADATTR_schedparam;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setinheritsched
** 功能描述: 设置一个线程属性块是否继承调度策略的方式.
** 输　入  : pattr         需要设置的 attr 指针.
**           iInherit      创建线程的继承属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setinheritsched (pthread_attr_t  *pattr, int  iInherit)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((iInherit != PTHREAD_INHERIT_SCHED) &&
        (iInherit != PTHREAD_EXPLICIT_SCHED)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_iInherit = iInherit;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getinheritsched
** 功能描述: 获取一个线程属性块是否继承调度策略的方式.
** 输　入  : pattr         需要获取的 attr 指针.
**           piInherit     创建线程的继承属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getinheritsched (const pthread_attr_t  *pattr, int  *piInherit)
{
    if ((pattr == LW_NULL) || (piInherit == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *piInherit = pattr->PTHREADATTR_iInherit;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setscope
** 功能描述: 设置一个线程属性块创建的线程作用域.
** 输　入  : pattr         需要设置的 attr 指针.
**           iScope        作用域
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setscope (pthread_attr_t  *pattr, int  iScope)
{
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (iScope == PTHREAD_SCOPE_PROCESS) {
        pattr->PTHREADATTR_ulOption |= LW_OPTION_THREAD_SCOPE_PROCESS;
    
    } else if (iScope == PTHREAD_SCOPE_SYSTEM) {
        pattr->PTHREADATTR_ulOption &= ~LW_OPTION_THREAD_SCOPE_PROCESS;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getscope
** 功能描述: 获取一个线程属性块创建的线程作用域.
** 输　入  : pattr         需要获取的 attr 指针.
**           piScope       作用域
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getscope (const pthread_attr_t  *pattr, int  *piScope)
{
    if ((pattr == LW_NULL) || (piScope == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pattr->PTHREADATTR_ulOption & LW_OPTION_THREAD_SCOPE_PROCESS) {
        *piScope = PTHREAD_SCOPE_PROCESS;
    
    } else {
        *piScope = PTHREAD_SCOPE_SYSTEM;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setname
** 功能描述: 设置一个线程属性块的名字, 创建的线程将使用同样的名字.
** 输　入  : pattr         需要设置的 attr 指针.
**           pcName        名字
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setname (pthread_attr_t  *pattr, const char  *pcName)
{
    if ((pattr == LW_NULL) || (pcName == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    pattr->PTHREADATTR_pcName = (char *)pcName;                         /*  pcName 内存不会被修改       */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getname
** 功能描述: 获得一个线程属性块的名字, 创建的线程将使用同样的名字.
** 输　入  : pattr         需要设置的 attr 指针.
**           ppcName       名字
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_getname (const pthread_attr_t  *pattr, char  **ppcName)
{
    if ((pattr == LW_NULL) || (ppcName == LW_NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *ppcName = pattr->PTHREADATTR_pcName;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_get_np
** 功能描述: 获取线程属性控制块 (FreeBSD 扩展接口)
** 输　入  : thread        线程 ID
**           pattr         需要设置的 attr 指针.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_get_np (pthread_t  thread, pthread_attr_t *pattr)
{
    LW_CLASS_THREADATTR     lwattr;
    UINT8                   ucPolicy = LW_OPTION_SCHED_RR;
    
    if (pattr == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    lwattr = API_ThreadAttrGet(thread);
    
    if (lwattr.THREADATTR_stStackByteSize == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    API_ThreadGetSchedParam(thread, &ucPolicy, LW_NULL);
    
    pattr->PTHREADATTR_pcName          = LW_NULL;
    pattr->PTHREADATTR_pvStackAddr     = (PVOID)lwattr.THREADATTR_pstkLowAddr;
    pattr->PTHREADATTR_stStackGuard    = lwattr.THREADATTR_stGuardSize;
    pattr->PTHREADATTR_stStackByteSize = lwattr.THREADATTR_stStackByteSize;
    pattr->PTHREADATTR_iSchedPolicy    = (INT)ucPolicy;
    pattr->PTHREADATTR_iInherit        = PTHREAD_EXPLICIT_SCHED;        /*  目前不能确定                */
    pattr->PTHREADATTR_ulOption        = lwattr.THREADATTR_ulOption;
    pattr->PTHREADATTR_schedparam.sched_priority = PX_PRIORITY_CONVERT(lwattr.THREADATTR_ucPriority);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getattr_np
** 功能描述: 获取线程属性控制块 (Linux 扩展接口)
** 输　入  : thread        线程 ID
**           pattr         需要设置的 attr 指针.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getattr_np (pthread_t thread, pthread_attr_t *pattr)
{
    return  (pthread_attr_get_np(thread, pattr));
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getstackfilled
** 功能描述: 获得线程属性块栈填充特性
** 输　入  : pattr         线程属性
**           stackfilled   堆栈填充设置
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_attr_getstackfilled (const pthread_attr_t *pattr, int *stackfilled)
{
    if (!pattr || !stackfilled) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pattr->PTHREADATTR_ulOption & LW_OPTION_THREAD_STK_CLR) {
        *stackfilled = PTHREAD_STACK_FILLED;
    
    } else {
        *stackfilled = PTHREAD_NO_STACK_FILLED;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setstackfilled
** 功能描述: 设置线程属性块栈填充特性
** 输　入  : pattr         线程属性
**           stackfilled   堆栈填充设置
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_attr_setstackfilled (pthread_attr_t *pattr, int stackfilled)
{
    if (!pattr) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (stackfilled == PTHREAD_NO_STACK_FILLED) {
        pattr->PTHREADATTR_ulOption &= ~LW_OPTION_THREAD_STK_CHK;
    
    } else if (stackfilled == PTHREAD_STACK_FILLED) {
        pattr->PTHREADATTR_ulOption |= LW_OPTION_THREAD_STK_CHK;
    
    } else {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getbreakallowed
** 功能描述: 获得线程属性块是否允许断点
** 输　入  : pattr         线程属性
**           breakallowed  是否允许断点
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  pthread_attr_getbreakallowed (const pthread_attr_t *pattr, int *breakallowed)
{
    if (!pattr || !breakallowed) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *breakallowed = PTHREAD_BREAK_ALLOWED;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setbreakallowed
** 功能描述: 设置线程属性块是否允许断点
** 输　入  : pattr         线程属性
**           breakallowed  是否允许断点
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  pthread_attr_setbreakallowed (pthread_attr_t *pattr, int breakallowed)
{
    if (!pattr) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (breakallowed == PTHREAD_BREAK_DISALLOWED) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_getfpallowed
** 功能描述: 获得线程属性块是否允许浮点
** 输　入  : pattr         线程属性
**           fpallowed     是否允许浮点
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  pthread_attr_getfpallowed (const pthread_attr_t *pattr, int *fpallowed)
{
    if (!pattr || !fpallowed) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *fpallowed = PTHREAD_FP_ALLOWED;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_attr_setfpallowed
** 功能描述: 设置线程属性块是否允许浮点
** 输　入  : pattr         线程属性
**           fpallowed     是否允许浮点
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  pthread_attr_setfpallowed (pthread_attr_t *pattr, int fpallowed)
{
    if (!pattr) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (fpallowed == PTHREAD_FP_DISALLOWED) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
