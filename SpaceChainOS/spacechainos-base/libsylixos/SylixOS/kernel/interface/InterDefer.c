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
** 文   件   名: InterDefer.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 05 月 09 日
**
** 描        述: 中断延迟队列处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪配置
*********************************************************************************************************/
#if LW_CFG_ISR_DEFER_EN > 0
/*********************************************************************************************************
  每一个 CPU 的 DEFER ISR QUEUE
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ISR_DEFER_PER_CPU > 0)
static LW_JOB_QUEUE     _K_jobqIsrDefer[LW_CFG_MAX_PROCESSORS];
static LW_JOB_MSG       _K_jobmsgIsrDefer[LW_CFG_MAX_PROCESSORS][LW_CFG_ISR_DEFER_SIZE];
#else
static LW_JOB_QUEUE     _K_jobqIsrDefer[1];
static LW_JOB_MSG       _K_jobmsgIsrDefer[LW_CFG_ISR_DEFER_SIZE];
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: __interDeferTask
** 功能描述: 获得中断堆栈使用量
** 输　入  : ulCPUId                       CPU 号
**           pstFreeByteSize               空闲堆栈大小   (可为 LW_NULL)
**           pstUsedByteSize               使用堆栈大小   (可为 LW_NULL)
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  _interDeferTask (PVOID  pvArg)
{
    PLW_JOB_QUEUE   pjobq = (PLW_JOB_QUEUE)pvArg;
    
    for (;;) {
        _jobQueueExec(pjobq, LW_OPTION_WAIT_INFINITE);
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _interDeferInit
** 功能描述: 初始化中断延迟处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _interDeferInit (VOID)
{
    CHAR                  cDefer[LW_CFG_OBJECT_NAME_SIZE] = "t_isrdefer";
    LW_CLASS_THREADATTR   threadattr;
    LW_OBJECT_HANDLE      ulId;
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ISR_DEFER_PER_CPU > 0)
    INT                   i;
    LW_CLASS_CPUSET       cpuset;
    
    LW_CPU_ZERO(&cpuset);
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_DEFER_STK_SIZE, 
                        LW_CFG_ISR_DEFER_PRIO, 
                        (LW_OPTION_THREAD_STK_CHK | 
                        LW_OPTION_THREAD_SAFE | 
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_AFFINITY_ALWAYS), 
                        LW_NULL);
    
    LW_CPU_FOREACH (i) {
        if (_jobQueueInit(&_K_jobqIsrDefer[i], 
                          &_K_jobmsgIsrDefer[i][0], 
                          LW_CFG_ISR_DEFER_SIZE, LW_FALSE)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer queue.\r\n");
            return;
        }
        
        lib_itoa(i, &cDefer[10], 10);
        API_ThreadAttrSetArg(&threadattr, &_K_jobqIsrDefer[i]);
        ulId = API_ThreadInit(cDefer, _interDeferTask, &threadattr, LW_NULL);
        if (ulId == LW_OBJECT_HANDLE_INVALID) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer task.\r\n");
            return;
        }
        
        LW_CPU_SET(i, &cpuset);
        API_ThreadSetAffinity(ulId, sizeof(LW_CLASS_CPUSET), &cpuset);  /*  锁定到指定 CPU              */
        LW_CPU_CLR(i, &cpuset);
        
        API_ThreadStart(ulId);
    }
    
#else
    if (_jobQueueInit(&_K_jobqIsrDefer[0], 
                      &_K_jobmsgIsrDefer[0], 
                      LW_CFG_ISR_DEFER_SIZE, LW_FALSE)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer queue.\r\n");
        return;
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_DEFER_STK_SIZE, 
                        LW_CFG_ISR_DEFER_PRIO, 
                        (LW_OPTION_THREAD_STK_CHK | 
                        LW_OPTION_THREAD_SAFE | 
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_AFFINITY_ALWAYS), 
                        &_K_jobqIsrDefer[0]);
    
    ulId = API_ThreadInit(cDefer, _interDeferTask, &threadattr, LW_NULL);
    if (ulId == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create ISR defer task.\r\n");
        return;
    }
    
    API_ThreadStart(ulId);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */ 
}                                                                       /*  LW_CFG_ISR_DEFER_PER_CPU    */
/*********************************************************************************************************
** 函数名称: API_InterDeferGet
** 功能描述: 获得对应 CPU 的中断延迟队列
** 输　入  : ulCPUId       CPU 号
** 输　出  : 中断延迟队列
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_JOB_QUEUE  API_InterDeferGet (ULONG  ulCPUId)
{
    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(ERANGE);
        return  (LW_NULL);
    }
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ISR_DEFER_PER_CPU > 0)
    return  (&_K_jobqIsrDefer[ulCPUId]);
#else
    return  (&_K_jobqIsrDefer[0]);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */ 
}                                                                       /*  LW_CFG_ISR_DEFER_PER_CPU    */
/*********************************************************************************************************
** 函数名称: API_InterDeferJobAdd
** 功能描述: 向中断延迟处理队列加入一个任务
** 输　入  : pjobq         队列
**           pfunc         处理函数
**           pvArg         处理参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_InterDeferJobAdd (PLW_JOB_QUEUE  pjobq, VOIDFUNCPTR  pfunc, PVOID  pvArg)
{
    if (!pjobq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    return  (_jobQueueAdd(pjobq, pfunc, pvArg, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: API_InterDeferJobDelete
** 功能描述: 从中断延迟处理队列删除任务
** 输　入  : pjobq         队列
**           bMatchArg     是否进行参数匹配判断
**           pfunc         处理函数
**           pvArg         处理参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_InterDeferJobDelete (PLW_JOB_QUEUE  pjobq, BOOL  bMatchArg, VOIDFUNCPTR  pfunc, PVOID  pvArg)
{
    if (!pjobq) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    _jobQueueDel(pjobq, (bMatchArg) ? 1 : 0,
                 pfunc, pvArg, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_ISR_DEFER_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
