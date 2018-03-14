/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ThreadCond.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 05 日
**
** 描        述: 这是系统类 pthread_cond_??? 条件变量支持.

** BUG:
2009.07.11  修正动态初始化的一处问题.
2009.12.30  API_ThreadCondWait() 在超时时, 仍需要获取互斥信号量.
2010.01.05  API_ThreadCondWait() 在释放互斥量和等待信号量时, 使用原子的操作.
2012.10.17  API_ThreadCondSignal() 在没有任务等待时, 直接退出.
2013.04.02  API_ThreadCondDestroy() 不再判断忙标志.
2013.04.08  API_ThreadCondDestroy() 遇到没有初始化的条件变量直接退出.
            API_ThreadCondInit() 不管有没有创建过, 都进行初始化.
2013.09.17  使用 POSIX 规定的取消点动作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  s_internal.h 中也有相关定义
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
#define __THREAD_CANCEL_POINT()         API_ThreadTestCancel()
#else
#define __THREAD_CANCEL_POINT()
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
/*********************************************************************************************************
** 函数名称: __threadCondInit
** 功能描述: 初始化条件变量(动态).
** 输　入  : ptcd               条件变量
**           pulAttr            属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_THREAD_EXT_EN > 0

static ULONG  __threadCondInit (PLW_THREAD_COND  ptcd, ULONG  ulAttr)
{
    ptcd->TCD_ulSignal = API_SemaphoreBCreate("cond_signal", LW_FALSE, 
                                              LW_OPTION_WAIT_PRIORITY, 
                                              LW_NULL);
    if (!ptcd->TCD_ulSignal) {
        return  (EAGAIN);
    }
    
    ptcd->TCD_ulMutex   = 0ul;
    ptcd->TCD_ulCounter = 0ul;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondAttrInit
** 功能描述: 初始化一个条件变量属性控制块.
** 输　入  : pulAttr            属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrInit (ULONG  *pulAttr)
{
    if (pulAttr) {
        *pulAttr = 0ul;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondAttrDestroy
** 功能描述: 销毁一个条件变量属性控制块.
** 输　入  : pulAttr            属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrDestroy (ULONG  *pulAttr)
{
    if (pulAttr) {
        *pulAttr = 0ul;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondAttrSetPshared
** 功能描述: 设置一个条件变量属性控制块的共享属性.
** 输　入  : pulAttr            属性
**           iShared            共享属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrSetPshared (ULONG  *pulAttr, INT  iShared)
{
    if (pulAttr) {
        if (iShared) {
            (*pulAttr) |= LW_THREAD_PROCESS_SHARED;
        } else {
            (*pulAttr) &= ~LW_THREAD_PROCESS_SHARED;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondAttrGetPshared
** 功能描述: 获得一个条件变量属性控制块的共享属性.
** 输　入  : pulAttr            属性
**           piShared           共享属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrGetPshared (const ULONG  *pulAttr, INT  *piShared)
{
    if (pulAttr && piShared) {
        *piShared = ((*pulAttr) & LW_THREAD_PROCESS_SHARED) ? 1 : 0;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondInit
** 功能描述: 初始化一个条件变量控制块.
** 输　入  : ptcd              条件变量控制块
**           ulAttr            控制属性 (当前忽略, 主要用作进程共享处理)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondInit (PLW_THREAD_COND  ptcd, ULONG  ulAttr)
{
    ULONG      ulError = ERROR_NONE;
    
    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulError = __threadCondInit(ptcd, ulAttr);                           /*  初始化                      */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondDestroy
** 功能描述: 销毁一个条件变量控制块.
** 输　入  : ptcd              条件变量控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondDestroy (PLW_THREAD_COND  ptcd)
{
    REGISTER ULONG      ulError = ERROR_NONE;

    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        return  (ERROR_NONE);                                           /*  直接退出                    */
    }
    
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    if (ptcd->TCD_ulSignal) {
        ulError = API_SemaphoreBDelete(&ptcd->TCD_ulSignal);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondSignal
** 功能描述: 向等待条件变量的线程发送信号.
** 输　入  : ptcd              条件变量控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondSignal (PLW_THREAD_COND  ptcd)
{
    REGISTER ULONG      ulError = ERROR_NONE;

    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        ulError = __threadCondInit(ptcd, (ULONG)(~0));                  /*  初始化                      */
    }
    
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    if (ptcd->TCD_ulSignal) {
        ULONG   ulThreadBlockNum = 0;
        
        ulError = API_SemaphoreBStatus(ptcd->TCD_ulSignal, LW_NULL, LW_NULL, &ulThreadBlockNum);
        if (ulError) {
            return  (ulError);
        }
            
        if (ulThreadBlockNum == 0) {                                    /*  没有任务等待, 则直接退出    */
            return  (ulError);
        }
    
        ulError = API_SemaphoreBPost(ptcd->TCD_ulSignal);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondBroadcast
** 功能描述: 向所有等待条件变量的线程发送信号.
** 输　入  : ptcd              条件变量控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondBroadcast (PLW_THREAD_COND  ptcd)
{
    REGISTER ULONG      ulError = ERROR_NONE;

    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        ulError = __threadCondInit(ptcd, (ULONG)(~0));                  /*  初始化                      */
    }
    
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    if (ptcd->TCD_ulSignal) {
        ulError = API_SemaphoreBFlush(ptcd->TCD_ulSignal, LW_NULL);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_ThreadCondWait
** 功能描述: 等待条件变量.
** 输　入  : ptcd              条件变量控制块
**           ulMutex           互斥信号量
**           ulTimeout         超时时间
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondWait (PLW_THREAD_COND  ptcd, LW_OBJECT_HANDLE  ulMutex, ULONG  ulTimeout)
{
             LW_OBJECT_HANDLE   ulOwerThread;
    REGISTER ULONG              ulError = ERROR_NONE;
    
    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        ulError = __threadCondInit(ptcd, (ULONG)(~0));                  /*  初始化                      */
    }
    
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    if (!ulMutex) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulError = API_SemaphoreMStatusEx(ulMutex, LW_NULL, LW_NULL, LW_NULL, &ulOwerThread);
    if (ulError) {
        return  (ulError);
    }
    
    if (((ptcd->TCD_ulMutex) && (ptcd->TCD_ulMutex != ulMutex)) ||
        (ulOwerThread != API_ThreadIdSelf())) {
        _ErrorHandle(EINVAL);
        return (EINVAL);
    
    } else {
        ptcd->TCD_ulMutex = ulMutex;
    }
    
    ptcd->TCD_ulCounter++;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    ulError = API_SemaphorePostBPend(ulMutex, 
                                     ptcd->TCD_ulSignal, 
                                     ulTimeout);                        /*  原子释放并等待信号量        */
    if (ulError) {                                                      /*  如果等待超时                */
        API_SemaphoreMPend(ulMutex, LW_OPTION_WAIT_INFINITE);           /*  重新获取互斥量              */
        if (ulError != ERROR_EVENT_WAS_DELETED) {
            ptcd->TCD_ulCounter--;
            if (ptcd->TCD_ulCounter == 0) {
                ptcd->TCD_ulMutex = 0ul;
            }
        }
        return  (ulError);
    }
    
    ulError = API_SemaphoreMPend(ulMutex, LW_OPTION_WAIT_INFINITE);
    if (ulError) {
        return  (ulError);
    }
    
    ptcd->TCD_ulCounter--;
    if (ptcd->TCD_ulCounter == 0) {
        ptcd->TCD_ulMutex = 0ul;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
