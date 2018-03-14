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
** 文   件   名: lwip_jobqueue.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 03 月 30 日
**
** 描        述: 网络异步处理工作队列. (驱动程序应该把主要的处理都放在 job queue 中, 
                                        数据包内存处理函数不能在中断上下文中处理.)

** BUG:
2009.05.20  netjob 线程应该具有安全属性.
2009.12.09  修改注释.
2013.12.01  不再使用消息队列, 使用内核提供的工作队列模型.
2016.11.04  支持多条并行处理队列.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/tcpip.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_LWIP_JOBQUEUE_NUM != 1) && \
    ((LW_CFG_LWIP_JOBQUEUE_NUM & (LW_CFG_LWIP_JOBQUEUE_NUM - 1)) != 0)
#error "N must be equal to 1 or Pow of 2!"
#endif
/*********************************************************************************************************
  网络工作队列
*********************************************************************************************************/
static LW_JOB_QUEUE         _G_jobqNet[LW_CFG_LWIP_JOBQUEUE_NUM];
static LW_JOB_MSG           _G_jobmsgNet[LW_CFG_LWIP_JOBQUEUE_NUM][LW_CFG_LWIP_JOBQUEUE_SIZE];
static UINT                 _G_uiJobqNum;
/*********************************************************************************************************
  INTERNAL FUNC
*********************************************************************************************************/
static VOID    _NetJobThread(PLW_JOB_QUEUE  pjobq);                     /*  作业处理程序                */
/*********************************************************************************************************
** 函数名称: _netJobqueueInit
** 功能描述: 初始化 Net jobqueue 处理 机制
** 输　入  : NONE
** 输　出  : 是否初始化成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _netJobqueueInit (VOID)
{
    INT                 i;
    LW_OBJECT_HANDLE    hNetJobThread[LW_CFG_LWIP_JOBQUEUE_NUM];
    LW_CLASS_THREADATTR threadattr;
    
    _G_uiJobqNum = LW_CFG_LWIP_JOBQUEUE_NUM;
    
    while (_G_uiJobqNum > LW_NCPUS) {
        _G_uiJobqNum >>= 1;
    }
    
    for (i = 0; i < _G_uiJobqNum; i++) {
        if (_jobQueueInit(&_G_jobqNet[i], &_G_jobmsgNet[i][0], 
                          LW_CFG_LWIP_JOBQUEUE_SIZE, LW_FALSE)) {
            break;
        }
    }
    if (i < _G_uiJobqNum) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create netjob queue.\r\n");
        for (; i >= 0; i--) {
            _jobQueueFinit(&_G_jobqNet[i]);
        }
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, LW_CFG_LWIP_JOBQUEUE_STK_SIZE, 
                        LW_PRIO_T_NETJOB, 
                        (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL),
                        LW_NULL);
    
    for (i = 0; i < _G_uiJobqNum; i++) {
        threadattr.THREADATTR_pvArg = (PVOID)&_G_jobqNet[i];
        hNetJobThread[i] = API_ThreadCreate("t_netjob",
                                            (PTHREAD_START_ROUTINE)_NetJobThread,
                                            (PLW_CLASS_THREADATTR)&threadattr,
                                            LW_NULL);                   /*  建立 job 处理线程           */
        if (hNetJobThread[i] == LW_OBJECT_HANDLE_INVALID) {
            break;
        }
    }
    if (i < _G_uiJobqNum) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create netjob task.\r\n");
        for (; i >= 0; i--) {
            API_ThreadDelete(&hNetJobThread[i], LW_NULL);
        }
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NetJobAdd
** 功能描述: 加入网络异步处理作业队列
** 输　入  : pfunc                      函数指针
**           pvArg0                     函数参数
**           pvArg1                     函数参数
**           pvArg2                     函数参数
**           pvArg3                     函数参数
**           pvArg4                     函数参数
**           pvArg5                     函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_NetJobAdd (VOIDFUNCPTR  pfunc, 
                    PVOID        pvArg0,
                    PVOID        pvArg1,
                    PVOID        pvArg2,
                    PVOID        pvArg3,
                    PVOID        pvArg4,
                    PVOID        pvArg5)
{
    if (!pfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_jobQueueAdd(&_G_jobqNet[0], pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NetJobAddEx
** 功能描述: 加入网络异步处理作业队列
** 输　入  : uiQ                        队列识别号
**           pfunc                      函数指针
**           pvArg0                     函数参数
**           pvArg1                     函数参数
**           pvArg2                     函数参数
**           pvArg3                     函数参数
**           pvArg4                     函数参数
**           pvArg5                     函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_NetJobAddEx (UINT         uiQ,
                      VOIDFUNCPTR  pfunc, 
                      PVOID        pvArg0,
                      PVOID        pvArg1,
                      PVOID        pvArg2,
                      PVOID        pvArg3,
                      PVOID        pvArg4,
                      PVOID        pvArg5)
{
    if (!pfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_jobQueueAdd(&_G_jobqNet[uiQ & (_G_uiJobqNum - 1)], 
                     pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NetJobDelete
** 功能描述: 从网络异步处理作业队列中删除
** 输　入  : uiMatchArgNum              匹配参数的个数
**           pfunc                      函数指针
**           pvArg0                     函数参数
**           pvArg1                     函数参数
**           pvArg2                     函数参数
**           pvArg3                     函数参数
**           pvArg4                     函数参数
**           pvArg5                     函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_NetJobDelete (UINT         uiMatchArgNum,
                        VOIDFUNCPTR  pfunc, 
                        PVOID        pvArg0,
                        PVOID        pvArg1,
                        PVOID        pvArg2,
                        PVOID        pvArg3,
                        PVOID        pvArg4,
                        PVOID        pvArg5)
{
    if (!pfunc) {
        return;
    }
    
    _jobQueueDel(&_G_jobqNet[0], uiMatchArgNum, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
}
/*********************************************************************************************************
** 函数名称: API_NetJobDeleteEx
** 功能描述: 从网络异步处理作业队列中删除
** 输　入  : uiQ                        队列识别号
**           uiMatchArgNum              匹配参数的个数
**           pfunc                      函数指针
**           pvArg0                     函数参数
**           pvArg1                     函数参数
**           pvArg2                     函数参数
**           pvArg3                     函数参数
**           pvArg4                     函数参数
**           pvArg5                     函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_NetJobDeleteEx (UINT         uiQ,
                          UINT         uiMatchArgNum,
                          VOIDFUNCPTR  pfunc, 
                          PVOID        pvArg0,
                          PVOID        pvArg1,
                          PVOID        pvArg2,
                          PVOID        pvArg3,
                          PVOID        pvArg4,
                          PVOID        pvArg5)
{
    UINT  i;

    if (!pfunc) {
        return;
    }
    
    if (uiQ == LW_NETJOB_Q_ALL) {
        for (i = 0; i < _G_uiJobqNum; i++) {
            _jobQueueDel(&_G_jobqNet[i], uiMatchArgNum, 
                         pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
        }
    
    } else {
        _jobQueueDel(&_G_jobqNet[uiQ & (_G_uiJobqNum - 1)], uiMatchArgNum, 
                     pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
}
/*********************************************************************************************************
** 函数名称: _NetJobThread
** 功能描述: 网络工作队列处理线程
** 输　入  : pjobq     网络队列
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _NetJobThread (PLW_JOB_QUEUE  pjobq)
{
    for (;;) {
        _jobQueueExec(pjobq, LW_OPTION_WAIT_INFINITE);
    }
}
/*********************************************************************************************************
** 函数名称: API_NetJobGetLost
** 功能描述: 获得网络消息丢失的数量
** 输　入  : NONE
** 输　出  : 消息丢失的数量
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
size_t  API_NetJobGetLost (VOID)
{
    INT     i;
    size_t  stTotal = 0;
    
    for (i = 0; i < _G_uiJobqNum; i++) {
        stTotal += _jobQueueLost(&_G_jobqNet[i]);
    }
    
    return  (stTotal);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
