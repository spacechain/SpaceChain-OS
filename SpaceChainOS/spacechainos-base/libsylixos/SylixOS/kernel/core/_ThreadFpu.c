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
** 文   件   名: _ThreadFpu.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 11 日
**
** 描        述: 这是系统线程 FPU 相关功能库.
**
** BUG:
2014.07.22  加入 _ThreadFpuSave() 作为 CPU 停止时保存运行的线程 FPU.
2014.01.18  不进行多余的浮点关闭操作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadFpuSwith
** 功能描述: 线程 FPU 切换 (在关闭中断状态下被调用)
** 输　入  : bIntSwitch    是否为 _SchedInt() 等中断状态下的调度函数调用
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

VOID  _ThreadFpuSwith (BOOL bIntSwitch)
{
    PLW_CLASS_TCB   ptcbCur;
    PLW_CLASS_TCB   ptcbHigh;
    REGISTER BOOL   bDisable = LW_FALSE;
    
    LW_TCB_GET_CUR(ptcbCur);
    LW_TCB_GET_HIGH(ptcbHigh);
    
#if LW_CFG_INTER_FPU > 0
    if (LW_KERN_FPU_EN_GET()) {                                         /*  中断状态支持 FPU            */
        /*
         *  由于内核支持 FPU 所以, 中断函数会保存当前任务的 FPU 上下文
         *  所以如果是中断环境的调度函数, 则不需要保存当前任务 FPU 上下文
         */
        if (bIntSwitch == LW_FALSE) {
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
                __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                /*  需要保存当前 FPU CTX        */
                bDisable = LW_TRUE;
            }
        } else {
            bDisable = LW_TRUE;
        }
    } else 
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
    {
        /*
         *  由于中断状态不支持 FPU 所以, 中断函数中不会对 FPU 上下文有任何操作
         *  这里不管是 _Sched() 还是 _SchedInt() 都需要保存当前任务的 FPU 上下文
         */
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                    /*  需要保存当前 FPU CTX        */
            bDisable = LW_TRUE;
        }
    }
        
        
    if (ptcbHigh->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        __ARCH_FPU_RESTORE(ptcbHigh->TCB_pvStackFP);                    /*  需要恢复新任务 FPU CTX      */
        
    } else if (bDisable) {
        __ARCH_FPU_DISABLE();                                           /*  新任务不需要 FPU 支持       */
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadFpuSave
** 功能描述: 线程 FPU 保存 (在关闭中断状态下被调用)
** 输　入  : bIntSwitch    是否为 _SchedInt() 等中断状态下的调度函数调用
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

VOID  _ThreadFpuSave (PLW_CLASS_TCB   ptcbCur, BOOL bIntSwitch)
{
    REGISTER BOOL   bDisable = LW_FALSE;

#if LW_CFG_INTER_FPU > 0
    if (LW_KERN_FPU_EN_GET()) {                                         /*  中断状态支持 FPU            */
        /*
         *  由于内核支持 FPU 所以, 中断函数会保存当前任务的 FPU 上下文
         *  所以如果是中断环境的调度函数, 则不需要保存当前任务 FPU 上下文
         */
        if (bIntSwitch == LW_FALSE) {
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
                __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                /*  需要保存当前 FPU CTX        */
                bDisable = LW_TRUE;
            }
        } else {
            bDisable = LW_TRUE;
        }
    } else 
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
    {
        /*
         *  由于中断状态不支持 FPU 所以, 中断函数中不会对 FPU 上下文有任何操作
         *  这里不管是 _Sched() 还是 _SchedInt() 都需要保存当前任务的 FPU 上下文
         */
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                    /*  需要保存当前 FPU CTX        */
            bDisable = LW_TRUE;
        }
    }
    
    if (bDisable) {
        __ARCH_FPU_DISABLE();                                           /*  不再需要 FPU 支持           */
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
