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
** 文   件   名: k_class.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 12 日
**
** 描        述: 这是系统内核基本控件类型定义文件。

** BUG
2007.11.04  将所有 posix 相关类型转移到 k_ptype.h 文件.
2007.11.07  加入 select() 功能相关文件与结构变量.
2007.11.13  tcb 中加入等待的线程 tcb 指针.
2007.11.21  将函数指针类型定义放在 k_functype.h 中.
2007.12.22  tcb 中加入 bIsInDeleteProc 来判断线程是否正在被删除.
2007.12.24  tcb 中加入几个备选了句柄, 主要用于网络系统.
2008.01.24  加入 HEAP 锁管理机制.
2008.03.02  LW_CLASS_HOOK 中加入内核重启回调函数.
2008.03.04  中断向量表中加入名字, 方便调试.
2008.03.28  将 sigContext 移出 TCB , TCB通过索引寻找 sigContext, 减少堆栈的使用量.
2008.03.28  TCB 中加入 Delay 和 Wdtimer 专用的链表, 减少 tick 的遍历时间.
2008.03.28  当系统允许信号工作时, TCB 中完成对 eventset node 的保存.
2008.04.12  定时器中加入struct sigevent结构保留 posix 系统兼容性.
2009.04.08  需要多核互斥访问的对象中加入 spinlock 结构.
2009.05.21  堆控制块加入最大使用量统计字段.
2009.07.11  条件变量本身就保证了互斥操作, 所以不需要自旋锁.
2009.09.29  TCB 中加入 FPU 上下文.
2009.11.21  加入线程私有的 io 环境指针.
2009.12.14  加入每一个线程 kernel 利用率计数器.
2011.02.23  事件集加入选项记录.
2011.03.31  加入 vector queue 类型中断向量支持.
2011.08.17  线程上下文不再包含 Clib 中的 reent 信息, 而交由外围统一管理.
2011.11.03  heap 空闲分段使用环形链表管理, 优先使用 free 的内存, 而不是新建的分段.
            此方法主要是为了配合进程虚拟空间堆使用的缺页中断机制.
2013.05.07  去掉线程 ID 控制块, TCB 设置为独立于堆栈的控制块池.
2013.11.14  加入对象资源管理结构.
2013.12.12  修改中断向量表结构.
2015.04.18  加入 timing 参数组.
*********************************************************************************************************/

#ifndef __K_CLASS_H
#define __K_CLASS_H

/*********************************************************************************************************
  错误代号
*********************************************************************************************************/

typedef ULONG             LW_ERROR;                                     /*  系统错误代码                */

/*********************************************************************************************************
  系统定时参数组
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    ULONG                TIMING_ulTickHz;                               /*  系统 tick 频率              */
    ULONG                TIMING_ulNsecPerTick;                          /*  每一次 tick 的纳秒间隔      */
    ULONG                TIMING_ulHTimerHz;                             /*  高速定时器频率              */
    ULONG                TIMING_ulITimerRate;                           /*  应用定时器分辨率            */
    ULONG                TIMING_ulHotplugSec;                           /*  热插拔检测时间              */
    ULONG                TIMING_ulRebootToSec;                          /*  重启超时时间                */
    UINT16               TIMING_usSlice;                                /*  默认时间片 tick 数          */
} LW_CLASS_TIMING;

/*********************************************************************************************************
  系统对象资源结构
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO_HEADER   RESRC_pmonoFreeHeader;                        /*  空闲链表头                  */
    LW_LIST_MONO_HEADER   RESRC_pmonoFreeTail;                          /*  空闲链表尾                  */
    
    UINT                  RESRC_uiUsed;                                 /*  当前使用量                  */
    UINT                  RESRC_uiMaxUsed;                              /*  最大使用量                  */
} LW_CLASS_OBJECT_RESRC;

/*********************************************************************************************************
  HOOK 函数
*********************************************************************************************************/

typedef struct {
    LW_HOOK_FUNC          HOOK_ThreadCreate;                            /*  线程建立钩子                */
    LW_HOOK_FUNC          HOOK_ThreadDelete;                            /*  线程删除钩子                */
    LW_HOOK_FUNC          HOOK_ThreadSwap;                              /*  线程切换钩子                */
    LW_HOOK_FUNC          HOOK_ThreadTick;                              /*  系统时钟中断钩子            */
    LW_HOOK_FUNC          HOOK_ThreadInit;                              /*  线程初始化钩子              */
    LW_HOOK_FUNC          HOOK_ThreadIdle;                              /*  空闲线程钩子                */
    LW_HOOK_FUNC          HOOK_KernelInitBegin;                         /*  内核初始化开始钩子          */
    LW_HOOK_FUNC          HOOK_KernelInitEnd;                           /*  内核初始化结束钩子          */
    LW_HOOK_FUNC          HOOK_KernelReboot;                            /*  内核重新启动                */
    LW_HOOK_FUNC          HOOK_WatchDogTimer;                           /*  看门狗定时器钩子            */
    
    LW_HOOK_FUNC          HOOK_ObjectCreate;                            /*  创建内核对象钩子            */
    LW_HOOK_FUNC          HOOK_ObjectDelete;                            /*  删除内核对象钩子            */
    LW_HOOK_FUNC          HOOK_FdCreate;                                /*  文件描述符创建钩子          */
    LW_HOOK_FUNC          HOOK_FdDelete;                                /*  文件描述符删除钩子          */
    
    LW_HOOK_FUNC          HOOK_CpuIdleEnter;                            /*  CPU 进入空闲模式            */
    LW_HOOK_FUNC          HOOK_CpuIdleExit;                             /*  CPU 退出空闲模式            */
    LW_HOOK_FUNC          HOOK_CpuIntEnter;                             /*  CPU 进入中断(异常)模式      */
    LW_HOOK_FUNC          HOOK_CpuIntExit;                              /*  CPU 退出中断(异常)模式      */
    
    LW_HOOK_FUNC          HOOK_VpCreate;                                /*  进程创建钩子                */
    LW_HOOK_FUNC          HOOK_VpDelete;                                /*  进程删除钩子                */
    
    LW_HOOK_FUNC          HOOK_StkOverflow;                             /*  线程堆栈溢出                */
    LW_HOOK_FUNC          HOOK_FatalError;                              /*  段错误                      */
} LW_CLASS_HOOK;

