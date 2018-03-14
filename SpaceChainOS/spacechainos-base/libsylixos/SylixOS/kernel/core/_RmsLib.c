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
** 文   件   名: _RmsInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 22 日
**
** 描        述: RMS 函数库。(这些函数均在锁定内核时被调用)

** BUG
2007.11.04  将 0xFFFFFFFF 改为 __ARCH_ULONG_MAX.
2007.11.04  加入独立的任务执行时间测算函数.
2007.11.11  当 _RmsInitExpire() 发现超时时,应该记录当前的系统时间,这样就可以避免一次超时,以后都错误.
2008.03.29  相关的地方加入关闭中断的函数.
2010.08.03  对时间的操作加入相关的自旋锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _RmsActive
** 功能描述: 第一次激活 RMS 并开始对线程执行时间进行测量 (进入内核后被调用)
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

VOID  _RmsActive (PLW_CLASS_RMS  prms)
{
    prms->RMS_ucStatus = LW_RMS_ACTIVE;
    __KERNEL_TIME_GET_NO_SPINLOCK(prms->RMS_ulTickSave, ULONG);
}
/*********************************************************************************************************
** 函数名称: _RmsGetExecTime
** 功能描述: 计算任务执行的时间 (进入内核后被调用)
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG   _RmsGetExecTime (PLW_CLASS_RMS  prms)
{
    REGISTER ULONG            ulThreadExecTime;
             ULONG            ulKernelTime;
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);
    ulThreadExecTime = (ulKernelTime >= prms->RMS_ulTickSave) ? 
                       (ulKernelTime -  prms->RMS_ulTickSave) :
                       (ulKernelTime + (__ARCH_ULONG_MAX - prms->RMS_ulTickSave) + 1);
                          
    return  (ulThreadExecTime);
}
/*********************************************************************************************************
** 函数名称: _RmsInitExpire
** 功能描述: 开始进行时间等待 (进入内核后被调用)
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _RmsInitExpire (PLW_CLASS_RMS  prms, ULONG  ulPeriod, ULONG  *pulWaitTick)
{
             PLW_CLASS_TCB    ptcbCur;
    REGISTER ULONG            ulThreadExecTime;                         /*  计算线程执行时间            */
             ULONG            ulKernelTime;
    
    LW_TCB_GET_CUR(ptcbCur);
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);
    ulThreadExecTime = (ulKernelTime >= prms->RMS_ulTickSave) ? 
                       (ulKernelTime -  prms->RMS_ulTickSave) :
                       (ulKernelTime + (__ARCH_ULONG_MAX - prms->RMS_ulTickSave) + 1);
                          
    if (ulThreadExecTime > ulPeriod) {
        __KERNEL_TIME_GET_NO_SPINLOCK(prms->RMS_ulTickSave, ULONG);     /*  重新记录系统时钟            */
        return  (ERROR_RMS_TICK);
    }
    
    if (ulThreadExecTime == ulPeriod) {
        *pulWaitTick = 0;
        __KERNEL_TIME_GET_NO_SPINLOCK(prms->RMS_ulTickSave, ULONG);     /*  重新记录系统时钟            */
        return  (ERROR_NONE);
    }
    
    *pulWaitTick = ulPeriod - ulThreadExecTime;                         /*  计算睡眠时间                */
    
    prms->RMS_ucStatus   = LW_RMS_EXPIRED;                              /*  改变状态                    */
    prms->RMS_ptcbOwner  = ptcbCur;                                     /*  记录当前TCB                 */
    
    __KERNEL_TIME_GET_NO_SPINLOCK(prms->RMS_ulTickNext, ULONG);
    prms->RMS_ulTickNext += *pulWaitTick;                               /*  计算下次到时时间            */
                                                                        /*  自然溢出                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _RmsEndExpire
** 功能描述: 一个周期完毕，等待下一个周期 (进入内核后被调用)
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _RmsEndExpire (PLW_CLASS_RMS  prms)
{
    if (prms->RMS_ucStatus != LW_RMS_EXPIRED) {                         /*  被 cancel 或 删除了         */
        return  (ERROR_RMS_WAS_CHANGED);                                /*  被改变了                    */
    }
    
    prms->RMS_ucStatus = LW_RMS_ACTIVE;                                 /*  改变状态                    */
    __KERNEL_TIME_GET_NO_SPINLOCK(prms->RMS_ulTickSave, ULONG);         /*  重新记录系统时钟            */
    
    if (prms->RMS_ulTickNext != prms->RMS_ulTickSave) {                 /*  是否 TIME OUT               */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    
    } else {
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
