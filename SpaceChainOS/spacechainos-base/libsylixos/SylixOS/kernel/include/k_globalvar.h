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
** 文   件   名: k_globalvar.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 12 日
**
** 描        述: 这是系统全局变量定义文件。

** BUG
2007.09.12  加入了可裁剪宏限制。
2007.11.07  将用户堆正式命名为内核堆.
2007.12.06  系统线程建立时,直接进入安全模式,不能被删除.
2008.01.20  删除了全局的调度器锁变量, 改由线程私有锁代替.
2008.01.24  增加 启动时临时截获 TCB .
2008.01.24  增加可以裁剪的 LOGO 图标.
2008.05.18  加入 _K_iKernelStatus 变量, 此变量为内核态的状态变量!
2008.06.27  线程属性快不进入安全模式.
2009.09.30  加入原子操作自旋锁.
2010.01.22  内核任务扫描连加入链尾.
2010.08.03  加入 tick spinlock.
2012.03.19  将调度器参数改为多核兼容方式.
2013.07.17  加入 _K_bMultiTaskStart 主核在初始化结束后将此变量设为 LW_TRUE 告知其他的 CPU 可以进入多任务
            模式.
2013.07.19  内核状态不再放在此处, 而应该放在每一个 CPU 中.
*********************************************************************************************************/

#ifndef __K_GLOBALVAR_H
#define __K_GLOBALVAR_H

#ifdef  __KERNEL_MAIN_FILE
#define __KERNEL_EXT
#else
#define __KERNEL_EXT    extern
#endif

/*********************************************************************************************************
  内核调试信息打印接口
*********************************************************************************************************/
__KERNEL_EXT  VOIDFUNCPTR             _K_pfuncKernelDebugLog;           /*  内核调试信息                */
__KERNEL_EXT  VOIDFUNCPTR             _K_pfuncKernelDebugError;         /*  内核错误信息                */
/*********************************************************************************************************
  HOOK TABLE
*********************************************************************************************************/
#ifdef __KERNEL_MAIN_FILE
              LW_CLASS_TIMING         _K_timingKernel = {100, 10000000, 100, 5, 1, 10, LW_CFG_SLICE_DEFAULT};
#else
__KERNEL_EXT  LW_CLASS_TIMING         _K_timingKernel;                  /*  内核时间参数集合            */
#endif                                                                  /*  __KERNEL_MAIN_FILE          */
/*********************************************************************************************************
  HOOK TABLE
*********************************************************************************************************/
__KERNEL_EXT  LW_CLASS_HOOK           _K_hookKernel;                    /*  内核动态钩子函数定义        */
/*********************************************************************************************************
  内核配置参数
*********************************************************************************************************/
__KERNEL_EXT  ULONG                   _K_ulKernFlags;                   /*  内核配置参数                */
/*********************************************************************************************************
  系统中断向量表
*********************************************************************************************************/
__KERNEL_EXT  LW_CLASS_INTDESC        _K_idescTable[LW_CFG_MAX_INTER_SRC];
#ifdef __KERNEL_MAIN_FILE
LW_SPINLOCK_DEFINE_CACHE_ALIGN       (_K_slVectorTable);
#else
__KERNEL_EXT  LW_SPINLOCK_DECLARE    (_K_slVectorTable);
#endif                                                                  /*  __KERNEL_MAIN_FILE          */
/*********************************************************************************************************
  系统状态
*********************************************************************************************************/
__KERNEL_EXT  UINT8                   _K_ucSysStatus;                   /*  系统状态                    */
__KERNEL_EXT  ULONG                   _K_ulNotRunError;                 /*  系统未启动错误代码存放      */
/*********************************************************************************************************
  内核工作队列
*********************************************************************************************************/
__KERNEL_EXT  LW_JOB_QUEUE            _K_jobqKernel;                    /*  内核工作队列                */
__KERNEL_EXT  LW_JOB_MSG              _K_jobmsgKernel[LW_CFG_MAX_EXCEMSGS];
/*********************************************************************************************************
  TICK (由于大量的应用软件都将 SylixOS 时钟定义为 ulong 型, 在 32 位机器上, 扩展使用 64 位 tick 会有历史
        遗留问题, 所以这里只能加入一个溢出计数器)
*********************************************************************************************************/
__KERNEL_EXT  INT64                   _K_i64KernelTime;                 /*  系统时间计数器              */

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
__KERNEL_EXT  ULONG                   _K_ulCPUUsageTicks;               /*  利用率测算计数器            */
__KERNEL_EXT  ULONG                   _K_ulCPUUsageKernelTicks;         /*  内核利用率测算              */
__KERNEL_EXT  BOOL                    _K_ulCPUUsageEnable;              /*  是否使能CPU利用率测算       */
#endif
/*********************************************************************************************************
  线程数计数器
*********************************************************************************************************/
__KERNEL_EXT  UINT16                  _K_usThreadCounter;               /*  线程数量                    */
/*********************************************************************************************************
  IDLE 线程定义
*********************************************************************************************************/
__KERNEL_EXT  LW_OBJECT_ID            _K_ulIdleId[LW_CFG_MAX_PROCESSORS];
__KERNEL_EXT  PLW_CLASS_TCB           _K_ptcbIdle[LW_CFG_MAX_PROCESSORS];
                                                                        /*  空闲线程 ID 及 TCB          */
