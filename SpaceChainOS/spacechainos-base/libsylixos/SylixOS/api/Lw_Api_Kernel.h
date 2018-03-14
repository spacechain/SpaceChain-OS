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
** 文   件   名: Lw_Api_Kernel.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统提供给 C / C++ 用户的内核应用程序接口层。
                 为了适应不同语言习惯的人，这里使用了很多重复功能.
*********************************************************************************************************/

#ifndef __LW_API_KERNEL_H
#define __LW_API_KERNEL_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/

#define Lw_Atomic_Add                           API_AtomicAdd
#define Lw_Atomic_Sub                           API_AtomicSub
#define Lw_Atomic_Inc                           API_AtomicInc
#define Lw_Atomic_Dec                           API_AtomicDec
#define Lw_Atomic_And                           API_AtomicAnd
#define Lw_Atomic_Nand                          API_AtomicNand
#define Lw_Atomic_Or                            API_AtomicOr
#define Lw_Atomic_Xor                           API_AtomicXor
#define Lw_Atomic_Set                           API_AtomicSet
#define Lw_Atomic_Get                           API_AtomicGet
#define Lw_Atomic_Swp                           API_AtomicSwp

/*********************************************************************************************************
  OBJECT
*********************************************************************************************************/

#define Lw_Object_GetClass                      API_ObjectGetClass
#define Lw_Object_GetNode                       API_ObjectGetNode
#define Lw_Object_GetIndex                      API_ObjectGetIndex
#define Lw_Object_IsGlobal                      API_ObjectIsGlobal
#define Lw_Object_ShareAdd                      API_ObjectShareAdd
#define Lw_Object_ShareDelete                   API_ObjectShareDelete
#define Lw_Object_ShareFind                     API_ObjectShareFind

/*********************************************************************************************************
  THREAD
*********************************************************************************************************/

#define Lw_Thread_SetAffinity                   API_ThreadSetAffinity
#define Lw_Thread_GetAffinity                   API_ThreadGetAffinity

#define Lw_ThreadAttr_GetDefault                API_ThreadAttrGetDefault
#define Lw_ThreadAttr_Get                       API_ThreadAttrGet

#define Lw_ThreadAttr_Build                     API_ThreadAttrBuild
#define Lw_ThreadAttr_BuildEx                   API_ThreadAttrBuildEx
#define Lw_ThreadAttr_BuildFP                   API_ThreadAttrBuildFP

#define Lw_ThreadAttr_SetGuardSize              API_ThreadAttrSetGuardSize
#define Lw_ThreadAttr_SetStackSize              API_ThreadAttrSetStackSize
#define Lw_ThreadAttr_SetArg                    API_ThreadAttrSetArg

#define Lw_Thread_Create                        API_ThreadCreate
#define Lw_Thread_Init                          API_ThreadInit
#define Lw_Thread_Self                          API_ThreadIdSelf
#define Lw_Thread_Tcb                           API_ThreadTcbSelf

#define Lw_Thread_Desc                          API_ThreadDesc

#define Lw_Thread_Exit                          API_ThreadExit
#define Lw_Thread_Delete                        API_ThreadDelete
#define Lw_Thread_ForceDelete                   API_ThreadForceDelete
#define Lw_Thread_Restart                       API_ThreadRestart
#define Lw_Thread_RestartEx                     API_ThreadRestartEx

#define Lw_Thread_SetCancelState                API_ThreadSetCancelState
#define Lw_Thread_SetCancelType                 API_ThreadSetCancelType

#define Lw_Thread_Cancel                        API_ThreadCancel
#define Lw_Thread_TestCancel                    API_ThreadTestCancel

#define Lw_Thread_Start                         API_ThreadStart
#define Lw_Thread_StartEx                       API_ThreadStartEx
#define Lw_Thread_Activate                      API_ThreadStart

#define Lw_Thread_Join                          API_ThreadJoin
#define Lw_Thread_Detach                        API_ThreadDetach

#define Lw_Thread_Safe                          API_ThreadSafe
#define Lw_Thread_Unsafe                        API_ThreadUnsafe
#define Lw_Thread_IsSafe                        API_ThreadIsSafe

#define Lw_Thread_Suspend                       API_ThreadSuspend
#define Lw_Thread_Resume                        API_ThreadResume
#define Lw_Thread_ForceResume                   API_ThreadForceResume