/*********************************************************************************************************
  中断向量表结构 (这里不保存中断向量优先级和 target cpu 使用时调用 bsp 服务获取)
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE          IACT_plineManage;                             /*  管理链表                    */
    INT64                 IACT_iIntCnt[LW_CFG_MAX_PROCESSORS];          /*  中断计数器                  */
    PINT_SVR_ROUTINE      IACT_pfuncIsr;                                /*  中断服务函数                */
    VOIDFUNCPTR           IACT_pfuncClear;                              /*  中断清理函数                */
    PVOID                 IACT_pvArg;                                   /*  中断服务函数参数            */
    CHAR                  IACT_cInterName[LW_CFG_OBJECT_NAME_SIZE];
} LW_CLASS_INTACT;                                                      /*  中断描述符                  */
typedef LW_CLASS_INTACT  *PLW_CLASS_INTACT;

typedef struct {
    LW_LIST_LINE_HEADER   IDESC_plineAction;                            /*  判断中断服务函数列表        */
    ULONG                 IDESC_ulFlag;                                 /*  中断向量选项                */
    LW_SPINLOCK_DEFINE   (IDESC_slLock);                                /*  自旋锁                      */
} LW_CLASS_INTDESC;
typedef LW_CLASS_INTDESC *PLW_CLASS_INTDESC;

/*********************************************************************************************************
  唤醒表 (差分时间链表)
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE          WUN_lineManage;                               /*  扫描链表                    */
    BOOL                  WUN_bInQ;                                     /*  是否在扫描链中              */
    ULONG                 WUN_ulCounter;                                /*  相对等待时间                */
} LW_CLASS_WAKEUP_NODE;
typedef LW_CLASS_WAKEUP_NODE   *PLW_CLASS_WAKEUP_NODE;

typedef struct {
    LW_LIST_LINE_HEADER   WU_plineHeader;
    PLW_LIST_LINE         WU_plineOp;
} LW_CLASS_WAKEUP;
typedef LW_CLASS_WAKEUP        *PLW_CLASS_WAKEUP;

/*********************************************************************************************************
  定时器控制块
*********************************************************************************************************/
#if ((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)

struct sigevent;
typedef struct {
    LW_LIST_MONO               TIMER_monoResrcList;                     /*  定时器资源表                */
    LW_CLASS_WAKEUP_NODE       TIMER_wunTimer;                          /*  等待唤醒链表                */
#define TIMER_ulCounter        TIMER_wunTimer.WUN_ulCounter
    
    UINT8                      TIMER_ucType;                            /*  定时器类型                  */
    ULONG                      TIMER_ulCounterSave;                     /*  定时器计数值保留值          */
    ULONG                      TIMER_ulOption;                          /*  定时器操作选项              */
    UINT8                      TIMER_ucStatus;                          /*  定时器状态                  */
    PTIMER_CALLBACK_ROUTINE    TIMER_cbRoutine;                         /*  执行函数                    */
    PVOID                      TIMER_pvArg;                             /*  定时器参数                  */
    UINT16                     TIMER_usIndex;                           /*  数组中的索引                */
    
#if LW_CFG_PTIMER_AUTO_DEL_EN > 0
    LW_OBJECT_HANDLE           TIMER_ulTimer;
#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
    
    LW_OBJECT_HANDLE           TIMER_ulThreadId;                        /*  线程 ID                     */
    struct sigevent            TIMER_sigevent;                          /*  定时器信号相关属性          */
                                                                        /*  SIGEV_THREAD 必须使能 POSIX */
    UINT64                     TIMER_u64Overrun;                        /*  timer_getoverrun            */
    clockid_t                  TIMER_clockid;                           /*  仅对 POSIX 定时器有效       */
    
#if LW_CFG_TIMERFD_EN > 0
    PVOID                      TIMER_pvTimerfd;                         /*  timerfd 结构                */
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */
    
    CHAR                       TIMER_cTmrName[LW_CFG_OBJECT_NAME_SIZE]; /*  定时器名                    */
    LW_SPINLOCK_DEFINE        (TIMER_slLock);                           /*  自旋锁                      */
} LW_CLASS_TIMER;
typedef LW_CLASS_TIMER        *PLW_CLASS_TIMER;

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS)         */
/*********************************************************************************************************
  事件集控制块
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

typedef struct {                                                        /*  事件集                      */
    LW_LIST_MONO         EVENTSET_monoResrcList;                        /*  空闲资源表                  */
    UINT8                EVENTSET_ucType;                               /*  类型                        */
    PLW_LIST_LINE        EVENTSET_plineWaitList;                        /*  指向第一个等待线程          */
    ULONG                EVENTSET_ulEventSets;                          /*  32 bit 事件位               */
    ULONG                EVENTSET_ulOption;                             /*  事件集选项                  */
    UINT16               EVENTSET_usIndex;                              /*  数组中的索引                */
    CHAR                 EVENTSET_cEventSetName[LW_CFG_OBJECT_NAME_SIZE];
                                                                        /*  事件标志组名                */
} LW_CLASS_EVENTSET;
typedef LW_CLASS_EVENTSET *PLW_CLASS_EVENTSET;

