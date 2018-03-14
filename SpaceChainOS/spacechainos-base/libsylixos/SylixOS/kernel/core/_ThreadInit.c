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
** 文   件   名: _ThreadInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统线程初始化函数库。

** BUG
2007.05.08  加入对线程标准输入、输出和错误文件描述符的初始化，初始化为全局输入、输出和错误文件描述符
2007.11.07  CreateHook 只在回调链中加入 option 参数.
2007.11.13  加入 TCB_ptcbJoin 初始化.
2007.12.22  加入 TCB_bIsInDeleteProc 的初始化.
2007.12.22  将 pulNote 改为 ULONG 类型.
2008.01.20  改进了线程调度器锁的机制, 锁变量为线程私有变量. 这里增加线程锁变量的初始化.
2008.03.30  使用新的就绪环操作.
2008.04.14  为了实现信号系统加入调度器返回值的初始化.
2008.06.01  今天是儿童节, 预祝在汶川地震中失去家园的孩子, 儿童节快乐.
            这里不再使用 _signalInit() 直接待用, 而使用 hook 来完成, 这样 ThreadRestart() 在处理信号时
            不会有初始化问题, 间接的解决了一个隐藏的内存泄露问题.
2008.06.06  加入 thread_cancel() 相关内容.
2008.06.19  加入操作系统对协程的支持.
2009.02.03  加入 TCB 扩展模块的初始化.
2009.04.09  修改回调参数.
2009.04.28  加入 TCB_bRunning 初始化.
2009.10.11  调整了回调函数调用顺序.
2009.11.21  初始化使用全局 io 环境.
2009.12.14  加入对线程内核利用率的测算初始化.
2009.12.30  加入对 TCB_pvPosixContext 的初始化.
2010.08.03  修改一些注释.
2011.08.17  对 libc 线程上下文的初始化统一交由 lib_nlreent_init 处理.
2012.03.30  加入对 uid euid gid egid 的初始化.
2012.08.24  加入对进程信息的继承.
2012.09.11  加入 OSFpuInitCtx() 初始化 TCB 的 FPU 保存上下文.
2012.12.06  LW_OPTION_OBJECT_GLOBAL 线程不属于任何进程.
2013.05.07  TCB 从今后独立与堆栈存在.
2013.08.28  _TCBBuild() 不再处理就绪队列.
            修正不合理的函数命名.
2014.05.13  加入对进程内线程链表的支持.
2014.07.26  加入对 GDB 变量的初始化.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  主堆栈示意图 (不包括 FP 堆栈区 和 扩展栈区)
*********************************************************************************************************/
/*********************************************************************************************************
1:  从高地址向低地址方向增长

*         - 高内存地址 -
*    __________________________ 
*     |//////////////////////|  <--- pstkStackTop ----------|
*     |//////////////////////|                              |
*     |//// THREAD STACK ////|                              |
*     |//////////////////////|       堆栈区                 | 
*     |//////////////////////|                              |
*     |//////////////////////|                              |
*    _|//////////////////////|_ <--- pstkStackLowAddr ------|
*         - 低内存地址 -            (pstkStackButtom)

2:  从低地址向高地址方向增长

*         - 高内存地址 -
*    __________________________
*     |//////////////////////|  <--- pstkStackButtom -------|
*     |//////////////////////|                              |
*     |//////////////////////|                              |
*     |//// THREAD STACK ////|       堆栈区                 |
*     |//////////////////////|								|
*     |//////////////////////|                              |
*    _|//////////////////////|_ <--- pstkStackTop ----------|
*         - 低内存地址 -           (pstkStackLowAddr) 

*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: _TCBBuild
** 功能描述: 创建一个TCB
** 输　入  : ucPriority                  优先级
**           pstkStackNow                当前栈顶
**           pstkStackTop                真实数据主栈区堆栈顶
**           pstkStackButtom             真实数据主栈区堆栈底
**           pstkStackGuard              堆栈警戒点
**           pvStackExt                  扩展堆栈区栈定
**           pstkStackLowAddr            包括 TCB 的全部栈区最低内存地址
**           ulStackSize                 包括 TCB 的全部栈区大小(单位：字)
**           pulId                       建立的 TCB ID号
**           pulOption                   建立线程选项
**           pThread                     线程起始地址
**           ptcb                        需要初始化的 TCB
**           pvArg                       线程启动参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _TCBBuild (UINT8                    ucPriority,
                 PLW_STACK                pstkStackNow,
                 PLW_STACK                pstkStackTop,
                 PLW_STACK                pstkStackButtom,
                 PLW_STACK                pstkStackGuard,
                 PVOID                    pvStackExt,
                 PLW_STACK                pstkStackLowAddr,
                 size_t                   stStackSize,
                 LW_OBJECT_ID             ulId,
                 ULONG                    ulOption,
                 PTHREAD_START_ROUTINE    pThread,
                 PLW_CLASS_TCB            ptcb,
                 PVOID                    pvArg)
{
    REGISTER PLW_CLASS_TCB        ptcbCur;
    
#if LW_CFG_COROUTINE_EN > 0
    REGISTER PLW_CLASS_COROUTINE  pcrcb;
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

#if LW_CFG_THREAD_NOTE_PAD_EN > 0
    REGISTER UINT8            ucI;
    REGISTER ULONG           *pulNote;
#endif                                                                  /*  LW_CFG_THREAD_NOTE_PAD_EN   */

    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前 ptcb                   */

