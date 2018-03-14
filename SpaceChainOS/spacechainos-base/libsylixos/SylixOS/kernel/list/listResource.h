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
** 文   件   名: ListResource.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统资源链表操作函数集合。

** BUG
2007.04.08  加入了对裁剪的宏支持
*********************************************************************************************************/

#ifndef  __LISTRESOURCE_H
#define  __LISTRESOURCE_H

/*********************************************************************************************************
  THREAD ID RESOURCE
*********************************************************************************************************/

PLW_CLASS_TCB        _Allocate_Tcb_Object(VOID);
VOID                 _Free_Tcb_Object(PLW_CLASS_TCB    ptcbFree);

/*********************************************************************************************************
  EVENT RESOURCE (Semaphore & MsgQueue)
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

PLW_CLASS_EVENT      _Allocate_Event_Object(VOID);
VOID                 _Free_Event_Object(PLW_CLASS_EVENT  pevent);

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  THREAD PRIVATE VAR RESOURCE
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

PLW_CLASS_THREADVAR  _Allocate_ThreadVar_Object(VOID);
VOID                 _Free_ThreadVar_Object(PLW_CLASS_THREADVAR    pthreadvarFree);

#endif                                                                  /*  (LW_CFG_THREAD_PRIVATE...   */
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VARS */
/*********************************************************************************************************
  EVENT SET RESOURCE
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

PLW_CLASS_EVENTSET   _Allocate_EventSet_Object(VOID);
VOID                 _Free_EventSet_Object(PLW_CLASS_EVENTSET  pesFree);

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  HEAP RESOURCE
*********************************************************************************************************/

PLW_CLASS_HEAP       _Allocate_Heap_Object(VOID);
VOID                 _Free_Heap_Object(PLW_CLASS_HEAP    pheapFree);

/*********************************************************************************************************
  MESSAGE QUEUE RESOURCE
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

PLW_CLASS_MSGQUEUE   _Allocate_MsgQueue_Object(VOID);
VOID                 _Free_MsgQueue_Object(PLW_CLASS_MSGQUEUE    pmsgqueueFree);

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  TIMER RESOURCE
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)

PLW_CLASS_TIMER      _Allocate_Timer_Object(VOID);
VOID                 _Free_Timer_Object(PLW_CLASS_TIMER  ptmrFree);

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS)         */
/*********************************************************************************************************
  PARTITION RESOURCE
*********************************************************************************************************/
#if	(LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

PLW_CLASS_PARTITION  _Allocate_Partition_Object(VOID);
VOID                 _Free_Partition_Object(PLW_CLASS_PARTITION    p_partFree);

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  RMS RESOURCE
*********************************************************************************************************/
#if	(LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

PLW_CLASS_RMS        _Allocate_Rms_Object(VOID);
VOID                 _Free_Rms_Object(PLW_CLASS_RMS    prmsFree);

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
#endif                                                                  /*  __LISTRESOURCE_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