typedef struct {                                                        /*  事件集节点                  */
    LW_LIST_LINE         EVENTSETNODE_lineManage;                       /*  事件标志组管理表            */
    
    PVOID                EVENTSETNODE_ptcbMe;                           /*  指向等待任务的TCB           */
    PVOID                EVENTSETNODE_pesEventSet;                      /*  指向标志组                  */
    ULONG                EVENTSETNODE_ulEventSets;                      /*  标志组开始等待              */
    UINT8                EVENTSETNODE_ucWaitType;                       /*  等待类型                    */
} LW_CLASS_EVENTSETNODE;
typedef LW_CLASS_EVENTSETNODE   *PLW_CLASS_EVENTSETNODE;
typedef PLW_CLASS_EVENTSETNODE   PLW_EVENTSETNODE;

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  等待队列控制块
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
/*********************************************************************************************************
  这里使用哈希表散列函数使用优先级作为散列的参数.
*********************************************************************************************************/

typedef union { 
    PLW_LIST_RING         WL_pringFifo;                                 /*  基于先入先出的等待队列      */
    PLW_LIST_RING         WL_pringPrio[__EVENT_Q_SIZE];                 /*  基于优先级的等待队列        */
} LW_UNION_WAITLIST;
typedef LW_UNION_WAITLIST *PLW_UNION_WAITLIST;

typedef struct {
    LW_UNION_WAITLIST     WQ_wlQ;                                       /*  等待队列                    */
    UINT16                WQ_usNum;                                     /*  等待队列中的线程个数        */
} LW_CLASS_WAITQUEUE;
typedef LW_CLASS_WAITQUEUE *PLW_CLASS_WAITQUEUE;

/*********************************************************************************************************
  事件控制块 (可用于计数及二进制信号量、互斥信号量量、消息队列、PART内存池)
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO          EVENT_monoResrcList;                          /*  空闲资源表                  */
    UINT8                 EVENT_ucType;                                 /*  事件类型                    */
    PVOID                 EVENT_pvTcbOwn;                               /*  占有资源的TCB指针           */
                                                                        /*  可以在加入死锁检测机制      */
    ULONG                 EVENT_ulCounter;                              /*  计数器值                    */
    ULONG                 EVENT_ulMaxCounter;                           /*  最大技术值                  */
    
    INT                   EVENT_iStatus;
#define EVENT_RW_STATUS_R 0
#define EVENT_RW_STATUS_W 1
    
    ULONG                 EVENT_ulOption;                               /*  事件选项                    */
    UINT8                 EVENT_ucCeilingPriority;                      /*  天花板优先级                */
    PVOID                 EVENT_pvPtr;                                  /*  多用途指针                  */
                                                                        /*  包括二进制信号量消息、      */
                                                                        /*  子控制块包括消息队列，      */
                                                                        /*  内存等                      */
    LW_CLASS_WAITQUEUE    EVENT_wqWaitQ[2];                             /*  双等待队列                  */
#define EVENT_SEM_Q       0
    
#define EVENT_RW_Q_R      0
#define EVENT_RW_Q_W      1

#define EVENT_MSG_Q_R     0
#define EVENT_MSG_Q_S     1
    
    UINT16                EVENT_usIndex;                                /*  缓冲区中的下标              */
    CHAR                  EVENT_cEventName[LW_CFG_OBJECT_NAME_SIZE];    /*  事件名                      */
} LW_CLASS_EVENT;
typedef LW_CLASS_EVENT   *PLW_CLASS_EVENT;

/*********************************************************************************************************
  消息队列定义
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO          MSGQUEUE_monoResrcList;                       /*  空闲资源表                  */
    UINT8                *MSGQUEUE_pucBufferLowAddr;                    /*  消息缓冲区首地址            */ 
                                                                        /*  通过 KERNEL HEAP 分派       */   
    UINT8                *MSGQUEUE_pucBufferHighAddr;                   /*  消息缓冲区最高地址          */
    UINT8                *MSGQUEUE_pucInputPtr;                         /*  入队列指针                  */
    UINT8                *MSGQUEUE_pucOutputPtr;                        /*  出队列指针                  */
    size_t                MSGQUEUE_stEachMsgByteSize;                   /*  每一个消息单元的大小        */
} LW_CLASS_MSGQUEUE;
typedef LW_CLASS_MSGQUEUE   *PLW_CLASS_MSGQUEUE;

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  协程数据结构
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0

typedef struct {
    PLW_STACK             COROUTINE_pstkStackNow;                       /*  线程当前堆栈指针            */
    PLW_STACK             COROUTINE_pstkStackTop;                       /*  线程主堆栈栈顶              */
                                                                        /*  不包括 CRCB 堆栈区          */
    PLW_STACK             COROUTINE_pstkStackBottom;                    /*  线程主堆栈栈底              */
                                                                        /*  不包括 CRCB 堆栈区          */
    size_t                COROUTINE_stStackSize;                        /*  线程堆栈大小(单位：字)      */
                                                                        /*  包括 CRCB 在内的所有堆栈    */
    PLW_STACK             COROUTINE_pstkStackLowAddr;                   /*  总堆栈最低地址              */
    
    LW_LIST_RING          COROUTINE_ringRoutine;                        /*  协程中的协程列表            */
    PVOID                 COROUTINE_pvArg;                              /*  协程运行参数                */
    
    LW_OBJECT_HANDLE      COROUTINE_ulThread;                           /*  所属线程                    */
    ULONG                 COROUTINE_ulFlags;
#define LW_COROUTINE_FLAG_DELETE    0x1                                 /*  需要删除                    */
#define LW_COROUTINE_FLAG_DYNSTK    0x2                                 /*  需要释放堆栈                */
} LW_CLASS_COROUTINE;
typedef LW_CLASS_COROUTINE     *PLW_CLASS_COROUTINE;

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  记事本数据结构
*********************************************************************************************************/
#if  (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)

typedef struct {
    ULONG                NOTEPAD_ulNotePad[LW_CFG_MAX_NOTEPADS];
} LW_CLASS_NOTEPAD;
typedef LW_CLASS_NOTEPAD    *PLW_CLASS_NOTEPAD;

#endif                                                                  /*  LW_CFG_THREAD_NOTE_PAD_EN   */
                                                                        /*  (LW_CFG_MAX_NOTEPADS > 0)   */