#define Lw_Thread_IsSuspend                     API_ThreadIsSuspend
#define Lw_Thread_IsReady                       API_ThreadIsReady
#define Lw_Thread_IsRunning                     API_ThreadIsRunning

#define Lw_Thread_SetName                       API_ThreadSetName
#define Lw_Thread_GetName                       API_ThreadGetName

#define Lw_Thread_Lock                          API_ThreadLock
#define Lw_Thread_Unlock                        API_ThreadUnlock

#define Lw_Thread_SetPriority                   API_ThreadSetPriority
#define Lw_Thread_GetPriority                   API_ThreadGetPriority

#define Lw_Thread_SetSlice                      API_ThreadSetSlice
#define Lw_Thread_GetSlice                      API_ThreadGetSlice
#define Lw_Thread_GetSliceEx                    API_ThreadGetSliceEx

#define Lw_Thread_SetSchedParam                 API_ThreadSetSchedParam
#define Lw_Thread_GetSchedParam                 API_ThreadGetSchedParam

#define Lw_Thread_SetNotePad                    API_ThreadSetNotePad
#define Lw_Thread_GetNotePad                    API_ThreadGetNotePad

#define Lw_Thread_FeedWatchDog                  API_ThreadFeedWatchDog
#define Lw_Thread_CancelWatchDog                API_ThreadCancelWatchDog

#define Lw_Thread_StackCheck                    API_ThreadStackCheck
#define Lw_Thread_GetStackMini                  API_ThreadGetStackMini

#define Lw_Thread_CPUUsageOn                    API_ThreadCPUUsageOn
#define Lw_Thread_CPUUsageOff                   API_ThreadCPUUsageOff
#define Lw_Thread_CPUUsageIsOn                  API_ThreadCPUUsageIsOn

#define Lw_Thread_GetCPUUsage                   API_ThreadGetCPUUsage
#define Lw_Thread_GetCPUUsageAll                API_ThreadGetCPUUsageAll
#define Lw_Thread_CPUUsageRefresh               API_ThreadCPUUsageRefresh

#define Lw_Thread_Wakeup                        API_ThreadWakeup
#define Lw_Thread_WakeupEx                      API_ThreadWakeupEx
#define Lw_Thread_Yield                         API_ThreadYield

#define Lw_Thread_VarAdd                        API_ThreadVarAdd
#define Lw_Thread_VarDelete                     API_ThreadVarDelete

#define Lw_Thread_VarSet                        API_ThreadVarSet
#define Lw_Thread_VarGet                        API_ThreadVarGet

#define Lw_Thread_VarStatus                     API_ThreadVarInfo
#define Lw_Thread_VarInfo                       API_ThreadVarInfo

#define Lw_Thread_Verify                        API_ThreadVerify

/*********************************************************************************************************
  THREAD EXT
*********************************************************************************************************/

#define Lw_Thread_Once                          API_ThreadOnce
#define Lw_Thread_Once2                         API_ThreadOnce2

#define Lw_Thread_Cleanup_Push                  API_ThreadCleanupPush
#define Lw_Thread_Cleanup_PushEx                API_ThreadCleanupPushEx
#define Lw_Thread_Cleanup_Pop                   API_ThreadCleanupPop

#define Lw_Thread_Condattr_Init                 API_ThreadCondAttrInit
#define Lw_Thread_Condattr_Destroy              API_ThreadCondAttrDestroy
#define Lw_Thread_Condattr_Getpshared           API_ThreadCondAttrGetPshared
#define Lw_Thread_Condattr_Setpshared           API_ThreadCondAttrSetPshared

#define Lw_Thread_Cond_Init                     API_ThreadCondInit
#define Lw_Thread_Cond_Destroy                  API_ThreadCondDestroy
#define Lw_Thread_Cond_Signal                   API_ThreadCondSignal
#define Lw_Thread_Cond_Broadcast                API_ThreadCondBroadcast
#define Lw_Thread_Cond_Wait                     API_ThreadCondWait

/*********************************************************************************************************
  COROUTINE
*********************************************************************************************************/

#define Lw_Coroutine_Create                     API_CoroutineCreate
#define Lw_Coroutine_Delete                     API_CoroutineDelete
#define Lw_Coroutine_Exit                       API_CoroutineExit
#define Lw_Coroutine_Yield                      API_CoroutineYield
#define Lw_Coroutine_Resume                     API_CoroutineResume
#define Lw_Coroutine_StackCheck                 API_CoroutineStackCheck

