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
** 文   件   名: k_api.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统内部 API 函数声明文件

** BUG
2007.04.08  加入了对裁剪的宏支持
2008.01.16  加入了 API_ThreadForceDelete() 函数.
2008.01.16  加入了 API_ThreadIsSafe() 函数.
2008.01.20  API_ThreadIsLock() 修改了返回类型.
*********************************************************************************************************/

#ifndef  __K_API_H
#define  __K_API_H

/*********************************************************************************************************
  uid gid
*********************************************************************************************************/

LW_API gid_t            getgid(void);
LW_API int              setgid(gid_t  gid);
LW_API gid_t            getegid(void);
LW_API int              setegid(gid_t  egid);
LW_API uid_t            getuid(void);
LW_API int              setuid(uid_t  uid);
LW_API uid_t            geteuid(void);
LW_API int              seteuid(uid_t  euid);

LW_API int              setgroups(int groupsun, const gid_t grlist[]);
LW_API int              getgroups(int groupsize, gid_t grlist[]);

LW_API int              getlogin_r(char *name, size_t namesize);
LW_API char            *getlogin(void);

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/

LW_API INT              API_AtomicAdd(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicSub(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicInc(atomic_t  *patomic);

LW_API INT              API_AtomicDec(atomic_t  *patomic);

LW_API INT              API_AtomicAnd(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicNand(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicOr(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicXor(INT  iVal, atomic_t  *patomic);

LW_API VOID             API_AtomicSet(INT  iVal, atomic_t  *patomic);

LW_API INT              API_AtomicGet(atomic_t  *patomic);

LW_API INT              API_AtomicSwp(INT  iVal, atomic_t  *patomic);

LW_API VOID             API_AtomicLock(INTREG  *piregInterLevel);

LW_API VOID             API_AtomicUnlock(INTREG  iregInterLevel);

/*********************************************************************************************************
  CPU
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_SMP_EN > 0
LW_API ULONG            API_CpuUp(ULONG  ulCPUId);

#if LW_CFG_SMP_CPU_DOWN_EN > 0
LW_API ULONG            API_CpuDown(ULONG  ulCPUId);
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */

LW_API BOOL             API_CpuIsUp(ULONG  ulCPUId);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#if LW_CFG_POWERM_EN > 0
LW_API ULONG            API_CpuPowerSet(UINT  uiPowerLevel);

LW_API ULONG            API_CpuPowerGet(UINT  *puiPowerLevel);
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API ULONG            API_CpuBogoMips(ULONG  ulCPUId, ULONG  *pulKInsPerSec);

/*********************************************************************************************************
  SPINLOCK (此 API 仅供内核程序使用)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API INT              API_SpinInit(spinlock_t *psl);

LW_API INT              API_SpinDestory(spinlock_t *psl);

LW_API INT              API_SpinLock(spinlock_t *psl);

LW_API INT              API_SpinLockIrq(spinlock_t *psl, INTREG  *iregInterLevel);

LW_API INT              API_SpinLockQuick(spinlock_t *psl, INTREG  *iregInterLevel);

LW_API INT              API_SpinTryLock(spinlock_t *psl);

LW_API INT              API_SpinTryLockIrq(spinlock_t *psl, INTREG  *iregInterLevel);

LW_API INT              API_SpinUnlock(spinlock_t *psl);

LW_API INT              API_SpinUnlockIrq(spinlock_t *psl, INTREG  iregInterLevel);

LW_API INT              API_SpinUnlockQuick(spinlock_t *psl, INTREG  iregInterLevel);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  OBJECT
*********************************************************************************************************/

LW_API UINT8            API_ObjectGetClass(LW_OBJECT_HANDLE  ulId);     /*  获得对象类型                */

LW_API BOOL             API_ObjectIsGlobal(LW_OBJECT_HANDLE  ulId);     /*  获得对象是否为全局对象      */

LW_API ULONG            API_ObjectGetNode(LW_OBJECT_HANDLE  ulId);      /*  获得对象所在的处理器号      */

LW_API ULONG            API_ObjectGetIndex(LW_OBJECT_HANDLE  ulId);     /*  获得对象缓冲区内地址        */

#if LW_CFG_OBJECT_SHARE_EN > 0
LW_API ULONG            API_ObjectShareAdd(LW_OBJECT_HANDLE  ulId, UINT64  u64Key);

LW_API ULONG            API_ObjectShareDelete(UINT64  u64Key);

LW_API LW_OBJECT_HANDLE API_ObjectShareFind(UINT64  u64Key);
#endif                                                                  /*  LW_CFG_OBJECT_SHARE_EN > 0  */

/*********************************************************************************************************
  THREAD
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
LW_API ULONG            API_ThreadSetAffinity(LW_OBJECT_HANDLE        ulId, 
                                              size_t                  stSize, 
                                              const PLW_CLASS_CPUSET  pcpuset);
                                                                        /*  设置线程 CPU 亲和度         */
LW_API ULONG            API_ThreadGetAffinity(LW_OBJECT_HANDLE        ulId, 
                                              size_t                  stSize, 
                                              PLW_CLASS_CPUSET        pcpuset);
                                                                        /*  获取线程 CPU 亲和度         */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

LW_API  
LW_CLASS_THREADATTR     API_ThreadAttrGetDefault(VOID);                 /*  获得默认线程属性块          */

LW_API  
LW_CLASS_THREADATTR     API_ThreadAttrGet(LW_OBJECT_HANDLE  ulId);      /*  获得指定线程当前属性块      */

LW_API ULONG            API_ThreadAttrBuild(PLW_CLASS_THREADATTR    pthreadattr,
                                            size_t                  stStackByteSize, 
                                            UINT8                   ucPriority, 
                                            ULONG                   ulOption, 
                                            PVOID                   pvArg);
                                                                        /*  生成一个线程属性块          */
                                            
LW_API ULONG            API_ThreadAttrBuildEx(PLW_CLASS_THREADATTR    pthreadattr,
                                              PLW_STACK               pstkStackTop, 
                                              size_t                  stStackByteSize, 
                                              UINT8                   ucPriority, 
                                              ULONG                   ulOption, 
                                              PVOID                   pvArg,
                                              PVOID                   pvExt);
                                                                        /*  生成一个高级线程属性块      */

LW_API ULONG            API_ThreadAttrBuildFP(PLW_CLASS_THREADATTR    pthreadattr,
                                              PVOID                   pvFP);
                                                                        /*  配置浮点运算器上下文        */

LW_API ULONG            API_ThreadAttrSetGuardSize(PLW_CLASS_THREADATTR    pthreadattr,
                                                   size_t                  stGuardSize);
                                                                        /*  设置堆栈警戒区大小          */
LW_API ULONG            API_ThreadAttrSetStackSize(PLW_CLASS_THREADATTR    pthreadattr,
                                                   size_t                  stStackByteSize);
                                                                        /*  设置堆栈大小                */
LW_API ULONG            API_ThreadAttrSetArg(PLW_CLASS_THREADATTR    pthreadattr,
                                             PVOID                   pvArg);
                                                                        /*  设置线程启动参数            */

LW_API LW_OBJECT_HANDLE API_ThreadInit(CPCHAR                   pcName,
                                       PTHREAD_START_ROUTINE    pfuncThread,
                                       PLW_CLASS_THREADATTR     pthreadattr,
                                       LW_OBJECT_ID            *pulId); /*  线程初始化                  */
                                       
LW_API LW_OBJECT_HANDLE API_ThreadCreate(CPCHAR                   pcName,
                                         PTHREAD_START_ROUTINE    pfuncThread,
                                         PLW_CLASS_THREADATTR     pthreadattr,
                                         LW_OBJECT_ID            *pulId);
                                                                        /*  建立一个线程                */
#if LW_CFG_THREAD_DEL_EN > 0
LW_API ULONG            API_ThreadExit(PVOID  pvRetVal);                /*  线程自行退出                */

LW_API int              atexit(void (*func)(void));

LW_API void             exit(int  iCode);

LW_API void             _exit(int  iCode);

LW_API void             _Exit(int  iCode);

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */

#if LW_CFG_THREAD_DEL_EN > 0
LW_API ULONG            API_ThreadDelete(LW_OBJECT_HANDLE  *pulId, 
                                         PVOID  pvRetVal);              /*  删除一个线程                */

LW_API ULONG            API_ThreadForceDelete(LW_OBJECT_HANDLE  *pulId, 
                                              PVOID pvRetVal);          /*  强制删除一个线程            */
#endif

#if LW_CFG_THREAD_RESTART_EN > 0
LW_API ULONG            API_ThreadRestart(LW_OBJECT_HANDLE  ulId, 
                                          PVOID  pvArg);                /*  线程重新启动                */
                                          
LW_API ULONG            API_ThreadRestartEx(LW_OBJECT_HANDLE       ulId, 
                                            PTHREAD_START_ROUTINE  pfuncThread, 
                                            PVOID                  pvArg);
#endif

LW_API LW_OBJECT_HANDLE API_ThreadIdSelf(VOID);                         /*  获得线程自己的句柄          */

#ifdef __SYLIXOS_KERNEL
LW_API PLW_CLASS_TCB    API_ThreadTcbSelf(VOID);                        /*  获得线程自己的 TCB          */

LW_API LW_OBJECT_HANDLE API_ThreadIdInter(VOID);                        /*  获得被中断线程 ID           */

LW_API PLW_CLASS_TCB    API_ThreadTcbInter(VOID);                       /*  获得被中断线程 TCB          */
#endif                                                                  /*  只在内核模块中使用          */

LW_API ULONG            API_ThreadDesc(LW_OBJECT_HANDLE     ulId, 
                                       PLW_CLASS_TCB_DESC   ptcbdesc);  /*  获得线程概要信息            */

LW_API ULONG            API_ThreadStart(LW_OBJECT_HANDLE    ulId);      /*  启动一个已经初始化的线程    */

LW_API ULONG            API_ThreadStartEx(LW_OBJECT_HANDLE  ulId, 
                                          BOOL              bJoin, 
                                          PVOID            *ppvRetValAddr);

LW_API ULONG            API_ThreadJoin(LW_OBJECT_HANDLE  ulId, 
                                       PVOID  *ppvRetValAddr);          /*  线程合并                    */

LW_API ULONG            API_ThreadDetach(LW_OBJECT_HANDLE  ulId);       /*  禁止指定线程合并            */

LW_API ULONG            API_ThreadSafe(VOID);                           /*  进入安全模式                */

LW_API ULONG            API_ThreadUnsafe(VOID);                         /*  退出安全模式                */

LW_API BOOL             API_ThreadIsSafe(LW_OBJECT_HANDLE  ulId);       /*  检测目的线程是否安全        */

#if LW_CFG_THREAD_SUSPEND_EN > 0
LW_API ULONG            API_ThreadSuspend(LW_OBJECT_HANDLE    ulId);    /*  线程挂起                    */

LW_API ULONG            API_ThreadResume(LW_OBJECT_HANDLE    ulId);     /*  线程挂起恢复                */
#endif

#if LW_CFG_THREAD_SUSPEND_EN > 0
LW_API ULONG            API_ThreadForceResume(LW_OBJECT_HANDLE   ulId); /*  强制恢复挂起的线程          */

LW_API ULONG            API_ThreadIsSuspend(LW_OBJECT_HANDLE    ulId);  /*  检查线程是否被挂起          */
#endif

LW_API BOOL             API_ThreadIsReady(LW_OBJECT_HANDLE    ulId);    /*  检查线程是否就绪            */

LW_API ULONG            API_ThreadIsRunning(LW_OBJECT_HANDLE   ulId, BOOL  *pbIsRunning);
                                                                        /*  检查线程是否正在运行        */
LW_API ULONG            API_ThreadSetName(LW_OBJECT_HANDLE  ulId, 
                                          CPCHAR            pcName);    /*  设置线程名                  */

LW_API ULONG            API_ThreadGetName(LW_OBJECT_HANDLE  ulId, 
                                          PCHAR             pcName);    /*  获得线程名                  */

LW_API VOID             API_ThreadLock(VOID);                           /*  线程锁定(调度器锁定)        */

LW_API INT              API_ThreadUnlock(VOID);                         /*  线程解锁(调度器解锁)        */

LW_API ULONG            API_ThreadIsLock(VOID);                         /*  检测是否锁定调度器          */

#if LW_CFG_THREAD_CHANGE_PRIO_EN > 0
LW_API ULONG            API_ThreadSetPriority(LW_OBJECT_HANDLE  ulId, 
                                              UINT8  ucPriority);       /*  设置线程优先级              */
                                              
LW_API ULONG            API_ThreadGetPriority(LW_OBJECT_HANDLE  ulId, 
                                              UINT8            *pucPriority);
                                                                        /*  获得线程优先级              */
#endif

LW_API ULONG            API_ThreadSetSlice(LW_OBJECT_HANDLE  ulId, 
                                           UINT16            usSlice);  /*  设置线程时间片              */

LW_API ULONG            API_ThreadGetSlice(LW_OBJECT_HANDLE  ulId, 
                                           UINT16           *pusSlice); /*  获得线程时间片              */

LW_API ULONG            API_ThreadGetSliceEx(LW_OBJECT_HANDLE  ulId, 
                                             UINT16           *pusSlice, 
                                             UINT16           *pusCounter);
                                                                        /*  获得当前时间片              */

LW_API ULONG            API_ThreadSetSchedParam(LW_OBJECT_HANDLE  ulId,
                                                UINT8             ucPolicy,
                                                UINT8             ucActivatedMode);
                                                                        /*  设置调度器参数              */
                                                 
LW_API ULONG            API_ThreadGetSchedParam(LW_OBJECT_HANDLE  ulId,
                                                UINT8            *pucPolicy,
                                                UINT8            *pucActivatedMode);
                                                                        /*  获得调度器参数              */

#if (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)
LW_API ULONG            API_ThreadSetNotePad(LW_OBJECT_HANDLE  ulId,
                                             UINT8             ucNodeIndex,
                                             ULONG             ulVal);  /*  设置线程记事本              */
                                             
LW_API ULONG            API_ThreadGetNotePad(LW_OBJECT_HANDLE  ulId,
                                             UINT8             ucNodeIndex);
                                                                        /*  获得线程记事本              */
#endif

#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
LW_API ULONG            API_ThreadFeedWatchDog(ULONG  ulWatchDogTicks); /*  喂养线程看门狗              */

LW_API VOID             API_ThreadCancelWatchDog(VOID);                 /*  关闭线程看门狗              */
#endif

LW_API ULONG            API_ThreadStackCheck(LW_OBJECT_HANDLE  ulId,
                                             size_t           *pstFreeByteSize,
                                             size_t           *pstUsedByteSize,
                                             size_t           *pstTcbByteSize);
                                                                        /*  检查线程堆栈                */
                                             
LW_API ULONG            API_ThreadGetStackMini(VOID);                   /*  获得线程最小许可堆栈大小    */

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
LW_API VOID             API_ThreadCPUUsageOn(VOID);

LW_API VOID             API_ThreadCPUUsageOff(VOID);

LW_API BOOL             API_ThreadCPUUsageIsOn(VOID);

LW_API ULONG            API_ThreadGetCPUUsage(LW_OBJECT_HANDLE  ulId, 
                                              UINT             *puiThreadUsage,
                                              UINT             *puiCPUUsage,
                                              UINT             *puiKernelUsage);
                                                                        /*  获得 CPU 利用率             */
LW_API INT              API_ThreadGetCPUUsageAll(LW_OBJECT_HANDLE  ulId[], 
                                                 UINT              uiThreadUsage[],
                                                 UINT              uiKernelUsage[],
                                                 INT               iSize);
#endif

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
LW_API ULONG            API_ThreadCPUUsageRefresh(VOID);                /*  刷新 CPU 利用率测量功能     */
#endif

LW_API ULONG            API_ThreadWakeup(LW_OBJECT_HANDLE    ulId);     /*  唤醒一个睡眠线程            */

LW_API ULONG            API_ThreadWakeupEx(LW_OBJECT_HANDLE  ulId, BOOL  bWithInfPend);

#if LW_CFG_THREAD_SCHED_YIELD_EN > 0
LW_API ULONG            API_ThreadYield(LW_OBJECT_HANDLE     ulId);     /*  POSIX 线程 yield 操作       */
#endif

#ifdef __SYLIXOS_KERNEL
LW_API ULONG            API_ThreadStop(LW_OBJECT_HANDLE  ulId);         /*  线程停止                    */

LW_API ULONG            API_ThreadContinue(LW_OBJECT_HANDLE  ulId);     /*  恢复停止的线程              */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
LW_API ULONG            API_ThreadVarAdd(LW_OBJECT_HANDLE     ulId, 
                                         ULONG               *pulAddr); /*  加入私有变量                */

LW_API ULONG            API_ThreadVarDelete(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulAddr); /*  释放私有变量                */

LW_API ULONG            API_ThreadVarGet(LW_OBJECT_HANDLE     ulId, 
                                         ULONG               *pulAddr); /*  获得私有变量                */

LW_API ULONG            API_ThreadVarSet(LW_OBJECT_HANDLE     ulId, 
                                         ULONG               *pulAddr, 
                                         ULONG                ulValue); /*  设置私有变量                */
                                                                        /*  获得线程私有变量信息        */
LW_API ULONG            API_ThreadVarInfo(LW_OBJECT_HANDLE    ulId, ULONG  *pulAddr[], INT  iMaxCounter);
#endif

LW_API BOOL             API_ThreadVerify(LW_OBJECT_HANDLE  ulId);       /*  检查一个线程的 ID 是否正确  */

#if LW_CFG_THREAD_CANCEL_EN > 0
LW_API VOID             API_ThreadTestCancel(VOID);

LW_API ULONG            API_ThreadSetCancelState(INT  iNewState, INT  *piOldState);

LW_API INT              API_ThreadSetCancelType(INT  iNewType, INT  *piOldType);

LW_API ULONG            API_ThreadCancel(LW_OBJECT_HANDLE  *pulId);
#endif

/*********************************************************************************************************
  COROUTINE
*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
LW_API PVOID            API_CoroutineCreate(PCOROUTINE_START_ROUTINE pCoroutineStartAddr,
                                            size_t                   stStackByteSize,
                                            PVOID                    pvArg);
                                                                        /*  创建一个协程                */

LW_API ULONG            API_CoroutineDelete(PVOID      pvCrcb);
                                                                        /*  删除一个协程(本线程内)      */

LW_API ULONG            API_CoroutineExit(VOID);                        /*  删除当前协程                */

LW_API VOID             API_CoroutineYield(VOID);                       /*  协程轮转调度                */

LW_API ULONG            API_CoroutineResume(PVOID  pvCrcb);             /*  回复指定协程                */

LW_API ULONG            API_CoroutineStackCheck(PVOID      pvCrcb,
                                                size_t    *pstFreeByteSize,
                                                size_t    *pstUsedByteSize,
                                                size_t    *pstCrcbByteSize);
                                                                        /*  检测一个协程的堆栈使用量    */
#endif
/*********************************************************************************************************
  SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMCBM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API ULONG            API_SemaphorePend(LW_OBJECT_HANDLE  ulId, 
                                          ULONG             ulTimeout); /*  等待任意信号量              */

LW_API ULONG            API_SemaphorePost(LW_OBJECT_HANDLE  ulId);      /*  释放任意信号量              */

LW_API ULONG            API_SemaphoreFlush(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulThreadUnblockNum);
                                                                        /*  解锁等待任意信号量的线程    */
LW_API ULONG            API_SemaphoreDelete(LW_OBJECT_HANDLE  *pulId);  /*  删除信号量                  */
#endif

#if ((LW_CFG_SEMB_EN > 0) || (LW_CFG_SEMM_EN > 0)) && (LW_CFG_MAX_EVENTS > 0)
LW_API ULONG            API_SemaphorePostBPend(LW_OBJECT_HANDLE  ulIdPost, 
                                               LW_OBJECT_HANDLE  ulId,
                                               ULONG             ulTimeout);
                                                                        /*  原子释放获取二值信号量      */
LW_API ULONG            API_SemaphorePostCPend(LW_OBJECT_HANDLE  ulIdPost, 
                                               LW_OBJECT_HANDLE  ulId,
                                               ULONG             ulTimeout);
                                                                        /*  原子释放获取计数型信号量    */
#endif

/*********************************************************************************************************
  COUNTING SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API ULONG            API_SemaphoreGetName(LW_OBJECT_HANDLE  ulId, 
                                             PCHAR             pcName); /*  获得信号量的名字            */
#endif

#if (LW_CFG_SEMC_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreCCreate(CPCHAR             pcName,
                                             ULONG              ulInitCounter, 
                                             ULONG              ulMaxCounter,
                                             ULONG              ulOption,
                                             LW_OBJECT_ID      *pulId); /*  建立计数型信号量            */

LW_API ULONG            API_SemaphoreCDelete(LW_OBJECT_HANDLE  *pulId); /*  删除计数型信号量            */

LW_API ULONG            API_SemaphoreCPend(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulTimeout);/*  等待计数型信号量            */

LW_API ULONG            API_SemaphoreCTryPend(LW_OBJECT_HANDLE  ulId);  /*  无阻塞等待计数型信号量      */

LW_API ULONG            API_SemaphoreCRelease(LW_OBJECT_HANDLE  ulId,
                                              ULONG             ulReleaseCounter, 
                                              ULONG            *pulPreviousCounter);
                                                                        /*  WIN32 释放信号量            */ 
LW_API ULONG            API_SemaphoreCPost(LW_OBJECT_HANDLE  ulId);     /*  RT 释放信号量               */

LW_API ULONG            API_SemaphoreCFlush(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulThreadUnblockNum);
                                                                        /*  解锁所有等待线程            */
LW_API ULONG            API_SemaphoreCClear(LW_OBJECT_HANDLE  ulId);    /*  清除信号量信号              */

LW_API ULONG            API_SemaphoreCStatus(LW_OBJECT_HANDLE   ulId,
                                             ULONG             *pulCounter,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum);
                                                                        /*  检查计数型信号量状态        */
LW_API ULONG            API_SemaphoreCStatusEx(LW_OBJECT_HANDLE   ulId,
                                               ULONG             *pulCounter,
                                               ULONG             *pulOption,
                                               ULONG             *pulThreadBlockNum,
                                               ULONG             *pulMaxCounter);
                                                                        /*  检查计数型信号量状态        */
#endif

/*********************************************************************************************************
  BINARY SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMB_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreBCreate(CPCHAR             pcName,
                                             BOOL               bInitValue,
                                             ULONG              ulOption,
                                             LW_OBJECT_ID      *pulId); /*  建立二进制信号量            */
                                             
LW_API ULONG            API_SemaphoreBDelete(LW_OBJECT_HANDLE  *pulId); /*  删除二进制信号量            */

LW_API ULONG            API_SemaphoreBPend(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulTimeout);/*  等待二进制信号量            */

LW_API ULONG            API_SemaphoreBPendEx(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulTimeout,
                                             PVOID            *ppvMsgPtr);
                                                                        /*  等待二进制信号量消息        */

LW_API ULONG            API_SemaphoreBTryPend(LW_OBJECT_HANDLE  ulId);  /*  无阻塞等待二进制信号量      */

LW_API ULONG            API_SemaphoreBRelease(LW_OBJECT_HANDLE  ulId, 
                                              ULONG             ulReleaseCounter, 
                                              BOOL             *pbPreviousValue);
                                                                        /*  WIN32 释放信号量            */

LW_API ULONG            API_SemaphoreBPost(LW_OBJECT_HANDLE  ulId);     /*  RT 释放信号量               */

LW_API ULONG            API_SemaphoreBPost2(LW_OBJECT_HANDLE  ulId, LW_OBJECT_HANDLE  *pulId);

LW_API ULONG            API_SemaphoreBPostEx(LW_OBJECT_HANDLE  ulId, 
                                             PVOID      pvMsgPtr);      /*  RT 释放信号量消息           */
                                             
LW_API ULONG            API_SemaphoreBPostEx2(LW_OBJECT_HANDLE  ulId, 
                                              PVOID             pvMsgPtr, 
                                              LW_OBJECT_HANDLE *pulId);

LW_API ULONG            API_SemaphoreBClear(LW_OBJECT_HANDLE  ulId);    /*  清除信号量信号              */

LW_API ULONG            API_SemaphoreBFlush(LW_OBJECT_HANDLE  ulId, 
                                            ULONG            *pulThreadUnblockNum);
                                                                        /*  解锁所有等待线程            */

LW_API ULONG            API_SemaphoreBStatus(LW_OBJECT_HANDLE   ulId,
                                             BOOL              *pbValue,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum);
                                                                        /*  检查二进制信号量状态        */
#endif

/*********************************************************************************************************
  MUTEX SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreMCreate(CPCHAR             pcName,
                                             UINT8              ucCeilingPriority,
                                             ULONG              ulOption,
                                             LW_OBJECT_ID      *pulId); /*  建立互斥信号量              */

LW_API ULONG            API_SemaphoreMDelete(LW_OBJECT_HANDLE  *pulId); /*  删除互斥信号量              */

LW_API ULONG            API_SemaphoreMPend(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulTimeout);/*  等待互斥信号量              */

LW_API ULONG            API_SemaphoreMPost(LW_OBJECT_HANDLE  ulId);     /*  释放互斥信号量              */

LW_API ULONG            API_SemaphoreMStatus(LW_OBJECT_HANDLE   ulId,
                                             BOOL              *pbValue,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum);
                                                                        /*  获得互斥信号量的状态        */

LW_API ULONG            API_SemaphoreMStatusEx(LW_OBJECT_HANDLE   ulId,
                                               BOOL              *pbValue,
                                               ULONG             *pulOption,
                                               ULONG             *pulThreadBlockNum,
                                               LW_OBJECT_HANDLE  *pulOwnerId);
                                                                        /*  获得互斥信号量的状态        */
#endif

/*********************************************************************************************************
  RW SEMAPHORE
*********************************************************************************************************/

#if (LW_CFG_SEMRW_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API LW_OBJECT_HANDLE API_SemaphoreRWCreate(CPCHAR             pcName,
                                              ULONG              ulOption,
                                              LW_OBJECT_ID      *pulId);/*  创建读写信号量              */

LW_API ULONG            API_SemaphoreRWDelete(LW_OBJECT_HANDLE  *pulId);/*  删除读写信号量              */

LW_API ULONG            API_SemaphoreRWPendR(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulTimeout);
                                                                        /*  读写信号量等待读            */
LW_API ULONG            API_SemaphoreRWPendW(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulTimeout);
                                                                        /*  读写信号量等待写            */
LW_API ULONG            API_SemaphoreRWPost(LW_OBJECT_HANDLE  ulId);
                                                                        /*  读写信号量释放              */
LW_API ULONG            API_SemaphoreRWStatus(LW_OBJECT_HANDLE   ulId,
                                              ULONG             *pulRWCount,
                                              ULONG             *pulRPend,
                                              ULONG             *pulWPend,
                                              ULONG             *pulOption,
                                              LW_OBJECT_HANDLE  *pulOwnerId);
                                                                        /*  获得读写信号量的状态        */
#endif

/*********************************************************************************************************
  MESSAGE QUEUE
*********************************************************************************************************/

#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)
LW_API LW_OBJECT_HANDLE API_MsgQueueCreate(CPCHAR             pcName,
                                           ULONG              ulMaxMsgCounter,
                                           size_t             stMaxMsgByteSize,
                                           ULONG              ulOption,
                                           LW_OBJECT_ID      *pulId);   /*  建立一个消息队列            */

LW_API ULONG            API_MsgQueueDelete(LW_OBJECT_HANDLE  *pulId);   /*  删除一个消息队列            */

LW_API ULONG            API_MsgQueueReceive(LW_OBJECT_HANDLE  ulId,
                                            PVOID             pvMsgBuffer,
                                            size_t            stMaxByteSize,
                                            size_t           *pstMsgLen,
                                            ULONG             ulTimeout);
                                                                        /*  从消息队列中获取一条消息    */

LW_API ULONG            API_MsgQueueReceiveEx(LW_OBJECT_HANDLE    ulId,
                                              PVOID               pvMsgBuffer,
                                              size_t              stMaxByteSize,
                                              size_t             *pstMsgLen,
                                              ULONG               ulTimeout,
                                              ULONG               ulOption);
                                                                        /*  带有选项的获取消息          */

LW_API ULONG            API_MsgQueueTryReceive(LW_OBJECT_HANDLE    ulId,
                                               PVOID               pvMsgBuffer,
                                               size_t              stMaxByteSize,
                                               size_t             *pstMsgLen);
                                                                        /*  无等待的获取一条消息        */

LW_API ULONG            API_MsgQueueSend(LW_OBJECT_HANDLE  ulId,
                                         const PVOID       pvMsgBuffer,
                                         size_t            stMsgLen);   /*  向队列中发送一条消息        */
                                         
LW_API ULONG            API_MsgQueueSend2(LW_OBJECT_HANDLE  ulId,
                                          const PVOID       pvMsgBuffer,
                                          size_t            stMsgLen,
                                          ULONG             ulTimeout); /*  带有超时的发送一条消息      */

LW_API ULONG            API_MsgQueueSendEx(LW_OBJECT_HANDLE  ulId,
                                           const PVOID       pvMsgBuffer,
                                           size_t            stMsgLen,
                                           ULONG             ulOption); /*  发送消息高级接口            */
                                           
LW_API ULONG            API_MsgQueueSendEx2(LW_OBJECT_HANDLE  ulId,
                                            const PVOID       pvMsgBuffer,
                                            size_t            stMsgLen,
                                            ULONG             ulTimeout,
                                            ULONG             ulOption);/*  带有超时的发送消息高级接口  */

LW_API ULONG            API_MsgQueueClear(LW_OBJECT_HANDLE  ulId);      /*  清空所有消息                */

LW_API ULONG            API_MsgQueueStatus(LW_OBJECT_HANDLE   ulId,
                                           ULONG             *pulMaxMsgNum,
                                           ULONG             *pulCounter,
                                           size_t            *pstMsgLen,
                                           ULONG             *pulOption,
                                           ULONG             *pulThreadBlockNum);
                                                                        /*  获得队列的当前状态          */

LW_API ULONG            API_MsgQueueStatusEx(LW_OBJECT_HANDLE   ulId,
                                             ULONG             *pulMaxMsgNum,
                                             ULONG             *pulCounter,
                                             size_t            *pstMsgLen,
                                             ULONG             *pulOption,
                                             ULONG             *pulThreadBlockNum,
                                             size_t            *pstMaxMsgLen);
                                                                        /*  获得队列的当前状态          */

LW_API ULONG            API_MsgQueueFlush(LW_OBJECT_HANDLE  ulId, 
                                          ULONG            *pulThreadUnblockNum);
                                                                        /*  就绪所有等待消息的线程      */
LW_API ULONG            API_MsgQueueFlushSend(LW_OBJECT_HANDLE  ulId, 
                                              ULONG  *pulThreadUnblockNum);
                                                                        /*  就绪所有等待写消息的线程    */
LW_API ULONG            API_MsgQueueFlushReceive(LW_OBJECT_HANDLE  ulId, 
                                                 ULONG  *pulThreadUnblockNum);
                                                                        /*  就绪所有等待读消息的线程    */
LW_API ULONG            API_MsgQueueGetName(LW_OBJECT_HANDLE  ulId, 
                                            PCHAR             pcName);  /*  获得消息队列名字            */
#endif

/*********************************************************************************************************
  EVENT SET
*********************************************************************************************************/

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
LW_API LW_OBJECT_HANDLE API_EventSetCreate(CPCHAR             pcName, 
                                           ULONG              ulInitEvent, 
                                           ULONG              ulOption,
                                           LW_OBJECT_ID      *pulId);   /*  建立事件集                  */

LW_API ULONG            API_EventSetDelete(LW_OBJECT_HANDLE  *pulId);   /*  删除事件集                  */

LW_API ULONG            API_EventSetSet(LW_OBJECT_HANDLE  ulId, 
                                        ULONG             ulEvent,
                                        ULONG             ulOption);    /*  发送事件                    */

LW_API ULONG            API_EventSetGet(LW_OBJECT_HANDLE  ulId, 
                                        ULONG             ulEvent,
                                        ULONG             ulOption,
                                        ULONG             ulTimeout);   /*  接收事件                    */
                                        
LW_API ULONG            API_EventSetGetEx(LW_OBJECT_HANDLE  ulId, 
                                          ULONG             ulEvent,
                                          ULONG             ulOption,
                                          ULONG             ulTimeout,
                                          ULONG            *pulEvent);  /*  接收事件扩展接口            */

LW_API ULONG            API_EventSetTryGet(LW_OBJECT_HANDLE  ulId, 
                                           ULONG             ulEvent,
                                           ULONG             ulOption); /*  无阻塞接收事件              */
                                           
LW_API ULONG            API_EventSetTryGetEx(LW_OBJECT_HANDLE  ulId, 
                                             ULONG             ulEvent,
                                             ULONG             ulOption,
                                             ULONG            *pulEvent);
                                                                        /*  无阻塞接收事件扩展接口      */
LW_API ULONG            API_EventSetStatus(LW_OBJECT_HANDLE  ulId, 
                                           ULONG            *pulEvent,
                                           ULONG            *pulOption);/*  获得事件集状态              */

LW_API ULONG            API_EventSetGetName(LW_OBJECT_HANDLE  ulId, 
                                            PCHAR             pcName);  /*  事件集名                    */
#endif

/*********************************************************************************************************
  TIME
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API INT              API_TimeNanoSleepMethod(INT  iMethod);          /*  设置 nanosleep 使用的算法   */
#endif

LW_API VOID             API_TimeSleep(ULONG    ulTick);                 /*  当前线程睡眠                */

LW_API ULONG            API_TimeSleepEx(ULONG   ulTick, BOOL  bSigRet); /*  当前线程睡眠                */

LW_API ULONG            API_TimeSleepUntil(clockid_t  clockid, 
                                           const struct timespec  *tv, 
                                           BOOL  bSigRet);              /*  当前线程睡眠直到指定的时间  */

LW_API VOID             API_TimeMSleep(ULONG   ulMSeconds);             /*  以毫秒为单位睡眠            */

LW_API VOID             API_TimeSSleep(ULONG   ulSeconds);              /*  以秒为单位睡眠              */

LW_API VOID             API_TimeSet(ULONG  ulKernenlTime);              /*  设置系统时钟嘀嗒数          */

LW_API ULONG            API_TimeGet(VOID);                              /*  获得系统时钟嘀嗒数          */

LW_API INT64            API_TimeGet64(VOID);                            /*  获得系统时钟嘀嗒数 64bit    */

LW_API ULONG            API_TimeGetFrequency(VOID);                     /*  获得系统时钟嘀嗒频率        */

LW_API VOID             API_TimeTodAdj(INT32  *piDelta, 
                                       INT32  *piOldDelta);             /*  微调系统 TOD 时间           */

/*********************************************************************************************************
  TIMER
*********************************************************************************************************/

#if	(LW_CFG_HTIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)
LW_API VOID             API_TimerHTicks(VOID);                          /*  高速定时器周期中断          */
#endif

LW_API ULONG            API_TimerHGetFrequency(VOID);                   /*  获得高速定时器频率          */

#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)
LW_API LW_OBJECT_HANDLE API_TimerCreate(CPCHAR             pcName,
                                        ULONG              ulOption,
                                        LW_OBJECT_ID      *pulId);      /*  建立一个定时器              */

LW_API ULONG            API_TimerDelete(LW_OBJECT_HANDLE  *pulId);      /*  删除一个定时器              */

LW_API ULONG            API_TimerStart(LW_OBJECT_HANDLE         ulId,
                                       ULONG                    ulCounter,
                                       ULONG                    ulOption,
                                       PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                                       PVOID                    pvArg); /*  启动一个定时器              */
                                       
LW_API ULONG            API_TimerStartEx(LW_OBJECT_HANDLE         ulId,
                                         ULONG                    ulInitCounter,
                                         ULONG                    ulCounter,
                                         ULONG                    ulOption,
                                         PTIMER_CALLBACK_ROUTINE  cbTimerRoutine,
                                         PVOID                    pvArg);
                                                                        /*  启动一个定时器              */

LW_API ULONG            API_TimerCancel(LW_OBJECT_HANDLE         ulId); /*  停止一个定时器              */

LW_API ULONG            API_TimerReset(LW_OBJECT_HANDLE          ulId); /*  复位一个定时器              */

LW_API ULONG            API_TimerStatus(LW_OBJECT_HANDLE          ulId,
                                        BOOL                     *pbTimerRunning,
                                        ULONG                    *pulOption,
                                        ULONG                    *pulCounter,
                                        ULONG                    *pulInterval);
                                                                        /*  获得一个定时器状态          */
                                                                        
LW_API ULONG            API_TimerStatusEx(LW_OBJECT_HANDLE          ulId,
                                          BOOL                     *pbTimerRunning,
                                          ULONG                    *pulOption,
                                          ULONG                    *pulCounter,
                                          ULONG                    *pulInterval,
                                          clockid_t                *pclockid);
                                                                        /*  获得一个定时器状态扩展接口  */

LW_API ULONG            API_TimerGetName(LW_OBJECT_HANDLE  ulId, 
                                         PCHAR             pcName);     /*  获得定时器名字              */
#endif

/*********************************************************************************************************
  PARTITION
*********************************************************************************************************/

#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
LW_API LW_OBJECT_HANDLE API_PartitionCreate(CPCHAR             pcName,
                                            PVOID              pvLowAddr,
                                            ULONG              ulBlockCounter,
                                            size_t             stBlockByteSize,
                                            ULONG              ulOption,
                                            LW_OBJECT_ID      *pulId);  /*  建立一个 PARTITION          */

LW_API ULONG            API_PartitionDelete(LW_OBJECT_HANDLE   *pulId); /*  删除一个 PARTITION          */

LW_API ULONG            API_PartitionDeleteEx(LW_OBJECT_HANDLE   *pulId, BOOL  bForce);
                                                                        /*  扩展接口                    */

LW_API PVOID            API_PartitionGet(LW_OBJECT_HANDLE  ulId);       /*  获得 PARTITION 一个分块     */

LW_API PVOID            API_PartitionPut(LW_OBJECT_HANDLE  ulId, 
                                         PVOID             pvBlock);    /*  交还 PARTITION 一个分块     */

LW_API ULONG            API_PartitionStatus(LW_OBJECT_HANDLE    ulId,
                                            ULONG              *pulBlockCounter,
                                            ULONG              *pulFreeBlockCounter,
                                            size_t             *pstBlockByteSize);
                                                                        /*  获得 PARTITION 状态         */
LW_API ULONG            API_PartitionGetName(LW_OBJECT_HANDLE  ulId, 
                                             PCHAR             pcName); /*  获得 PARTITION 名字         */
#endif

/*********************************************************************************************************
  REGION
*********************************************************************************************************/

#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
LW_API LW_OBJECT_HANDLE API_RegionCreate(CPCHAR             pcName,
                                         PVOID              pvLowAddr,
                                         size_t             stRegionByteSize,
                                         ULONG              ulOption,
                                         LW_OBJECT_ID      *pulId);     /*  建立一个 REGION             */

LW_API ULONG            API_RegionDelete(LW_OBJECT_HANDLE   *pulId);    /*  删除一个 REGION             */

LW_API ULONG            API_RegionDeleteEx(LW_OBJECT_HANDLE   *pulId, BOOL  bForce);

LW_API ULONG            API_RegionAddMem(LW_OBJECT_HANDLE  ulId, PVOID  pvMem, size_t  stByteSize);
                                                                        /*  向 REGION 里添加一段内存    */
LW_API PVOID            API_RegionGet(LW_OBJECT_HANDLE  ulId, 
                                      size_t            stByteSize);    /*  获得 REGION 一个分段        */

LW_API PVOID            API_RegionGetAlign(LW_OBJECT_HANDLE  ulId, 
                                           size_t            stByteSize, 
                                           size_t            stAlign);  /*  带有内存对齐特性的内存分配  */

LW_API PVOID            API_RegionReget(LW_OBJECT_HANDLE    ulId, 
                                        PVOID               pvOldMem, 
                                        size_t              stNewByteSize);
                                                                        /*  重新分配一个分段            */

LW_API PVOID            API_RegionPut(LW_OBJECT_HANDLE  ulId, 
                                      PVOID             pvSegmentData); /*  交还 REGION 一个分段        */

LW_API ULONG            API_RegionGetMax(LW_OBJECT_HANDLE  ulId, size_t  *pstMaxFreeSize);
                                                                        /*  获得最大空闲分段长度        */
LW_API ULONG            API_RegionStatus(LW_OBJECT_HANDLE    ulId,
                                         size_t             *pstByteSize,
                                         ULONG              *pulSegmentCounter,
                                         size_t             *pstUsedByteSize,
                                         size_t             *pstFreeByteSize,
                                         size_t             *pstMaxUsedByteSize);
                                                                        /*  获得 REGION 状态            */

LW_API ULONG            API_RegionStatusEx(LW_OBJECT_HANDLE    ulId,
                                           size_t             *pstByteSize,
                                           ULONG              *pulSegmentCounter,
                                           size_t             *pstUsedByteSize,
                                           size_t             *pstFreeByteSize,
                                           size_t             *pstMaxUsedByteSize,
                                           PLW_CLASS_SEGMENT   psegmentList[],
                                           INT                 iMaxCounter);
                                                                        /*  获得 REGION 状态高级接口    */

LW_API ULONG            API_RegionGetName(LW_OBJECT_HANDLE  ulId, 
                                          PCHAR             pcName);    /*  获得 REGION 名字            */
#endif

/*********************************************************************************************************
  RMS
*********************************************************************************************************/

#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
LW_API LW_OBJECT_HANDLE API_RmsCreate(CPCHAR             pcName,
                                      ULONG              ulOption,
                                      LW_OBJECT_ID      *pulId);        /*  建立一个 RMS 调度器         */

LW_API ULONG            API_RmsDelete(LW_OBJECT_HANDLE  *pulId);        /*  删除一个 RMS 调度器         */

LW_API ULONG            API_RmsDeleteEx(LW_OBJECT_HANDLE  *pulId, BOOL  bForce);
                                                                        /*  扩展接口                    */

LW_API ULONG            API_RmsExecTimeGet(LW_OBJECT_HANDLE  ulId, 
                                           ULONG    *pulExecTime);      /*  获得线程执行时间            */

LW_API ULONG            API_RmsPeriod(LW_OBJECT_HANDLE   ulId, 
                                      ULONG              ulPeriod);     /*  指定一个 RMS 周期           */

LW_API ULONG            API_RmsCancel(LW_OBJECT_HANDLE   ulId);         /*  停止一个 RMS 调度器         */

LW_API ULONG            API_RmsStatus(LW_OBJECT_HANDLE   ulId,
                                      UINT8             *pucStatus,
                                      ULONG             *pulTimeLeft,
                                      LW_OBJECT_HANDLE  *pulOwnerId);   /*  获得一个 RMS 调度器状态     */

LW_API ULONG            API_RmsGetName(LW_OBJECT_HANDLE  ulId, 
                                       PCHAR             pcName);       /*  获得一个 RMS 调度器名字     */
#endif

/*********************************************************************************************************
  INTERRUPT
*********************************************************************************************************/

LW_API ULONG            API_InterLock(INTREG  *piregInterLevel);        /*  关闭中断                    */

LW_API ULONG            API_InterUnlock(INTREG  iregInterLevel);        /*  打开中断                    */

LW_API BOOL             API_InterContext(VOID);                         /*  是否在中断中                */

LW_API ULONG            API_InterGetNesting(VOID);                      /*  获得中断嵌套层数            */

LW_API ULONG            API_InterGetNestingById(ULONG  ulCPUId, ULONG *pulMaxNesting);
                                                                        /*  获得指定 CPU 中断嵌套层数   */
#ifdef __SYLIXOS_KERNEL
LW_API ULONG            API_InterEnter(VOID);                           /*  进入中断                    */

LW_API VOID             API_InterExit(VOID);                            /*  退出中断                    */

LW_API PVOID            API_InterVectorBaseGet(VOID);                   /*  获得中断向量表基址          */
                                           
LW_API ULONG            API_InterVectorConnect(ULONG                 ulVector,
                                               PINT_SVR_ROUTINE      pfuncIsr,
                                               PVOID                 pvArg,
                                               CPCHAR                pcName);
                                                                        /*  设置指定向量的服务程序      */
LW_API ULONG            API_InterVectorConnectEx(ULONG                 ulVector,
                                                 PINT_SVR_ROUTINE      pfuncIsr,
                                                 VOIDFUNCPTR           pfuncClear,
                                                 PVOID                 pvArg,
                                                 CPCHAR                pcName);
                                                                        /*  设置指定向量的服务程序      */
LW_API ULONG            API_InterVectorDisconnect(ULONG                 ulVector,
                                                  PINT_SVR_ROUTINE      pfuncIsr,
                                                  PVOID                 pvArg);
                                                                        /*  删除指定向量的服务程序      */
LW_API ULONG            API_InterVectorDisconnectEx(ULONG             ulVector,
                                                    PINT_SVR_ROUTINE  pfuncIsr,
                                                    PVOID             pvArg,
                                                    ULONG             ulOption);
                                                                        /*  删除指定向量的服务程序      */
                                                                        
LW_API ULONG            API_InterVectorEnable(ULONG  ulVector);         /*  使能指定向量中断            */

LW_API ULONG            API_InterVectorDisable(ULONG  ulVector);        /*  禁能指定向量中断            */

LW_API ULONG            API_InterVectorIsEnable(ULONG  ulVector, 
                                                BOOL  *pbIsEnable);     /*  获得指定中断状态            */

#if LW_CFG_INTER_PRIO > 0
LW_API ULONG            API_InterVectorSetPriority(ULONG  ulVector, UINT  uiPrio);
                                                                        /*  设置中断优先级              */
LW_API ULONG            API_InterVectorGetPriority(ULONG  ulVector, UINT  *puiPrio);
                                                                        /*  获取中断优先级              */
#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */

#if LW_CFG_INTER_TARGET > 0
LW_API ULONG            API_InterSetTarget(ULONG  ulVector, size_t  stSize, 
                                           const PLW_CLASS_CPUSET  pcpuset);
                                                                        /*  设置中断目标 CPU            */
LW_API ULONG            API_InterGetTarget(ULONG  ulVector, size_t  stSize, 
                                           PLW_CLASS_CPUSET  pcpuset);  /*  获取中断目标 CPU            */
#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */

LW_API ULONG            API_InterVectorSetFlag(ULONG  ulVector, ULONG  ulFlag);
                                                                        /*  设置中断向量属性            */
                                                                        
LW_API ULONG            API_InterVectorGetFlag(ULONG  ulVector, ULONG  *pulFlag);
                                                                        /*  获取中断向量属性            */

#if LW_CFG_INTER_MEASURE_HOOK_EN > 0
LW_API VOID             API_InterVectorMeasureHook(VOIDFUNCPTR  pfuncEnter, VOIDFUNCPTR  pfuncExit);
                                                                        /*  中断测量 HOOK               */
#endif

LW_API irqreturn_t      API_InterVectorIsr(ULONG    ulVector);          /*  中断服务程序，BSP中断调用   */

#if LW_CFG_SMP_EN > 0
LW_API VOID             API_InterVectorIpi(ULONG  ulCPUId, 
                                           ULONG  ulIPIVector);         /*  设置核间中断向量号          */
                                           
LW_API VOID             API_InterVectorIpiEx(ULONG    ulCPUId, 
                                             ULONG    ulIPIVector, 
                                             FUNCPTR  pfuncClear, 
                                             PVOID    pvArg);           /*  设置核间中断向量号          */
#endif                                                                  /*  LW_CFG_SMP_EN               */

#if LW_CFG_ISR_DEFER_EN > 0
LW_API PLW_JOB_QUEUE    API_InterDeferGet(ULONG  ulCPUId);              /*  获得对应 CPU 的中断延迟队列 */

LW_API ULONG            API_InterDeferJobAdd(PLW_JOB_QUEUE  pjobq, VOIDFUNCPTR  pfunc, PVOID  pvArg);
                                                                        /*  向中断延迟处理队列加入任务  */
LW_API ULONG            API_InterDeferJobDelete(PLW_JOB_QUEUE  pjobq, BOOL  bMatchArg, 
                                                VOIDFUNCPTR  pfunc, PVOID  pvArg);
                                                                        /*  从中断延迟处理队列删除任务  */
#endif                                                                  /*  LW_CFG_ISR_DEFER_EN > 0     */

LW_API PVOID            API_InterStackBaseGet(VOID);                    /*  获得当前中断堆栈首地址      */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API VOID             API_InterStackCheck(ULONG   ulCPUId,
                                            size_t *pstFreeByteSize,
                                            size_t *pstUsedByteSize);   /*  中断堆栈检查                */

/*********************************************************************************************************
  LAST ERROR
*********************************************************************************************************/

LW_API ULONG            API_GetLastError(VOID);                         /*  获得系统最后一次错误        */

LW_API ULONG            API_GetLastErrorEx(LW_OBJECT_HANDLE  ulId, 
                                           ULONG  *pulError);           /*  获得指定任务的最后一次错误  */

LW_API VOID             API_SetLastError(ULONG  ulError);               /*  设置系统最后一次错误        */

LW_API ULONG            API_SetLastErrorEx(LW_OBJECT_HANDLE  ulId, 
                                           ULONG  ulError);             /*  设置指定任务的最后一次错误  */

/*********************************************************************************************************
  WORK QUEUE
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_WORKQUEUE_EN > 0
LW_API PVOID            API_WorkQueueCreate(CPCHAR                  pcName,
                                            UINT                    uiQSize, 
                                            BOOL                    bDelayEn, 
                                            ULONG                   ulScanPeriod, 
                                            PLW_CLASS_THREADATTR    pthreadattr);
                                                                        /*  创建一个工作队列            */
LW_API ULONG            API_WorkQueueDelete(PVOID  pvWQ);               /*  删除一个工作队列            */

LW_API ULONG            API_WorkQueueInsert(PVOID           pvWQ, 
                                            ULONG           ulDelay,
                                            VOIDFUNCPTR     pfunc, 
                                            PVOID           pvArg0,
                                            PVOID           pvArg1,
                                            PVOID           pvArg2,
                                            PVOID           pvArg3,
                                            PVOID           pvArg4,
                                            PVOID           pvArg5);    /*  将一个工作插入到工作队列    */
                                            
LW_API ULONG            API_WorkQueueFlush(PVOID  pvWQ);                /*  清空工作队列                */
                                            
LW_API ULONG            API_WorkQueueStatus(PVOID  pvWQ, UINT  *puiCount);
                                                                        /*  获得工作队列状态            */

#endif                                                                  /*  LW_CFG_WORKQUEUE_EN > 0     */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  KERNEL
*********************************************************************************************************/

LW_API VOID             API_KernelNop(CPCHAR  pcArg, LONG  lArg);       /*  内核空操作                  */

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_CPU_FPU_EN > 0
LW_API VOID             API_KernelFpuPrimaryInit(CPCHAR  pcMachineName, 
                                                 CPCHAR  pcFpuName);    /*  初始化浮点运算器            */
                                                 
#define API_KernelFpuInit   API_KernelFpuPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API VOID             API_KernelFpuSecondaryInit(CPCHAR  pcMachineName, 
                                                   CPCHAR  pcFpuName);
#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API VOID             API_KernelReboot(INT  iRebootType);             /*  系统重新启动                */

LW_API VOID             API_KernelRebootEx(INT  iRebootType, 
                                           addr_t  ulStartAddress);     /*  系统重新启动扩展            */

LW_API VOID             API_KernelTicks(VOID);                          /*  通知系统到达一个实时时钟    */

LW_API VOID             API_KernelTicksContext(VOID);                   /*  保存时钟中断时的线程控制块  */

LW_API ULONG            API_KernelVersion(VOID);                        /*  获得系统版本号              */

LW_API PCHAR            API_KernelVerinfo(VOID);                        /*  获得系统版本信息字串        */

LW_API ULONG            API_KernelVerpatch(VOID);                       /*  获得系统当前补丁版本        */

LW_API LW_OBJECT_HANDLE API_KernelGetIdle(VOID);                        /*  获得空闲线程句柄            */

#if	(LW_CFG_ITIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)
LW_API LW_OBJECT_HANDLE API_KernelGetItimer(VOID);                      /*  获得定时器扫描线程句柄      */
#endif

LW_API LW_OBJECT_HANDLE API_KernelGetExc(VOID);                         /*  获得异常截获线程句柄        */

LW_API UINT8            API_KernelGetPriorityMax(VOID);                 /*  获得最低优先级              */

LW_API UINT8            API_KernelGetPriorityMin(VOID);                 /*  获得最高优先级              */

LW_API UINT16           API_KernelGetThreadNum(VOID);                   /*  获得当前线程总数量          */

LW_API UINT16           API_KernelGetThreadNumByPriority(UINT8  ucPriority);             
                                                                        /*  获得指定优先级线程数量      */

LW_API BOOL             API_KernelIsRunning(VOID);                      /*  检测系统内核是否启动        */

LW_API ULONG            API_KernelHeapInfo(ULONG   ulOption, 
                                           size_t  *pstByteSize,
                                           ULONG   *pulSegmentCounter,
                                           size_t  *pstUsedByteSize,
                                           size_t  *pstFreeByteSize,
                                           size_t  *pstMaxUsedByteSize);/*  检察系统堆状态              */
                                           
LW_API ULONG            API_KernelHeapInfoEx(ULONG                ulOption, 
                                             size_t              *pstByteSize,
                                             ULONG               *pulSegmentCounter,
                                             size_t              *pstUsedByteSize,
                                             size_t              *pstFreeByteSize,
                                             size_t              *pstMaxUsedByteSize,
                                             PLW_CLASS_SEGMENT    psegmentList[],
                                             INT                  iMaxCounter);
                                                                        /*  检察系统堆状态高级接口      */
                                           
#ifdef __SYLIXOS_KERNEL
#if LW_CFG_POWERM_EN > 0
LW_API VOID             API_KernelSuspend(VOID);                        /*  内核休眠                    */

LW_API VOID             API_KernelResume(VOID);                         /*  内核从休眠状态唤醒          */
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

LW_API ULONG            API_KernelHookSet(LW_HOOK_FUNC   hookfuncPtr, ULONG  ulOpt);     
                                                                        /*  设置系统钩子函数            */

LW_API LW_HOOK_FUNC     API_KernelHookGet(ULONG  ulOpt);                /*  获得系统钩子函数            */

LW_API ULONG            API_KernelHookDelete(ULONG  ulOpt);             /*  删除系统钩子函数            */

#if LW_CFG_SMP_EN > 0                                                   /*  核间中断调用                */
LW_API INT              API_KernelSmpCall(ULONG  ulCPUId, FUNCPTR  pfunc, PVOID  pvArg,
                                          VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);

LW_API VOID             API_KernelSmpCallAll(FUNCPTR  pfunc,  PVOID  pvArg, 
                                             VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);

LW_API VOID             API_KernelSmpCallAllOther(FUNCPTR  pfunc, PVOID  pvArg,
                                                  VOIDFUNCPTR  pfuncAsync, PVOID  pvAsync, INT  iOpt);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  STARTUP
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API ULONG            API_KernelStartParam(CPCHAR  pcParam);          /*  设置内核启动参数            */

LW_API ssize_t          API_KernelStartParamGet(PCHAR  pcParam, size_t  stLen);

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
LW_API VOID             API_KernelPrimaryStart(PKERNEL_START_ROUTINE  pStartHook,
                                               PVOID                  pvKernelHeapMem,
                                               size_t                 stKernelHeapSize,
                                               PVOID                  pvSystemHeapMem,
                                               size_t                 stSystemHeapSize);
                                                                        /*  启动系统内核 (逻辑主核)     */
#else
LW_API VOID             API_KernelPrimaryStart(PKERNEL_START_ROUTINE  pStartHook);
                                                                        /*  启动系统内核 (逻辑主核)     */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

#define API_KernelStart API_KernelPrimaryStart

#if LW_CFG_SMP_EN > 0
LW_API VOID             API_KernelSecondaryStart(PKERNEL_START_ROUTINE  pStartHook);
                                                                        /*  SMP 系统, 逻辑从核启动      */
#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  SHOW
*********************************************************************************************************/

#if LW_CFG_FIO_LIB_EN > 0
LW_API VOID             API_BacktraceShow(INT  iFd, INT  iMaxDepth);    /*  显示调用栈信息              */

LW_API VOID             API_BacktracePrint(PVOID  pvBuffer, size_t  stSize, INT  iMaxDepth);

LW_API VOID             API_ThreadShow(VOID);                           /*  从 STD_OUT 打印所有线程信息 */

LW_API VOID             API_ThreadShowEx(pid_t  pid);                   /*  显示指定进程内线程          */

LW_API VOID             API_ThreadPendShow(VOID);                       /*  显示阻塞线程信息            */

LW_API VOID             API_ThreadPendShowEx(pid_t  pid);               /*  显示阻塞线程信息            */

LW_API VOID             API_StackShow(VOID);                            /*  打印所有线程堆栈使用信息    */

#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
LW_API VOID             API_RegionShow(LW_OBJECT_HANDLE  ulId);         /*  打印指定内存池信息          */
#endif

#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
LW_API VOID             API_PartitionShow(LW_OBJECT_HANDLE  ulId);      /*  打印指定内存分区信息        */
#endif

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
LW_API VOID             API_CPUUsageShow(INT  iWaitSec, INT  iTimes);   /*  显示 CPU 利用率信息         */
#endif

#if LW_CFG_EVENTSET_EN > 0
LW_API VOID             API_EventSetShow(LW_OBJECT_HANDLE  ulId);       /*  显示事件集的相关信息        */
#endif

#if LW_CFG_MSGQUEUE_EN > 0
LW_API VOID             API_MsgQueueShow(LW_OBJECT_HANDLE  ulId);       /*  显示指定消息队列信息        */
#endif

LW_API VOID             API_InterShow(ULONG ulCPUStart, ULONG ulCPUEnd);/*  显示操作系统中断向量表内容  */

LW_API VOID             API_TimeShow(VOID);                             /*  显示时间                    */

#if (LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)
LW_API VOID             API_TimerShow(LW_OBJECT_HANDLE  ulId);          /*  显示定时器信息              */
#endif

#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
LW_API VOID             API_RmsShow(LW_OBJECT_HANDLE  ulId);            /*  显示 RMS                    */
#endif

#if LW_CFG_VMM_EN > 0
LW_API VOID             API_VmmPhysicalShow(VOID);                      /*  显示 vmm 物理存储器信息     */
LW_API VOID             API_VmmVirtualShow(VOID);                       /*  显示 vmm 虚拟存储器信息     */
LW_API VOID             API_VmmAbortShow(VOID);                         /*  显示 vmm 访问中止统计信息   */
#endif

#if (LW_CFG_SEM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
LW_API VOID             API_SemaphoreShow(LW_OBJECT_HANDLE  ulId);      /*  显示指定信号量信息          */

#endif                                                                  /*  (LW_CFG_SEM_EN > 0) &&      */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
#endif                                                                  /*  __K_API_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
