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
** 文   件   名: _ThreadSafe.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 线程安全模式函数库

** BUG
2007.05.08  _ThreadUnsafeInternal() 和 _ThreadUnsafeInternalEx() 可能出现没有打开中断的情况
2008.03.30  使用新的就绪环操作.
2008.03.30  自我删除时, 需要快速清除 TCB_ptcbDeleteThread 标志, 否则可能在 Thread Delete 对 heap 
            的锁有影响.
2010.01.22  支持 SMP.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.09.10  如果请求删除自己, 则退出安全模式时不需要唤醒.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadSafeSuspend
** 功能描述: 线程阻塞等待对方从安全模式退出 (在进入内核并关闭中断中调用)
** 输　入  : ptcbCur       当前任务
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadSafeSuspend (PLW_CLASS_TCB  ptcbCur)
{
    REGISTER PLW_CLASS_PCB   ppcbMe;
             
    ptcbCur->TCB_ulSuspendNesting++;
    ppcbMe = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcbMe);                             /*  从就绪环中删除              */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
}
/*********************************************************************************************************
** 函数名称: _ThreadSafeResume
** 功能描述: 线程进入安全模式 (在进入内核并关闭中断中调用)
** 输　入  : ptcb          任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadSafeResume (PLW_CLASS_TCB  ptcb)
{
    REGISTER PLW_CLASS_PCB  ppcb;
    
    if (ptcb->TCB_ulSuspendNesting) {
        ptcb->TCB_ulSuspendNesting--;
    } else {
        return;
    }
    
    if (ptcb->TCB_ulSuspendNesting) {                                   /*  出现嵌套用户操作不当造成    */
        return;
    }
    
    ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SUSPEND);
    
    if (__LW_THREAD_IS_READY(ptcb)) {
       ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;              /*  中断激活方式                */
       ppcb = _GetPcb(ptcb);
       __ADD_TO_READY_RING(ptcb, ppcb);                                 /*  加入就绪环                  */
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadSafeInternal
** 功能描述: 当前线程进入安全模式
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadSafeInternal (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TCB   ptcbCur;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    LW_TCB_GET_CUR(ptcbCur);
    
    ptcbCur->TCB_ulThreadSafeCounter++;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SAFE, 
                      ptcbCur->TCB_ulId, ptcbCur->TCB_ulThreadSafeCounter, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _ThreadUnsafeInternal
** 功能描述: 当前线程进入安全模式
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadUnsafeInternal (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    _ThreadUnsafeInternalEx(ptcbCur);
}
/*********************************************************************************************************
** 函数名称: _ThreadUnsafeInternalEx
** 功能描述: 指定线程退出安全模式
** 输　入  : ptcbDes       目标线程
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadUnsafeInternalEx (PLW_CLASS_TCB   ptcbDes)
{
             INTREG             iregInterLevel;
    REGISTER PLW_CLASS_TCB      ptcbResume;
    
#if LW_CFG_THREAD_DEL_EN > 0
             LW_OBJECT_HANDLE   ulIdMe;
    REGISTER PVOID              pvRetValue;
#endif

    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_UNSAFE, 
                      ptcbDes->TCB_ulId, ptcbDes->TCB_ulThreadSafeCounter - 1, LW_NULL);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    if (ptcbDes->TCB_ulThreadSafeCounter) {
        ptcbDes->TCB_ulThreadSafeCounter--;
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
        return;
    }

    if (ptcbDes->TCB_ulThreadSafeCounter) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
        return;
    }
    
    if (ptcbDes->TCB_ptcbDeleteMe) {
        ptcbResume = ptcbDes->TCB_ptcbDeleteMe;
        ptcbDes->TCB_ptcbDeleteMe = LW_NULL;                            /*  清除标识                    */
        
        if ((ptcbResume != ptcbDes) && 
            ((addr_t)ptcbResume != 1ul) &&
            (!_Thread_Invalid(_ObjectGetIndex(ptcbResume->TCB_ulId)))) {
            ptcbResume->TCB_ptcbDeleteWait = LW_NULL;                   /*  退出等待模式                */
            _ThreadSafeResume(ptcbResume);                              /*  激活等待线程                */
        }
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
                                                                        /*  可能产生一次调度            */

#if LW_CFG_THREAD_DEL_EN > 0
        ulIdMe     = ptcbDes->TCB_ulId;
        pvRetValue = ptcbDes->TCB_pvRetValue;
        
        API_ThreadDelete(&ulIdMe, pvRetValue);                          /*  删除自己                    */
#endif
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