/*********************************************************************************************************
  私有化变量控制块
*********************************************************************************************************/

typedef struct {
     LW_LIST_LINE         PRIVATEVAR_lineVarList;                       /*  双向线表                    */
     LW_LIST_MONO         PRIVATEVAR_monoResrcList;                     /*  空闲资源表                  */
                                                                        /*  为了快速计算 将 LW_LIST_LINE*/
                                                                        /*  放在 0 偏移量上             */
     ULONG               *PRIVATEVAR_pulAddress;                        /*  4/8 BYTE 私有化变量地址     */
     ULONG                PRIVATEVAR_ulValueSave;                       /*  保存的变量的值              */
} LW_CLASS_THREADVAR;
typedef LW_CLASS_THREADVAR *PLW_CLASS_THREADVAR;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  线程属性块
*********************************************************************************************************/

typedef struct {
     PLW_STACK            THREADATTR_pstkLowAddr;                       /*  全部堆栈区低内存起始地址    */
     size_t               THREADATTR_stGuardSize;                       /*  堆栈警戒区大小              */
     size_t               THREADATTR_stStackByteSize;                   /*  全部堆栈区大小(字节)        */
     UINT8                THREADATTR_ucPriority;                        /*  线程优先级                  */
     ULONG                THREADATTR_ulOption;                          /*  任务选项                    */
     PVOID                THREADATTR_pvArg;                             /*  线程参数                    */
     PVOID                THREADATTR_pvExt;                             /*  扩展数据段指针              */
} LW_CLASS_THREADATTR;
typedef LW_CLASS_THREADATTR     *PLW_CLASS_THREADATTR;

/*********************************************************************************************************
  线程扩展控制块
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
typedef struct {
    LW_LIST_MONO          CUR_monoNext;                                 /*  单链表下一个节点            */
    VOIDFUNCPTR           CUR_pfuncClean;                               /*  清除函数                    */
    PVOID                 CUR_pvArg;                                    /*  函数参数                    */
} __LW_CLEANUP_ROUTINE;
typedef __LW_CLEANUP_ROUTINE    *__PLW_CLEANUP_ROUTINE;

typedef struct {
    LW_OBJECT_HANDLE      TEX_ulMutex;                                  /*  互斥量                      */
    PLW_LIST_MONO         TEX_pmonoCurHeader;                           /*  cleanup node header         */
} __LW_THREAD_EXT;
typedef __LW_THREAD_EXT  *__PLW_THREAD_EXT;
#endif                                                                  /*  __SYLIXOS_KERNEL            */

typedef struct {
    LW_OBJECT_HANDLE      TCD_ulSignal;                                 /*  等待信号量句柄              */
    LW_OBJECT_HANDLE      TCD_ulMutex;                                  /*  互斥信号量                  */
    ULONG                 TCD_ulCounter;                                /*  引用计数器                  */
} LW_THREAD_COND;
typedef LW_THREAD_COND   *PLW_THREAD_COND;

/*********************************************************************************************************
  浮点运算器上下文
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#if LW_CFG_CPU_FPU_EN > 0
typedef struct {
    ARCH_FPU_CTX          FPUCTX_fpuctxContext;                         /*  体系结构相关 FPU 上下文     */
    ULONG                 FPUCTX_ulReserve[2];                          /*  调试保留                    */
} LW_FPU_CONTEXT;
typedef LW_FPU_CONTEXT   *PLW_FPU_CONTEXT;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

/*********************************************************************************************************
  线程 shell 信息
*********************************************************************************************************/

#if LW_CFG_SHELL_EN > 0
typedef struct {
    ULONG                 SHC_ulShellMagic;                             /*  shell 背景识别号            */
    ULONG                 SHC_ulShellStdFile;                           /*  shell stdfile               */
    ULONG                 SHC_ulShellError;                             /*  shell 系统错误              */
    ULONG                 SHC_ulShellOption;                            /*  shell 设置掩码              */
    addr_t                SHC_ulShellHistoryCtx;                        /*  shell Input History         */
    FUNCPTR               SHC_pfuncShellCallback;                       /*  shell 启动回调              */
    LW_OBJECT_HANDLE      SHC_ulShellMain;                              /*  shell 主线程                */
    addr_t                SHC_ulGetOptCtx;                              /*  getopt() 全局变量切换地址   */
} LW_SHELL_CONTEXT;
typedef LW_SHELL_CONTEXT *PLW_SHELL_CONTEXT;
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

/*********************************************************************************************************
  线程控制块
  
  注意: 浮点运算器上下文指针是否有效, 由 BSP FPU 相关实现决定, 
        用户禁止使用 LW_OPTION_THREAD_USED_FP / LW_OPTION_THREAD_STK_MAIN 选项.
*********************************************************************************************************/

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)                    /*  select() 文件               */
#include "../SylixOS/system/select/select.h"
#endif