/*********************************************************************************************************
  SEMAPHORE
*********************************************************************************************************/

#define Lw_Semaphore_Wait                       API_SemaphorePend
#define Lw_Semaphore_Get                        API_SemaphorePend
#define Lw_Semaphore_Take                       API_SemaphorePend

#define Lw_Semaphore_Post                       API_SemaphorePost
#define Lw_Semaphore_Give                       API_SemaphorePost
#define Lw_Semaphore_Send                       API_SemaphorePost

#define Lw_Semaphore_Flush                      API_SemaphoreFlush
#define Lw_Semaphore_Delete                     API_SemaphoreDelete

#define Lw_Semaphore_PostBPend                  API_SemaphorePostBPend
#define Lw_Semaphore_PostCPend                  API_SemaphorePostCPend

/*********************************************************************************************************
  COUNTING SEMAPHORE
*********************************************************************************************************/

#define Lw_SemaphoreC_Create                    API_SemaphoreCCreate
#define Lw_SemaphoreC_Delete                    API_SemaphoreCDelete

#define Lw_SemaphoreC_TryWait                   API_SemaphoreCTryPend
#define Lw_SemaphoreC_TryGet                    API_SemaphoreCTryPend
#define Lw_SemaphoreC_TryTake                   API_SemaphoreCTryPend

#define Lw_SemaphoreC_Wait                      API_SemaphoreCPend
#define Lw_SemaphoreC_Get                       API_SemaphoreCPend
#define Lw_SemaphoreC_Take                      API_SemaphoreCPend

#define Lw_SemaphoreC_Release                   API_SemaphoreCRelease

#define Lw_SemaphoreC_Post                      API_SemaphoreCPost
#define Lw_SemaphoreC_Give                      API_SemaphoreCPost
#define Lw_SemaphoreC_Send                      API_SemaphoreCPost

#define Lw_SemaphoreC_Flush                     API_SemaphoreCFlush
#define Lw_SemaphoreC_Clear                     API_SemaphoreCClear

#define Lw_SemaphoreC_Status                    API_SemaphoreCStatus
#define Lw_SemaphoreC_Info                      API_SemaphoreCStatus
#define Lw_SemaphoreC_StatusEx                  API_SemaphoreCStatusEx
#define Lw_SemaphoreC_InfoEx                    API_SemaphoreCStatusEx

#define Lw_SemaphoreC_GetName                   API_SemaphoreGetName

/*********************************************************************************************************
  BINARY SEMAPHORE
*********************************************************************************************************/

#define Lw_SemaphoreB_Create                    API_SemaphoreBCreate
#define Lw_SemaphoreB_Delete                    API_SemaphoreBDelete

#define Lw_SemaphoreB_TryWait                   API_SemaphoreBTryPend
#define Lw_SemaphoreB_TryGet                    API_SemaphoreBTryPend
#define Lw_SemaphoreB_TryTake                   API_SemaphoreBTryPend

#define Lw_SemaphoreB_Wait                      API_SemaphoreBPend
#define Lw_SemaphoreB_Get                       API_SemaphoreBPend
#define Lw_SemaphoreB_Take                      API_SemaphoreBPend

#define Lw_SemaphoreB_WaitEx                    API_SemaphoreBPendEx
#define Lw_SemaphoreB_GetEx                     API_SemaphoreBPendEx
#define Lw_SemaphoreB_TakeEx                    API_SemaphoreBPendEx

#define Lw_SemaphoreB_Release                   API_SemaphoreBRelease

#define Lw_SemaphoreB_Post                      API_SemaphoreBPost
#define Lw_SemaphoreB_Give                      API_SemaphoreBPost
#define Lw_SemaphoreB_Send                      API_SemaphoreBPost

#define Lw_SemaphoreB_Post2                     API_SemaphoreBPost2
#define Lw_SemaphoreB_Give2                     API_SemaphoreBPost2
#define Lw_SemaphoreB_Send2                     API_SemaphoreBPost2

#define Lw_SemaphoreB_PostEx                    API_SemaphoreBPostEx
#define Lw_SemaphoreB_GiveEx                    API_SemaphoreBPostEx
#define Lw_SemaphoreB_SendEx                    API_SemaphoreBPostEx

