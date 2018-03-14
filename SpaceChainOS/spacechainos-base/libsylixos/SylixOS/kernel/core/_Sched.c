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
** 文   件   名: _Sched.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 这是系统内核调度器。

** BUG
2008.01.04  修改代码格式与注释.
2008.04.06  支持线程上下文返回值的功能.
2009.04.29  加入 SMP 支持.
2011.02.22  加入 _SchedInt() 中断调度(没有将当前现场压栈), 这样所有的调度都集中在这里.
2012.09.07  优化代码.
2012.09.23  加入 IDLE ENTER 和 IDLE EXIT 回调功能.
2013.07.17  调度器只负责本核的 cpu 结构赋值, 其他的 cpu 只需用核间中断通知即可.
2013.07.19  合并 _SchedInt 和 _SchedCoreInt 通过 Cur 和 High 同时判断是否需要发送核间中断.
2013.07.21  调度器应该首先判断其他核是否需要调度, 如果需要, 则发送核间中断, 然后再处理本核调度.
2013.07.29  如果候选运行表产生优先级卷绕, 则首先处理卷绕.
2013.08.28  加入内核事件监控.
2013.12.02  _SchedGetCandidate() 允许抢占锁定层数为 1.
2014.01.05  将调度器 BUG 跟踪放在此处.
2014.01.07  加入 _SchedCrSwp, 统一任务与协程移植的格式.
2014.07.21  加入 CPU 停止功能.
2015.11.13  加入 spinlock bug trace.
2016.05.14  SMP 调度支持最快调度与核间中断最少调度.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  scheduler bug trace
*********************************************************************************************************/
#ifdef  __LW_SCHEDULER_BUG_TRACE_EN
#define __LW_SCHEDULER_BUG_TRACE(ptcb)  \
        _BugHandle((ptcb == LW_NULL), LW_TRUE, \
                   "scheduler candidate serious error, ptcb == NULL.\r\n"); \
        _BugFormat((!__LW_THREAD_IS_READY(ptcb)), LW_TRUE,  \
                   "scheduler candidate serious error, "    \
                   "ptcb %p, name \"%s\", status 0x%x.\r\n",    \
                   ptcb, ptcb->TCB_cThreadName, ptcb->TCB_usStatus);
#else
#define __LW_SCHEDULER_BUG_TRACE(ptcb)
#endif                                                                  /*  __LW_SCHEDULER_BUG_TRACE_EN */
/*********************************************************************************************************
  spinlock bug trace
*********************************************************************************************************/
#ifdef  __LW_SPINLOCK_BUG_TRACE_EN
#define __LW_SPINLOCK_BUG_TRACE(ptcb)   \
        _BugHandle((LW_CPU_SPIN_NESTING_GET(ptcb) > 0ul), LW_FALSE, \
                   "pend on spin lock nesting status!\r\n");
