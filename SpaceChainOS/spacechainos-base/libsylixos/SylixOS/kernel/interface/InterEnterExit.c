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
** 文   件   名: InterEnterExit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统内核中断出口函数。

** BUG
2008.01.04  修改代码格式与注释.
2009.04.29  加入 SMP 支持.
2011.02.22  直接调用 _SchedInt() 进行中断调度.
2012.09.05  API_InterEnter() 第一次进入中断时, 需要保存当前任务的 FPU 上下文.
            API_InterExit() 如果没有产生调度, 则恢复被中断任务的 FPU 上下文.
2012.09.23  加入 INT ENTER 和 INT EXIT 回调.
2013.07.17  多核启动不再通过核间中断完成.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.07.19  合并普通 CPU 中断和核间中断出口, 不再区分核间中断与普通中断出口.
2013.12.12  这里不再处理中断 hook.
2015.05.14  优化中断进入与退出处理.
2017.04.28  中断返回操作完全交给 API_InterExit() 函数处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  macro
*********************************************************************************************************/
#if LW_CFG_INTER_FPU > 0
#define __INTER_FPU_CTX(nest)      &pcpu->CPU_fpuctxContext[nest]
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
/*********************************************************************************************************
** 函数名称: __fpuInterEnter
** 功能描述: 进入中断状态 FPU 处理 (在关中断的情况下被调用)
** 输　入  : pcpu  当前 CPU 控制块
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)

static VOID  __fpuInterEnter (PLW_CLASS_CPU  pcpu)
{
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulInterNesting = pcpu->CPU_ulInterNesting;
    
    if (ulInterNesting == 1) {                                          /*  从任务态进入中断            */
        ptcbCur = pcpu->CPU_ptcbTCBCur;
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                    /*  保存当前被中断线程 FPU CTX  */
        
        } else {
            __ARCH_FPU_ENABLE();                                        /*  使能当前中断下 FPU          */
        }

    } else {
        REGISTER ULONG  ulOldNest = ulInterNesting - 1;
        __ARCH_FPU_SAVE(__INTER_FPU_CTX(ulOldNest));
    }
}
/*********************************************************************************************************
** 函数名称: __fpuInterExit
** 功能描述: 退出中断状态 FPU 处理 (在关中断的情况下被调用)
** 输　入  : pcpu  当前 CPU 控制块
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fpuInterExit (PLW_CLASS_CPU  pcpu)
{
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulInterNesting = pcpu->CPU_ulInterNesting;
    
    if (ulInterNesting == 0) {                                          /*  退出到任务状态              */
        ptcbCur = pcpu->CPU_ptcbTCBCur;
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_RESTORE(ptcbCur->TCB_pvStackFP);                 /*  没有产生调度, 则恢复 FPU CTX*/
        
        } else {
            __ARCH_FPU_DISABLE();                                       /*  继续执行的任务不需要 FPU    */
        }

    } else {                                                            /*  退出后还在中断中            */
        __ARCH_FPU_RESTORE(__INTER_FPU_CTX(ulInterNesting));
    }
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
/*********************************************************************************************************
** 函数名称: API_InterEnter
** 功能描述: 内核中断入口函数 (在关中断的情况下被调用)
** 输　入  : 
** 输　出  : 中断层数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG    API_InterEnter (VOID)
{
    PLW_CLASS_CPU  pcpu;
    
    pcpu = LW_CPU_GET_CUR();
    pcpu->CPU_ulInterNesting++;

#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)
    if (LW_KERN_FPU_EN_GET()) {                                         /*  中断状态允许使用浮点运算    */
        __fpuInterEnter(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
    return  (pcpu->CPU_ulInterNesting);
}
/*********************************************************************************************************
** 函数名称: API_InterExit
** 功能描述: 内核中断出口函数 (在关中断的情况下被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
VOID    API_InterExit (VOID)
{
    PLW_CLASS_CPU  pcpu;
    
    pcpu = LW_CPU_GET_CUR();
    
#if LW_CFG_INTER_INFO > 0
    if (pcpu->CPU_ulInterNestingMax < pcpu->CPU_ulInterNesting) {
        pcpu->CPU_ulInterNestingMax = pcpu->CPU_ulInterNesting;
    }
#endif                                                                  /*  LW_CFG_INTER_INFO > 0       */

    if (pcpu->CPU_ulInterNesting) {                                     /*  系统中断嵌套层数--          */
        pcpu->CPU_ulInterNesting--;
    }
    
    if (pcpu->CPU_ulInterNesting) {                                     /*  查看系统是否在中断嵌套中    */
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)                   /*  恢复上一等级中断 FPU CTX    */
        if (LW_KERN_FPU_EN_GET()) {
            __fpuInterExit(pcpu);
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        return;                                                         /*  LW_CFG_INTER_FPU > 0        */
    }
    
    __KERNEL_SCHED_INT(pcpu);                                           /*  中断中的调度                */
    
#if (LW_CFG_CPU_FPU_EN > 0) && (LW_CFG_INTER_FPU > 0)
    if (LW_KERN_FPU_EN_GET()) {
        __fpuInterExit(pcpu);
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
                                                                        /*  LW_CFG_INTER_FPU > 0        */
                                                                        
    archIntCtxLoad(pcpu);                                               /*  中断返回 (当前任务 CTX 加载)*/
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