#define Lw_SemaphoreB_PostEx2                   API_SemaphoreBPostEx2
#define Lw_SemaphoreB_GiveEx2                   API_SemaphoreBPostEx2
#define Lw_SemaphoreB_SendEx2                   API_SemaphoreBPostEx2

#define Lw_SemaphoreB_Flush                     API_SemaphoreBFlush
#define Lw_SemaphoreB_Clear                     API_SemaphoreBClear

#define Lw_SemaphoreB_Status                    API_SemaphoreBStatus
#define Lw_SemaphoreB_Info                      API_SemaphoreBStatus

#define Lw_SemaphoreB_GetName                   API_SemaphoreGetName

/*********************************************************************************************************
  MUTEX SEMAPHORE
*********************************************************************************************************/

#define Lw_SemaphoreM_Create                    API_SemaphoreMCreate
#define Lw_SemaphoreM_Delete                    API_SemaphoreMDelete

#define Lw_SemaphoreM_Wait                      API_SemaphoreMPend
#define Lw_SemaphoreM_Get                       API_SemaphoreMPend
#define Lw_SemaphoreM_Take                      API_SemaphoreMPend

#define Lw_SemaphoreM_Give                      API_SemaphoreMPost
#define Lw_SemaphoreM_Post                      API_SemaphoreMPost
#define Lw_SemaphoreM_Send                      API_SemaphoreMPost

#define Lw_SemaphoreM_Status                    API_SemaphoreMStatus
#define Lw_SemaphoreM_Info                      API_SemaphoreMStatus
#define Lw_SemaphoreM_StatusEx                  API_SemaphoreMStatusEx
#define Lw_SemaphoreM_InfoEx                    API_SemaphoreMStatusEx

#define Lw_SemaphoreM_GetName                   API_SemaphoreGetName

/*********************************************************************************************************
  RW SEMAPHORE
*********************************************************************************************************/

#define Lw_SemaphoreRW_Create                   API_SemaphoreRWCreate
#define Lw_SemaphoreRW_Delete                   API_SemaphoreRWDelete

#define Lw_SemaphoreRW_RWait                    API_SemaphoreRWPendR
#define Lw_SemaphoreRW_RGet                     API_SemaphoreRWPendR
#define Lw_SemaphoreRW_RTake                    API_SemaphoreRWPendR

#define Lw_SemaphoreRW_WWait                    API_SemaphoreRWPendW
#define Lw_SemaphoreRW_WGet                     API_SemaphoreRWPendW
#define Lw_SemaphoreRW_WTake                    API_SemaphoreRWPendW

#define Lw_SemaphoreRW_Give                     API_SemaphoreRWPost
#define Lw_SemaphoreRW_Post                     API_SemaphoreRWPost
#define Lw_SemaphoreRW_Send                     API_SemaphoreRWPost

#define Lw_SemaphoreRW_Status                   API_SemaphoreRWStatus
#define Lw_SemaphoreRW_Info                     API_SemaphoreRWStatus

/*********************************************************************************************************
  MESSAGE QUEUE
*********************************************************************************************************/

#define Lw_MsgQueue_Create                      API_MsgQueueCreate
#define Lw_MsgQueue_Delete                      API_MsgQueueDelete

#define Lw_MsgQueue_Receive                     API_MsgQueueReceive
#define Lw_MsgQueue_ReceiveEx                   API_MsgQueueReceiveEx
#define Lw_MsgQueue_TryReceive                  API_MsgQueueTryReceive

#define Lw_MsgQueue_Send                        API_MsgQueueSend
#define Lw_MsgQueue_Send2                       API_MsgQueueSend2
#define Lw_MsgQueue_SendEx                      API_MsgQueueSendEx
#define Lw_MsgQueue_SendEx2                     API_MsgQueueSendEx2

#define Lw_MsgQueue_Flush                       API_MsgQueueFlush
#define Lw_MsgQueue_FlushReceive                API_MsgQueueFlushReceive
#define Lw_MsgQueue_FlushSend                   API_MsgQueueFlushSend
#define Lw_MsgQueue_Clear                       API_MsgQueueClear

#define Lw_MsgQueue_Status                      API_MsgQueueStatus
#define Lw_MsgQueue_Info                        API_MsgQueueStatus
#define Lw_MsgQueue_StatusEx                    API_MsgQueueStatusEx
#define Lw_MsgQueue_InfoEx                      API_MsgQueueStatusEx

#define Lw_MsgQueue_GetName                     API_MsgQueueGetName