#if defined(LW_CFG_CPU_ARCH_C6X)
    ptcb->TCB_ulContextType   = 0;                                      /*  小上下文                    */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

    ptcb->TCB_pstkStackNow    = pstkStackNow;                           /*  堆栈指针连接                */
    ptcb->TCB_pstkStackTop    = pstkStackTop;                           /*  真实主栈区 头               */
    ptcb->TCB_pstkStackBottom = pstkStackButtom;                        /*  真实主栈区 底               */
    ptcb->TCB_pstkStackGuard  = pstkStackGuard;                         /*  堆栈警戒区                  */
    
    ptcb->TCB_pvStackExt   = pvStackExt;
    ptcb->TCB_stStackSize  = stStackSize;                               /*  以CPU字为单位的堆栈区大小   */

    ptcb->TCB_pstkStackLowAddr = pstkStackLowAddr;                      /*  包括 TCB 在内的堆栈基地址   */

    ptcb->TCB_iSchedRet     = ERROR_NONE;                               /*  初始化调度器返回值          */
    ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                       /*  没有超时                    */
    
    ptcb->TCB_ulOption      = ulOption;                                 /*  选项                        */
    ptcb->TCB_ulId          = ulId;                                     /*  Id                          */
    ptcb->TCB_ulLastError   = ERROR_NONE;                               /*  最后一个错误                */
    ptcb->TCB_pvArg         = pvArg;                                    /*  参数                        */
    
    __WAKEUP_NODE_INIT(&ptcb->TCB_wunDelay);
    
    ptcb->TCB_ucPriority          = ucPriority;                         /*  优先级                      */
    ptcb->TCB_ucSchedPolicy       = LW_OPTION_SCHED_RR;                 /*  初始化同优先级调度策略      */
    ptcb->TCB_usSchedSlice        = LW_SCHED_SLICE;                     /*  时间片初始化                */
    ptcb->TCB_usSchedCounter      = LW_SCHED_SLICE;
    ptcb->TCB_ucSchedActivateMode = LW_OPTION_RESPOND_AUTO;             /*  相同优先级响应程度          */
                                                                        /*  初始化为内核自动识别        */
    ptcb->TCB_ucSchedActivate     = LW_SCHED_ACT_OTHER;                 /*  初始化为非中断激活方式      */
    ptcb->TCB_iDeleteProcStatus   = LW_TCB_DELETE_PROC_NONE;            /*  没有被删除                  */
    
#if	LW_CFG_THREAD_RESTART_EN > 0
    ptcb->TCB_pthreadStartAddress = pThread;                            /*  保存任务起始代码指针        */
#endif

    ptcb->TCB_ulCPUTicks       = 0ul;
    ptcb->TCB_ulCPUKernelTicks = 0ul;

#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    ptcb->TCB_ulCPUUsageTicks       = 0ul;                              /*  CPU利用率计算               */
    ptcb->TCB_ulCPUUsageKernelTicks = 0ul;
#endif

#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0                                     /*  看门狗不工作                */
    __WAKEUP_NODE_INIT(&ptcb->TCB_wunWatchDog);
#endif

    _LIST_LINE_INIT_IN_CODE(ptcb->TCB_lineManage);
    _LIST_RING_INIT_IN_CODE(ptcb->TCB_ringReady);

