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
** 文   件   名: _UpSpinlock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 07 月 21 日
**
** 描        述: 单 CPU 系统自旋锁.
**
** 注        意: 单 CPU 情况下, 不需要处理 CPU_ulSpinNesting;
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN == 0
/*********************************************************************************************************
** 函数名称: _UpSpinInit
** 功能描述: 自旋锁初始化
** 输　入  : psl           自旋锁
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinInit (spinlock_t *psl)
{
    (VOID)psl;
}
/*********************************************************************************************************
** 函数名称: _UpSpinLock
** 功能描述: 自旋锁加锁操作
** 输　入  : psl           自旋锁
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinLock (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: _UpSpinTryLock
** 功能描述: 自旋锁尝试加锁操作
** 输　入  : psl           自旋锁
** 输　出  : LW_TRUE 加锁正常 LW_FALSE 加锁失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  _UpSpinTryLock (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: _UpSpinUnlock
** 功能描述: 自旋锁解锁操作
** 输　入  : psl           自旋锁
** 输　出  : 调度器返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _UpSpinUnlock (spinlock_t *psl)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur   = LW_CPU_GET_CUR();
    BOOL            bTrySched = LW_FALSE;
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解除任务锁定                */
        if (__COULD_SCHED(pcpuCur, 0)) {
            bTrySched = LW_TRUE;                                        /*  需要尝试调度                */
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    if (bTrySched) {
        return  (_ThreadSched(pcpuCur->CPU_ptcbTCBCur));
    
    } else {
        return  (ERROR_NONE);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _UpSpinLockIgnIrq
** 功能描述: 自旋锁加锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : psl           自旋锁
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinLockIgnIrq (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: _UpSpinTryLockIgnIrq
** 功能描述: 自旋锁尝试加锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : psl           自旋锁
** 输　出  : LW_TRUE 加锁正常 LW_FALSE 加锁失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  _UpSpinTryLockIgnIrq (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: _UpSpinUnlockIgnIrq
** 功能描述: 自旋锁解锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : psl           自旋锁
** 输　出  : NONE (不进行调度尝试)
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinUnlockIgnIrq (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解锁任务在当前 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: _UpSpinLockIrq
** 功能描述: 自旋锁加锁操作, 连同锁定中断
** 输　入  : psl               自旋锁
**           piregInterLevel   中断锁定信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinLockIrq (spinlock_t *psl, INTREG  *piregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
    
    *piregInterLevel = KN_INT_DISABLE();
}
/*********************************************************************************************************
** 函数名称: _UpSpinTryLockIrq
** 功能描述: 自旋锁尝试加锁操作, 连同锁定中断
** 输　入  : psl               自旋锁
**           piregInterLevel   中断锁定信息
** 输　出  : LW_TRUE 加锁正常 LW_FALSE 加锁失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  _UpSpinTryLockIrq (spinlock_t *psl, INTREG  *piregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
    
    *piregInterLevel = KN_INT_DISABLE();
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: _UpSpinUnlockIrq
** 功能描述: 自旋锁解锁操作, 连同解锁中断
** 输　入  : psl               自旋锁
**           iregInterLevel    中断锁定信息
** 输　出  : 调度器返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _UpSpinUnlockIrq (spinlock_t *psl, INTREG  iregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur   = LW_CPU_GET_CUR();
    BOOL            bTrySched = LW_FALSE;
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解除任务锁定                */
        if (__COULD_SCHED(pcpuCur, 0)) {
            bTrySched = LW_TRUE;                                        /*  需要尝试调度                */
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    if (bTrySched) {
        return  (_ThreadSched(pcpuCur->CPU_ptcbTCBCur));
    
    } else {
        return  (ERROR_NONE);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _UpSpinUnlockIrqQuick
** 功能描述: 自旋锁解锁操作, 连同解锁中断, 不进行尝试调度
** 输　入  : psl               自旋锁
**           iregInterLevel    中断锁定信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinUnlockIrqQuick (spinlock_t *psl, INTREG  iregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解锁任务在当前 CPU          */
    }
    
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _UpSpinLockRaw
** 功能描述: 自旋锁原始加锁操作.
** 输　入  : psl               自旋锁
**           piregInterLevel   中断状态
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinLockRaw (spinlock_t *psl, INTREG  *piregInterLevel)
{
    *piregInterLevel = KN_INT_DISABLE();
}
/*********************************************************************************************************
** 函数名称: _UpSpinTryLockRaw
** 功能描述: 自旋锁尝试原始加锁操作.
** 输　入  : psl               自旋锁
**           piregInterLevel   中断状态
** 输　出  : LW_TRUE 加锁正常 LW_FALSE 加锁失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  _UpSpinTryLockRaw (spinlock_t *psl, INTREG  *piregInterLevel)
{
    *piregInterLevel = KN_INT_DISABLE();
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: _UpSpinUnlockRaw
** 功能描述: 自旋锁原始解锁操作.
** 输　入  : psl               自旋锁
**           iregInterLevel    中断状态
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpSpinUnlockRaw (spinlock_t *psl, INTREG  iregInterLevel)
{
    KN_INT_ENABLE(iregInterLevel);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
