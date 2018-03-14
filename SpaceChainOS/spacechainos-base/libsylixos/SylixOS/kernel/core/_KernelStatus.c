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
** 文   件   名: _KernelStatus.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 05 月 18 日
**
** 描        述: 这是系统内核状态控制

** BUG
2008.05.31  不使用 _K_ucSysStatus 进行状态保护. 在启动过程中使用其他的保护方法可以提高效率.
2009.04.14  加入 SMP 支持, 进入内核时将锁定内核 spinlock
2010.01.22  加入与 IRQ 同步操作.
2011.02.22  加入 __kernelExitInt() 从信号虚拟中断中退出内核, 没有将当前现场压栈.
2011.02.24  SIGNAL 去掉了 suspend 自动激活能力, 所以不再需要 kernel exit int 支持.
2011.12.08  加入查看内核拥有线程(或中断)的操作. (方便调试)
2012.12.25  加入内核空间操作的函数.
2013.07.17  在需要的地方加入内存屏障. 同时将内核计数器移至 CPU 结构中.
2013.08.28  加入内核事件监控器.
2014.11.06  加入新的 SMP 内核调度机制.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  kernel status macro
*********************************************************************************************************/
#ifdef __LW_KERNLOCK_BUG_TRACE_EN
#define __LW_SAVE_KERNEL_OWNER(pcpuCur, pcFunc) \
        do {    \
            _K_klKernel.KERN_pvCpuOwner        = (PVOID)pcpuCur;    \
            _K_klKernel.KERN_pcKernelEnterFunc = pcFunc;  \
            if (pcpuCur->CPU_ulInterNesting) {  \
                _K_klKernel.KERN_ulKernelOwner = ~0;  \
            } else {    \
                _K_klKernel.KERN_ulKernelOwner = pcpuCur->CPU_ptcbTCBCur->TCB_ulId;   \
            }   \
        } while (0)
        
#define __LW_CLEAR_KERNEL_OWNER()   \
        do {    \
            _K_klKernel.KERN_pvCpuOwner        = LW_NULL; \
            _K_klKernel.KERN_pcKernelEnterFunc = LW_NULL; \
            _K_klKernel.KERN_ulKernelOwner     = LW_OBJECT_HANDLE_INVALID;    \
        } while (0)
#else
#define __LW_SAVE_KERNEL_OWNER(pcpuCur, pcFunc) (_K_klKernel.KERN_pvCpuOwner = (PVOID)pcpuCur)
#define __LW_CLEAR_KERNEL_OWNER()               (_K_klKernel.KERN_pvCpuOwner = LW_NULL)
#endif                                                                  /*  __LW_KERNLOCK_BUG_TRACE_EN  */
/*********************************************************************************************************
** 函数名称: __kernelEnter
** 功能描述: 进入内核状态
** 输　入  : pcFunc        进入内核函数 (调试用)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __kernelEnter (CPCHAR  pcFunc)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);                           /*  锁内核 spinlock 并关闭中断  */
    
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_iKernelCounter++;
    KN_SMP_WMB();                                                       /*  等待以上操作完成            */
    
    __LW_SAVE_KERNEL_OWNER(pcpuCur, pcFunc);

    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
}
/*********************************************************************************************************
** 函数名称: __kernelEnterIrq
** 功能描述: 进入内核状态并关闭中断
** 输　入  : pcFunc        进入内核函数 (调试用)
** 输　出  : 中断寄存器
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INTREG  __kernelEnterIrq (CPCHAR  pcFunc)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);                           /*  锁内核 spinlock 并关闭中断  */
    
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_iKernelCounter++;
    KN_SMP_WMB();                                                       /*  等待以上操作完成            */
    
    __LW_SAVE_KERNEL_OWNER(pcpuCur, pcFunc);
    
    return  (iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: __kernelExit
** 功能描述: 退出内核状态
** 输　入  : NONE
** 输　出  : 调度器返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __kernelExit (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    INT             iRetVal;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_iKernelCounter) {
        pcpuCur->CPU_iKernelCounter--;
        KN_SMP_WMB();                                                   /*  等待以上操作完成            */
        
        if (!pcpuCur->CPU_iKernelCounter) {
            __LW_CLEAR_KERNEL_OWNER();
            
            iRetVal = _Schedule();                                      /*  尝试调度                    */
            LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);                  /*  解锁内核 spinlock 并打开中断*/
            
            return  (iRetVal);
        }
    }
    
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);                          /*  解锁内核 spinlock 并打开中断*/
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kernelExitIrq
** 功能描述: 退出内核状态同时打开中断
** 输　入  : iregInterLevel     中断寄存器
** 输　出  : 调度器返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __kernelExitIrq (INTREG     iregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRetVal;
    
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_iKernelCounter) {
        pcpuCur->CPU_iKernelCounter--;
        KN_SMP_WMB();                                                   /*  等待以上操作完成            */

        if (!pcpuCur->CPU_iKernelCounter) {
            __LW_CLEAR_KERNEL_OWNER();
            
            iRetVal = _Schedule();                                      /*  尝试调度                    */
            LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);                  /*  解锁内核 spinlock 并打开中断*/
            
            return  (iRetVal);
        }
    }
    
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);                          /*  解锁内核 spinlock 并打开中断*/
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kernelIsEnter
** 功能描述: 是否进入了进入内核状态
** 输　入  : NONE
** 输　出  : LW_TRUE or LW_FALSE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  __kernelIsEnter (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    INT             iKernelCounter;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    pcpuCur = LW_CPU_GET_CUR();
    iKernelCounter = pcpuCur->CPU_iKernelCounter;
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    if (iKernelCounter) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: __kernelIsLockByMe
** 功能描述: 内核自旋锁是否被当前 CPU 锁定
** 输　入  : bIntLock      外部是否已经关闭了中断
** 输　出  : LW_TRUE or LW_FALSE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  __kernelIsLockByMe (BOOL  bIntLock)
{
    INTREG         iregInterLevel;
    PLW_CLASS_CPU  pcpuCur;
    BOOL           bRet;

    if (!bIntLock) {
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
    }

    pcpuCur = LW_CPU_GET_CUR();

    KN_SMP_MB();
    bRet = ((PLW_CLASS_CPU)_K_klKernel.KERN_slLock.SL_pcpuOwner == pcpuCur)
         ? LW_TRUE : LW_FALSE;
    
    if (!bIntLock) {
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
    }
    
    return  (bRet);
}
/*********************************************************************************************************
** 函数名称: __kernelSched
** 功能描述: 内核调度
** 输　入  : NONE
** 输　出  : 调度器返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __kernelSched (VOID)
{
    INTREG          iregInterLevel;
    INT             iRetVal;
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);                           /*  锁内核 spinlock 并关闭中断  */
    iRetVal = _Schedule();                                              /*  尝试调度                    */
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);                          /*  解锁内核 spinlock 并打开中断*/
    
    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: __kernelSchedInt