typedef struct __lw_tcb {
    PLW_STACK             TCB_pstkStackNow;                             /*  线程当前堆栈指针            */

#if defined(LW_CFG_CPU_ARCH_C6X)
    ULONG                 TCB_ulContextType;                            /*  c6x 上下文类型              */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

    PVOID                 TCB_pvStackFP;                                /*  浮点运算器上下文指针        */
    PVOID                 TCB_pvStackExt;                               /*  扩展堆栈区                  */
    
    LW_LIST_MONO          TCB_monoResrcList;                            /*  空闲资源表                  */
    UINT16                TCB_usIndex;                                  /*  缓冲区中的下标              */
    
    PLW_STACK             TCB_pstkStackTop;                             /*  线程堆栈栈顶(起始点)        */
    PLW_STACK             TCB_pstkStackBottom;                          /*  线程堆栈栈底(结束点)        */
    size_t                TCB_stStackSize;                              /*  线程堆栈大小(单位: 字)      */
    PLW_STACK             TCB_pstkStackLowAddr;                         /*  总堆栈最低地址              */
    
    PLW_STACK             TCB_pstkStackGuard;                           /*  堆栈警戒点                  */
#if LW_CFG_MODULELOADER_EN > 0
    INT                   TCB_iStkLocation;                             /*  堆栈位置                    */
#define LW_TCB_STK_NONE   0
#define LW_TCB_STK_HEAP   1
#define LW_TCB_STK_VMM    2
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
#if LW_CFG_CPU_FPU_EN > 0
    LW_FPU_CONTEXT        TCB_fpuctxContext;                            /*  FPU 上下文                  */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    INT                   TCB_iSchedRet;                                /*  调度器返回的值, signal      */
    ULONG                 TCB_ulOption;                                 /*  线程选项                    */
    LW_OBJECT_ID          TCB_ulId;                                     /*  线程Id                      */
    ULONG                 TCB_ulLastError;                              /*  线程最后一次错误            */
    
    PVOID                 TCB_pvArg;                                    /*  线程入口参数                */
    
    BOOL                  TCB_bDetachFlag;                              /*  线程 DETACH 标志位          */
    PVOID                *TCB_ppvJoinRetValSave;                        /*  JOIN 线程返回值保存地址     */
    PLW_LIST_LINE         TCB_plineJoinHeader;                          /*  其他线程等待自己结束表头    */
    LW_LIST_LINE          TCB_lineJoin;                                 /*  线程合并线表                */
    struct __lw_tcb      *TCB_ptcbJoin;                                 /*  合并的目标线程              */
    ULONG                 TCB_ulJoinType;                               /*  合并类型                    */
    
    LW_LIST_LINE          TCB_lineManage;                               /*  内核管理用线表              */
    LW_LIST_RING          TCB_ringReady;                                /*  相同优先级就绪环            */
    
    LW_CLASS_WAKEUP_NODE  TCB_wunDelay;                                 /*  等待节点                    */
#define TCB_ulDelay       TCB_wunDelay.WUN_ulCounter
    
#if (LW_CFG_THREAD_POOL_EN > 0) && ( LW_CFG_MAX_THREAD_POOLS > 0)       /*  线程池表                    */
    LW_LIST_RING          TCB_ringThreadPool;                           /*  线程池表                    */
#endif
    
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    INT                   TCB_iPendQ;
    LW_LIST_RING          TCB_ringEvent;                                /*  事件等待队列表              */
    PLW_CLASS_EVENT       TCB_peventPtr;                                /*  等待事件指针                */
    PLW_LIST_RING        *TCB_ppringPriorityQueue;                      /*  在 PRIORITY 队列位置        */
                                                                        /*  由于删除线程时需要解等待链，*/
                                                                        /*  这时需要确定队列位置        */
#endif

#if LW_CFG_COROUTINE_EN > 0
    LW_CLASS_COROUTINE    TCB_crcbOrigent;                              /*  协程起源点                  */
    PLW_LIST_RING         TCB_pringCoroutineHeader;                     /*  协程首地址                  */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

                                                                        /*  调度器相关部分              */
    LW_SPINLOCK_DEFINE   (TCB_slLock);                                  /*  自旋锁                      */
    
#if LW_CFG_SMP_EN > 0
    BOOL                  TCB_bCPULock;                                 /*  是否锁定运行 CPU ID         */
    ULONG                 TCB_ulCPULock;                                /*  锁定运行的 CPU ID           */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    volatile ULONG        TCB_ulCPUId;                                  /*  如果正在运行, 表示运行 CPU  */
    volatile BOOL         TCB_bIsCand;                                  /*  是否在候选运行表中          */

#if LW_CFG_SMP_EN > 0
    struct __lw_tcb      *TCB_ptcbWaitStatus;                           /*  等待状态修改的目标线程      */
    PLW_LIST_LINE         TCB_plineStatusReqHeader;                     /*  等待修改本线程状态          */
    LW_LIST_LINE          TCB_lineStatusPend;                           /*  等待目标线程状态修改        */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    UINT                  TCB_uiStatusChangeReq;                        /*  状态转变请求(并用于反馈结果)*/
    ULONG                 TCB_ulStopNesting;                            /*  停止嵌套层数                */

#define LW_TCB_REQ_SUSPEND          1                                   /*  请求阻塞                    */
#define LW_TCB_REQ_STOP             2                                   /*  请求停止                    */
#define LW_TCB_REQ_WDEATH           3                                   /*  请求删除                    */

    UINT8                 TCB_ucSchedPolicy;                            /*  调度策略                    */
    UINT8                 TCB_ucSchedActivate;                          /*  在同优先级中线程优先级      */
    UINT8                 TCB_ucSchedActivateMode;                      /*  线程就绪方式                */
    
    UINT16                TCB_usSchedSlice;                             /*  线程时间片保存值            */
    UINT16                TCB_usSchedCounter;                           /*  线程当前剩余时间片          */
    
    UINT8                 TCB_ucPriority;                               /*  线程优先级                  */
    UINT16                TCB_usStatus;                                 /*  线程当前状态                */
    
    UINT8                 TCB_ucWaitTimeout;                            /*  等待超时                    */
    UINT8                 TCB_ucIsEventDelete;                          /*  事件是否被删除              */
    
    ULONG                 TCB_ulSuspendNesting;                         /*  线程挂起嵌套层数            */
    INT                   TCB_iDeleteProcStatus;                        /*  是否正在被删除或者重启      */
#define LW_TCB_DELETE_PROC_NONE     0                                   /*  没有进入删除流程            */
#define LW_TCB_DELETE_PROC_DEL      1                                   /*  正在被删除                  */
#define LW_TCB_DELETE_PROC_RESTART  2                                   /*  正在被重启                  */
    
    PVOID                 TCB_pvMsgBoxMessage;                          /*  邮箱传来的信息              */
    PVOID                 TCB_pvRetValue;                               /*  线程暂存的返回值            */

#if LW_CFG_MSGQUEUE_EN > 0
    PVOID                 TCB_pvMsgQueueMessage;                        /*  保存用户消息缓冲地址        */
    size_t                TCB_stMaxByteSize;                            /*  缓冲区大小                  */
    size_t               *TCB_pstMsgByteSize;                           /*  消息长短指针                */
    ULONG                 TCB_ulRecvOption;                             /*  消息队列接收选项            */
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
#if (LW_CFG_THREAD_DEL_EN > 0) || (LW_CFG_THREAD_RESTART_EN > 0) || (LW_CFG_SIGNAL_EN > 0)
    PLW_EVENTSETNODE      TCB_pesnPtr;                                  /*  指向该线程等待              */
#endif
    ULONG                 TCB_ulEventSets;                              /*  用户等待的 EVENTSET         */
#endif

#if LW_CFG_THREAD_CANCEL_EN > 0
    BOOL                  TCB_bCancelRequest;                           /*  取消请求                    */
    INT                   TCB_iCancelState;                             /*  取消状态                    */
    INT                   TCB_iCancelType;                              /*  取消类型                    */
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
    
#if LW_CFG_THREAD_RESTART_EN > 0
    PTHREAD_START_ROUTINE TCB_pthreadStartAddress;                      /*  线程重新启动时的启动地址    */
#endif

    ULONG                 TCB_ulCPUTicks;                               /*  从线程创建起 CPU ticks      */
    ULONG                 TCB_ulCPUKernelTicks;                         /*  从线程创建起 CPU kernel tick*/

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    ULONG                 TCB_ulCPUUsageTicks;                          /*  该线程 CPU 利用率计时器     */
    ULONG                 TCB_ulCPUUsageKernelTicks;                    /*  利用时间中, 内核占用的时间  */
#endif

#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0                                     /*  系统线程看门狗              */
    LW_CLASS_WAKEUP_NODE  TCB_wunWatchDog;                              /*  看门狗等待节点              */
#define TCB_bWatchDogInQ  TCB_wunWatchDog.WUN_bInQ
#define TCB_ulWatchDog    TCB_wunWatchDog.WUN_ulCounter
#endif
    
    volatile ULONG        TCB_ulThreadLockCounter;                      /*  线程锁定计数器              */
    volatile ULONG        TCB_ulThreadSafeCounter;                      /*  线程安全模式标志            */
    
#if LW_CFG_THREAD_DEL_EN > 0
    struct __lw_tcb      *TCB_ptcbDeleteMe;                             /*  要将本线程删除的 TCB        */
    struct __lw_tcb      *TCB_ptcbDeleteWait;                           /*  等待对方删除的 TCB          */
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */

    UINT8                 TCB_ucStackAutoAllocFlag;                     /*  堆栈是否有系统在堆中开辟    */

#if (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)
    LW_CLASS_NOTEPAD      TCB_notepadThreadNotePad;                     /*  线程记事本                  */
#endif

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
    PLW_LIST_LINE         TCB_plinePrivateVars;                         /*  全局变量私有化线表          */
#endif

    CHAR                  TCB_cThreadName[LW_CFG_OBJECT_NAME_SIZE];     /*  线程名                      */

/*********************************************************************************************************
  POSIX EXT
*********************************************************************************************************/

#if LW_CFG_THREAD_EXT_EN > 0
    __LW_THREAD_EXT       TCB_texExt;                                   /*  线程扩展数据部分            */
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

/*********************************************************************************************************
  SYSTEM (每个线程拥有自己的私有标准文件描述符, 其优先级高于全局标准文件描述符)
  默认所有线程都不拥有私有的 io 环境, 只有调用了 ioPrivateEvn() 后才拥有自己独立的 io 环境.
*********************************************************************************************************/

#if (LW_CFG_DEVICE_EN > 0)
    PVOID                 TCB_pvIoEnv;                                  /*  线程私有 io 环境            */
    INT                   TCB_iTaskStd[3];                              /*  标准 in out err 文件描述符  */
#endif

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
    LW_SEL_CONTEXT       *TCB_pselctxContext;                           /*  select 上下文               */
#endif

/*********************************************************************************************************
  POSIX 扩展区
*********************************************************************************************************/

    PVOID                 TCB_pvPosixContext;                           /*  pthread 相关                */

/*********************************************************************************************************
  vprocess 扩展区
*********************************************************************************************************/

    atomic_t              TCB_atomicKernelSpace;                        /*  是否在内核空间              */
    PVOID                 TCB_pvVProcessContext;                        /*  进程上下文                  */
#if LW_CFG_MODULELOADER_EN > 0
    LW_LIST_LINE          TCB_lineProcess;
#endif                                                                  /*  进程表                      */
    
/*********************************************************************************************************
  缺页中断信息
*********************************************************************************************************/

    INT64                 TCB_i64PageFailCounter;                       /*  缺页中断次数                */

/*********************************************************************************************************
  TCB 综合区扩展区
*********************************************************************************************************/

#if LW_CFG_SHELL_EN > 0
    LW_SHELL_CONTEXT      TCB_shc;                                      /*  shell 信息                  */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

/*********************************************************************************************************
  GDB 扩展区
*********************************************************************************************************/

#if LW_CFG_GDB_EN > 0
    addr_t                TCB_ulStepAddr;                               /*  单步地址，-1 表示非单步模式 */
    ULONG                 TCB_ulStepInst;                               /*  单步地址指令备份            */
    BOOL                  TCB_bStepClear;                               /*  单步断点是否被清除          */
#endif

/*********************************************************************************************************
  TCB 权限管理扩展
*********************************************************************************************************/
    
    uid_t                 TCB_uid;                                      /*  用户                        */
    gid_t                 TCB_gid;
    
    uid_t                 TCB_euid;                                     /*  执行                        */
    gid_t                 TCB_egid;
    
    uid_t                 TCB_suid;                                     /*  设置                        */
    gid_t                 TCB_sgid;
    
    gid_t                 TCB_suppgid[16];                              /*  附加组 (16 MAX)             */
    UINT                  TCB_iNumSuppGid;
    
/*********************************************************************************************************
  MIPS FPU 模拟
*********************************************************************************************************/

#ifdef LW_CFG_CPU_ARCH_MIPS
    ULONG                 TCB_ulBdEmuBranchPC;                          /*  分支的 PC                   */
    ULONG                 TCB_ulBdEmuContPC;                            /*  恢复的 PC                   */
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */

/*********************************************************************************************************
  TCB 保留扩展
*********************************************************************************************************/

    PVOID                 TCB_pvReserveContext;

} LW_CLASS_TCB;
typedef LW_CLASS_TCB     *PLW_CLASS_TCB;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  线程信息
*********************************************************************************************************/

