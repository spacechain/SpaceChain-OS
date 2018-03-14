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
** 文   件   名: _SmpIpi.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 07 月 19 日
**
** 描        述: CPU 核间中断, (用于 SMP 多核系统)

** BUG:
2014.04.09  不能像没有 ACTIVE 的 CPU 发送核间中断.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  IPI LOCK
*********************************************************************************************************/
#define LW_IPI_LOCK(pcpu, bIntLock)                                     \
        if (bIntLock) {                                                 \
            LW_SPIN_LOCK_IGNIRQ(&pcpu->CPU_slIpi);                      \
        } else {                                                        \
            LW_SPIN_LOCK_QUICK(&pcpu->CPU_slIpi, &iregInterLevel);      \
        }
#define LW_IPI_UNLOCK(pcpu, bIntLock)                                   \
        if (bIntLock) {                                                 \
            LW_SPIN_UNLOCK_IGNIRQ(&pcpu->CPU_slIpi);                    \
        } else {                                                        \
            LW_SPIN_UNLOCK_QUICK(&pcpu->CPU_slIpi, iregInterLevel);     \
        }
#define LW_IPI_INT_LOCK(bIntLock)                                       \
        if (bIntLock == LW_FALSE) {                                     \
            iregInterLevel = KN_INT_DISABLE();                          \
        }
#define LW_IPI_INT_UNLOCK(bIntLock)                                     \
        if (bIntLock == LW_FALSE) {                                     \
            KN_INT_ENABLE(iregInterLevel);                              \
        }
