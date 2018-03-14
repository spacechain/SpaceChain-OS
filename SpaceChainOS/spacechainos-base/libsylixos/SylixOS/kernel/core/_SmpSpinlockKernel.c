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
** 文   件   名: _SmpSpinlockKernel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 11 月 13 日
**
** 描        述: 多 CPU 系统内核自旋锁.
** 
** 注        意: 内核自旋锁加锁是在不断试探过程中进行的, 期间需要打开中断.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  自旋锁驱动返回值
*********************************************************************************************************/
#define LW_SPIN_OK      1
#define LW_SPIN_ERROR   0
/*********************************************************************************************************
  内核自旋锁
*********************************************************************************************************/
#define LW_KERN_SL      (_K_klKernel.KERN_slLock)
/*********************************************************************************************************
** 函数名称: _SmpSpinLockIgnIrq
** 功能描述: 内核自旋锁加锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpKernelLockIgnIrq (VOID)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    pcpuCur = LW_CPU_GET_CUR();
    
    for (;;) {
        iRet = __ARCH_SPIN_TRYLOCK(&LW_KERN_SL);
        if (iRet != LW_SPIN_OK) {
            _SmpTryProcIpi(pcpuCur);                                    /*  尝试执行 IPI                */
            LW_SPINLOCK_DELAY();
            
        } else {
            if (!pcpuCur->CPU_ulInterNesting) {
                __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);             /*  锁定任务在当前 CPU          */
            }
            KN_SMP_MB();
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: _SmpKernelUnlockIgnIrq
** 功能描述: 内核自旋锁解锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpKernelUnlockIgnIrq (VOID)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    KN_SMP_MB();
    iRet = __ARCH_SPIN_UNLOCK(&LW_KERN_SL);
    _BugFormat((iRet != LW_SPIN_OK), LW_TRUE, "unlock error %p!\r\n", &LW_KERN_SL);
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解除任务锁定                */
    }
}
/*********************************************************************************************************
** 函数名称: _SmpKernelLockQuick
** 功能描述: 内核自旋锁加锁操作, 连同锁定中断
** 输　入  : piregInterLevel   中断锁定信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpKernelLockQuick (INTREG  *piregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    for (;;) {
        *piregInterLevel = KN_INT_DISABLE();
        
        iRet = __ARCH_SPIN_TRYLOCK(&LW_KERN_SL);
        if (iRet != LW_SPIN_OK) {
            _SmpTryProcIpi(LW_CPU_GET_CUR());                           /*  尝试执行 IPI                */
            KN_INT_ENABLE(*piregInterLevel);
            LW_SPINLOCK_DELAY();
            
        } else {
            pcpuCur = LW_CPU_GET_CUR();
            if (!pcpuCur->CPU_ulInterNesting) {
                __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);             /*  锁定任务在当前 CPU          */
            }
            KN_SMP_MB();
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: _SmpKernelUnlockQuick
** 功能描述: 内核自旋锁解锁操作, 连同解锁中断, 不进行尝试调度
** 输　入  : iregInterLevel    中断锁定信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpKernelUnlockQuick (INTREG  iregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    KN_SMP_MB();
    iRet = __ARCH_SPIN_UNLOCK(&LW_KERN_SL);
    _BugFormat((iRet != LW_SPIN_OK), LW_TRUE, "unlock error %p!\r\n", &LW_KERN_SL);
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解除任务锁定                */
    }
    
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _SmpKernelUnlockSched
** 功能描述: 内核 SMP 调度器切换完成后专用释放函数 (关中断状态下被调用)
** 输　入  : ptcbOwner     锁的持有者
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpKernelUnlockSched (PLW_CLASS_TCB  ptcbOwner)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    KN_SMP_MB();
    iRet = __ARCH_SPIN_UNLOCK(&LW_KERN_SL);
    _BugFormat((iRet != LW_SPIN_OK), LW_TRUE, "unlock error %p!\r\n", &LW_KERN_SL);
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(ptcbOwner);                                   /*  解除任务锁定                */
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