typedef struct __lw_tcb_desc {
    PLW_STACK             TCBD_pstkStackNow;                            /*  线程当前堆栈指针            */
    PVOID                 TCBD_pvStackFP;                               /*  浮点运算器上下文指针        */
    PVOID                 TCBD_pvStackExt;                              /*  扩展堆栈区                  */
    
    PLW_STACK             TCBD_pstkStackTop;                            /*  线程堆栈栈顶(起始点)        */
    PLW_STACK             TCBD_pstkStackBottom;                         /*  线程堆栈栈底(结束点)        */
    size_t                TCBD_stStackSize;                             /*  线程堆栈大小(单位：字)      */
    PLW_STACK             TCBD_pstkStackLowAddr;                        /*  总堆栈最低地址              */
    PLW_STACK             TCBD_pstkStackGuard;                          /*  警戒区指针                  */
    
    INT                   TCBD_iSchedRet;                               /*  调度器返回的值, signal      */
    ULONG                 TCBD_ulOption;                                /*  线程选项                    */
    LW_OBJECT_ID          TCBD_ulId;                                    /*  线程Id                      */
    ULONG                 TCBD_ulLastError;                             /*  线程最后一次错误            */
    
    BOOL                  TCBD_bDetachFlag;                             /*  线程 DETACH 标志位          */
    ULONG                 TCBD_ulJoinType;                              /*  合并类型                    */
    
    ULONG                 TCBD_ulWakeupLeft;                            /*  等待时间                    */
    
    ULONG                 TCBD_ulCPUId;                                 /*  优先使用的 CPU 号           */
                                                                        /*  如果正在运行, 表示运行 CPU  */
    BOOL                  TCBD_bIsCand;                                 /*  是否在候选运行表中          */
    
    UINT8                 TCBD_ucSchedPolicy;                           /*  调度策略                    */
    UINT8                 TCBD_ucSchedActivate;                         /*  在同优先级中线程优先级      */
    UINT8                 TCBD_ucSchedActivateMode;                     /*  线程就绪方式                */
    
    UINT16                TCBD_usSchedSlice;                            /*  线程时间片保存值            */
    UINT16                TCBD_usSchedCounter;                          /*  线程当前剩余时间片          */
    
    UINT8                 TCBD_ucPriority;                              /*  线程优先级                  */
    UINT16                TCBD_usStatus;                                /*  线程当前状态                */
    
    UINT8                 TCBD_ucWaitTimeout;                           /*  等待超时                    */
    
    ULONG                 TCBD_ulSuspendNesting;                        /*  线程挂起嵌套层数            */
    INT                   TCBD_iDeleteProcStatus;                       /*  是否正在被删除或者重启      */
    
    BOOL                  TCBD_bCancelRequest;                          /*  取消请求                    */
    INT                   TCBD_iCancelState;                            /*  取消状态                    */
    INT                   TCBD_iCancelType;                             /*  取消类型                    */
    
    PTHREAD_START_ROUTINE TCBD_pthreadStartAddress;                     /*  线程重新启动时的启动地址    */
    
    ULONG                 TCBD_ulCPUTicks;                              /*  从线程创建起 CPU ticks      */
    ULONG                 TCBD_ulCPUKernelTicks;                        /*  从线程创建起 CPU kernel tick*/

    ULONG                 TCBD_ulCPUUsageTicks;                         /*  该线程 CPU 利用率计时器     */
    ULONG                 TCBD_ulCPUUsageKernelTicks;                   /*  利用时间中, 内核占用的时间  */
    
    ULONG                 TCBD_ulWatchDogLeft;                          /*  看门狗剩余时间              */
    
    ULONG                 TCBD_ulThreadLockCounter;                     /*  线程锁定计数器              */
    ULONG                 TCBD_ulThreadSafeCounter;                     /*  线程安全模式标志            */
    
    UINT8                 TCBD_ucStackAutoAllocFlag;                    /*  堆栈是否有系统在堆中开辟    */
    
    CHAR                  TCBD_cThreadName[LW_CFG_OBJECT_NAME_SIZE];    /*  线程名                      */
    
    PVOID                 TCBD_pvVProcessContext;                       /*  进程上下文                  */
    
    INT64                 TCBD_i64PageFailCounter;                      /*  缺页中断次数                */
    
    uid_t                 TCBD_uid;                                     /*  用户                        */
    gid_t                 TCBD_gid;
    
    uid_t                 TCBD_euid;                                    /*  执行                        */
    gid_t                 TCBD_egid;
    
    uid_t                 TCBD_suid;                                    /*  设置                        */
    gid_t                 TCBD_sgid;
    
    gid_t                 TCBD_suppgid[16];                             /*  附加组 (16 MAX)             */
    UINT                  TCBD_iNumSuppGid;
} LW_CLASS_TCB_DESC;
typedef LW_CLASS_TCB_DESC   *PLW_CLASS_TCB_DESC;