#else
#define __LW_SPINLOCK_BUG_TRACE(ptcb)
#endif                                                                  /*  __LW_SPINLOCK_BUG_TRACE_EN  */
/*********************************************************************************************************
  任务私有变量切换
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
#define __LW_TASK_SWITCH_VAR(ptcbCur, ptcbHigh)     _ThreadVarSwith(ptcbCur, ptcbHigh)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_VAR(ptcbCur)                 _ThreadVarSave(ptcbCur)
#endif
#else
#define __LW_TASK_SWITCH_VAR(ptcbCur, ptcbHigh)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_VAR(ptcbCur)
#endif
#endif
/*********************************************************************************************************
  任务 FPU 上下文切换
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#define __LW_TASK_SWITCH_FPU(bIntSwitch)            _ThreadFpuSwith(bIntSwitch)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_FPU(ptcbCur, bIntSwitch)     _ThreadFpuSave(ptcbCur, bIntSwitch)
#endif
#else
#define __LW_TASK_SWITCH_FPU(bIntSwitch)
#if LW_CFG_SMP_CPU_DOWN_EN > 0
#define __LW_TASK_SAVE_FPU(ptcbCur, bIntSwitch)
#endif
#endif
/*********************************************************************************************************
  任务状态迁移判断
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define __LW_STATUS_CHANGE_EN(ptcb, pcpu)           (!(pcpu)->CPU_ulInterNesting && \
                                                     (__THREAD_LOCK_GET(ptcb) <= 1ul))
/*********************************************************************************************************
  scheduler 通知机制
*********************************************************************************************************/
static VOIDFUNCPTR                                  _K_pfuncSchedSmpNotify;
#if LW_CFG_CPU_ARCH_SMT > 0
static VOIDFUNCPTR                                  _K_pfuncSchedSecNotify;
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#define __LW_SMP_NOTIFY(ulCPUIdCur)                 _K_pfuncSchedSmpNotify(ulCPUIdCur)
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: _SchedSmpFschedNotify
** 功能描述: 通知需要调度的 CPU (fast sched)
** 输　入  : ulCPUIdCur 当前 CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 当另一个核产生了调度器卷绕, 并且没有在处理 sched 核间中断, 
             则发送核间中断.
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  _SchedSmpFschedNotify (ULONG  ulCPUIdCur)
{
    INT             i;
    PLW_CLASS_CPU   pcpu;
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUIdCur) {                      /*  遍历 CPU 检查是否需要调度   */
        pcpu = LW_CPU_GET(i);
        if (LW_CAND_ROT(pcpu) &&
            ((LW_CPU_GET_IPI_PEND(i) & LW_IPI_SCHED_MSK) == 0)) {
            _SmpSendIpi(i, LW_IPI_SCHED, 0, LW_TRUE);                   /*  产生核间中断                */
        }
    }
}
/*********************************************************************************************************
** 函数名称: _SchedSmpNormalNotify
** 功能描述: 通知需要调度的 CPU (normal sched)
** 输　入  : ulCPUIdCur 当前 CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 当另一个核产生了调度器卷绕, 并且没有在处理 sched 核间中断, 
             则选择运行任务优先级最低的 CPU 发送核间中断.
*********************************************************************************************************/
static VOID  _SchedSmpNormalNotify (ULONG  ulCPUIdCur)
{
    INT             i;
    INT             iCPUSend;
    UINT8           ucPrioLow = 0;
    PLW_CLASS_CPU   pcpu;
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUIdCur) {                      /*  遍历 CPU 检查是否需要调度   */
        pcpu = LW_CPU_GET(i);
        if (LW_CAND_ROT(pcpu) &&
            ((LW_CPU_GET_IPI_PEND(i) & LW_IPI_SCHED_MSK) == 0)) {
            if (LW_PRIO_IS_HIGH(ucPrioLow, 
                                LW_CAND_TCB(pcpu)->TCB_ucPriority)) {
                ucPrioLow = LW_CAND_TCB(pcpu)->TCB_ucPriority;
                iCPUSend  = i;
            }
        }
    }
    if (ucPrioLow) {
        _SmpSendIpi(iCPUSend, LW_IPI_SCHED, 0, LW_TRUE);                /*  产生核间中断                */
    }
}
/*********************************************************************************************************
** 函数名称: _SchedSmpSmtNotify
** 功能描述: 通知需要调度的 CPU (smt balance sched)
** 输　入  : ulCPUIdCur 当前 CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 当另一个核产生了调度器卷绕, 并且没有在处理 sched 核间中断, 
             则优先选择物理 CPU 空闲的 CPU 发送核间中断.
*********************************************************************************************************/
#if LW_CFG_CPU_ARCH_SMT > 0

static VOID  _SchedSmpSmtNotify (ULONG  ulCPUIdCur)
{
    INT                i;
    PLW_CLASS_CPU      pcpu;
    PLW_CLASS_PHYCPU   pphycpu;
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUIdCur) {                      /*  遍历 CPU 检查是否需要调度   */
        pcpu = LW_CPU_GET(i);
        if (LW_CAND_ROT(pcpu) &&
            ((LW_CPU_GET_IPI_PEND(i) & LW_IPI_SCHED_MSK) == 0)) {
            pphycpu = LW_PHYCPU_GET(pcpu->CPU_ulPhyId);
            if (pphycpu->PHYCPU_uiNonIdle == 0) {                       /*  此物理 CPU 处于 idle 任务   */
                _SmpSendIpi(i, LW_IPI_SCHED, 0, LW_TRUE);               /*  产生核间中断                */
                return;
            }
        }
    }
    
    _K_pfuncSchedSecNotify(ulCPUIdCur);                                 /*  尝试普通 SMP 调度通知       */
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
/*********************************************************************************************************
** 函数名称: _SchedCpuDown
** 功能描述: CPU 停止工作
** 输　入  : pcpuCur       当前 CPU 控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

static LW_INLINE VOID  _SchedCpuDown (PLW_CLASS_CPU  pcpuCur, BOOL  bIsIntSwtich)
{
    REGISTER PLW_CLASS_TCB  ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    REGISTER ULONG          ulCPUId = pcpuCur->CPU_ulCPUId;

    _CpuInactive(pcpuCur);                                              /*  停止 CPU                    */
    
    __LW_TASK_SAVE_VAR(ptcbCur);
    __LW_TASK_SAVE_FPU(ptcbCur, bIsIntSwtich);
    
    _SchedSmpFschedNotify(ulCPUId);                                     /*  请求其他 CPU 调度           */
    