** 功能描述: 退出中断时内核调度 (在关中断的情况下被调用)
** 输　入  : pcpuCur   当前 CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __kernelSchedInt (PLW_CLASS_CPU    pcpuCur)
{
    LW_SPIN_KERN_LOCK_IGNIRQ();                                         /*  锁内核 spinlock 并关闭中断  */
    _ScheduleInt();                                                     /*  尝试调度                    */
    LW_SPIN_KERN_UNLOCK_IGNIRQ();                                       /*  解锁内核 spinlock 并打开中断*/
}
/*********************************************************************************************************
** 函数名称: __kernelOwner
** 功能描述: 获取内核拥有者线程
** 输　入  : NONE
** 输　出  : 线程句柄, ~0 表示中断中进入, 0 没有进入内核
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_OBJECT_HANDLE __kernelOwner (VOID)
{
#ifdef __LW_KERNLOCK_BUG_TRACE_EN
    INTREG  iregInterLevel;
    ULONG   ulKernelOwner;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    ulKernelOwner = _K_klKernel.KERN_ulKernelOwner;
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    return  (ulKernelOwner);

#else
    return  (1ul);
#endif                                                                  /*  __LW_KERNLOCK_BUG_TRACE_EN  */
}
/*********************************************************************************************************
** 函数名称: __kernelEnterFunc
** 功能描述: 获取内核拥有者进入函数
** 输　入  : NONE
** 输　出  : 内核进入函数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
CPCHAR __kernelEnterFunc (VOID)
{
#ifdef __LW_KERNLOCK_BUG_TRACE_EN
    INTREG  iregInterLevel;
    CPCHAR  pcKernelEnterFunc;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    pcKernelEnterFunc = _K_klKernel.KERN_pcKernelEnterFunc;
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    return  (pcKernelEnterFunc);
    
#else
    return  ("<No kernel bug trace>");
#endif                                                                  /*  __LW_KERNLOCK_BUG_TRACE_EN  */
}
/*********************************************************************************************************
  内核环境中对 IO 的访问为内核文件描述符表
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: __kernelSpaceEnter
** 功能描述: 进入内核环境 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __kernelSpaceEnter (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __LW_ATOMIC_INC(&ptcbCur->TCB_atomicKernelSpace);
}
/*********************************************************************************************************
** 函数名称: __kernelSpaceExit
** 功能描述: 退出内核环境
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __kernelSpaceExit (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    if (__LW_ATOMIC_DEC(&ptcbCur->TCB_atomicKernelSpace) < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel space counter error.\r\n");
        __LW_ATOMIC_SET(0, &ptcbCur->TCB_atomicKernelSpace);
    }
}
/*********************************************************************************************************
** 函数名称: __kernelSpaceGet
** 功能描述: 获得内核环境
** 输　入  : NONE
** 输　出  : 嵌套层数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __kernelSpaceGet (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (__LW_ATOMIC_GET(&ptcbCur->TCB_atomicKernelSpace));
}
/*********************************************************************************************************
** 函数名称: __kernelSpaceGet2
** 功能描述: 获得内核环境
** 输　入  : ptcb          任务控制块
** 输　出  : 嵌套层数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __kernelSpaceGet2 (PLW_CLASS_TCB  ptcb)
{
    return  (__LW_ATOMIC_GET(&ptcb->TCB_atomicKernelSpace));
}
/*********************************************************************************************************
** 函数名称: __kernelSpaceSet
** 功能描述: 进入内核环境 
** 输　入  : iNesting      嵌套层数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __kernelSpaceSet (INT  iNesting)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);

    __LW_ATOMIC_SET(iNesting, &ptcbCur->TCB_atomicKernelSpace);
}
/*********************************************************************************************************
** 函数名称: __kernelSpaceSet2
** 功能描述: 进入内核环境 
** 输　入  : ptcb          任务控制块
**           iNesting      嵌套层数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __kernelSpaceSet2 (PLW_CLASS_TCB  ptcb, INT  iNesting)
{
    __LW_ATOMIC_SET(iNesting, &ptcb->TCB_atomicKernelSpace);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