/*********************************************************************************************************
  位图表
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    volatile UINT32       BMAP_uiMap;                                   /*  主位图掩码                  */
    volatile UINT32       BMAP_uiSubMap[(LW_PRIO_LOWEST >> 5) + 1];     /*  辅位图掩码                  */
} LW_CLASS_BMAP;
typedef LW_CLASS_BMAP    *PLW_CLASS_BMAP;

/*********************************************************************************************************
  优先级控制块
*********************************************************************************************************/

typedef struct {
    LW_LIST_RING_HEADER   PCB_pringReadyHeader;                         /*  就绪非运行线程环表          */
    UINT8                 PCB_ucPriority;                               /*  优先级                      */
} LW_CLASS_PCB;
typedef LW_CLASS_PCB     *PLW_CLASS_PCB;

/*********************************************************************************************************
  就绪表
*********************************************************************************************************/

typedef struct {
    LW_CLASS_BMAP         PCBM_bmap;
    LW_CLASS_PCB          PCBM_pcb[LW_PRIO_LOWEST + 1];
} LW_CLASS_PCBBMAP;
typedef LW_CLASS_PCBBMAP *PLW_CLASS_PCBBMAP;

/*********************************************************************************************************
  RATE MONO SCHEDLER
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO         RMS_monoResrcList;                             /*  空闲资源表                  */
    
    UINT8                RMS_ucType;
    UINT8                RMS_ucStatus;                                  /*  状态                        */
    ULONG                RMS_ulTickNext;                                /*  激活时间                    */
    ULONG                RMS_ulTickSave;                                /*  系统时间保存                */
    PLW_CLASS_TCB        RMS_ptcbOwner;                                 /*  所有者                      */
    
    UINT16               RMS_usIndex;                                   /*  下标                        */

    CHAR                 RMS_cRmsName[LW_CFG_OBJECT_NAME_SIZE];         /*  名字                        */
} LW_CLASS_RMS;
typedef LW_CLASS_RMS    *PLW_CLASS_RMS;