__KERNEL_EXT  LW_OBJECT_ID            _K_ulThreadItimerId;              /*  内部定时器服务线程          */
/*********************************************************************************************************
  堆栈相关
*********************************************************************************************************/
__KERNEL_EXT  LW_STACK                _K_stkFreeFlag;                   /*  空闲堆栈区域标志            */
/*********************************************************************************************************
  定时器
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)
__KERNEL_EXT  LW_CLASS_TIMER          _K_tmrBuffer[LW_CFG_MAX_TIMERS];  /*  定时器缓冲区                */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcTmr;                      /*  定时器对象资源管理          */
#if (LW_CFG_HTIMER_EN > 0)
__KERNEL_EXT  LW_CLASS_WAKEUP         _K_wuHTmr;                        /*  高速定时器管理表            */
#endif
#if (LW_CFG_ITIMER_EN > 0)
__KERNEL_EXT  LW_CLASS_WAKEUP         _K_wuITmr;                        /*  普通定时器管理表            */
#endif
#endif
/*********************************************************************************************************
  RTC
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0
__KERNEL_EXT  INT32                   _K_iTODDelta;                     /*  系统时钟微调参数            */
__KERNEL_EXT  struct timespec         _K_tvTODCurrent;                  /*  当前TOD时间 CLOCK_REALTIME  */
__KERNEL_EXT  struct timespec         _K_tvTODMono;                     /*  MonoTOD时间 CLOCK_MONOTONIC */
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
  PARTITION 缓冲区 (_part 为类型，避免 ppart 前缀命名出现歧异，ppart 应为 p_part)
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
__KERNEL_EXT  LW_CLASS_PARTITION      _K__partBuffer[LW_CFG_MAX_PARTITIONS];
                                                                        /*  PARTITION 缓冲区            */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcPart;                     /*  PARTITION 对象资源管理      */
#endif
/*********************************************************************************************************
  堆操作缓冲区
*********************************************************************************************************/
__KERNEL_EXT  PLW_CLASS_HEAP          _K_pheapKernel;                   /*  内核堆                      */
__KERNEL_EXT  PLW_CLASS_HEAP          _K_pheapSystem;                   /*  系统堆                      */
/*********************************************************************************************************
  堆控制块
*********************************************************************************************************/
__KERNEL_EXT  LW_CLASS_HEAP           _K_heapBuffer[2 + LW_CFG_MAX_REGIONS];
                                                                        /*  系统堆建立                  */
                                                                        /*  系统使用两个堆              */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcHeap;                     /*  内存堆对象资源管理          */
__KERNEL_EXT  BOOL                    _K_bHeapCrossBorderEn;            /*  内存越界检查                */
/*********************************************************************************************************
  RMS
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
__KERNEL_EXT  LW_CLASS_RMS            _K_rmsBuffer[LW_CFG_MAX_RMSS];    /*  RMS 缓冲区                  */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcRms;                      /*  RMS 对象资源管理            */
#endif
/*********************************************************************************************************
  TCB 管理表表头
*********************************************************************************************************/
__KERNEL_EXT  LW_LIST_LINE_HEADER     _K_plineTCBHeader;                /*  管理表头                    */
/*********************************************************************************************************
  TCB 内核扫描链
*********************************************************************************************************/
__KERNEL_EXT  LW_CLASS_WAKEUP         _K_wuDelay;                       /*  超时唤醒的链表              */
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
__KERNEL_EXT  LW_CLASS_WAKEUP         _K_wuWatchDog;                    /*  看门狗扫描表                */
#endif
/*********************************************************************************************************
  事件控件缓冲区
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
__KERNEL_EXT  LW_CLASS_EVENT          _K_eventBuffer[LW_CFG_MAX_EVENTS];/*  事件控制块缓冲区            */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcEvent;                    /*  事件对象资源管理            */
#endif
/*********************************************************************************************************
  消息队列缓冲区
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)
__KERNEL_EXT  LW_CLASS_MSGQUEUE       _K_msgqueueBuffer[LW_CFG_MAX_MSGQUEUES];
                                                                        /*  消息队列控制块缓冲区        */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcMsgQueue;                 /*  消息队列对象资源管理        */
