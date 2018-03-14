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
** 文   件   名: inlUnpreemptiveCheck.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统检查当前线程是否允许抢占。

** BUG
2007.11.21  修改注释.
2008.01.20  将调度器的全局锁改为局部锁.
2008.05.18  加入对内核状态的判断.
2013.07.17  SMP 安全升级. 修正获取当前任务的方法.
*********************************************************************************************************/

#ifndef  __INLUNPREEMPTIVECHECK_H
#define  __INLUNPREEMPTIVECHECK_H

/*********************************************************************************************************
  任务调度锁
*********************************************************************************************************/

#define __THREAD_LOCK_GET(ptcb)      ((ptcb)->TCB_ulThreadLockCounter)
#define __THREAD_LOCK_INC(ptcb)      ((ptcb)->TCB_ulThreadLockCounter++)
#define __THREAD_LOCK_DEC(ptcb)      ((ptcb)->TCB_ulThreadLockCounter--)

/*********************************************************************************************************
  检查是否需要调度. (必须是关中断的情况下, 确保当前 CPU 不会发生调度)
*********************************************************************************************************/

static LW_INLINE BOOL __can_preemptive (PLW_CLASS_CPU  pcpu, ULONG  ulMaxLockCounter)
{
    PLW_CLASS_TCB   ptcbCur = pcpu->CPU_ptcbTCBCur;

    /*
     *  中断中或者在内核中执行, 不允许调度.
     */
    if (pcpu->CPU_ulInterNesting || pcpu->CPU_iKernelCounter) {
        return  (LW_FALSE);
    }
    
    /*
     *  当前线程就绪且被锁定, 不允许调度.
     */
    if ((__THREAD_LOCK_GET(ptcbCur) > ulMaxLockCounter) && __LW_THREAD_IS_READY(ptcbCur)) {
        return  (LW_FALSE);
    }
    
    return  (LW_TRUE);
}

/*********************************************************************************************************
  判断调度是否可执行宏. (没有嵌套发生并且有线程请求调度被激活)
*********************************************************************************************************/

#define __COULD_SCHED(pcpu, ulMaxLockCounter)    __can_preemptive(pcpu, ulMaxLockCounter)

/*********************************************************************************************************
  判断是否需要调度
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#define __ISNEED_SCHED(pcpu, ulMaxLockCounter)   (__COULD_SCHED(pcpu, ulMaxLockCounter) && \
                                                  LW_CAND_ROT(pcpu))
#endif

/*********************************************************************************************************
  多核 CPU 内部锁定
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static LW_INLINE BOOL  __smp_cpu_lock (VOID)
{
    BOOL    bSchedLock = _SchedIsLock(0);
    
    if (bSchedLock == LW_FALSE) {
        _ThreadLock();                                                  /*  锁定当前 CPU 执行           */
    }
    
    return  (bSchedLock ? LW_FALSE : LW_TRUE);
}

static LW_INLINE VOID  __smp_cpu_unlock (BOOL  bUnlock)
{
    if (bUnlock) {
        _ThreadUnlock();                                                /*  解锁当前 CPU 执行           */
    }
}

#define __SMP_CPU_LOCK()        __smp_cpu_lock()
#define __SMP_CPU_UNLOCK(prev)  __smp_cpu_unlock(prev)

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#endif                                                                  /*  __INLUNPREEMPTIVECHECK_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
