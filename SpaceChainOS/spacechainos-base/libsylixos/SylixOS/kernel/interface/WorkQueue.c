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
** 文   件   名: WorkQueue.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 09 月 07 日
**
** 描        述: 内核工作队列.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_WORKQUEUE_EN > 0
/*********************************************************************************************************
  简单作业队列控制结构
*********************************************************************************************************/
typedef struct {
    PLW_JOB_QUEUE        SWQ_pjobQ;                                     /*  非延迟作业队列              */
} LW_WORK_SQUEUE;
/*********************************************************************************************************
  带有延迟属性作业队列控制结构
*********************************************************************************************************/
typedef struct {
    LW_LIST_MONO         DWQN_monoList;
    LW_CLASS_WAKEUP_NODE DWQN_wun;
    VOIDFUNCPTR          DWQN_pfunc;
    PVOID                DWQN_pvArg[LW_JOB_ARGS];
} LW_WORK_DNODE;

typedef struct {
    LW_CLASS_WAKEUP      DWQ_wakeup;                                    /*  延迟作业队列                */
    PLW_LIST_MONO        DWQ_pmonoPool;                                 /*  节点池                      */
    LW_WORK_DNODE       *DWQ_pwdnPool;
    
    LW_OBJECT_HANDLE     DWQ_ulLock;                                    /*  操作锁                      */
    LW_OBJECT_HANDLE     DWQ_ulSem;                                     /*  通知信号量                  */
    
    UINT                 DWQ_uiCount;                                   /*  等待的作业数量              */
    UINT64               DWQ_u64Time;                                   /*  最后一次执行时间点          */
    ULONG                DWQ_ulScanPeriod;                              /*  循环扫描周期                */
} LW_WORK_DQUEUE;
/*********************************************************************************************************
  作业队列控制结构
*********************************************************************************************************/
typedef struct {
    union {
        LW_WORK_SQUEUE   WQ_sq;
        LW_WORK_DQUEUE   WQ_dq;
    } q;
    
#define LW_WQ_TYPE_S     0                                              /*  简单任务队列                */
#define LW_WQ_TYPE_D     1                                              /*  带有延迟属性的任务队列      */
    INT                  WQ_iType;                                      /*  队列类型                    */
    LW_OBJECT_HANDLE     WQ_ulTask;                                     /*  服务线程                    */
    BOOL                 WQ_bDelReq;
} LW_WORK_QUEUE;
typedef LW_WORK_QUEUE   *PLW_WORK_QUEUE;
/*********************************************************************************************************
** 函数名称: __wqSCreate
** 功能描述: 创建一个简单工作队列
** 输　入  : pwq           工作队列控制块
**           uiQSize       队列大小
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_WORK_QUEUE  __wqSCreate (PLW_WORK_QUEUE  pwq, UINT  uiQSize)
{
    pwq->WQ_iType = LW_WQ_TYPE_S;
    pwq->q.WQ_sq.SWQ_pjobQ = _jobQueueCreate(uiQSize, LW_FALSE);
    if (pwq->q.WQ_sq.SWQ_pjobQ == LW_NULL) {
        return  (LW_NULL);
    }
    
    return  (pwq);
}
/*********************************************************************************************************
** 函数名称: __wqDCreate
** 功能描述: 创建一个带有延迟功能的工作队列
** 输　入  : pwq           工作队列控制块
**           uiQSize       队列大小
**           ulScanPeriod  服务线程扫描周期
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_WORK_QUEUE  __wqDCreate (PLW_WORK_QUEUE  pwq, UINT  uiQSize, ULONG  ulScanPeriod)
{
    UINT            i;
    LW_WORK_DNODE  *pwdn;
    
    pwq->WQ_iType = LW_WQ_TYPE_D;
    pwdn = (LW_WORK_DNODE *)__KHEAP_ALLOC(sizeof(LW_WORK_DNODE) * uiQSize);
    if (pwdn == LW_NULL) {
        return  (LW_NULL);
    }
    pwq->q.WQ_dq.DWQ_pwdnPool = pwdn;
    
    pwq->q.WQ_dq.DWQ_ulLock = API_SemaphoreMCreate("wqd_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY | 
                                           LW_OPTION_DELETE_SAFE | 
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pwq->q.WQ_dq.DWQ_ulLock == LW_OBJECT_HANDLE_INVALID) {
        __KHEAP_FREE(pwdn);
        return  (LW_NULL);
    }
    
    pwq->q.WQ_dq.DWQ_ulSem = API_SemaphoreBCreate("wqd_sem", LW_FALSE, 
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pwq->q.WQ_dq.DWQ_ulSem == LW_OBJECT_HANDLE_INVALID) {
        API_SemaphoreMDelete(&pwq->q.WQ_dq.DWQ_ulLock);
        __KHEAP_FREE(pwdn);
        return  (LW_NULL);
    }
    
    for (i = 0; i < uiQSize; i++) {
        _list_mono_free(&pwq->q.WQ_dq.DWQ_pmonoPool, &pwdn->DWQN_monoList);
        pwdn++;
    }
    
    pwq->q.WQ_dq.DWQ_u64Time      = API_TimeGet64();
    pwq->q.WQ_dq.DWQ_ulScanPeriod = ulScanPeriod;
    
    return  (pwq);
}
/*********************************************************************************************************
** 函数名称: __wqSDelete
** 功能描述: 删除一个简单工作队列
** 输　入  : pwq           工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqSDelete (PLW_WORK_QUEUE  pwq)
{
    _jobQueueDelete(pwq->q.WQ_sq.SWQ_pjobQ);
}
/*********************************************************************************************************
** 函数名称: __wqDDelete
** 功能描述: 删除一个带有延迟功能的工作队列
** 输　入  : pwq           工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqDDelete (PLW_WORK_QUEUE  pwq)
{
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    API_SemaphoreMDelete(&pwq->q.WQ_dq.DWQ_ulLock);
    API_SemaphoreBDelete(&pwq->q.WQ_dq.DWQ_ulSem);
    
    __KHEAP_FREE(pwq->q.WQ_dq.DWQ_pwdnPool);
}
/*********************************************************************************************************
** 函数名称: __wqSFlush
** 功能描述: 清空一个简单工作队列
** 输　入  : pwq           工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqSFlush (PLW_WORK_QUEUE  pwq)
{
    _jobQueueFlush(pwq->q.WQ_sq.SWQ_pjobQ);
}
/*********************************************************************************************************
** 函数名称: __wqDFlush
** 功能描述: 清空一个带有延迟功能的工作队列
** 输　入  : pwq           工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqDFlush (PLW_WORK_QUEUE  pwq)
{
    PLW_CLASS_WAKEUP_NODE   pwun;
    LW_WORK_DNODE          *pwdn;
    
    API_SemaphoreBClear(pwq->q.WQ_dq.DWQ_ulSem);
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    while (pwq->q.WQ_dq.DWQ_wakeup.WU_plineHeader) {
        pwun = _LIST_ENTRY(pwq->q.WQ_dq.DWQ_wakeup.WU_plineHeader, 
                           LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        pwdn = _LIST_ENTRY(pwun, LW_WORK_DNODE, DWQN_wun);
    
        _List_Line_Del(&pwun->WUN_lineManage, 
                       &pwq->q.WQ_dq.DWQ_wakeup.WU_plineHeader);
        _list_mono_free(&pwq->q.WQ_dq.DWQ_pmonoPool, &pwdn->DWQN_monoList);
    }
    
    pwq->q.WQ_dq.DWQ_uiCount = 0;
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
}
/*********************************************************************************************************
** 函数名称: __wqSInsert
** 功能描述: 将一个工作插入到工作队列
** 输　入  : pwq          工作队列控制块
**           pfunc        执行函数
**           pvArg0 ~ 5   执行参数
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __wqSInsert (PLW_WORK_QUEUE  pwq, 
                           VOIDFUNCPTR     pfunc, 
                           PVOID           pvArg0,
                           PVOID           pvArg1,
                           PVOID           pvArg2,
                           PVOID           pvArg3,
                           PVOID           pvArg4,
                           PVOID           pvArg5)
{
    return  (_jobQueueAdd(pwq->q.WQ_sq.SWQ_pjobQ,
                          pfunc, pvArg0, pvArg1, pvArg2,
                          pvArg3, pvArg4, pvArg5));
}
/*********************************************************************************************************
** 函数名称: __wqDInsert
** 功能描述: 将一个工作插入到工作队列
** 输　入  : pwq          工作队列控制块
**           ulDelay      最小延迟执行时间
**           pfunc        执行函数
**           pvArg0 ~ 5   执行参数
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __wqDInsert (PLW_WORK_QUEUE  pwq, 
                           ULONG           ulDelay,
                           VOIDFUNCPTR     pfunc, 
                           PVOID           pvArg0,
                           PVOID           pvArg1,
                           PVOID           pvArg2,
                           PVOID           pvArg3,
                           PVOID           pvArg4,
                           PVOID           pvArg5)
{
    PLW_LIST_MONO   pmonoWDN;
    LW_WORK_DNODE  *pwdn;
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    pmonoWDN = _list_mono_allocate(&pwq->q.WQ_dq.DWQ_pmonoPool);
    if (pmonoWDN == LW_NULL) {
        API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
        _ErrorHandle(ENOSPC);
        return  (ENOSPC);
    }
    
    pwdn = _LIST_ENTRY(pmonoWDN, LW_WORK_DNODE, DWQN_monoList);
    pwdn->DWQN_pfunc    = pfunc;
    pwdn->DWQN_pvArg[0] = pvArg0;
    pwdn->DWQN_pvArg[1] = pvArg1;
    pwdn->DWQN_pvArg[2] = pvArg2;
    pwdn->DWQN_pvArg[3] = pvArg3;
    pwdn->DWQN_pvArg[4] = pvArg4;
    pwdn->DWQN_pvArg[5] = pvArg5;
    
    pwdn->DWQN_wun.WUN_ulCounter = ulDelay;
    _WakeupAdd(&pwq->q.WQ_dq.DWQ_wakeup, &pwdn->DWQN_wun);
    pwq->q.WQ_dq.DWQ_uiCount++;
    
    API_SemaphoreBPost(pwq->q.WQ_dq.DWQ_ulSem);
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __wqSStatus
** 功能描述: 获取工作队列状态
** 输　入  : pwq          工作队列控制块
**           pulCount     当前队列中作业数量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqSStatus (PLW_WORK_QUEUE  pwq, UINT  *puiCount)
{
    *puiCount = pwq->q.WQ_sq.SWQ_pjobQ->JOBQ_uiCnt;
}
/*********************************************************************************************************
** 函数名称: __wqDStatus
** 功能描述: 获取工作队列状态
** 输　入  : pwq          工作队列控制块
**           pulCount     当前队列中作业数量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqDStatus (PLW_WORK_QUEUE  pwq, UINT  *puiCount)
{
    *puiCount = pwq->q.WQ_dq.DWQ_uiCount;
}
/*********************************************************************************************************
** 函数名称: __wqSExec
** 功能描述: 执行一次工作队列
** 输　入  : pwq          工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqSExec (PLW_WORK_QUEUE  pwq)
{
    _jobQueueExec(pwq->q.WQ_sq.SWQ_pjobQ, LW_OPTION_WAIT_INFINITE);
}
/*********************************************************************************************************
** 函数名称: __wqDExec
** 功能描述: 执行一次工作队列
** 输　入  : pwq          工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __wqDExec (PLW_WORK_QUEUE  pwq)
{
    UINT                   i;
    PLW_CLASS_WAKEUP_NODE  pwun;
    LW_WORK_DNODE         *pwdn;
    
    VOIDFUNCPTR            pfunc;
    PVOID                  pvArg[LW_JOB_ARGS];
    
    UINT64                 u64Now;
    ULONG                  ulCounter;

    API_SemaphoreBPend(pwq->q.WQ_dq.DWQ_ulSem, pwq->q.WQ_dq.DWQ_ulScanPeriod);
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    u64Now = API_TimeGet64();
    ulCounter = (ULONG)(u64Now - pwq->q.WQ_dq.DWQ_u64Time);
    pwq->q.WQ_dq.DWQ_u64Time = u64Now;
    
    __WAKEUP_PASS_FIRST(&pwq->q.WQ_dq.DWQ_wakeup, pwun, ulCounter);
    
    pwdn = _LIST_ENTRY(pwun, LW_WORK_DNODE, DWQN_wun);
    
    _WakeupDel(&pwq->q.WQ_dq.DWQ_wakeup, pwun);
    pwq->q.WQ_dq.DWQ_uiCount--;
    
    pfunc = pwdn->DWQN_pfunc;
    for (i = 0; i < LW_JOB_ARGS; i++) {
        pvArg[i] = pwdn->DWQN_pvArg[i];
    }
    
    _list_mono_free(&pwq->q.WQ_dq.DWQ_pmonoPool, &pwdn->DWQN_monoList);
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
    
    if (pfunc) {
        pfunc(pvArg[0], pvArg[1], pvArg[2], pvArg[3], pvArg[4], pvArg[5]);
    }
    
    API_SemaphoreMPend(pwq->q.WQ_dq.DWQ_ulLock, LW_OPTION_WAIT_INFINITE);
    
    __WAKEUP_PASS_SECOND();
    
    __WAKEUP_PASS_END();
    
    API_SemaphoreMPost(pwq->q.WQ_dq.DWQ_ulLock);
}
/*********************************************************************************************************
** 函数名称: __wqTask
** 功能描述: 工作队列服务线程
** 输　入  : pvArg        工作队列控制块
** 输　出  : NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __wqTask (PVOID  pvArg)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvArg;
    
    for (;;) {
        switch (pwq->WQ_iType) {
        
        case LW_WQ_TYPE_S:
            __wqSExec(pwq);
            break;
            
        case LW_WQ_TYPE_D:
            __wqDExec(pwq);
            break;
            
        default:
            break;
        }
        
        if (pwq->WQ_bDelReq) {
            break;
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_WorkQueueCreate
** 功能描述: 创建一个工作队列
** 输　入  : pcName            队列名称
**           uiQSize           队列大小
**           bDelayEn          是否创建带有延迟执行功能的工作队列
**           ulScanPeriod      如果带有延迟选项, 此参数指定服务线程扫描周期.
**           pthreadattr       队列服务线程选项
** 输　出  : 工作队列句柄
** 全局变量: 
** 调用模块: 
** 注  意  : 如果不需要延迟执行功能, 则系统自动创建为简单工作队列, 内存占用量小, 执行效率高.

                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_WorkQueueCreate (CPCHAR                  pcName,
                            UINT                    uiQSize, 
                            BOOL                    bDelayEn, 
                            ULONG                   ulScanPeriod, 
                            PLW_CLASS_THREADATTR    pthreadattr)
{
    PLW_WORK_QUEUE         pwq;
    LW_CLASS_THREADATTR    threadattr;
    
    if (!uiQSize || (bDelayEn && !ulScanPeriod)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pwq = (PLW_WORK_QUEUE)__KHEAP_ALLOC(sizeof(LW_WORK_QUEUE));
    if (pwq == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(pwq, sizeof(LW_WORK_QUEUE));
    
    if (bDelayEn) {
        if (__wqDCreate(pwq, uiQSize, ulScanPeriod) == LW_NULL) {
            __KHEAP_FREE(pwq);
            return  (LW_NULL);
        }
    
    } else {
        if (__wqSCreate(pwq, uiQSize) == LW_NULL) {
            __KHEAP_FREE(pwq);
            return  (LW_NULL);
        }
    }
    
    if (pthreadattr) {
        threadattr = *pthreadattr;
    
    } else {
        threadattr = API_ThreadAttrGetDefault();
    }
    
    threadattr.THREADATTR_ulOption |= LW_OPTION_OBJECT_GLOBAL;
    threadattr.THREADATTR_pvArg     = pwq;
    
    pwq->WQ_ulTask = API_ThreadCreate(pcName, __wqTask, &threadattr, LW_NULL);
    if (pwq->WQ_ulTask == LW_OBJECT_HANDLE_INVALID) {
        if (bDelayEn) {
            __wqDDelete(pwq);
        
        } else {
            __wqSDelete(pwq);
        }
        __KHEAP_FREE(pwq);
        return  (LW_NULL);
    }
    
    return  ((PVOID)pwq);
}
/*********************************************************************************************************
** 函数名称: API_WorkQueueDelete
** 功能描述: 删除一个工作队列
** 输　入  : pvWQ      工作队列句柄
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueDelete (PVOID  pvWQ)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    
    if (!pwq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    KN_SMP_MB();
    if (pwq->WQ_bDelReq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pwq->WQ_bDelReq = LW_TRUE;
    KN_SMP_MB();
    
    switch (pwq->WQ_iType) {
    
    case LW_WQ_TYPE_S:
        __wqSFlush(pwq);
        __wqSInsert(pwq, LW_NULL, 0, 0, 0, 0, 0, 0);
        API_ThreadJoin(pwq->WQ_ulTask, LW_NULL);
        __wqSDelete(pwq);
        break;
        
    case LW_WQ_TYPE_D:
        __wqDFlush(pwq);
        __wqDInsert(pwq, 0, LW_NULL, 0, 0, 0, 0, 0, 0);
        API_ThreadJoin(pwq->WQ_ulTask, LW_NULL);
        __wqDDelete(pwq);
        break;

    default:
        break;
    }
    
    __KHEAP_FREE(pwq);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_WorkQueueInsert
** 功能描述: 将一个工作插入到工作队列
** 输　入  : pvWQ         工作队列句柄
**           ulDelay      最小延迟执行时间
**           pfunc        执行函数
**           pvArg0 ~ 5   执行参数
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueInsert (PVOID           pvWQ, 
                            ULONG           ulDelay,
                            VOIDFUNCPTR     pfunc, 
                            PVOID           pvArg0,
                            PVOID           pvArg1,
                            PVOID           pvArg2,
                            PVOID           pvArg3,
                            PVOID           pvArg4,
                            PVOID           pvArg5)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    ULONG           ulRet;
    
    if (!pwq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    switch (pwq->WQ_iType) {
        
    case LW_WQ_TYPE_S:
        ulRet = __wqSInsert(pwq, pfunc, pvArg0, pvArg1, pvArg2,
                            pvArg3, pvArg4, pvArg5);
        break;
        
    case LW_WQ_TYPE_D:
        ulRet = __wqDInsert(pwq, ulDelay, pfunc, pvArg0, pvArg1, pvArg2,
                            pvArg3, pvArg4, pvArg5);
        break;
        
    default:
        _ErrorHandle(ENXIO);
        ulRet = ENXIO;
        break;
    }
    
    return  (ulRet);
}
/*********************************************************************************************************
** 函数名称: API_WorkQueueFlush
** 功能描述: 清空工作队列
** 输　入  : pvWQ         工作队列句柄
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueFlush (PVOID  pvWQ)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    
    if (!pwq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    switch (pwq->WQ_iType) {
        
    case LW_WQ_TYPE_S:
        __wqSFlush(pwq);
        break;
        
    case LW_WQ_TYPE_D:
        __wqDFlush(pwq);
        break;
        
    default:
        _ErrorHandle(ENXIO);
        return  (ENXIO);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_WorkQueueStatus
** 功能描述: 获取工作队列状态
** 输　入  : pvWQ         工作队列句柄
**           puiCount     当前队列中作业数量
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_WorkQueueStatus (PVOID  pvWQ, UINT  *puiCount)
{
    PLW_WORK_QUEUE  pwq = (PLW_WORK_QUEUE)pvWQ;
    
    if (!pwq || !puiCount) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    switch (pwq->WQ_iType) {
        
    case LW_WQ_TYPE_S:
        __wqSStatus(pwq, puiCount);
        break;
        
    case LW_WQ_TYPE_D:
        __wqDStatus(pwq, puiCount);
        break;
        
    default:
        _ErrorHandle(ENXIO);
        return  (ENXIO);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_WORKQUEUE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
