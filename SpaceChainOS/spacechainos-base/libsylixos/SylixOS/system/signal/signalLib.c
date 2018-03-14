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
** 文   件   名: signalLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 06 月 03 日
**
** 描        述: 系统信号处理内部函数库.
**
** 注        意: 系统移植的时候, 必须将开关中断控制放在任务上下文中. 
                 
** BUG
2007.09.08  一旦进入 _SignalShell 就关中断,然后设置新的屏蔽码.
2008.01.16  初始化句柄都为 SIG_IGN 处理句柄.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.08.11  _doPendKill 加入对句柄类型的判断, 由于 shell 的一次误操作导致发送信号将 idle 删除了, 就是因为
            没有检查句柄类型.
2009.03.05  针对内核裁剪加入处理宏.
2009.04.14  加入 SMP 支持.
2009.05.26  线程在阻塞时收到信号, 当线程在执行信号句柄时被 resume, 那么信号返回时不应该再 suspend.
2010.01.22  __sigMakeReady() 需要锁定内核.
2011.02.22  重构整个 signal 系统, 实现 POSIX 要求的信号功能.
2011.02.24  __sigMakeReady() 对 suspend 状态的线程无能为力! 如果在信号返回途中被强占, 当前任务堆栈将会错误
            目前只能等 resume 后才能执行信号句柄.
2011.02.26  加入对 SIGEV_NONE SIGEV_SIGNAL SIGEV_THREAD 的支持.
2011.05.17  当收到需要结束线程的信号时, 果断结束线程.
2011.08.16  __sigRunHandle() 当为 default 处理时, 需要判断是否为中止信号, 如果是需要终止任务.
2011.11.17  加入 SIGSTOP SIGTSTP 处理句柄.
2012.04.09  系统标准信号服务函数加入 psiginfo 参数.
2012.04.22  SIGKILL 默认处理可导致线程被终止.
2012.05.07  使用 OSStkSetFp() 函数类同步 fp 指针, 保证 unwind 堆栈的正确性.
2012.09.14  _doSigEvent() 支持进程号.
2012.10.23  解决 SMP 系统下, 发送信号时, 如果目标线程在另一个 CPU 执行时的错误.
2012.12.09  进程暂停时, 需要对父亲进行通知.
2012.12.25  针对 SIGSEGV 信号, 默认信号句柄显示更加完整的信息.
            信号进入之前记录发送目标任务的内核状态, 然后信号句柄中需要在用户环境中运行, 退出信号时需要回复
            之前的状态.
2013.01.15  进程异常信号退出时需要使用 _exit() api.
            __sigRunHandle() 遇到非可屏蔽信号, 不管是否有用户代码执行都必须杀死自己.
2013.05.05  支持 SA_RESTART 信号.
2013.06.17  保持信号上下文不会修改当前任务 errno
2013.09.04  父进程如果含有 SA_NOCLDSTOP 标示, 则不接受子进程暂停信号.
2013.09.17  加入对 SIGCNCL 信号的处理.
2013.11.24  加入对 signalfd 功能的支持.
2013.12.12  _sigGetLsb() 使用 archFindLsb() 寻找最需要递送的信号编号最小的信号.
2014.06.01  遇到紧急停止信号, 进程将会被强制停止.
2014.09.30  SA_SIGINFO 加入对信号上下文参数的传递, 由于解除阻塞运行的信号, 则信号上下文参数为 LW_NULL.
2014.10.31  支持 POSIX 定义的指定堆栈的信号上下文操作.
2014.11.04  支持 SA_NOCLDWAIT 自动回收子进程.
2015.11.16  __sigMakeReady() 仅需要关闭中断即可.
            _sigPendAlloc() 与 _sigPendFree() 不需要关闭中断.
2016.04.15  信号上下文需要保存 FPU 上下文.
2016.07.21  简化任务就绪设计.
2016.08.11  新建线程集成主线程信号掩码.
2017.08.17  __sigCtlCreate() 需要处理堆栈对齐.
2017.08.18  修正 __sigReturn() 调度器返回值可能出错的错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0
#include "signalPrivate.h"
/*********************************************************************************************************
  进程相关处理
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "sys/vproc.h"
#include "sys/wait.h"
#include "unistd.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
#define __tcb_pid(ptcb)     vprocGetPidByTcbNoLock(ptcb)
#else
#define __tcb_pid(ptcb)     0
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  LW_CFG_MAX_SIGQUEUE_NODES 必须大于 2
*********************************************************************************************************/
#if     LW_CFG_MAX_SIGQUEUE_NODES < 2
#undef  LW_CFG_MAX_SIGQUEUE_NODES
#define LW_CFG_MAX_SIGQUEUE_NODES   2
#endif                                                                  /*  LW_CFG_MAX_SIGQUEUE_NODES   */
/*********************************************************************************************************
  信号内部全局变量
*********************************************************************************************************/
static LW_CLASS_SIGCONTEXT      _K_sigctxTable[LW_CFG_MAX_THREADS];
static LW_CLASS_SIGPEND         _K_sigpendBuffer[LW_CFG_MAX_SIGQUEUE_NODES];
static LW_LIST_RING_HEADER      _K_pringSigPendFreeHeader;
#if LW_CFG_SIGNALFD_EN > 0
static LW_OBJECT_HANDLE         _K_hSigfdSelMutex;
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
/*********************************************************************************************************
  信号内部函数
*********************************************************************************************************/
extern VOID                     __sigEventArgInit(VOID);
static VOID                     __sigShell(PLW_CLASS_SIGCTLMSG  psigctlmsg);
       PLW_CLASS_SIGCONTEXT     _signalGetCtx(PLW_CLASS_TCB  ptcb);
static PLW_CLASS_SIGPEND        _sigPendAlloc(VOID);
       VOID                     _sigPendFree(PLW_CLASS_SIGPEND  psigpendFree);
       BOOL                     _sigPendRun(PLW_CLASS_TCB  ptcb);
