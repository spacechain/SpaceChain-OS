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
** 文   件   名: pthread_barrier.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 10 月 04 日
**
** 描        述: pthread 线程屏障兼容库. (barrier 从来不会发生 EINTR)

** BUG:
2011.03.06  修正 gcc 4.5.1 相关 warning.
2013.05.01  Upon successful completion, these functions shall return zero; 
            otherwise, an error number shall be returned to indicate the error.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
** 函数名称: pthread_barrierattr_init
** 功能描述: 初始化线程屏障属性块.
** 输　入  : pbarrierattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_init (pthread_barrierattr_t  *pbarrierattr)
{
    if (pbarrierattr) {
        *pbarrierattr = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_barrierattr_destroy
** 功能描述: 销毁一个线程屏障属性块.
** 输　入  : pbarrierattr     属性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_destroy (pthread_barrierattr_t  *pbarrierattr)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_barrierattr_setpshared
** 功能描述: 设置线程屏障属性块共享状态.
** 输　入  : pbarrierattr     属性
**           shared           PTHREAD_PROCESS_SHARED    PTHREAD_PROCESS_PRIVATE
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_setpshared (pthread_barrierattr_t  *pbarrierattr, int  shared)
{
    if (pbarrierattr) {
        *pbarrierattr = shared;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_barrierattr_getpshared
** 功能描述: 获取线程屏障属性块共享状态.
** 输　入  : pbarrierattr     属性
**           pshared          保存 shared 状态
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_getpshared (const pthread_barrierattr_t  *pbarrierattr, int  *pshared)
{
    if (pbarrierattr && pshared) {
        *pshared = *pbarrierattr;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_barrier_init
** 功能描述: 创建(初始化)线程屏障.
** 输　入  : pbarrier         创建后保存屏障句柄
**           pbarrierattr     属性
**           count            线程屏障的计数数量
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_barrier_init (pthread_barrier_t            *pbarrier, 
                           const pthread_barrierattr_t  *pbarrierattr,
                           unsigned int                  count)
{
    if (pbarrier == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    if (count == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    /*
     *  注意, 这里只能按 LW_OPTION_WAIT_FIFO 等待, 这样才能保证激活的线程是先早等待的那几个!
     */
    pbarrier->PBARRIER_ulSync = API_SemaphoreBCreate("barrier_sync", 
                                                     LW_FALSE, 
                                                     LW_OPTION_WAIT_FIFO,
                                                     LW_NULL);          /*  创建同步信号量              */
    if (pbarrier->PBARRIER_ulSync == LW_OBJECT_HANDLE_INVALID) {
        errno = ENOSPC;
        return  (ENOSPC);
    }
    
    pbarrier->PBARRIER_iCounter   = count;
    pbarrier->PBARRIER_iInitValue = count;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_barrier_destroy
** 功能描述: 删除(销毁)线程屏障.
** 输　入  : pbarrier         线程屏障句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_barrier_destroy (pthread_barrier_t  *pbarrier)
{
    if (pbarrier == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pbarrier->PBARRIER_ulSync) {
        API_SemaphoreBDelete(&pbarrier->PBARRIER_ulSync);               /*  删除同步信号量              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_barrier_wait
** 功能描述: 等待线程屏障.
** 输　入  : pbarrier         线程屏障句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_barrier_wait (pthread_barrier_t  *pbarrier)
{
             INTREG  iregInterLevel;
             BOOL    bRelease = LW_FALSE;
    REGISTER ULONG   ulReleaseNum = 0;

    if (pbarrier == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __LW_ATOMIC_LOCK(iregInterLevel);                                   /*  进入 atomic 临界区          */
    pbarrier->PBARRIER_iCounter -= 1;
    if (pbarrier->PBARRIER_iCounter == 0) {                             /*  需要激活等待线程            */
        pbarrier->PBARRIER_iCounter =  pbarrier->PBARRIER_iInitValue;
        ulReleaseNum = (ULONG)(pbarrier->PBARRIER_iInitValue - 1);      /*  需要激活线程的数量          */
        bRelease     = LW_TRUE;
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);                                 /*  退出 atomic 临界区          */
    
    if (bRelease) {
        if (ulReleaseNum) {
            API_SemaphoreBRelease(pbarrier->PBARRIER_ulSync, 
                                  ulReleaseNum,
                                  LW_NULL);                             /*  激活先前等待的线程          */
        }
        return  (PTHREAD_BARRIER_SERIAL_THREAD);
    
    } else {
        __THREAD_CANCEL_POINT();                                        /*  测试取消点                  */
    
        API_SemaphoreBPend(pbarrier->PBARRIER_ulSync, 
                           LW_OPTION_WAIT_INFINITE);                    /*  等待最后一个 wait 释放      */
        
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