#if	LW_CFG_THREAD_POOL_EN > 0
    _LIST_RING_INIT_IN_CODE(ptcb->TCB_ringThreadPool);                  /*  清除线程池指针              */
#endif

#if LW_CFG_EVENT_EN > 0
    _LIST_RING_INIT_IN_CODE(ptcb->TCB_ringEvent);
    ptcb->TCB_peventPtr           = LW_NULL;                            /*  清除任务等待事件            */
    ptcb->TCB_ucIsEventDelete     = LW_EVENT_EXIST;                     /*  事件存在                    */
    ptcb->TCB_ppringPriorityQueue = LW_NULL;                            /*  初始化无等待队列            */
#endif

#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
#if (LW_CFG_THREAD_DEL_EN > 0) || (LW_CFG_SIGNAL_EN > 0)
    ptcb->TCB_pesnPtr = LW_NULL;                                        /*  指向该线程等待的            */
#endif                                                                  /*  EVENTSETNODE 块             */
#endif
    
    ptcb->TCB_ulThreadLockCounter = 0;                                  /*  线程锁变量                  */
    
    if (ulOption & LW_OPTION_THREAD_SAFE) {
        ptcb->TCB_ulThreadSafeCounter = 1ul;                            /*  任务初始化为安全状态        */
    } else {
        ptcb->TCB_ulThreadSafeCounter = 0ul;                            /*  任务初始化为不安全状态      */
    }

#if LW_CFG_THREAD_DEL_EN > 0
    ptcb->TCB_ptcbDeleteMe   = LW_NULL;                                 /*  没有任务删除这个新任务      */
    ptcb->TCB_ptcbDeleteWait = LW_NULL;
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
    ptcb->TCB_plinePrivateVars  = LW_NULL;                              /*  初始化没有私有变量          */
#endif
    
#if LW_CFG_COROUTINE_EN > 0
    pcrcb = &ptcb->TCB_crcbOrigent;
    pcrcb->COROUTINE_pstkStackNow     = pstkStackNow;
    pcrcb->COROUTINE_pstkStackTop     = pstkStackTop;
    pcrcb->COROUTINE_pstkStackBottom  = pstkStackButtom;
    pcrcb->COROUTINE_stStackSize      = stStackSize;
    pcrcb->COROUTINE_pstkStackLowAddr = pstkStackLowAddr;
    pcrcb->COROUTINE_ulThread         = ulId;
    pcrcb->COROUTINE_ulFlags          = 0ul;                            /*  不需要独立释放              */

    ptcb->TCB_pringCoroutineHeader = LW_NULL;
    _List_Ring_Add_Ahead(&pcrcb->COROUTINE_ringRoutine,
                         &ptcb->TCB_pringCoroutineHeader);              /*  插入协程表                  */
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
    
#if LW_CFG_THREAD_CANCEL_EN > 0
    ptcb->TCB_bCancelRequest = LW_FALSE;                                /*  没有请求被 CANCEL           */
    ptcb->TCB_iCancelState   = LW_THREAD_CANCEL_ENABLE;                 /*  PTHREAD_CANCEL_ENABLE       */
    ptcb->TCB_iCancelType    = LW_THREAD_CANCEL_DEFERRED;               /*  PTHREAD_CANCEL_DEFERRED     */
#endif
    
    if (ulOption & LW_OPTION_THREAD_DETACHED) {                         /*  不使用合并算法              */
        ptcb->TCB_bDetachFlag = LW_TRUE;
    } else {
        ptcb->TCB_bDetachFlag = LW_FALSE;                               /*  使用合并                    */
    }
    ptcb->TCB_ppvJoinRetValSave = LW_NULL;                              /*  返回值地址                  */
    ptcb->TCB_plineJoinHeader   = LW_NULL;                              /*  没有线程合并自己            */
    _LIST_LINE_INIT_IN_CODE(ptcb->TCB_lineJoin);                        /*  没有合并其他线程            */
    ptcb->TCB_ptcbJoin = LW_NULL;                                       /*  没有合并到其他线程          */

    LW_SPIN_INIT(&ptcb->TCB_slLock);                                    /*  初始化自旋锁                */
    