/*********************************************************************************************************
  EVENT SET
*********************************************************************************************************/

#define Lw_Event_Create                         API_EventSetCreate
#define Lw_Event_Delete                         API_EventSetDelete

#define Lw_Event_TryWait                        API_EventSetTryGet
#define Lw_Event_TryGet                         API_EventSetTryGet
#define Lw_Event_TryTake                        API_EventSetTryGet

#define Lw_Event_TryWaitEx                      API_EventSetTryGetEx
#define Lw_Event_TryGetEx                       API_EventSetTryGetEx
#define Lw_Event_TryTakeEx                      API_EventSetTryGetEx

#define Lw_Event_Wait                           API_EventSetGet
#define Lw_Event_Get                            API_EventSetGet
#define Lw_Event_Take                           API_EventSetGet

#define Lw_Event_WaitEx                         API_EventSetGetEx
#define Lw_Event_GetEx                          API_EventSetGetEx
#define Lw_Event_TakeEx                         API_EventSetGetEx

#define Lw_Event_Send                           API_EventSetSet
#define Lw_Event_Post                           API_EventSetSet
#define Lw_Event_Give                           API_EventSetSet

#define Lw_Event_Status                         API_EventSetStatus
#define Lw_Event_Info                           API_EventSetStatus

#define Lw_Event_GetName                        API_EventSetGetName

/*********************************************************************************************************
  TIME
*********************************************************************************************************/

#define Lw_Time_Sleep                           API_TimeSleep
#define Lw_Time_SleepEx                         API_TimeSleepEx
#define Lw_Time_SleepUntil                      API_TimeSleepUntil

#define Lw_Time_SSleep                          API_TimeSSleep
#define Lw_Time_SDelay                          API_TimeSSleep

#define Lw_Time_MSleep                          API_TimeMSleep
#define Lw_Time_MDelay                          API_TimeMSleep

#define Lw_Time_Set                             API_TimeSet
#define Lw_Time_Get                             API_TimeGet
#define Lw_Time_Get64                           API_TimeGet64

#define Lw_Time_GetFrequency                    API_TimeGetFrequency

#define Lw_Time_Ticks                           API_KernelTicks
#define Lw_Time_TicksContext                    API_KernelTicksContext

#define Lw_Time_TodAdj                          API_TimeTodAdj

/*********************************************************************************************************
  RMS
*********************************************************************************************************/

#define Lw_Rms_Create                           API_RmsCreate
#define Lw_Rms_Delete                           API_RmsDelete
#define Lw_Rms_DeleteEx                         API_RmsDeleteEx

#define Lw_Rms_ExecTimeGet                      API_RmsExecTimeGet

#define Lw_Rms_Period                           API_RmsPeriod
#define Lw_Rms_Cancel                           API_RmsCancel

#define Lw_Rms_Status                           API_RmsStatus
#define Lw_Rms_Info                             API_RmsStatus

#define Lw_Rms_GetName                          API_RmsGetName

/*********************************************************************************************************
  PARTITION
*********************************************************************************************************/

#define Lw_Partition_Create                     API_PartitionCreate
#define Lw_Partition_Delete                     API_PartitionDelete
#define Lw_Partition_DeleteEx                   API_PartitionDeleteEx

#define Lw_Partition_Get                        API_PartitionGet
#define Lw_Partition_Take                       API_PartitionGet
#define Lw_Partition_Allocate                   API_PartitionGet

#define Lw_Partition_Put                        API_PartitionPut
#define Lw_Partition_Give                       API_PartitionPut
#define Lw_Partition_Free                       API_PartitionPut

#define Lw_Partition_Status                     API_PartitionStatus
#define Lw_Partition_Info                       API_PartitionStatus

#define Lw_Partition_GetName                    API_PartitionGetName

/*********************************************************************************************************
  REGION
*********************************************************************************************************/

#define Lw_Region_Create                        API_RegionCreate
#define Lw_Region_Delete                        API_RegionDelete
#define Lw_Region_DeleteEx                      API_RegionDeleteEx

#define Lw_Region_AddMem                        API_RegionAddMem

#define Lw_Region_Get                           API_RegionGet
#define Lw_Region_Take                          API_RegionGet
#define Lw_Region_Allocate                      API_RegionGet

#define Lw_Region_GetAlign                      API_RegionGetAlign
#define Lw_Region_TakeAlign                     API_RegionGetAlign
#define Lw_Region_AllocateAlign                 API_RegionGetAlign

