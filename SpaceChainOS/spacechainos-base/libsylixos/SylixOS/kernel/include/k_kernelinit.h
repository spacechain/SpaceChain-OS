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
** 文   件   名: k_kernelinit.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 12 日
**
** 描        述: 这是系统初始化文件库。
*********************************************************************************************************/

#ifndef  __K_KERNELINIT_H
#define  __K_KERNELINIT_H

/*********************************************************************************************************
  kernel init
*********************************************************************************************************/

VOID  _GlobalPrimaryInit(VOID);                                         /*  全局变量                    */
#if LW_CFG_SMP_EN > 0
VOID  _GlobalSecondaryInit(VOID);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

VOID  _InterVectInit(VOID);                                             /*  中断向量表初始化            */
VOID  _ThreadIdInit(VOID);                                              /*  线程 ID 控制块初始化        */
VOID  _StackCheckInit(VOID);                                            /*  堆栈检查初始化              */
VOID  _ReadyTableInit(VOID);                                            /*  就绪表初始化                */
VOID  _PriorityInit(VOID);                                              /*  优先级表初始化              */
VOID  _EventSetInit(VOID);                                              /*  事件集初始化                */
VOID  _EventInit(VOID);                                                 /*  事件初始化                  */
VOID  _ThreadVarInit(VOID);                                             /*  线程私有变量初始化          */
VOID  _HeapInit(VOID);                                                  /*  堆控制块初始化              */
VOID  _MsgQueueInit(VOID);                                              /*  消息队列初始化              */
VOID  _TimerInit(VOID);                                                 /*  定时器初始化                */
VOID  _TimeCvtInit(VOID);                                               /*  初始化时间转换函数          */
VOID  _PartitionInit(VOID);                                             /*  PARTITION 初始化            */
VOID  _RmsInit(VOID);                                                   /*  RMS 调度器初始化            */
VOID  _RtcInit(VOID);                                                   /*  RTC 时间初始化              */

/*********************************************************************************************************
  kernel & system heap
*********************************************************************************************************/

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _HeapKernelInit(PVOID     pvKernelHeapMem,
                      size_t    stKernelHeapSize);                      /*  内核堆建立                  */
VOID  _HeapSystemInit(PVOID     pvSystemHeapMem,
                      size_t    stSystemHeapSize);                      /*  系统堆建立                  */
#else
VOID  _HeapKernelInit(VOID);                                            /*  内核堆建立                  */
VOID  _HeapSystemInit(VOID);                                            /*  系统堆建立                  */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

/*********************************************************************************************************
  kernel
*********************************************************************************************************/

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _KernelPrimaryLowLevelInit(PVOID     pvKernelHeapMem,
                                 size_t    stKernelHeapSize,
                                 PVOID     pvSystemHeapMem,
                                 size_t    stSystemHeapSize);           /*  系统低端初始化              */
#else
VOID  _KernelPrimaryLowLevelInit(VOID);                                 /*  系统低端初始化              */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

#if LW_CFG_SMP_EN > 0
VOID  _KernelSecondaryLowLevelInit(VOID);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

VOID  _KernelHighLevelInit(VOID);                                       /*  系统高端初始化              */

#endif                                                                  /*  __K_KERNELINIT_H            */

/*********************************************************************************************************
  END
*********************************************************************************************************/