#if LW_CFG_SMP_EN > 0
    if (ulOption & LW_OPTION_THREAD_NO_AFFINITY) {                      /*  是否继承 CPU 亲和度关系     */
        ptcb->TCB_bCPULock  = LW_FALSE;
        ptcb->TCB_ulCPULock = 0ul;
    } else {
        ptcb->TCB_bCPULock  = ptcbCur->TCB_bCPULock;
        ptcb->TCB_ulCPULock = ptcbCur->TCB_ulCPULock;                   /*  继承 CPU 亲和度关系         */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcb->TCB_ulCPUId = 0ul;                                            /*  默认使用 0 号 CPU           */
    ptcb->TCB_bIsCand = LW_FALSE;                                       /*  没有加入运行表              */
    
#if LW_CFG_SMP_EN > 0
    ptcb->TCB_ptcbWaitStatus       = LW_NULL;
    ptcb->TCB_plineStatusReqHeader = LW_NULL;
    _LIST_LINE_INIT_IN_CODE(ptcb->TCB_lineStatusPend);
#endif                                                                  /*  LW_CFG_SMP_EN               */

    ptcb->TCB_uiStatusChangeReq = 0;
    ptcb->TCB_ulStopNesting     = 0ul;
    
#if (LW_CFG_DEVICE_EN > 0)                                              /*  设备管理                    */
    ptcb->TCB_pvIoEnv = LW_NULL;                                        /*  默认使用全局 io 环境        */
    
    ptcb->TCB_iTaskStd[0] = 0;                                          /*  与 GLOBAL IN OUT ERR 相同   */
    ptcb->TCB_iTaskStd[1] = 1;
    ptcb->TCB_iTaskStd[2] = 2;
#endif

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    lib_nlreent_init(ulId);                                             /*  init libc reent             */
#endif

#if LW_CFG_GDB_EN > 0
    ptcb->TCB_ulStepAddr = (addr_t)PX_ERROR;
    ptcb->TCB_ulStepInst = 0ul;
    ptcb->TCB_bStepClear = LW_TRUE;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

#if LW_CFG_THREAD_NOTE_PAD_EN > 0                                       /*  任务记事本                  */
    pulNote = &ptcb->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[0];
    for (ucI = 0; ucI < LW_CFG_MAX_NOTEPADS; ucI++) {
        *pulNote++ = 0ul;
    }
#endif

    __KERNEL_SPACE_SET2(ptcb, 0);                                       /*  初始化为用户空间            */
    ptcb->TCB_pvPosixContext = LW_NULL;                                 /*  非 posix 线程               */

    ptcb->TCB_usStatus = LW_THREAD_STATUS_RDY;                          /*  暂设为就绪状态              */

#if LW_CFG_THREAD_SUSPEND_EN > 0                                        /*  阻塞                        */
    if (ulOption & LW_OPTION_THREAD_SUSPEND) {
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
        ptcb->TCB_ulSuspendNesting = 1;                                 /*  阻塞                        */
    } else {
        ptcb->TCB_ulSuspendNesting = 0;
    }
#else
    ptcb->TCB_ulSuspendNesting = 0;
#endif                                                                  /*  LW_CFG_THREAD_SUSPEND_EN    */
    
    ptcb->TCB_usStatus |= LW_THREAD_STATUS_INIT;                        /*  初始化                      */

    if (LW_SYS_STATUS_IS_RUNNING()) {
        INT i;
        
        ptcb->TCB_uid  = ptcbCur->TCB_uid;
        ptcb->TCB_gid  = ptcbCur->TCB_gid;
        
        ptcb->TCB_euid = ptcbCur->TCB_euid;
        ptcb->TCB_egid = ptcbCur->TCB_egid;
        
        ptcb->TCB_suid = ptcbCur->TCB_suid;
        ptcb->TCB_sgid = ptcbCur->TCB_sgid;
    
        for (i = 0; i < ptcbCur->TCB_iNumSuppGid; i++) {                /*  保存附加组信息              */
            ptcb->TCB_suppgid[i] = ptcbCur->TCB_suppgid[i];
        }
        ptcb->TCB_iNumSuppGid = ptcbCur->TCB_iNumSuppGid;
    
    } else {
        ptcb->TCB_uid  = 0;                                             /*  root 权限                   */
        ptcb->TCB_gid  = 0;
        
        ptcb->TCB_euid = 0;
        ptcb->TCB_egid = 0;
        
        ptcb->TCB_suid = 0;
        ptcb->TCB_sgid = 0;
        
        ptcb->TCB_iNumSuppGid = 0;                                      /*  没有附加组信息              */
    }

#if LW_CFG_CPU_FPU_EN > 0
    ptcb->TCB_pvStackFP = &ptcb->TCB_fpuctxContext;
    __ARCH_FPU_CTX_INIT(ptcb->TCB_pvStackFP);                           /*  初始化当前任务 FPU 上下文   */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    _K_ptcbTCBIdTable[_ObjectGetIndex(ulId)] = ptcb;                    /*  保存TCB控制块               */
    _List_Line_Add_Ahead(&ptcb->TCB_lineManage, 
                         &_K_plineTCBHeader);                           /*  进入 TCB 管理链表           */
    __KERNEL_EXIT();                                                    /*  退出内核                    */

#if LW_CFG_MODULELOADER_EN > 0                                          /*  如果为 GLOBAL 则不属于进程  */
    if (ptcbCur && !(ulOption & LW_OPTION_OBJECT_GLOBAL)) {             /*  继承进程控制块              */
        ptcb->TCB_pvVProcessContext = ptcbCur->TCB_pvVProcessContext;
        vprocThreadAdd(ptcb->TCB_pvVProcessContext, ptcb);
    }
#endif

    bspTCBInitHook(ulId, ptcb);                                         /*  调用钩子函数                */
    __LW_THREAD_INIT_HOOK(ulId, ptcb);
    bspTaskCreateHook(ulId);                                            /*  调用钩子函数                */
    __LW_THREAD_CREATE_HOOK(ulId, ulOption);
}
/*********************************************************************************************************
** 函数名称: _TCBDestroy
** 功能描述: 销毁一个TCB
** 输　入  : ptcb              任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _TCBDestroy (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_MODULELOADER_EN > 0
    if (!(ptcb->TCB_ulOption & LW_OPTION_OBJECT_GLOBAL)) {
        vprocThreadDelete(ptcb->TCB_pvVProcessContext, ptcb);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** 函数名称: _TCBBuildExt
** 功能描述: 创建一个TCB扩展模块
** 输　入  : ptcb              任务控制块
** 输　出  : ERROR_NONE        表示正确
**           其他值错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _TCBBuildExt (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_THREAD_EXT_EN > 0
    REGISTER __PLW_THREAD_EXT  ptex = &ptcb->TCB_texExt;
    
    ptex->TEX_ulMutex        = 0ul;                                     /*  暂时不需要内部互斥量        */
    ptex->TEX_pmonoCurHeader = LW_NULL;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _TCBDestroyExt
