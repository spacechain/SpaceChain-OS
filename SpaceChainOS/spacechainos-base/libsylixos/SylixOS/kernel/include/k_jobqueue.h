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
** 文   件   名: k_jobqueue.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 01 日
**
** 描        述: 内核工作队列模型.
*********************************************************************************************************/

#ifndef __K_JOBQUEUE_H
#define __K_JOBQUEUE_H

/*********************************************************************************************************
  工作队列一般参数
*********************************************************************************************************/

#define LW_JOB_ARGS     6

/*********************************************************************************************************
  工作队列结构
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR             JOBM_pfuncFunc;                             /*  回调函数                    */
    PVOID                   JOBM_pvArg[LW_JOB_ARGS];                    /*  回调参数                    */
} LW_JOB_MSG;
typedef LW_JOB_MSG         *PLW_JOB_MSG;

typedef struct {
    PLW_JOB_MSG             JOBQ_pjobmsgQueue;                          /*  工作队列消息                */
    UINT                    JOBQ_uiIn;
    UINT                    JOBQ_uiOut;
    UINT                    JOBQ_uiCnt;
    UINT                    JOBQ_uiSize;
    size_t                  JOBQ_stLost;                                /*  丢失信息数量                */
    LW_OBJECT_HANDLE        JOBQ_ulSync;                                /*  同步等待                    */
    LW_SPINLOCK_DEFINE     (JOBQ_slLock);
} LW_JOB_QUEUE;
typedef LW_JOB_QUEUE       *PLW_JOB_QUEUE;

/*********************************************************************************************************
  工作队列 NOP 
*********************************************************************************************************/

#define LW_JOB_NOPFUNC      ((VOIDFUNCPTR)0)

/*********************************************************************************************************
  操作函数
*********************************************************************************************************/

PLW_JOB_QUEUE    _jobQueueCreate(UINT  uiQueueSize, BOOL  bNonBlock);
VOID             _jobQueueDelete(PLW_JOB_QUEUE pjobq);
ULONG            _jobQueueInit(PLW_JOB_QUEUE pjobq, PLW_JOB_MSG  pjobmsg, 
                               UINT uiQueueSize, BOOL bNonBlock);
VOID             _jobQueueFinit(PLW_JOB_QUEUE pjobq);
VOID             _jobQueueFlush(PLW_JOB_QUEUE pjobq);
ULONG            _jobQueueAdd(PLW_JOB_QUEUE pjobq,
                              VOIDFUNCPTR   pfunc,
                              PVOID         pvArg0,
                              PVOID         pvArg1,
                              PVOID         pvArg2,
                              PVOID         pvArg3,
                              PVOID         pvArg4,
                              PVOID         pvArg5);
VOID             _jobQueueDel(PLW_JOB_QUEUE pjobq,
                              UINT          uiMatchArgNum,
                              VOIDFUNCPTR   pfunc, 
                              PVOID         pvArg0,
                              PVOID         pvArg1,
                              PVOID         pvArg2,
                              PVOID         pvArg3,
                              PVOID         pvArg4,
                              PVOID         pvArg5);
size_t           _jobQueueLost(PLW_JOB_QUEUE pjobq);
ULONG            _jobQueueExec(PLW_JOB_QUEUE pjobq, ULONG  ulTimeout);

#endif                                                                  /*  __K_JOBQUEUE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