#if LW_CFG_CACHE_EN > 0
    API_CacheDisable(DATA_CACHE);                                       /*  禁能 CACHE                  */
    API_CacheDisable(INSTRUCTION_CACHE);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

#if LW_CFG_VMM_EN > 0
    API_VmmMmuDisable();                                                /*  关闭 MMU                    */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    
    LW_SPIN_KERN_UNLOCK_SCHED(ptcbCur);                                 /*  解锁内核 spinlock           */

    LW_SPINLOCK_NOTIFY();
    bspCpuDown(ulCPUId);                                                /*  BSP 停止 CPU                */
    
    _BugHandle(LW_TRUE, LW_TRUE, "CPU Down error!\r\n");                /*  不会运行到这里              */
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: _SchedSwap
** 功能描述: arch 任务切换函数首先保存当前线程上下文, 然后调用此函数, 然后再恢复需要执行任务的上下文.
** 输　入  : pcpuCur   当前CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
VOID _SchedSwp (PLW_CLASS_CPU pcpuCur)
{
    REGISTER PLW_CLASS_TCB      ptcbCur      = pcpuCur->CPU_ptcbTCBCur;
    REGISTER PLW_CLASS_TCB      ptcbHigh     = pcpuCur->CPU_ptcbTCBHigh;
    REGISTER LW_OBJECT_HANDLE   ulCurId      = ptcbCur->TCB_ulId;
    REGISTER LW_OBJECT_HANDLE   ulHighId     = ptcbHigh->TCB_ulId;
             BOOL               bIsIntSwtich = pcpuCur->CPU_bIsIntSwtich;

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_SMP_CPU_DOWN_EN > 0)
    if (LW_CPU_GET_IPI_PEND(pcpuCur->CPU_ulCPUId) & LW_IPI_DOWN_MSK) {  /*  当前 CPU 需要停止           */
        _SchedCpuDown(pcpuCur, bIsIntSwtich);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
                                                                        /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
    __LW_TASK_SWITCH_VAR(ptcbCur, ptcbHigh);                            /*  线程私有化变量切换          */
    __LW_TASK_SWITCH_FPU(bIsIntSwtich);
    
    bspTaskSwapHook(ulCurId, ulHighId);                                 /*  调用 hook 函数              */
    __LW_THREAD_SWAP_HOOK(ulCurId, ulHighId);
    
    if (_ObjectGetIndex(ulHighId) < LW_NCPUS) {                         /*  CPU 进入空闲模式            */
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
        LW_PHYCPU_GET(pcpuCur->CPU_ulPhyId)->PHYCPU_uiNonIdle--;
#endif
        __LW_CPU_IDLE_ENTER_HOOK(ulCurId);
        
    } else if (_ObjectGetIndex(ulCurId) < LW_NCPUS) {                   /*  CPU 退出空闲模式            */
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
        LW_PHYCPU_GET(pcpuCur->CPU_ulPhyId)->PHYCPU_uiNonIdle++;
#endif
        __LW_CPU_IDLE_EXIT_HOOK(ulHighId);
    }
    
#if LW_CFG_SMP_EN > 0
    __LW_SMP_NOTIFY(pcpuCur->CPU_ulCPUId);                              /*  SMP 调度通知                */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcpuCur->CPU_ptcbTCBCur = ptcbHigh;                                 /*  切换任务                    */

    LW_SPIN_KERN_UNLOCK_SCHED(ptcbCur);                                 /*  解锁内核 spinlock           */
    
    if (bIsIntSwtich) {
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SCHED, MONITOR_EVENT_SCHED_INT, 
                          ulCurId, ulHighId, LW_NULL);
    } else {
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SCHED, MONITOR_EVENT_SCHED_TASK,
                          ulCurId, ulHighId, LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: _CoroutineDeleteAll
** 功能描述: 释放指定线程所有的协程空间.
** 输　入  : ptcb                     线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID _SchedCrSwp (PLW_CLASS_CPU pcpuCur)
{
#if LW_CFG_COROUTINE_EN > 0
    pcpuCur->CPU_pcrcbCur = pcpuCur->CPU_pcrcbNext;                     /*  切换协程                    */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
}
/*********************************************************************************************************
** 函数名称: _SchedSafeStack
** 功能描述: arch 任务切换函数获得安全堆栈位置.
** 输　入  : pcpuCur   当前CPU
** 输　出  : 堆栈位置
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

PVOID _SchedSafeStack (PLW_CLASS_CPU pcpuCur)
{
    return  (pcpuCur->CPU_pstkInterBase);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: Schedule
** 功能描述: 内核退出时, 会调用此调度函数 (进入内核状态并关中断被调用)
** 输　入  : NONE
** 输　出  : 线程上下文返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _Schedule (VOID)
{
    ULONG            ulCPUId;
    PLW_CLASS_CPU    pcpuCur;
    PLW_CLASS_TCB    ptcbCur;
    PLW_CLASS_TCB    ptcbCand;
    INT              iRetVal = ERROR_NONE;
    
    ulCPUId = LW_CPU_GET_CUR_ID();                                      /*  当前 CPUID                  */
    pcpuCur = LW_CPU_GET(ulCPUId);                                      /*  当前 CPU 控制块             */
    ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    
#if LW_CFG_SMP_EN > 0
    if (ptcbCur->TCB_plineStatusReqHeader) {                            /*  请求当前任务改变状态        */
        if (__LW_STATUS_CHANGE_EN(ptcbCur, pcpuCur)) {                  /*  是否可以进行状态切换        */
            _ThreadStatusChangeCur(pcpuCur);                            /*  检查是否需要进行状态切换    */
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcbCand = _SchedGetCand(pcpuCur, 1ul);                             /*  获得需要运行的线程          */
    if (ptcbCand != ptcbCur) {                                          /*  如果与当前运行的不同, 切换  */
        __LW_SCHEDULER_BUG_TRACE(ptcbCand);                             /*  调度器 BUG 检测             */
        
#if LW_CFG_SMP_EN > 0                                                   /*  SMP 系统                    */
        __LW_SPINLOCK_BUG_TRACE(pcpuCur);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        pcpuCur->CPU_bIsIntSwtich = LW_FALSE;                           /*  非中断调度                  */
        pcpuCur->CPU_ptcbTCBHigh  = ptcbCand;
        
        /*
         *  TASK CTX SAVE();
         *  SWITCH to SAFE stack if in SMP system;
         *  _SchedSwp();
         *  TASK CTX LOAD();
         */
        archTaskCtxSwitch(pcpuCur);                                     /*  线程切换,并释放内核自旋锁   */
        LW_SPIN_KERN_LOCK_IGNIRQ();                                     /*  内核自旋锁重新加锁          */
    }
#if LW_CFG_SMP_EN > 0                                                   /*  SMP 系统                    */
      else {
        __LW_SMP_NOTIFY(ulCPUId);                                       /*  SMP 调度通知                */
        return  (iRetVal);
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    LW_TCB_GET_CUR(ptcbCur);                                            /*  获得新的当前 TCB            */
    
    iRetVal = ptcbCur->TCB_iSchedRet;                                   /*  获得调度器信号的返回值      */
    ptcbCur->TCB_iSchedRet = ERROR_NONE;                                /*  清空                        */
    
    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: _ScheduleInt
** 功能描述: 中断退出时, 会调用此调度函数 (进入内核状态并关中断被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ScheduleInt (VOID)
{
    ULONG            ulCPUId;
    PLW_CLASS_CPU    pcpuCur;
    PLW_CLASS_TCB    ptcbCur;
    PLW_CLASS_TCB    ptcbCand;
    
    ulCPUId = LW_CPU_GET_CUR_ID();                                      /*  当前 CPUID                  */
    pcpuCur = LW_CPU_GET(ulCPUId);                                      /*  当前 CPU 控制块             */
    ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    
#if LW_CFG_SMP_EN > 0
    if (__LW_STATUS_CHANGE_EN(ptcbCur, pcpuCur)) {                      /*  是否可以进行状态切换        */
        if (ptcbCur->TCB_plineStatusReqHeader) {                        /*  请求当前任务改变状态        */
            _ThreadStatusChangeCur(pcpuCur);                            /*  检查是否需要进行状态切换    */
        }
        
#if LW_CFG_SMP_CPU_DOWN_EN > 0
        if (LW_CPU_GET_IPI_PEND(pcpuCur->CPU_ulCPUId) & 
            LW_IPI_DOWN_MSK) {                                          /*  当前 CPU 需要停止           */
            _SchedCpuDown(pcpuCur, LW_TRUE);
        }
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcbCand = _SchedGetCand(pcpuCur, 1ul);                             /*  获得需要运行的线程          */
    if (ptcbCand != ptcbCur) {                                          /*  如果与当前运行的不同, 切换  */
        __LW_SCHEDULER_BUG_TRACE(ptcbCand);                             /*  调度器 BUG 检测             */

#if LW_CFG_SMP_EN > 0                                                   /*  SMP 系统                    */
        __LW_SPINLOCK_BUG_TRACE(pcpuCur);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        pcpuCur->CPU_bIsIntSwtich = LW_TRUE;                            /*  中断调度                    */
        pcpuCur->CPU_ptcbTCBHigh  = ptcbCand;
        
        _SchedSwp(pcpuCur);                                             /*  直接调用 _SchedSwp()        */
        /*
         *  TASK CTX LOAD();
         */
        archIntCtxLoad(pcpuCur);                                        /*  中断上下文中线程切换        */
        _BugHandle(LW_TRUE, LW_TRUE, "serious error!\r\n");
    }
#if LW_CFG_SMP_EN > 0                                                   /*  SMP 系统                    */
      else {
        __LW_SMP_NOTIFY(ulCPUId);                                       /*  SMP 调度通知                */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
}

/*********************************************************************************************************
** 函数名称: _ScheduleInit
** 功能描述: 初始化调度器
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ScheduleInit (VOID)
{
#if LW_CFG_SMP_EN > 0
#if LW_CFG_CPU_ARCH_SMT > 0
    if (LW_KERN_SMT_BSCHED_EN_GET()) {
        _K_pfuncSchedSmpNotify = _SchedSmpSmtNotify;
        if (LW_KERN_SMP_FSCHED_EN_GET()) {
            _K_pfuncSchedSecNotify = _SchedSmpFschedNotify;
        
        } else {
            _K_pfuncSchedSecNotify = _SchedSmpNormalNotify;
        }
    
    } else 
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
    {
        if (LW_KERN_SMP_FSCHED_EN_GET()) {
            _K_pfuncSchedSmpNotify = _SchedSmpFschedNotify;
        
        } else {
            _K_pfuncSchedSmpNotify = _SchedSmpNormalNotify;
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: _SchedSetRet
** 功能描述: 设置当前任务调度器返回值, 在产生主动调度时, 将获取这个值 (关中断情况下被调用)
** 输　入  : iSchedRet         调度器返回值
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SchedSetRet (INT  iSchedSetRet)
{
    PLW_CLASS_TCB    ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);
    if (ptcbCur->TCB_iSchedRet == ERROR_NONE) {
        ptcbCur->TCB_iSchedRet =  iSchedSetRet;
    }
}
/*********************************************************************************************************
** 函数名称: _SchedSetPrio
** 功能描述: 设置指定任务优先级 (进入内核状态下被调用)
** 输　入  : ptcb           任务
**           ucPriority     优先级
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 如果是当前正在候选表中, 只要设置目标任务所在 CPU 候选表卷绕标记即可, 当前任务最后解锁时, 将会
             检测调度器卷绕标志, 重新进行一次抢占调度测试, 如果有更加合适的任务就绪, 则抢占目标任务.
*********************************************************************************************************/
VOID  _SchedSetPrio (PLW_CLASS_TCB  ptcb, UINT8  ucPriority)
{
    INTREG           iregInterLevel;
    PLW_CLASS_PCB    ppcbFrom;
    PLW_CLASS_PCB    ppcbTo;
    
    ppcbFrom = _GetPcb(ptcb);
    ppcbTo   = _GetPcbEx(ptcb, ucPriority);
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_PRIO,
                      ptcb->TCB_ulId, ptcb->TCB_ucPriority, ucPriority, LW_NULL);
                      
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  线程就绪                    */
        if (ptcb->TCB_bIsCand) {                                        /*  在候选表中                  */
            LW_CAND_ROT(LW_CPU_GET(ptcb->TCB_ulCPUId)) =  LW_TRUE;      /*  退出内核时尝试抢占调度      */
            ptcb->TCB_ucPriority = ucPriority;                          /*  直接设置新的优先级          */
            
        } else {                                                        /*  不在候选表中                */
            __DEL_FROM_READY_RING(ptcb, ppcbFrom);                      /*  从就绪环中删除              */
            
            ptcb->TCB_ucPriority = ucPriority;                          /*  设置新的优先级              */
            __ADD_TO_READY_RING(ptcb, ppcbTo);                          /*  加入新的就绪环              */
        }
    } else {
        ptcb->TCB_ucPriority = ucPriority;                              /*  直接设置新的优先级          */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