** 功能描述: 销毁一个TCB扩展模块
** 输　入  : ptcb              任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _TCBDestroyExt (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_THREAD_EXT_EN > 0
    REGISTER __PLW_THREAD_EXT   ptex = &ptcb->TCB_texExt;
    
    if (ptex->TEX_ulMutex) {                                            /*  是否需要删除互斥量          */
        API_SemaphoreMDelete(&ptex->TEX_ulMutex);
    }
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
}
/*********************************************************************************************************
** 函数名称: _TCBCleanupPopExt
** 功能描述: 将所有压栈函数运行并释放
** 输　入  : ptcb              任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _TCBCleanupPopExt (PLW_CLASS_TCB  ptcb)
{
#if LW_CFG_THREAD_EXT_EN > 0
             INTREG                 iregInterLevel;
    REGISTER __PLW_THREAD_EXT       ptex = &ptcb->TCB_texExt;
    REGISTER __PLW_CLEANUP_ROUTINE  pcur;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    while (ptex->TEX_pmonoCurHeader) {
        pcur = (__PLW_CLEANUP_ROUTINE)(ptex->TEX_pmonoCurHeader);       /*  第一个元素                  */
        _list_mono_next(&ptex->TEX_pmonoCurHeader);
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        
		LW_SOFUNC_PREPARE(pcur->CUR_pfuncClean);
        pcur->CUR_pfuncClean(pcur->CUR_pvArg);                          /*  执行销毁程序                */
        __KHEAP_FREE(pcur);                                             /*  释放                        */
    
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
}
/*********************************************************************************************************
** 函数名称: _TCBTryRun
** 功能描述: 将创建好的 TCB 根据条件放入就续表
** 输　入  : ptcb              任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _TCBTryRun (PLW_CLASS_TCB  ptcb)
{
             INTREG         iregInterLevel;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */

    ptcb->TCB_usStatus &= ~LW_THREAD_STATUS_INIT;                       /*  去掉 init 标志              */
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  就绪                        */
        ppcb = _GetPcb(ptcb);
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  加入到相对优先级就绪环      */
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
