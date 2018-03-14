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
** 文   件   名: _ThreadAffinity.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 11 月 11 日
**
** 描        述: SMP 系统 CPU 亲和度模型.
** 
** 注        意: 当锁定的 CPU 停止运行时, 如果存在锁定线程, 则线程自动解锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
** 函数名称: _ThreadSetAffinity
** 功能描述: 将线程锁定到指定的 CPU 运行. (进入内核被调用且目标任务已经被停止)
** 输　入  : ptcb          线程控制块
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : LW_CPU_ZERO(pcpuset) 可以解除 CPU 锁定.
*********************************************************************************************************/
VOID  _ThreadSetAffinity (PLW_CLASS_TCB  ptcb, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    ULONG   i;
    ULONG   ulNumChk;
    
    ulNumChk = ((ULONG)stSize << 3);
    ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;
    
    for (i = 0; i < ulNumChk; i++) {
        if (ptcb->TCB_ulOption & LW_OPTION_THREAD_AFFINITY_ALWAYS) {
            if (LW_CPU_ISSET(i, pcpuset)) {
                ptcb->TCB_ulCPULock = i;
                ptcb->TCB_bCPULock  = LW_TRUE;                          /*  锁定执行 CPU                */
                return;
            }
        
        } else {
            if (LW_CPU_ISSET(i, pcpuset) && 
                LW_CPU_IS_ACTIVE(LW_CPU_GET(i)) &&
                !(LW_CPU_GET_IPI_PEND(i) & LW_IPI_DOWN_MSK)) {          /*  必须激活并且没有要被停止    */
                ptcb->TCB_ulCPULock = i;
                ptcb->TCB_bCPULock  = LW_TRUE;                          /*  锁定执行 CPU                */
                return;
            }
        }
    }
    
    ptcb->TCB_bCPULock = LW_FALSE;                                      /*  关闭 CPU 锁定               */
}
/*********************************************************************************************************
** 函数名称: _ThreadGetAffinity
** 功能描述: 获取线程 CPU 亲和度情况.  (进入内核被调用)
** 输　入  : ptcb          线程控制块
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadGetAffinity (PLW_CLASS_TCB  ptcb, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    if (ptcb->TCB_bCPULock) {
        LW_CPU_SET(ptcb->TCB_ulCPULock, pcpuset);
    
    } else {
        lib_bzero(pcpuset, stSize);                                     /*  没有亲和度设置              */
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadOffAffinity
** 功能描述: 将与指定 CPU 有亲和度设置的线程全部关闭亲和度调度. (进入内核被调用)
** 输　入  : pcpu          CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadOffAffinity (PLW_CLASS_CPU  pcpu)
{
             INTREG                iregInterLevel;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_PCB         ppcb;
    REGISTER PLW_LIST_LINE         plineList;
    REGISTER ULONG                 ulCPUId = pcpu->CPU_ulCPUId;
             
    for (plineList  = _K_plineTCBHeader;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {                 /*  遍历线程                    */
         
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
        ptcb = _LIST_ENTRY(plineList, LW_CLASS_TCB, TCB_lineManage);
        if (ptcb->TCB_bCPULock && 
            (ptcb->TCB_ulCPULock == ulCPUId) &&
            !(ptcb->TCB_ulOption & LW_OPTION_THREAD_AFFINITY_ALWAYS)) {
            if (__LW_THREAD_IS_READY(ptcb)) {
                if (ptcb->TCB_bIsCand) {
                    ptcb->TCB_bCPULock = LW_FALSE;
                
                } else {
                    ppcb = LW_CPU_RDY_PPCB(pcpu, ptcb->TCB_ucPriority);
                    _DelTCBFromReadyRing(ptcb, ppcb);                   /*  从 CPU 私有就绪表中退出     */
                    if (_PcbIsEmpty(ppcb)) {
                        __DEL_RDY_MAP(ptcb);
                    }
                    
                    ptcb->TCB_bCPULock = LW_FALSE;
                    ppcb = LW_GLOBAL_RDY_PPCB(ptcb->TCB_ucPriority);
                    _AddTCBToReadyRing(ptcb, ppcb, LW_FALSE);           /*  加入全局就绪表              */
                    if (_PcbIsOne(ppcb)) {
                        __ADD_RDY_MAP(ptcb);
                    }
                }
            } else {
                ptcb->TCB_bCPULock = LW_FALSE;                          /*  关闭 CPU 锁定               */
            }
        }
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