#endif
/*********************************************************************************************************
  事件标志组
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
__KERNEL_EXT  LW_CLASS_EVENTSET       _K_esBuffer[LW_CFG_MAX_EVENTSETS];/*  事件集控制块缓冲区          */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcEventSet;                 /*  事件集对象资源管理          */
#endif
/*********************************************************************************************************
  THREAD VAR
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
__KERNEL_EXT  LW_CLASS_THREADVAR      _K_threavarBuffer[LW_CFG_MAX_THREAD_GLB_VARS];
                                                                        /*  私有变量控制块              */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcThreadVar;                /*  私有变量对象资源管理        */
#endif
/*********************************************************************************************************
  THREAD
*********************************************************************************************************/
__KERNEL_EXT  LW_CLASS_TCB            _K_tcbBuffer[LW_CFG_MAX_THREADS]; /*  TCB 分配池                  */
__KERNEL_EXT  LW_CLASS_OBJECT_RESRC   _K_resrcTcb;                      /*  TCB 对象资源管理            */
/*********************************************************************************************************
  私有化全局变量
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
__KERNEL_EXT  LW_CLASS_THREADVAR      _K_privatevarBuffer[LW_CFG_MAX_THREAD_GLB_VARS];
#endif
/*********************************************************************************************************
  全局就绪位图表
*********************************************************************************************************/
__KERNEL_EXT  LW_CLASS_PCBBMAP        _K_pcbbmapGlobalReady;
/*********************************************************************************************************
  线程 TCB 地址表
*********************************************************************************************************/
__KERNEL_EXT  PLW_CLASS_TCB           _K_ptcbTCBIdTable[LW_CFG_MAX_THREADS];
/*********************************************************************************************************
  CPU 与 物理 CPU 个数
*********************************************************************************************************/
#ifdef __KERNEL_NCPUS_SET
#ifdef __KERNEL_MAIN_FILE
__KERNEL_EXT  ULONG                   _K_ulNCpus = 1;
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
__KERNEL_EXT  ULONG                   _K_ulNPhyCpus = 1;
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#else                                                                   /*  !__KERNEL_MAIN_FILE         */
__KERNEL_EXT  ULONG                   _K_ulNCpus;
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
__KERNEL_EXT  ULONG                   _K_ulNPhyCpus;
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#endif                                                                  /*  __KERNEL_MAIN_FILE          */
#else                                                                   /*  !__KERNEL_NCPUS_SET         */
__KERNEL_EXT  const  ULONG            _K_ulNCpus;
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
__KERNEL_EXT  const  ULONG            _K_ulNPhyCpus;
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
#endif                                                                  /*  __KERNEL_NCPUS_SET          */
/*********************************************************************************************************
  CPU 表与 内核锁
*********************************************************************************************************/
#ifdef __KERNEL_MAIN_FILE                                               /*  每个 CPU 的内容             */
              LW_CLASS_CPU            _K_cpuTable[LW_CFG_MAX_PROCESSORS] LW_CACHE_LINE_ALIGN;
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
              LW_CLASS_PHYCPU         _K_phycpuTable[LW_CFG_MAX_PROCESSORS] LW_CACHE_LINE_ALIGN;
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
              LW_CLASS_KERNLOCK       _K_klKernel LW_CACHE_LINE_ALIGN;
#else
__KERNEL_EXT  LW_CLASS_CPU            _K_cpuTable[LW_CFG_MAX_PROCESSORS];
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
__KERNEL_EXT  LW_CLASS_PHYCPU         _K_phycpuTable[LW_CFG_MAX_PROCESSORS];
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
__KERNEL_EXT  LW_CLASS_KERNLOCK       _K_klKernel;                      /*  内核锁                      */
#endif                                                                  /*  __KERNEL_MAIN_FILE          */
/*********************************************************************************************************
  原子操作锁
*********************************************************************************************************/
#ifdef __KERNEL_MAIN_FILE
LW_SPINLOCK_DEFINE_CACHE_ALIGN       (_K_slAtomic);
#else
__KERNEL_EXT  LW_SPINLOCK_DECLARE    (_K_slAtomic);                     /*  原子操作锁                  */
#endif                                                                  /*  __KERNEL_MAIN_FILE          */
/*********************************************************************************************************
  启动时临时截获 TCB
*********************************************************************************************************/
__KERNEL_EXT  LW_CLASS_TCB            _K_tcbDummy[LW_CFG_MAX_PROCESSORS];
/*********************************************************************************************************
  系统 LOGO
*********************************************************************************************************/
#if LW_CFG_KERNEL_LOGO > 0
#ifdef __KERNEL_MAIN_FILE
       const  CHAR                    _K_cKernelLogo[] = __SYLIXOS_LOGO;
#else
__KERNEL_EXT  const  CHAR             _K_cKernelLogo[];
#endif                                                                  /*  __KERNEL_MAIN_FILE          */
#endif                                                                  /*  LW_CFG_KERNEL_LOGO > 0      */

#endif                                                                  /*  __K_GLOBALVAR_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