/*********************************************************************************************************
** 函数名称: _SmpSendIpi
** 功能描述: 发送一个除自定义以外的核间中断给指定的 CPU. (如果不等待则外部可不锁定当前 CPU)
** 输　入  : ulCPUId       CPU ID
**           ulIPIVec      核间中断类型 (除自定义类型中断以外)
**           iWait         是否等待处理结束 (LW_IPI_SCHED 绝不允许等待, 否则会死锁)
**           bIntLock      外部是否关中断了.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 通知调度 (LW_IPI_SCHED) 与通知 CPU 停止 (LW_IPI_DOWN) 不需要等待结束.
*********************************************************************************************************/
VOID  _SmpSendIpi (ULONG  ulCPUId, ULONG  ulIPIVec, INT  iWait, BOOL  bIntLock)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuDst = LW_CPU_GET(ulCPUId);
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    ULONG           ulMask  = (ULONG)(1 << ulIPIVec);
    
    if (!LW_CPU_IS_ACTIVE(pcpuDst)) {                                   /*  CPU 必须被激活              */
        return;
    }
    
    LW_IPI_LOCK(pcpuDst, bIntLock);                                     /*  锁定目标 CPU IPI            */
    LW_CPU_ADD_IPI_PEND(ulCPUId, ulMask);                               /*  添加 PEND 位                */
    archMpInt(ulCPUId);
    LW_IPI_UNLOCK(pcpuDst, bIntLock);                                   /*  解锁目标 CPU IPI            */
    
    if (iWait) {
        while (LW_CPU_GET_IPI_PEND(ulCPUId) & ulMask) {                 /*  等待结束                    */
            LW_IPI_INT_LOCK(bIntLock);
            _SmpTryProcIpi(pcpuCur);                                    /*  尝试执行其他核发来的 IPI    */
            LW_IPI_INT_UNLOCK(bIntLock);
            LW_SPINLOCK_DELAY();
        }
    }
}
/*********************************************************************************************************
** 函数名称: _SmpSendIpiAllOther
** 功能描述: 发送一个除自定义以外的核间中断给所有其他 CPU, (外部必须锁定当前 CPU 调度)
** 输　入  : ulIPIVec      核间中断类型 (除自定义类型中断以外)
**           iWait         是否等待处理结束 (LW_IPI_SCHED 绝不允许等待, 否则会死锁)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpSendIpiAllOther (ULONG  ulIPIVec, INT  iWait)
{
    ULONG   i;
    ULONG   ulCPUId;
    
    ulCPUId = LW_CPU_GET_CUR_ID();
    
    KN_SMP_WMB();
    LW_CPU_FOREACH_EXCEPT (i, ulCPUId) {
        _SmpSendIpi(i, ulIPIVec, iWait, LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: _SmpCallIpi
** 功能描述: 发送一个自定义核间中断给指定的 CPU. (如果不等待则外部可不锁定当前 CPU)
** 输　入  : ulCPUId       CPU ID
**           pipim         核间中断参数
** 输　出  : 调用返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _SmpCallIpi (ULONG  ulCPUId, PLW_IPI_MSG  pipim)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuDst = LW_CPU_GET(ulCPUId);
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!LW_CPU_IS_ACTIVE(pcpuDst)) {                                   /*  CPU 必须被激活              */
        return  (ERROR_NONE);
    }
    
    LW_IPI_LOCK(pcpuDst, LW_FALSE);                                     /*  锁定目标 CPU IPI            */
    _List_Ring_Add_Last(&pipim->IPIM_ringManage, &pcpuDst->CPU_pringMsg);
    pcpuDst->CPU_uiMsgCnt++;
    LW_CPU_ADD_IPI_PEND(ulCPUId, LW_IPI_CALL_MSK);
    archMpInt(ulCPUId);
    LW_IPI_UNLOCK(pcpuDst, LW_FALSE);                                   /*  解锁目标 CPU IPI            */
    
    while (pipim->IPIM_iWait) {                                         /*  等待结束                    */
        LW_IPI_INT_LOCK(LW_FALSE);
        _SmpTryProcIpi(pcpuCur);
        LW_IPI_INT_UNLOCK(LW_FALSE);
        LW_SPINLOCK_DELAY();
    }
    
    return  (pipim->IPIM_iRet);
}
/*********************************************************************************************************
** 函数名称: _SmpCallIpiAllOther
** 功能描述: 发送一个自定义核间中断给其他所有 CPU. (外部必须锁定当前 CPU 调度)
** 输　入  : pipim         核间中断参数
** 输　出  : NONE (无法确定返回值)
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _SmpCallIpiAllOther (PLW_IPI_MSG  pipim)
{
    ULONG   i;
    ULONG   ulCPUId;
    INT     iWaitSave = pipim->IPIM_iWait;
    
    ulCPUId = LW_CPU_GET_CUR_ID();
    
    KN_SMP_WMB();
    LW_CPU_FOREACH_EXCEPT (i, ulCPUId) {
        _SmpCallIpi(i, pipim);
        
        KN_SMP_MB();
        pipim->IPIM_iWait = iWaitSave;
        KN_SMP_WMB();
    }
}
/*********************************************************************************************************
** 函数名称: _SmpCallFunc
** 功能描述: 利用核间中断让指定的 CPU 运行指定的函数. (外部必须锁定当前 CPU 调度)
** 输　入  : ulCPUId       CPU ID
**           pfunc         同步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvArg         同步参数
**           pfuncAsync    异步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvAsync       异步执行参数
**           iOpt          选项 IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** 输　出  : 调用返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _SmpCallFunc (ULONG        ulCPUId, 
                   FUNCPTR      pfunc, 
                   PVOID        pvArg,
                   VOIDFUNCPTR  pfuncAsync,
                   PVOID        pvAsync,
                   INT          iOpt)
{
    LW_IPI_MSG  ipim;
    
    ipim.IPIM_pfuncCall      = pfunc;
    ipim.IPIM_pvArg          = pvArg;
    ipim.IPIM_pfuncAsyncCall = pfuncAsync;
    ipim.IPIM_pvAsyncArg     = pvAsync;
    ipim.IPIM_iRet           = -1;
    ipim.IPIM_iOption        = iOpt;
    ipim.IPIM_iWait          = 1;
    
    return  (_SmpCallIpi(ulCPUId, &ipim));
}
/*********************************************************************************************************
** 函数名称: _SmpCallFuncAllOther
** 功能描述: 利用核间中断让指定的 CPU 运行指定的函数. (外部必须锁定当前 CPU 调度)
** 输　入  : pfunc         同步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvArg         同步参数
**           pfuncAsync    异步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvAsync       异步执行参数
**           iOpt          选项 IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** 输　出  : NONE (无法确定返回值)
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpCallFuncAllOther (FUNCPTR      pfunc, 
                            PVOID        pvArg,
                            VOIDFUNCPTR  pfuncAsync,
                            PVOID        pvAsync,
                            INT          iOpt)
{
    LW_IPI_MSG  ipim;
    
    ipim.IPIM_pfuncCall      = pfunc;
    ipim.IPIM_pvArg          = pvArg;
    ipim.IPIM_pfuncAsyncCall = pfuncAsync;
    ipim.IPIM_pvAsyncArg     = pvAsync;
    ipim.IPIM_iRet           = -1;
    ipim.IPIM_iOption        = iOpt;
    ipim.IPIM_iWait          = 1;
    
    _SmpCallIpiAllOther(&ipim);
}
/*********************************************************************************************************
** 函数名称: __smpProcCallfunc
** 功能描述: 处理核间中断调用函数
** 输　入  : pcpuCur       当前 CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __smpProcCallfunc (PLW_CLASS_CPU  pcpuCur)
{
#define LW_KERNEL_OWN_CPU()     (PLW_CLASS_CPU)(_K_klKernel.KERN_pvCpuOwner)

    UINT            i, uiCnt;
    PLW_IPI_MSG     pipim;
    PLW_LIST_RING   pringTemp;
    PLW_LIST_RING   pringDelete;
    VOIDFUNCPTR     pfuncAsync;
    PVOID           pvAsync;
    
    LW_SPIN_LOCK_IGNIRQ(&pcpuCur->CPU_slIpi);                           /*  锁定 CPU                    */
    
    pringTemp = pcpuCur->CPU_pringMsg;
    uiCnt     = pcpuCur->CPU_uiMsgCnt;
    
    for (i = 0; i < uiCnt; i++) {
        _BugHandle((!pcpuCur->CPU_pringMsg), LW_TRUE, "ipi call func error!\r\n");
        
        pipim = _LIST_ENTRY(pringTemp, LW_IPI_MSG, IPIM_ringManage);
        if ((LW_KERNEL_OWN_CPU() == pcpuCur) &&
            (pipim->IPIM_iOption & IPIM_OPT_NOKERN)) {                  /*  此函数不能再内核锁定状态执行*/
            pringTemp = _list_ring_get_next(pringTemp);
            continue;
        }
        
        pringDelete = pringTemp;
        pringTemp   = _list_ring_get_next(pringTemp);
        _List_Ring_Del(pringDelete, &pcpuCur->CPU_pringMsg);            /*  删除一个节点                */
        pcpuCur->CPU_uiMsgCnt--;
        
        if (pipim->IPIM_pfuncCall) {
            pipim->IPIM_iRet = pipim->IPIM_pfuncCall(pipim->IPIM_pvArg);/*  执行同步调用                */
        }
        
        pfuncAsync = pipim->IPIM_pfuncAsyncCall;
        pvAsync    = pipim->IPIM_pvAsyncArg;
        
        KN_SMP_MB();
        pipim->IPIM_iWait = 0;                                          /*  调用结束                    */
        KN_SMP_WMB();
        LW_SPINLOCK_NOTIFY();
        
        if (pfuncAsync) {
            pfuncAsync(pvAsync);                                        /*  执行异步调用                */
        }
    }
    
    KN_SMP_MB();
    if (pcpuCur->CPU_pringMsg == LW_NULL) {
        LW_CPU_CLR_IPI_PEND2(pcpuCur, LW_IPI_CALL_MSK);                 /*  清除                        */
    }
    
    LW_SPIN_UNLOCK_IGNIRQ(&pcpuCur->CPU_slIpi);                         /*  解锁 CPU                    */
    
    LW_SPINLOCK_NOTIFY();
}
/*********************************************************************************************************
** 函数名称: _SmpProcCallfunc
** 功能描述: 处理核间中断调用函数
** 输　入  : pcpuCur       当前 CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _SmpProcCallfunc (PLW_CLASS_CPU  pcpuCur)
{
    INTREG  iregInterLevel;
    
    iregInterLevel = KN_INT_DISABLE();
    __smpProcCallfunc(pcpuCur);
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _SmpProcCallfuncIgnIrq
** 功能描述: 处理核间中断调用函数 (已经关闭中断)
** 输　入  : pcpuCur       当前 CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _SmpProcCallfuncIgnIrq (PLW_CLASS_CPU  pcpuCur)
{
    __smpProcCallfunc(pcpuCur);
}
/*********************************************************************************************************
** 函数名称: _SmpProcIpi
** 功能描述: 处理核间中断 (这里不处理调度器消息)
** 输　入  : pcpuCur       当前 CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpProcIpi (PLW_CLASS_CPU  pcpuCur)
{
    pcpuCur->CPU_iIPICnt++;                                             /*  核间中断数量 ++             */

    if (LW_CPU_GET_IPI_PEND2(pcpuCur) & LW_IPI_CALL_MSK) {              /*  自定义调用 ?                */
        _SmpProcCallfunc(pcpuCur);
    }
}
/*********************************************************************************************************
** 函数名称: _SmpTryProcIpi
** 功能描述: 尝试处理核间中断 (必须在关中断情况下调用, 这里仅仅尝试执行 call 函数)
** 输　入  : pcpuCur       当前 CPU
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpTryProcIpi (PLW_CLASS_CPU  pcpuCur)
{
    if (LW_CPU_GET_IPI_PEND2(pcpuCur) & LW_IPI_CALL_MSK) {              /*  自定义调用 ?                */
        _SmpProcCallfuncIgnIrq(pcpuCur);
    }
}
/*********************************************************************************************************
** 函数名称: _SmpUpdateIpi
** 功能描述: 产生一个 IPI
** 输　入  : pcpuCur   CPU 控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _SmpUpdateIpi (PLW_CLASS_CPU  pcpu)
{
    if (!LW_CPU_IS_ACTIVE(pcpu)) {                                      /*  CPU 必须被激活              */
        return;
    }

    archMpInt(pcpu->CPU_ulCPUId);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