static BOOL                     _sigPendRunSelf(VOID);
/*********************************************************************************************************
** 函数名称: __signalCnclHandle
** 功能描述: SIGCNCL 信号的服务函数
** 输　入  : iSigNo        信号数值
**           psiginfo      信号信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __signalCnclHandle (INT  iSigNo, struct siginfo *psiginfo)
{
    LW_OBJECT_HANDLE    ulId;
    PLW_CLASS_TCB       ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulId = ptcbCur->TCB_ulId;
    
    if (ptcbCur->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE   &&
        ptcbCur->TCB_iCancelType  == LW_THREAD_CANCEL_DEFERRED &&
        (ptcbCur->TCB_bCancelRequest)) {
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadDelete(&ulId, LW_THREAD_CANCELED);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
    } else {
        ptcbCur->TCB_bCancelRequest = LW_TRUE;
    }
}
/*********************************************************************************************************
** 函数名称: __signalExitHandle
** 功能描述: SIGCANCEL 信号的服务函数
** 输　入  : iSigNo        信号数值
**           psiginfo      信号信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __signalExitHandle (INT  iSigNo, struct siginfo *psiginfo)
{
    LW_OBJECT_HANDLE    ulId;
    PLW_CLASS_TCB       ptcbCur;
#if LW_CFG_MODULELOADER_EN > 0
    pid_t               pid = getpid();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulId = ptcbCur->TCB_ulId;

#if LW_CFG_MODULELOADER_EN > 0
    if ((pid > 0) && (iSigNo != SIGTERM)) {
        vprocExitModeSet(pid, LW_VPROC_EXIT_FORCE);                     /*  强制进程退出                */
        vprocSetImmediatelyTerm(pid);                                   /*  立即退出模式                */
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if ((iSigNo == SIGBUS)  ||
        (iSigNo == SIGSEGV) ||
        (iSigNo == SIGILL)  ||
        (iSigNo == SIGFPE)  ||
        (iSigNo == SIGSYS)) {
#if LW_CFG_MODULELOADER_EN > 0
        __LW_FATAL_ERROR_HOOK(pid, ulId, psiginfo);                     /*  关键性异常                  */
#else
        __LW_FATAL_ERROR_HOOK(0, ulId, psiginfo);                       /*  关键性异常                  */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        _exit(psiginfo->si_int);
    
    } else {                                                            /*  非关键性异常                */
        API_ThreadDelete(&ulId, (PVOID)psiginfo->si_int);               /*  删除自己                    */
    }                                                                   /*  如果在安全模式, 则退出安全  */
}                                                                       /*  模式后, 自动被删除          */
/*********************************************************************************************************
** 函数名称: __signalWaitHandle
** 功能描述: 回收子进程资源
** 输　入  : iSigNo        信号数值
**           psiginfo      信号信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __signalWaitHandle (INT  iSigNo, struct siginfo *psiginfo)
{
#if LW_CFG_MODULELOADER_EN > 0
    reclaimchild();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** 函数名称: __signalStopHandle
** 功能描述: SIGSTOP / SIGTSTP 信号的服务函数
** 输　入  : iSigNo        信号数值
**           psiginfo      信号信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __signalStopHandle (INT  iSigNo, struct siginfo *psiginfo)
{
    sigset_t            sigsetMask;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC        *pvproc = __LW_VP_GET_CUR_PROC();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    sigsetMask = ~__SIGNO_UNMASK;
    
    sigdelset(&sigsetMask, SIGCONT);                                    /*  unmask SIGCONT              */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvproc) {
        vprocNotifyParent(pvproc, CLD_STOPPED, LW_TRUE);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    sigsuspend(&sigsetMask);                                            /*  阻塞线程                    */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvproc) {
        vprocNotifyParent(pvproc, CLD_CONTINUED, LW_TRUE);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** 函数名称: __signalStkShowHandle
** 功能描述: 打印上下文服务函数
** 输　入  : psigctlmsg              信号控制信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __signalStkShowHandle (PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
#if LW_CFG_ABORT_CALLSTACK_INFO_EN > 0
    API_BacktraceShow(STD_OUT, 100);
#endif                                                                  /*  LW_CFG_ABORT_CALLSTACK_IN...*/

#if LW_CFG_DEVICE_EN > 0
    archTaskCtxShow(STD_OUT, (PLW_STACK)psigctlmsg->SIGCTLMSG_pvStackRet);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
}
/*********************************************************************************************************
** 函数名称: __sigTaskCreateHook
** 功能描述: 线程建立时，初始化线程控制块中的信号控制部分
** 输　入  : ulId                  线程 ID 号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __sigTaskCreateHook (LW_OBJECT_HANDLE  ulId)
{
    PLW_CLASS_TCB          ptcb = __GET_TCB_FROM_INDEX(_ObjectGetIndex(ulId));
    PLW_CLASS_SIGCONTEXT   psigctx = _signalGetCtx(ptcb);
    
#if LW_CFG_MODULELOADER_EN > 0
    PLW_CLASS_TCB          ptcbCur;
    PLW_CLASS_SIGCONTEXT   psigctxCur;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    lib_bzero(psigctx, sizeof(LW_CLASS_SIGCONTEXT));                    /*  所有信号 DEFAULT 处理       */

#if LW_CFG_MODULELOADER_EN > 0
    if (LW_SYS_STATUS_IS_RUNNING() && __LW_VP_GET_TCB_PROC(ptcb)) {     /*  操作系统正在运行            */
        INT     i;
        
        LW_TCB_GET_CUR_SAFE(ptcbCur);
        psigctxCur = _signalGetCtx(ptcbCur);
        psigctx->SIGCTX_sigsetMask = psigctxCur->SIGCTX_sigsetMask;     /*  继承信号掩码                */
        
        for (i = 0; i < NSIG; i++) {
            psigctx->SIGCTX_sigaction[i] = psigctxCur->SIGCTX_sigaction[i];
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    psigctx->SIGCTX_stack.ss_flags = SS_DISABLE;                        /*  不使用自定义堆栈            */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (_K_hSigfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _K_hSigfdSelMutex =  API_SemaphoreMCreate("sigfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }
    psigctx->SIGCTX_selwulist.SELWUL_hListLock = _K_hSigfdSelMutex;
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
}
/*********************************************************************************************************
** 函数名称: __sigTaskDeleteHook
** 功能描述: 线程删除时调用的回调函数
** 输　入  : ulId                   线程 ID 号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0

static VOID    __sigTaskDeleteHook (LW_OBJECT_HANDLE  ulId)
{
    REGISTER INT                    iI;
             PLW_CLASS_SIGCONTEXT   psigctx;
             PLW_CLASS_TCB          ptcb = __GET_TCB_FROM_INDEX(_ObjectGetIndex(ulId));
    REGISTER PLW_CLASS_SIGPEND      psigpend;
    
    psigctx = _signalGetCtx(ptcb);                                      /*  获得 sig context            */

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    for (iI = 0; iI < NSIG; iI++) {
        if (psigctx->SIGCTX_pringSigQ[iI]) {                            /*  存在没有处理的排队信号      */
                     PLW_LIST_RING  pringHead = psigctx->SIGCTX_pringSigQ[iI];
            REGISTER PLW_LIST_RING  pringSigP = pringHead;
            
            do {
                psigpend  = _LIST_ENTRY(pringSigP, 
                                        LW_CLASS_SIGPEND, 
                                        SIGPEND_ringSigQ);              /*  获得 sigpend 控制块地址     */
                pringSigP = _list_ring_get_next(pringSigP);             /*  下一个节点                  */
                
                if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
                    (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {
                    _sigPendFree(psigpend);                             /*  需要交还空闲队列            */
                }
            } while (pringSigP != pringHead);
        
            psigctx->SIGCTX_pringSigQ[iI] = LW_NULL;
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    psigctx->SIGCTX_bRead      = LW_FALSE;
    psigctx->SIGCTX_sigsetWait = 0ull;
    SEL_WAKE_UP_TERM(&psigctx->SIGCTX_selwulist);
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
/*********************************************************************************************************
** 函数名称: __sigMakeReady
** 功能描述: 将指定线程设置为就绪状态并且设置调度器的返回值为 LW_SIGNAL_RESTART (此函数在内核状态被调用)
** 输　入  : ptcb                   目的线程控制块
**           iSigNo                 信号值
**           piSchedRet             退出信号句柄后, 调度器返回值.
**           iSaType                信号类型 (LW_SIGNAL_EINTR or LW_SIGNAL_RESTART)
** 输　出  : 正在等待时的剩余时间.
** 全局变量: 
** 调用模块: 
** 注  意  : 当目标线程处于等待事件,或者延迟时,需要将线程的调度器返回值设置为 iSaType.
*********************************************************************************************************/
static VOID  __sigMakeReady (PLW_CLASS_TCB  ptcb, 
                             INT            iSigNo,
                             INT           *piSchedRet,
                             INT            iSaType)
{
             INTREG                  iregInterLevel;
    REGISTER PLW_CLASS_PCB           ppcb;
             PLW_CLASS_SIGCONTEXT    psigctx;

    *piSchedRet = ERROR_NONE;                                           /*  默认为就绪状态              */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  处于就绪状态, 直接退出      */
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        return;
    }
    
    ppcb = _GetPcb(ptcb);                                               /*  获得优先级控制块            */
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {                  /*  存在于唤醒队列中            */
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  从等待链中删除              */
        ptcb->TCB_ulDelay = 0ul;
        *piSchedRet = iSaType;                                          /*  设置调度器返回值            */
    }
    
    if (__SIGNO_MUST_EXIT & __sigmask(iSigNo)) {                        /*  必须退出信号                */
        if (ptcb->TCB_ptcbJoin) {
            _ThreadDisjoin(ptcb->TCB_ptcbJoin, ptcb);                   /*  退出 join 状态, 不操作就绪表*/
        }                                                               /*  否则可能产生重复的操作就绪表*/
    }
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_PEND_ANY) {               /*  检查是否在等待事件          */
        *piSchedRet = iSaType;                                          /*  设置调度器返回值            */
        ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_PEND_ANY);             /*  等待超时清除事件等待位      */
        
        psigctx = _signalGetCtx(ptcb);
        if (psigctx->SIGCTX_sigwait &&
            psigctx->SIGCTX_sigwait->SIGWT_sigset & __sigmask(iSigNo)) {/*  sigwait                     */
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;
            
        } else {
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                 /*  等待超时                    */
        }

#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        if (ptcb->TCB_peventPtr) {
            _EventUnQueue(ptcb);
        } else 
#endif                                                                  /*  (LW_CFG_EVENT_EN > 0) &&    */
        {
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
            if (ptcb->TCB_pesnPtr) {
                _EventSetUnQueue(ptcb->TCB_pesnPtr);
            }
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0) && */
        }
    } else {
        ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;                   /*  没有等待事件                */
    }
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
        __ADD_TO_READY_RING(ptcb, ppcb);                                /*  加入就绪环                  */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
}
/*********************************************************************************************************
** 函数名称: __sigCtlCreate
** 功能描述: 在堆栈中创造一个用于执行信号和信号退出的环境
** 输　入  : ptcb                   任务控制块
**           psigctx                信号任务相关信息
**           psiginfo               信号信息
**           ulSuspendNesting       阻塞嵌套数
**           iSchedRet              期望的调度器返回值
**           psigsetMask            执行完信号句柄后需要重新设置的掩码
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  VOID  __sigCtlCreate (PLW_CLASS_TCB         ptcb,
                              PLW_CLASS_SIGCONTEXT  psigctx,
                              struct siginfo       *psiginfo,
                              INT                   iSchedRet,
                              sigset_t             *psigsetMask)
{
    PLW_CLASS_SIGCTLMSG  psigctlmsg;
    PLW_STACK            pstkSignalShell;                               /*  启动signalshell的堆栈点     */
    BYTE                *pucStkNow;
    stack_t             *pstack;

    if (psigctx && (psigctx->SIGCTX_stack.ss_flags == 0)) {             /*  使用用户指定的信号堆栈      */
        pstack = &psigctx->SIGCTX_stack;
        if ((ptcb->TCB_pstkStackNow >= (PLW_STACK)pstack->ss_sp) && 
            (ptcb->TCB_pstkStackNow < (PLW_STACK)((size_t)pstack->ss_sp + pstack->ss_size))) {
            pucStkNow = (BYTE *)ptcb->TCB_pstkStackNow;                 /*  已经在用户指定的信号堆栈中  */
        
        } else {
#if	CPU_STK_GROWTH == 0
            pucStkNow = (BYTE *)pstack->ss_sp;
#else
            pucStkNow = (BYTE *)pstack->ss_sp + pstack->ss_size;
#endif                                                                  /*  CPU_STK_GROWTH == 0         */
        }
    } else {
        pucStkNow = (BYTE *)ptcb->TCB_pstkStackNow;
    }

#if	CPU_STK_GROWTH == 0                                                 /*  CPU_STK_GROWTH == 0         */
    pucStkNow  += sizeof(LW_STACK);                                     /*  向空栈方向移动一个堆栈空间  */
    pucStkNow   = (BYTE *)ROUND_UP(pucStkNow, ARCH_STK_ALIGN_SIZE);
    psigctlmsg  = (PLW_CLASS_SIGCTLMSG)pucStkNow;                       /*  记录 signal contrl msg 位置 */
    pucStkNow  += __SIGCTLMSG_SIZE_ALIGN;                               /*  让出 signal contrl msg 空间 */

#if LW_CFG_CPU_FPU_EN > 0
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        pucStkNow = (BYTE *)ROUND_UP(pucStkNow, ARCH_FPU_CTX_ALIGN);    /*  必须保证浮点上下文对齐要求  */
        psigctlmsg->SIGCTLMSG_pfpuctx = (LW_FPU_CONTEXT *)pucStkNow;
        pucStkNow  += __SIGFPUCTX_SIZE_ALIGN;                           /*  让出 signal fpu ctx 空间    */
        
    } else {
        psigctlmsg->SIGCTLMSG_pfpuctx = LW_NULL;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#else                                                                   /*  CPU_STK_GROWTH == 1         */
    pucStkNow  -= __SIGCTLMSG_SIZE_ALIGN;                               /*  让出 signal contrl msg 空间 */
    pucStkNow   = (BYTE *)ROUND_DOWN(pucStkNow, ARCH_STK_ALIGN_SIZE);
    psigctlmsg  = (PLW_CLASS_SIGCTLMSG)pucStkNow;                       /*  记录 signal contrl msg 位置 */
    pucStkNow  -= sizeof(LW_STACK);                                     /*  向空栈方向移动一个堆栈空间  */
    
#if LW_CFG_CPU_FPU_EN > 0
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        pucStkNow  -= __SIGFPUCTX_SIZE_ALIGN;                           /*  让出 signal fpu ctx 空间    */
        pucStkNow   = (BYTE *)ROUND_DOWN(pucStkNow, ARCH_FPU_CTX_ALIGN);/*  必须保证浮点上下文对齐要求  */
        psigctlmsg->SIGCTLMSG_pfpuctx = (LW_FPU_CONTEXT *)pucStkNow;
        pucStkNow  -= sizeof(LW_STACK);                                 /*  向空栈方向移动一个堆栈空间  */
        
    } else {
        psigctlmsg->SIGCTLMSG_pfpuctx = LW_NULL;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  CPU_STK_GROWTH              */

    psigctlmsg->SIGCTLMSG_iSchedRet       = iSchedRet;
    psigctlmsg->SIGCTLMSG_iKernelSpace    = __KERNEL_SPACE_GET2(ptcb);
    psigctlmsg->SIGCTLMSG_pvStackRet      = ptcb->TCB_pstkStackNow;
#if defined(LW_CFG_CPU_ARCH_C6X)
    psigctlmsg->SIGCTLMSG_ulContextType   = ptcb->TCB_ulContextType;    /*  记录当前上下文类型          */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
    psigctlmsg->SIGCTLMSG_siginfo         = *psiginfo;
    psigctlmsg->SIGCTLMSG_sigsetMask      = *psigsetMask;
    psigctlmsg->SIGCTLMSG_ulLastError     = ptcb->TCB_ulLastError;      /*  记录当前错误号              */
    psigctlmsg->SIGCTLMSG_ucWaitTimeout   = ptcb->TCB_ucWaitTimeout;    /*  记录 timeout 标志           */
    psigctlmsg->SIGCTLMSG_ucIsEventDelete = ptcb->TCB_ucIsEventDelete;
    
    pstkSignalShell = archTaskCtxCreate((PTHREAD_START_ROUTINE)__sigShell, 
                                        (PVOID)psigctlmsg,
                                        (PLW_STACK)pucStkNow,
                                        0);                             /*  建立信号外壳环境            */
    
    archTaskCtxSetFp(pstkSignalShell, ptcb->TCB_pstkStackNow);          /*  保存 fp, 使 callstack 正常  */
    
    ptcb->TCB_pstkStackNow  = pstkSignalShell;                          /*  保存建立好的信号外壳堆栈    */
#if defined(LW_CFG_CPU_ARCH_C6X)
    ptcb->TCB_ulContextType = 0;                                        /*  小上下文类型                */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
    
    _StackCheckGuard(ptcb);                                             /*  堆栈警戒检查                */
    
    __KERNEL_SPACE_SET2(ptcb, 0);                                       /*  信号句柄运行在任务状态下    */
}
/*********************************************************************************************************
** 函数名称: __sigReturn
** 功能描述: 信号句柄的外壳函数调用此函数从信号上下文中返回任务上下文.
** 输　入  : psigctx                 信号任务相关信息
**           ptcbCur                 当前任务上下文
**           psigctlmsg              信号控制信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __sigReturn (PLW_CLASS_SIGCONTEXT  psigctx, 
                          PLW_CLASS_TCB         ptcbCur, 
                          PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
    INTREG   iregInterLevel;

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    psigctx->SIGCTX_sigsetMask = psigctlmsg->SIGCTLMSG_sigsetMask;
                                                                        /*  恢复原先的掩码              */
    _sigPendRunSelf();                                                  /*  检查并运行需要运行的信号    */
    __KERNEL_SPACE_SET(psigctlmsg->SIGCTLMSG_iKernelSpace);             /*  恢复成进入信号前的状态      */
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭当前 CPU 中断           */
    _SchedSetRet(psigctlmsg->SIGCTLMSG_iSchedRet);                      /*  通知调度器返回的情况        */
#if LW_CFG_CPU_FPU_EN > 0
    if (psigctlmsg->SIGCTLMSG_pfpuctx &&
        (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP)) {
        __ARCH_FPU_RESTORE((PVOID)psigctlmsg->SIGCTLMSG_pfpuctx);       /*  恢复被信号中断前 FPU 上下文 */
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
    
    ptcbCur->TCB_ulLastError     = psigctlmsg->SIGCTLMSG_ulLastError;   /*  恢复错误号                  */
    ptcbCur->TCB_ucWaitTimeout   = psigctlmsg->SIGCTLMSG_ucWaitTimeout; /*  恢复 timeout 标志           */
    ptcbCur->TCB_ucIsEventDelete = psigctlmsg->SIGCTLMSG_ucIsEventDelete;
    ptcbCur->TCB_pstkStackNow    = (PLW_STACK)psigctlmsg->SIGCTLMSG_pvStackRet;
#if defined(LW_CFG_CPU_ARCH_C6X)
    ptcbCur->TCB_ulContextType   = psigctlmsg->SIGCTLMSG_ulContextType; /*  恢复当前上下文类型          */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
    
    KN_SMP_MB();
#if defined(LW_CFG_CPU_ARCH_C6X)
    archSigCtxLoad(psigctlmsg->SIGCTLMSG_pvStackRet,
                   psigctlmsg->SIGCTLMSG_ulContextType);                /*  从信号上下文中返回          */
#else
    archSigCtxLoad(psigctlmsg->SIGCTLMSG_pvStackRet);                   /*  从信号上下文中返回          */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
    KN_INT_ENABLE(iregInterLevel);                                      /*  运行不到这里                */
}
/*********************************************************************************************************
** 函数名称: __sigRunHandle
** 功能描述: 信号运行已经安装的句柄
** 输　入  : psigctx               信号任务相关信息
**           iSigNo                信号的值
**           psiginfo              需要运行的信号信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __sigRunHandle (PLW_CLASS_SIGCONTEXT  psigctx, 
                             INT                   iSigNo, 
                             struct siginfo       *psiginfo, 
                             PLW_CLASS_SIGCTLMSG   psigctlmsg)
{
    REGISTER struct sigaction     *psigaction;
    
    REGISTER VOIDFUNCPTR           pfuncHandle;
             PVOID                 pvCtx;
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    psigaction  = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
    pfuncHandle = (VOIDFUNCPTR)psigaction->sa_handler;                  /*  获得信号执行函数句柄        */
    
    if (psigaction->sa_flags & SA_ONESHOT) {                            /*  仅仅截获这一次信号          */
        psigaction->sa_handler = SIG_DFL;                               /*  进入默认处理                */
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if ((pfuncHandle != SIG_IGN)   && 
        (pfuncHandle != SIG_ERR)   &&
        (pfuncHandle != SIG_DFL)   &&
        (pfuncHandle != SIG_CATCH) &&
        (pfuncHandle != SIG_HOLD)) {
        pvCtx = (psigctlmsg) 
              ? psigctlmsg->SIGCTLMSG_pvStackRet
              : LW_NULL;
              
        if (psigaction->sa_flags & SA_SIGINFO) {                        /*  需要 siginfo_t 信息         */
            LW_SOFUNC_PREPARE(pfuncHandle);
            pfuncHandle(iSigNo, psiginfo, pvCtx);                       /*  执行信号句柄                */
        
        } else {
            LW_SOFUNC_PREPARE(pfuncHandle);
            pfuncHandle(iSigNo, pvCtx);                                 /*  XXX 是否传入 pvCtx 参数 ?   */
        }
    
        if (__SIGNO_MUST_EXIT & __sigmask(iSigNo)) {                    /*  必须退出                    */
            __signalExitHandle(iSigNo, psiginfo);
        
        } else if (iSigNo == SIGCNCL) {                                 /*  线程取消信号                */
            __signalCnclHandle(iSigNo, psiginfo);
        }
    
    } else {
        switch (iSigNo) {                                               /*  默认处理句柄                */
        
        case SIGINT:
        case SIGQUIT:
        case SIGFPE:
        case SIGKILL:
        case SIGBUS:
        case SIGTERM:
        case SIGABRT:
        case SIGILL:
        case SIGSEGV:
        case SIGSYS:
            __signalExitHandle(iSigNo, psiginfo);
            break;
            
        case SIGSTOP:
        case SIGTSTP:
            __signalStopHandle(iSigNo, psiginfo);
            break;
            
        case SIGCHLD:
            if (((psiginfo->si_code == CLD_EXITED) ||
                 (psiginfo->si_code == CLD_KILLED) ||
                 (psiginfo->si_code == CLD_DUMPED)) &&
                (psigaction->sa_flags & SA_NOCLDWAIT)) {                /*  回收子进程资源              */
                __signalWaitHandle(iSigNo, psiginfo);
            }
            break;
            
        case SIGCNCL:
            __signalCnclHandle(iSigNo, psiginfo);
            break;
            
        case SIGSTKSHOW:
            __signalStkShowHandle(psigctlmsg);
            break;
            
        default:
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __sigShell
** 功能描述: 信号运行的外壳函数
** 输　入  : psigctlmsg              信号控制信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __sigShell (PLW_CLASS_SIGCTLMSG  psigctlmsg)
{
             INTREG                iregInterLevel;
             PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_SIGCONTEXT  psigctx;
    REGISTER struct siginfo       *psiginfo = &psigctlmsg->SIGCTLMSG_siginfo;
    REGISTER INT                   iSigNo   = psiginfo->si_signo;
    
#if defined(LW_CFG_CPU_ARCH_C6X)
    archIntEnableForce();                                               /*  使能中断                    */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
#if LW_CFG_CPU_FPU_EN > 0
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭当前 CPU 中断           */
    if (psigctlmsg->SIGCTLMSG_pfpuctx &&
        (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP)) {           /*  当前线程使用 FPU            */
        *psigctlmsg->SIGCTLMSG_pfpuctx = ptcbCur->TCB_fpuctxContext;
    }
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开当前 CPU 中断           */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGRUN, 
                      ptcbCur->TCB_ulId, iSigNo, psiginfo->si_code, LW_NULL);
    
    __sigRunHandle(psigctx, iSigNo, psiginfo, psigctlmsg);              /*  运行信号句柄                */
    
    __sigReturn(psigctx, ptcbCur, psigctlmsg);                          /*  信号返回                    */
}
/*********************************************************************************************************
** 函数名称: _signalInit
** 功能描述: 全局初始化信号队列
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _signalInit (VOID)
{
    REGISTER INT    i;
    
    __sigEventArgInit();
    
    for (i = 0; i < LW_CFG_MAX_SIGQUEUE_NODES; i++) {
        _List_Ring_Add_Last(&_K_sigpendBuffer[i].SIGPEND_ringSigQ, 
                            &_K_pringSigPendFreeHeader);                /*  链入空闲链表                */
    }
    
    API_SystemHookAdd(__sigTaskCreateHook, 
                      LW_OPTION_THREAD_CREATE_HOOK);                    /*  添加创建回调函数            */

#if LW_CFG_THREAD_DEL_EN > 0
    API_SystemHookAdd(__sigTaskDeleteHook, 
                      LW_OPTION_THREAD_DELETE_HOOK);                    /*  添加删除回调函数            */
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
}
/*********************************************************************************************************
** 函数名称: _sigGetMsb
** 功能描述: 从一个信号集中获取信号数值. (优先递送信号值小的信号)
** 输　入  : psigset        信号集
** 输　出  : 信号数值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _sigGetLsb (sigset_t  *psigset)
{
    UINT32  uiHigh = (UINT32)((*psigset) >> 32);
    UINT32  uiLow  = (UINT32)((*psigset) & 0xffffffff);
    
    if (uiHigh) {
        return  (archFindLsb(uiHigh) + 32);
    
    } else {
        return  (archFindLsb(uiLow));
    }
}
/*********************************************************************************************************
** 函数名称: _signalGetCtx
** 功能描述: 获得指定线程的 sig context 结构
** 输　入  : ptcb                   目的线程控制块
** 输　出  : sig context 结构
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_SIGCONTEXT  _signalGetCtx (PLW_CLASS_TCB  ptcb)
{
    return  (&_K_sigctxTable[_ObjectGetIndex(ptcb->TCB_ulId)]);
}
/*********************************************************************************************************
** 函数名称: _sigPendAlloc
** 功能描述: 获得一个空闲的信号队列节点 (此函数在进入内核后调用)
** 输　入  : NONE
** 输　出  : 如果存在空闲节点, 则返回地址, 否则返回 LW_NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_CLASS_SIGPEND   _sigPendAlloc (VOID)
{
    PLW_CLASS_SIGPEND   psigpendNew = LW_NULL;
    
    if (_K_pringSigPendFreeHeader) {
        psigpendNew = _LIST_ENTRY(_K_pringSigPendFreeHeader, LW_CLASS_SIGPEND, 
                                  SIGPEND_ringSigQ);                    /*  获得空闲控制块的地址        */
        _List_Ring_Del(_K_pringSigPendFreeHeader, 
                       &_K_pringSigPendFreeHeader);                     /*  从空闲队列中删除            */
    }
    
    return  (psigpendNew);
}
/*********************************************************************************************************
** 函数名称: _sigPendFree
** 功能描述: 释放一个信号队列节点. (此函数在进入内核后调用)
** 输　入  : psigpendFree      需要释放的节点地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _sigPendFree (PLW_CLASS_SIGPEND  psigpendFree)
{
    _List_Ring_Add_Last(&psigpendFree->SIGPEND_ringSigQ, 
                        &_K_pringSigPendFreeHeader);                    /*  归还到空闲队列中            */
}
/*********************************************************************************************************
** 函数名称: _sigPendInit
** 功能描述: 初始化一个信号队列节点.
** 输　入  : psigpend               需要初始化的节点地址
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _sigPendInit (PLW_CLASS_SIGPEND  psigpend)
{
    _LIST_RING_INIT_IN_CODE(psigpend->SIGPEND_ringSigQ);
    
    psigpend->SIGPEND_uiTimes = 0;
    psigpend->SIGPEND_iNotify = SIGEV_SIGNAL;
    psigpend->SIGPEND_psigctx = LW_NULL;
}
/*********************************************************************************************************
** 函数名称: _sigPendGet
** 功能描述: 获取一个需要被运行的信号, (此函数在进入内核后调用)
** 输　入  : psigctx               任务控制块中信号上下文
**           psigset               需要检查的信号集
**           psiginfo              需要运行的信号信息
** 输　出  : 信号数值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _sigPendGet (PLW_CLASS_SIGCONTEXT  psigctx, const sigset_t  *psigset, struct siginfo *psiginfo)
{
    INT                 iSigNo;
    INT                 iSigIndex;
    sigset_t            sigsetNeedRun;
    PLW_CLASS_SIGPEND   psigpend;
    
    sigsetNeedRun = *psigset & psigctx->SIGCTX_sigsetPending;
    if (sigsetNeedRun == 0ull) {                                        /*  需要检查的信号集都不需要运行*/
        return  (0);
    }
    
    sigsetNeedRun &= (-sigsetNeedRun);                                  /*  取 sigsetNeedRun 的最低位   */
    iSigNo         = _sigGetLsb(&sigsetNeedRun);
    iSigIndex      = __sigindex(iSigNo);
    
    if (sigsetNeedRun & psigctx->SIGCTX_sigsetKill) {                   /*  有 kill 的信号需要被运行    */
        psigctx->SIGCTX_sigsetKill  &= ~sigsetNeedRun;
        
        psiginfo->si_signo           = iSigNo;
        psiginfo->si_errno           = ERROR_NONE;
        psiginfo->si_code            = SI_KILL;
        psiginfo->si_value.sival_ptr = LW_NULL;
    
    } else {                                                            /*  没有 kill, 则一定有排队信号 */
        psigpend = _LIST_ENTRY(psigctx->SIGCTX_pringSigQ[iSigIndex], 
                               LW_CLASS_SIGPEND, SIGPEND_ringSigQ);

        if (psigpend->SIGPEND_uiTimes == 0) {                           /*  从 pend 队列中删除          */
            _List_Ring_Del(&psigpend->SIGPEND_ringSigQ, 
                           &psigctx->SIGCTX_pringSigQ[iSigIndex]);
        } else {
            psigpend->SIGPEND_uiTimes--;
        }
        
        *psiginfo = psigpend->SIGPEND_siginfo;
        
        if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
            (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {      /*  使用 queue 发送信号         */
            _sigPendFree(psigpend);                                     /*  需要交换空闲队列            */
        }
    }
    
    if (psigctx->SIGCTX_pringSigQ[iSigIndex] == LW_NULL) {              /*  此信号队列中已无 pend       */
        psigctx->SIGCTX_sigsetPending &= ~sigsetNeedRun;
    }
    
    return  (iSigNo);
}
/*********************************************************************************************************
** 函数名称: _sigPendRunSelf
** 功能描述: 当前线程运行所有等待的信号. (此函数在进入内核后调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
static BOOL _sigPendRunSelf (VOID)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    sigset_t                sigset;                                     /*  获得需要检查的信号集        */
    INT                     iSigNo;
    struct siginfo          siginfo;
    
    sigset_t                sigsetOld;                                  /*  信号执行完毕需要回复的掩码  */
    BOOL                    bIsRun = LW_FALSE;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    psigctx = _signalGetCtx(ptcbCur);
    sigset  = ~psigctx->SIGCTX_sigsetMask;                              /*  没有被屏蔽的信号            */
    if (sigset == 0) {
        return  (LW_FALSE);                                             /*  没有需要被运行的信号        */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  记录先前的掩码              */
    
    do {
        iSigNo = _sigPendGet(psigctx, &sigset, &siginfo);               /*  获得需要运行的信号          */
        if (__issig(iSigNo)) {
            struct sigaction     *psigaction;
                
            psigaction = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
            
            psigctx->SIGCTX_sigsetMask |= psigaction->sa_mask;
            if ((psigaction->sa_flags & SA_NOMASK) == 0) {              /*  阻止相同信号嵌套            */
                psigctx->SIGCTX_sigsetMask |= __sigmask(iSigNo);
            }
            
            __KERNEL_EXIT();                                            /*  退出内核                    */
            __sigRunHandle(psigctx, iSigNo, &siginfo, LW_NULL);         /*  直接运行信号句柄            */
            __KERNEL_ENTER();                                           /*  重新进入内核                */
            
            psigctx->SIGCTX_sigsetMask = sigsetOld;
            bIsRun = LW_TRUE;
        
        } else {
            break;
        }
    } while (1);
    
    return  (bIsRun);
}
/*********************************************************************************************************
** 函数名称: _sigPendRun
** 功能描述: 运行等待的信号. (此函数在进入内核后调用)
** 输　入  : ptcb                  任务控制块
** 输　出  : 是否运行了信号句柄
** 全局变量: 
** 调用模块: 
** 注  意  : 当 ptcb == CPU_ptcbTCBCur 时, 系统将直接运行信号句柄, 而且将所有可以运行的 pend 信号, 全部运行
                ptcb != CPU_ptcbTCBCur 时, 系统将在指定的线程内构造信号环境, 当该信号执行时, 尾部会自动执行
                                           所有可以运行的 pend 信号.
*********************************************************************************************************/
BOOL  _sigPendRun (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    sigset_t                sigset;                                     /*  获得需要检查的信号集        */
    INT                     iSigNo;
    struct siginfo          siginfo;
    
    sigset_t                sigsetOld;                                  /*  信号执行完毕需要回复的掩码  */
    INT                     iSchedRet;                                  /*  需要重新等待事件            */

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    if (ptcb == ptcbCur) {
        return  (_sigPendRunSelf());
    }

    psigctx = _signalGetCtx(ptcb);
    sigset  = ~psigctx->SIGCTX_sigsetMask;                              /*  没有被屏蔽的信号            */
    if (sigset == 0) {
        return  (LW_FALSE);                                             /*  没有需要被运行的信号        */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  记录先前的掩码              */
    
    iSigNo = _sigPendGet(psigctx, &sigset, &siginfo);                   /*  获得需要运行的信号          */
    if (__issig(iSigNo)) {
        struct sigaction *psigaction = &psigctx->SIGCTX_sigaction[__sigindex(iSigNo)];
        INT iSaType;
        
        if (psigaction->sa_flags & SA_RESTART) {
            iSaType = LW_SIGNAL_RESTART;                                /*  重启调用                    */
        } else {
            iSaType = LW_SIGNAL_EINTR;                                  /*  正常 EINTR                  */
        }
        
        __sigMakeReady(ptcb, iSigNo, &iSchedRet, iSaType);              /*  强制进入就绪状态            */
        __sigCtlCreate(ptcb, psigctx, &siginfo, iSchedRet, &sigsetOld); /*  创建信号上下文环境          */
        
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: _sigTimeoutRecalc
** 功能描述: 需要重新等待事件时, 重新计算等待时间.
** 输　入  : ulOrgKernelTime        开始等待时的系统时间
**           ulOrgTimeout           开始等待时的超时选项
** 输　出  : 新的等待时间
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _sigTimeoutRecalc (ULONG  ulOrgKernelTime, ULONG  ulOrgTimeout)
{
             INTREG     iregInterLevel;
    REGISTER ULONG      ulTimeRun;
             ULONG      ulKernelTime;
    
    if (ulOrgTimeout == LW_OPTION_WAIT_INFINITE) {                      /*  无限等待                    */
        return  (ulOrgTimeout);
    }
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);
    ulTimeRun = (ulKernelTime >= ulOrgKernelTime) ?
                (ulKernelTime -  ulOrgKernelTime) :
                (ulKernelTime + (__ARCH_ULONG_MAX - ulOrgKernelTime) + 1);
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
    if (ulTimeRun >= ulOrgTimeout) {                                    /*  已经产生了超时              */
        return  (0);
    }
    
    return  (ulOrgTimeout - ulTimeRun);
}
/*********************************************************************************************************
** 函数名称: _doSignal
** 功能描述: 将指定信号句柄内嵌入指定的线程，不能向自己发送信号. (此函数在内核状态被调用)
** 输　入  : ptcb                   目标线程控制块
**           psigpend               信号等待处理信息
** 输　出  : 发送结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_SEND_VAL  _doSignal (PLW_CLASS_TCB  ptcb, PLW_CLASS_SIGPEND   psigpend)
{
    REGISTER struct siginfo      *psiginfo = &psigpend->SIGPEND_siginfo;
    REGISTER INT                  iSigNo   = psiginfo->si_signo;

    REGISTER struct sigaction    *psigaction;
    REGISTER sigset_t             sigsetSigMaskBit  = __sigmask(iSigNo);/*  信号掩码                    */
    REGISTER INT                  iSigIndex = __sigindex(iSigNo);       /*  TCB_sigaction 下标          */
    
             PLW_CLASS_SIGCONTEXT psigctx;
             sigset_t             sigsetOld;                            /*  信号执行完毕需要回复的掩码  */
             INT                  iSchedRet;                            /*  需要重新等待事件            */
             INT                  iSaType;
             
    if (psigpend->SIGPEND_iNotify == SIGEV_NONE) {                      /*  不发送信号                  */
        return  (SEND_IGN);
    }
    
    psigctx = _signalGetCtx(ptcb);
    if (psigctx->SIGCTX_sigwait) {                                      /*  目标线程在等待信号          */
        if (psigctx->SIGCTX_sigwait->SIGWT_sigset & __sigmask(iSigNo)) {/*  属于关心的信号              */
            psigctx->SIGCTX_sigwait->SIGWT_siginfo = psigpend->SIGPEND_siginfo;
            __sigMakeReady(ptcb, iSigNo, &iSchedRet, LW_SIGNAL_EINTR);  /*  就绪任务                    */
            psigctx->SIGCTX_sigwait = LW_NULL;                          /*  删除等待信息                */
            return  (SEND_INFO);
        }
    }
    
    psigaction = &psigctx->SIGCTX_sigaction[iSigIndex];                 /*  获得目标线程的相关信号控制块*/
    if ((psigaction->sa_handler == SIG_ERR) ||
        (psigaction->sa_handler == SIG_IGN)) {                          /*  检查句柄有效性              */
        return  (SEND_IGN);
    }
    
    if ((psigaction->sa_flags & SA_NOCLDSTOP) &&
        (psigpend->SIGPEND_siginfo.si_signo == SIGCHLD) &&
        (__SI_CODE_STOP(psigpend->SIGPEND_siginfo.si_code))) {          /*  父进程不接收子进程暂停信号  */
        return  (SEND_IGN);
    }
    
    if (sigsetSigMaskBit & psigctx->SIGCTX_sigsetMask) {                /*  被屏蔽了                    */
        if (psiginfo->si_code == SI_KILL) {                             /*  kill 产生了信号, 不能排队   */
            psigctx->SIGCTX_sigsetKill    |= sigsetSigMaskBit;          /*  有 kill 的信号被屏蔽了      */
            psigctx->SIGCTX_sigsetPending |= sigsetSigMaskBit;          /*  iSigNo 由于屏蔽等待运行     */
        
        } else if (psigpend->SIGPEND_ringSigQ.RING_plistNext) {         /*  除了 kill 产生的信号, 如果  */
            psigpend->SIGPEND_uiTimes++;                                /*  在队列中, 就需要队列缓冲信号*/
                                                                        /*  已经存在在队列中            */
        } else {
            if (psigpend->SIGPEND_iNotify == SIGEV_SIGNAL) {            /*  需要排队信号                */
                PLW_CLASS_SIGPEND   psigpendNew = _sigPendAlloc();      /*  从缓冲区中获取一个空闲的    */
                LW_LIST_RING_HEADER *ppringHeader = 
                                    &psigctx->SIGCTX_pringSigQ[iSigIndex];
                
                if (psigpendNew == LW_NULL) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, 
                    "no node can allocate from free sigqueue.\r\n");
                    _ErrorHandle(ERROR_SIGNAL_SIGQUEUE_NODES_NULL);
                    return  (SEND_ERROR);
                }
                
                *psigpendNew = *psigpend;                               /*  拷贝信息                    */
                _List_Ring_Add_Last(&psigpendNew->SIGPEND_ringSigQ, 
                                    ppringHeader);                      /*  加入队列链表                */
                psigpendNew->SIGPEND_psigctx   = psigctx;
                psigctx->SIGCTX_sigsetPending |= sigsetSigMaskBit;      /*  iSigNo 由于屏蔽等待运行     */
            }
        }
        return  (SEND_BLOCK);                                           /*  被 mask 的都不执行          */
    }
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  信号执行完后需要重新设置为  */
                                                                        /*  这个掩码                    */
    psigctx->SIGCTX_sigsetMask |= psigaction->sa_mask;
    if ((psigaction->sa_flags & SA_NOMASK) == 0) {                      /*  阻止相同信号嵌套            */
        psigctx->SIGCTX_sigsetMask |= sigsetSigMaskBit;
    }
    if (psigaction->sa_flags & SA_RESTART) {
        iSaType = LW_SIGNAL_RESTART;                                    /*  需要重启调用                */
    } else {
        iSaType = LW_SIGNAL_EINTR;                                      /*  正常 EINTR                  */
    }
    
    __sigMakeReady(ptcb, iSigNo, &iSchedRet, iSaType);                  /*  强制进入就绪状态            */
    __sigCtlCreate(ptcb, psigctx, psiginfo, iSchedRet, &sigsetOld);     /*  创建信号上下文环境          */
    
    return  (SEND_OK);
}
/*********************************************************************************************************
** 函数名称: _doKill
** 功能描述: kill 函数将会调用这个函数产生一个 kill 的信号, 然后将会调用 doSignal 产生这个信号.
             (此函数在内核状态被调用)
** 输　入  : ptcb                   目标线程控制块
**           iSigNo                 信号
** 输　出  : 发送信号结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_SEND_VAL  _doKill (PLW_CLASS_TCB  ptcb, INT  iSigNo)
{
    struct siginfo    *psiginfo;
    PLW_CLASS_TCB      ptcbCur;
    LW_CLASS_SIGPEND   sigpend;                                         /*  由于是 kill 调用, 所以绝对  */
                                                                        /*  不会产生队列, 这里不用初始化*/
                                                                        /*  相关的链表                  */
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psiginfo = &sigpend.SIGPEND_siginfo;
    psiginfo->si_signo = iSigNo;
    psiginfo->si_errno = errno;
    psiginfo->si_code  = SI_KILL;                                       /*  不可排队                    */
    psiginfo->si_pid   = __tcb_pid(ptcbCur);
    psiginfo->si_uid   = ptcbCur->TCB_uid;
    psiginfo->si_int   = EXIT_FAILURE;                                  /*  默认信号参数                */

    sigpend.SIGPEND_iNotify = SIGEV_SIGNAL;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_KILL, 
                      ptcb->TCB_ulId, iSigNo, LW_NULL);
    
    return  (_doSignal(ptcb, &sigpend));                                /*  产生信号                    */
}
/*********************************************************************************************************
** 函数名称: _doSigQueue
** 功能描述: sigqueue 函数将会调用这个函数产生一个 queue 的信号, 然后将会调用 doSignal 产生这个信号.
             (此函数在内核状态被调用)
** 输　入  : ptcb                   目标线程控制块
**           iSigNo                 信号
**           sigvalue               信号 value
** 输　出  : 发送信号结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_SEND_VAL  _doSigQueue (PLW_CLASS_TCB  ptcb, INT  iSigNo, const union sigval sigvalue)
{
    struct siginfo    *psiginfo;
    PLW_CLASS_TCB      ptcbCur;
    LW_CLASS_SIGPEND   sigpend;

    _sigPendInit(&sigpend);                                             /*  初始化可产生排队信号        */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psiginfo = &sigpend.SIGPEND_siginfo;
    psiginfo->si_signo = iSigNo;
    psiginfo->si_errno = errno;
    psiginfo->si_code  = SI_QUEUE;                                       /*  排队信号                   */
    psiginfo->si_pid   = __tcb_pid(ptcbCur);
    psiginfo->si_uid   = ptcbCur->TCB_uid;
    psiginfo->si_value = sigvalue;
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGQUEUE, 
                      ptcb->TCB_ulId, iSigNo, sigvalue.sival_ptr, LW_NULL);
    
    return  (_doSignal(ptcb, &sigpend));                                /*  产生信号                    */
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
