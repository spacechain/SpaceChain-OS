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
** 文   件   名: _JobQueue.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 01 日
**
** 描        述: 这是系统异步工作队列.
**
** BUG:
2018.01.05  _jobQueueDel() 如果 pfunc 参数为 NULL 则表示任意函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _JobQueueCreate
** 功能描述: 创建一个工作队列
** 输　入  : uiQueueSize       队列大小
**           bNonBlock         执行函数是否为非阻塞方式
** 输　出  : 工作队列控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_JOB_QUEUE  _jobQueueCreate (UINT uiQueueSize, BOOL bNonBlock)
{
    PLW_JOB_QUEUE pjobq;
    
    pjobq = (PLW_JOB_QUEUE)__KHEAP_ALLOC((size_t)(sizeof(LW_JOB_QUEUE) + 
                                         (uiQueueSize * sizeof(LW_JOB_MSG))));
    if (pjobq == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pjobq->JOBQ_pjobmsgQueue = (PLW_JOB_MSG)(pjobq + 1);
    pjobq->JOBQ_uiIn         = 0;
    pjobq->JOBQ_uiOut        = 0;
    pjobq->JOBQ_uiCnt        = 0;
    pjobq->JOBQ_uiSize       = uiQueueSize;
    pjobq->JOBQ_stLost       = 0;
    
    if (bNonBlock == LW_FALSE) {
        pjobq->JOBQ_ulSync = API_SemaphoreBCreate("job_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pjobq->JOBQ_ulSync == LW_OBJECT_HANDLE_INVALID) {
            __KHEAP_FREE(pjobq);
            return  (LW_NULL);
        }
    } else {
        pjobq->JOBQ_ulSync = LW_OBJECT_HANDLE_INVALID;
    }
    
    LW_SPIN_INIT(&pjobq->JOBQ_slLock);
    
    return  (pjobq);
}
/*********************************************************************************************************
** 函数名称: _JobQueueDelete
** 功能描述: 删除一个工作队列
** 输　入  : pjobq         工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _jobQueueDelete (PLW_JOB_QUEUE pjobq)
{
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBDelete(&pjobq->JOBQ_ulSync);
    }
    
    __KHEAP_FREE(pjobq);
}
/*********************************************************************************************************
** 函数名称: _JobQueueInit
** 功能描述: 初始化一个工作队列 (静态创建)
** 输　入  : pjobq             需要初始化的工作队列控制块
**           pjobmsg           消息缓冲区
**           uiQueueSize       队列大小
**           bNonBlock         执行函数是否为非阻塞方式
** 输　出  : 工作队列控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _jobQueueInit (PLW_JOB_QUEUE pjobq, PLW_JOB_MSG  pjobmsg, UINT uiQueueSize, BOOL bNonBlock)
{
    pjobq->JOBQ_pjobmsgQueue = pjobmsg;
    pjobq->JOBQ_uiIn         = 0;
    pjobq->JOBQ_uiOut        = 0;
    pjobq->JOBQ_uiCnt        = 0;
    pjobq->JOBQ_uiSize       = uiQueueSize;
    pjobq->JOBQ_stLost       = 0;
    
    if (bNonBlock == LW_FALSE) {
        pjobq->JOBQ_ulSync = API_SemaphoreBCreate("job_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pjobq->JOBQ_ulSync == LW_OBJECT_HANDLE_INVALID) {
            return  (errno);
        }
    }
    
    LW_SPIN_INIT(&pjobq->JOBQ_slLock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _jobQueueFinit
** 功能描述: 销毁一个工作队列 (静态销毁)
** 输　入  : pjobq         工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _jobQueueFinit (PLW_JOB_QUEUE pjobq)
{
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBDelete(&pjobq->JOBQ_ulSync);
    }
}
/*********************************************************************************************************
** 函数名称: _jobQueueFlush
** 功能描述: 清空工作队列
** 输　入  : pjobq         工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _jobQueueFlush (PLW_JOB_QUEUE pjobq)
{
    INTREG   iregInterLevel;
    
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBClear(pjobq->JOBQ_ulSync);
    }
    
    LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    
    pjobq->JOBQ_uiIn  = 0;
    pjobq->JOBQ_uiOut = 0;
    pjobq->JOBQ_uiCnt = 0;
    
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _JobQueueAdd
** 功能描述: 添加一个工作到工作队列
** 输　入  : pjobq         工作队列控制块
**           pfunc         要执行的函数
**           pvArg0 ~ 5    函数参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _jobQueueAdd (PLW_JOB_QUEUE pjobq,
                     VOIDFUNCPTR   pfunc,
                     PVOID         pvArg0,
                     PVOID         pvArg1,
                     PVOID         pvArg2,
                     PVOID         pvArg3,
                     PVOID         pvArg4,
                     PVOID         pvArg5)
{
    INTREG           iregInterLevel;
    PLW_JOB_MSG      pjobmsg;
    
    LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    if (pjobq->JOBQ_uiCnt == pjobq->JOBQ_uiSize) {
        pjobq->JOBQ_stLost++;
        LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "job message lost.\r\n");
        return  (ENOSPC);
    }
    
    pjobmsg                 = &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiIn];
    pjobmsg->JOBM_pfuncFunc = pfunc;
    pjobmsg->JOBM_pvArg[0]  = pvArg0;
    pjobmsg->JOBM_pvArg[1]  = pvArg1;
    pjobmsg->JOBM_pvArg[2]  = pvArg2;
    pjobmsg->JOBM_pvArg[3]  = pvArg3;
    pjobmsg->JOBM_pvArg[4]  = pvArg4;
    pjobmsg->JOBM_pvArg[5]  = pvArg5;
    
    if (pjobq->JOBQ_uiIn == (pjobq->JOBQ_uiSize - 1)) {
        pjobq->JOBQ_uiIn =  0;
    } else {
        pjobq->JOBQ_uiIn++;
    }
    
    pjobq->JOBQ_uiCnt++;
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
    
    if (pjobq->JOBQ_ulSync) {
        API_SemaphoreBPost(pjobq->JOBQ_ulSync);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _JobQueueDel
** 功能描述: 从工作队列中删除一个工作 (懒惰删除)
** 输　入  : pjobq         工作队列控制块
**           uiMatchArgNum 匹配参数的个数
**           pfunc         要删除的函数
**           pvArg0 ~ 5    函数参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _jobQueueDel (PLW_JOB_QUEUE pjobq,
                    UINT          uiMatchArgNum,
                    VOIDFUNCPTR   pfunc, 
                    PVOID         pvArg0,
                    PVOID         pvArg1,
                    PVOID         pvArg2,
                    PVOID         pvArg3,
                    PVOID         pvArg4,
                    PVOID         pvArg5)
{
    INTREG          iregInterLevel;
    UINT            i = 0;
    PLW_JOB_MSG     pjobmsg;
    
    LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    pjobmsg = &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiOut];
    for (i = 0; i < pjobq->JOBQ_uiCnt; i++) {
        switch (uiMatchArgNum) {
        
        case 0:
            if (pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc) {
                pjobmsg->JOBM_pfuncFunc =  LW_NULL;
            }
            break;
            
        case 1:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc) &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 2:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 3:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 4:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2) &&
                (pjobmsg->JOBM_pvArg[3]  == pvArg3)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 5:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2) &&
                (pjobmsg->JOBM_pvArg[3]  == pvArg3) &&
                (pjobmsg->JOBM_pvArg[4]  == pvArg4)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
            
        case 6:
            if ((pjobmsg->JOBM_pfuncFunc == pfunc || !pfunc)  &&
                (pjobmsg->JOBM_pvArg[0]  == pvArg0) &&
                (pjobmsg->JOBM_pvArg[1]  == pvArg1) &&
                (pjobmsg->JOBM_pvArg[2]  == pvArg2) &&
                (pjobmsg->JOBM_pvArg[3]  == pvArg3) &&
                (pjobmsg->JOBM_pvArg[4]  == pvArg4) &&
                (pjobmsg->JOBM_pvArg[5]  == pvArg5)) {
                pjobmsg->JOBM_pfuncFunc  =  LW_NULL;
            }
            break;
        }
        pjobmsg++;
        if (pjobmsg > &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiSize - 1]) {
            pjobmsg = &pjobq->JOBQ_pjobmsgQueue[0];
        }
    }
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _jobQueueLost
** 功能描述: 获得工作队列丢失信息数量
** 输　入  : pjobq         工作队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
size_t  _jobQueueLost (PLW_JOB_QUEUE pjobq)
{
    return  (pjobq->JOBQ_stLost);
}
/*********************************************************************************************************
** 函数名称: _jobQueueExec
** 功能描述: 执行工作队列中的工作
** 输　入  : pjobq         工作队列控制块
**           ulTimeout     等待超时时间
** 输　出  : 不为 ERROR_NONE 表示超时
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _jobQueueExec (PLW_JOB_QUEUE pjobq, ULONG  ulTimeout)
{
    INTREG          iregInterLevel;
    PLW_JOB_MSG     pjobmsg;
    LW_JOB_MSG      jobmsgRun;
    
    if (pjobq->JOBQ_ulSync) {
        for (;;) {
            if (API_SemaphoreBPend(pjobq->JOBQ_ulSync, ulTimeout)) {
                return  (ERROR_THREAD_WAIT_TIMEOUT);
            }
            LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
            if (pjobq->JOBQ_uiCnt) {
                break;
            }
            LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
        }
    } else {
        LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    }
    
    while (pjobq->JOBQ_uiCnt) {
        pjobmsg   = &pjobq->JOBQ_pjobmsgQueue[pjobq->JOBQ_uiOut];
        jobmsgRun = *pjobmsg;
        if (pjobq->JOBQ_uiOut == (pjobq->JOBQ_uiSize - 1)) {
            pjobq->JOBQ_uiOut =  0;
        } else {
            pjobq->JOBQ_uiOut++;
        }
        pjobq->JOBQ_uiCnt--;
        LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
        
        if (jobmsgRun.JOBM_pfuncFunc) {
            jobmsgRun.JOBM_pfuncFunc(jobmsgRun.JOBM_pvArg[0],
                                     jobmsgRun.JOBM_pvArg[1],
                                     jobmsgRun.JOBM_pvArg[2],
                                     jobmsgRun.JOBM_pvArg[3],
                                     jobmsgRun.JOBM_pvArg[4],
                                     jobmsgRun.JOBM_pvArg[5]);
        }
        
        LW_SPIN_LOCK_QUICK(&pjobq->JOBQ_slLock, &iregInterLevel);
    }
    LW_SPIN_UNLOCK_QUICK(&pjobq->JOBQ_slLock, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