#define Lw_Region_Reget                         API_RegionReget
#define Lw_Region_Retake                        API_RegionReget
#define Lw_Region_Realloc                       API_RegionReget

#define Lw_Region_Put                           API_RegionPut
#define Lw_Region_Give                          API_RegionPut
#define Lw_Region_Free                          API_RegionPut

#define Lw_Region_Status                        API_RegionStatus
#define Lw_Region_Info                          API_RegionStatus

#define Lw_Region_StatusEx                      API_RegionStatusEx
#define Lw_Region_InfoEx                        API_RegionStatusEx

#define Lw_Region_GetMax                        API_RegionGetMax
#define Lw_Region_GetName                       API_RegionGetName

/*********************************************************************************************************
  INTERRUPT
*********************************************************************************************************/

#define Lw_Interrupt_Lock                       API_InterLock
#define Lw_Interrupt_Unlock                     API_InterUnlock

#define Lw_Interrupt_Context                    API_InterContext
#define Lw_Interrupt_GetNesting                 API_InterGetNesting
#define Lw_Interrupt_GetNestingById             API_InterGetNestingById

#define Lw_Interrupt_StackCheck                 API_InterStackCheck

/*********************************************************************************************************
  ERROR
*********************************************************************************************************/

#define Lw_Error_GetLast                        API_GetLastError
#define Lw_Error_GetLastEx                      API_GetLastErrorEx

#define Lw_Error_SetLast                        API_SetLastError
#define Lw_Error_SetLastEx                      API_SetLastErrorEx

/*********************************************************************************************************
  KERNEL
*********************************************************************************************************/

#define Lw_Kernel_Nop                           API_KernelNop

#define Lw_Kernel_Version                       API_KernelVersion
#define Lw_Kernel_Verinfo                       API_KernelVerinfo
#define Lw_Kernel_Verpatch                      API_KernelVerpatch

#define Lw_Kernel_Reboot                        API_KernelReboot
#define Lw_Kernel_RebootEx                      API_KernelRebootEx

#define Lw_Kernel_GetIdle                       API_KernelGetIdle
#define Lw_Kernel_GetItimer                     API_KernelGetItimer
#define Lw_Kernel_GetExc                        API_KernelGetExc

#define Lw_Kernel_GetPriorityMax                API_KernelGetPriorityMax
#define Lw_Kernel_GetPriorityMin                API_KernelGetPriorityMin

#define Lw_Kernel_GetThreadNum                  API_KernelGetThreadNum
#define Lw_Kernel_GetThreadNumByPriority        API_KernelGetThreadNumByPriority

#define Lw_Kernel_IsRunning                     API_KernelIsRunning

#define Lw_Kernel_HeapInfo                      API_KernelHeapInfo
#define Lw_Kernel_HeapStatus                    API_KernelHeapInfo
#define Lw_Kernel_HeapInfoEx                    API_KernelHeapInfoEx
#define Lw_Kernel_HeapStatusEx                  API_KernelHeapInfoEx

/*********************************************************************************************************
  SHOW
*********************************************************************************************************/

#define Lw_Backtrace_Show                       API_BacktraceShow
#define Lw_Thread_Show                          API_ThreadShow
#define Lw_Thread_ShowEx                        API_ThreadShowEx
#define Lw_Thread_PendShow                      API_ThreadPendShow
#define Lw_Thread_PendShowEx                    API_ThreadPendShowEx
#define Lw_Semaphore_Show                       API_SemaphoreShow
#define Lw_MsgQueue_Show                        API_MsgQueueShow
#define Lw_Interrupt_Show                       API_InterShow
#define Lw_Time_Show                            API_TimeShow
#define Lw_Timer_Show                           API_TimerShow
#define Lw_Rms_Show                             API_RmsShow
#define Lw_CPUUsage_Show                        API_CPUUsageShow
#define Lw_EventSet_Show                        API_EventSetShow
#define Lw_Stack_Show                           API_StackShow
#define Lw_Region_Show                          API_RegionShow
#define Lw_Partition_Show                       API_PartitionShow
#define Lw_Vmm_PhysicalShow                     API_VmmPhysicalShow
#define Lw_Vmm_VirtualShow                      API_VmmVirtualShow
#define Lw_Vmm_AbortShow                        API_VmmAbortShow

#endif                                                                  /*  __LW_API_KERNEL_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