/*********************************************************************************************************
  PARTITION 控制块
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO         PARTITION_monoResrcList;                       /*  空闲资源表                  */
    
    UINT8                PARTITION_ucType;                              /*  类型标志                    */
    PLW_LIST_MONO        PARTITION_pmonoFreeBlockList;                  /*  空闲内存块表                */
    size_t               PARTITION_stBlockByteSize;                     /*  每一块的大小                */
                                                                        /*  必须大于 sizeof(PVOID)      */
    ULONG                PARTITION_ulBlockCounter;                      /*  块数量                      */
    ULONG                PARTITION_ulFreeBlockCounter;                  /*  空闲块数量                  */
    
    UINT16               PARTITION_usIndex;                             /*  缓冲区索引                  */
    
    CHAR                 PARTITION_cPatitionName[LW_CFG_OBJECT_NAME_SIZE];
                                                                        /*  名字                        */
    LW_SPINLOCK_DEFINE  (PARTITION_slLock);                             /*  自旋锁                      */
} LW_CLASS_PARTITION;
typedef LW_CLASS_PARTITION   *PLW_CLASS_PARTITION;

/*********************************************************************************************************
  堆类型定义 (空闲链表使用 ring 结构)
*********************************************************************************************************/

typedef struct {                                                        /*  堆操作                      */
    LW_LIST_MONO         HEAP_monoResrcList;                            /*  空闲资源表                  */
    PLW_LIST_RING        HEAP_pringFreeSegment;                         /*  空闲段表                    */

#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    LW_OBJECT_HANDLE     HEAP_ulLock;                                   /*  使用 semaphore 作为锁       */
#endif                                                                  /*  (LW_CFG_SEMB_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
    PVOID                HEAP_pvStartAddress;                           /*  堆内存起始地址              */
    size_t               HEAP_stTotalByteSize;                          /*  堆内存总大小                */
    
    ULONG                HEAP_ulSegmentCounter;                         /*  分段数量                    */
    size_t               HEAP_stUsedByteSize;                           /*  使用的字节数量              */
    size_t               HEAP_stFreeByteSize;                           /*  空闲的字节数量              */
    size_t               HEAP_stMaxUsedByteSize;                        /*  最大使用的字节数量          */
    
    UINT16               HEAP_usIndex;                                  /*  堆缓冲索引                  */
    
    CHAR                 HEAP_cHeapName[LW_CFG_OBJECT_NAME_SIZE];       /*  名字                        */
    LW_SPINLOCK_DEFINE  (HEAP_slLock);                                  /*  自旋锁                      */
} LW_CLASS_HEAP;
typedef LW_CLASS_HEAP   *PLW_CLASS_HEAP;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  堆内分段类型, 注意: 由于是否使用标志和空闲队列复用free list, 所以为了使对齐内存释放时准确判断, 
                      判断是否空闲的标志必须在分段信息的最后一个元素.
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE         SEGMENT_lineManage;                            /*  左右邻居指针                */
    LW_LIST_RING         SEGMENT_ringFreeList;                          /*  下一个空闲分段链表          */
    size_t               SEGMENT_stByteSize;                            /*  分段大小                    */
    size_t               SEGMENT_stMagic;                               /*  分段标识                    */
} LW_CLASS_SEGMENT;
typedef LW_CLASS_SEGMENT    *PLW_CLASS_SEGMENT;

/*********************************************************************************************************
  候选运行表结构
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    volatile PLW_CLASS_TCB  CAND_ptcbCand;                              /*  候选运行线程                */
    volatile BOOL           CAND_bNeedRotate;                           /*  可能产生了优先级卷绕        */
} LW_CLASS_CAND;
typedef LW_CLASS_CAND      *PLW_CLASS_CAND;

/*********************************************************************************************************
  内核锁结构
*********************************************************************************************************/

typedef struct {
    LW_SPINLOCK_DEFINE     (KERN_slLock);
    PVOID                   KERN_pvCpuOwner;
    LW_OBJECT_HANDLE        KERN_ulKernelOwner;
    CPCHAR                  KERN_pcKernelEnterFunc;
} LW_CLASS_KERNLOCK;
typedef LW_CLASS_KERNLOCK  *PLW_CLASS_KERNLOCK;

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __K_CLASS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
