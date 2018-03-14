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
** 文   件   名: signalPrivate.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 06 月 03 日
**
** 描        述: 这是系统信号内部私有信息定义
*********************************************************************************************************/

#ifndef  __SIGNALPRIVATE_H
#define  __SIGNALPRIVATE_H

#if LW_CFG_SIGNAL_EN > 0

#undef sigaction                                                        /*  宏与结构体产生冲突          */
#undef sigvec                                                           /*  宏与结构体产生冲突          */

/*********************************************************************************************************
  注意:
  信号屏蔽与忽略:
  
  信号屏蔽: 当前如果是 kill 信号到来, 且被屏蔽, 那么信号将得不到运行. 但是, 一旦被解除屏蔽, 不过接收到多少
            次 kill 的信号, 将会被执行一次. 
            当前如果是 queue 信号到来, 且被屏蔽, 那么信号将得不到运行. 但是, 一旦被解除屏蔽, 在屏蔽的过程中
            接收到多少信号, 就运行多少次.
            当前如果是其他信号到来, 如 timer, aio, msgq 等等, 则直接忽略.
            
  信号忽略: 当信号的处理句柄为忽略时, 接收到的信号会直接抛弃, 完全不管屏蔽状态.
*********************************************************************************************************/

/*********************************************************************************************************
  实际信号发送函数返回状态
*********************************************************************************************************/

typedef enum {
    SEND_OK,                                                            /*  发送成功                    */
    SEND_ERROR,                                                         /*  发送失败                    */
    SEND_INFO,                                                          /*  以 wait info 激活等待任务   */
    SEND_IGN,                                                           /*  信号被忽略掉                */
    SEND_BLOCK                                                          /*  信号被阻塞                  */
} LW_SEND_VAL;

/*********************************************************************************************************
  信号等待信息
*********************************************************************************************************/

typedef struct __sig_wait {
    sigset_t              SIGWT_sigset;
    struct siginfo        SIGWT_siginfo;
} LW_CLASS_SIGWAIT;

/*********************************************************************************************************
  信号线程上下文
*********************************************************************************************************/

typedef struct __sig_context {
    sigset_t              SIGCTX_sigsetMask;                            /*  当前信号屏蔽位              */
    sigset_t              SIGCTX_sigsetPending;                         /*  当前由于被屏蔽无法运行的信号*/
    sigset_t              SIGCTX_sigsetKill;                            /*  由 kill 发送但被屏蔽的信号  */
    
    struct sigaction      SIGCTX_sigaction[NSIG];                       /*  所有的信号控制块            */
    LW_LIST_RING_HEADER   SIGCTX_pringSigQ[NSIG];                       /*  由于被屏蔽无法运行的信号排队*/
    stack_t               SIGCTX_stack;                                 /*  用户指定的信号堆栈情况      */

    LW_CLASS_SIGWAIT     *SIGCTX_sigwait;                               /*  等待信息                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    BOOL                  SIGCTX_bRead;                                 /*  是否在读 signalfd           */
    sigset_t              SIGCTX_sigsetWait;                            /*  正在等待的 sigset           */
    LW_SEL_WAKEUPLIST     SIGCTX_selwulist;                             /*  signalfd select list        */
#endif
} LW_CLASS_SIGCONTEXT;
typedef LW_CLASS_SIGCONTEXT *PLW_CLASS_SIGCONTEXT;

/*********************************************************************************************************
  信号阻塞等待处理的信息
*********************************************************************************************************/

typedef struct {
    LW_LIST_RING          SIGPEND_ringSigQ;                             /*  环形链表                    */
    
    struct siginfo        SIGPEND_siginfo;                              /*  信号相关信息                */
    PLW_CLASS_SIGCONTEXT  SIGPEND_psigctx;                              /*  信号上下文                  */
    UINT                  SIGPEND_uiTimes;                              /*  被阻塞的次数                */
    
    INT                   SIGPEND_iNotify;                              /*  sigevent.sigev_notify       */
} LW_CLASS_SIGPEND;
typedef LW_CLASS_SIGPEND    *PLW_CLASS_SIGPEND;

/*********************************************************************************************************
  SIGNAL CONTRL MESSAGE (每产生一个信号 _doSignal() 都会自动生成一个以下结构数据, 存放在堆栈间隙)
*********************************************************************************************************/

typedef struct {
    PVOID                 SIGCTLMSG_pvStackRet;                         /*  跳跃返回堆栈的地址          */
#if defined(LW_CFG_CPU_ARCH_C6X)
    ULONG                 SIGCTLMSG_ulContextType;                      /*  上下文类型                  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */
    INT                   SIGCTLMSG_iSchedRet;                          /*  信号调度器返回值            */
    INT                   SIGCTLMSG_iKernelSpace;                       /*  产生信号是的内核空间情况    */
                                                                        /*  信号退出时需要返回之前的状态*/
    sigset_t              SIGCTLMSG_sigsetMask;                         /*  信号句柄退出需要恢复的掩码  */
    struct siginfo        SIGCTLMSG_siginfo;                            /*  信号相关信息                */
    ULONG                 SIGCTLMSG_ulLastError;                        /*  保存的原始错误号            */
    UINT8                 SIGCTLMSG_ucWaitTimeout;                      /*  保存的原始 timeout 标记     */
    UINT8                 SIGCTLMSG_ucIsEventDelete;                    /*  事件是否被删除              */
    
#if LW_CFG_CPU_FPU_EN > 0
    LW_FPU_CONTEXT       *SIGCTLMSG_pfpuctx;                            /*  FPU 上下文                  */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
} LW_CLASS_SIGCTLMSG;
typedef LW_CLASS_SIGCTLMSG  *PLW_CLASS_SIGCTLMSG;

/*********************************************************************************************************
  SIGNAL CONTRL MESSAGE 结构大小定义
*********************************************************************************************************/

#define __SIGCTLMSG_SIZE_ALIGN      ROUND_UP(sizeof(LW_CLASS_SIGCTLMSG), sizeof(LW_STACK))
#define __SIGFPUCTX_SIZE_ALIGN      ROUND_UP(sizeof(LW_FPU_CONTEXT),     sizeof(LW_STACK))
        
/*********************************************************************************************************
  UNMASK SIG
*********************************************************************************************************/

#define __SIGNO_UNMASK              (__sigmask(SIGKILL) |       \
                                     __sigmask(SIGABRT) |       \
                                     __sigmask(SIGSTOP) |       \
                                     __sigmask(SIGFPE)  |       \
                                     __sigmask(SIGILL)  |       \
                                     __sigmask(SIGBUS)  |       \
                                     __sigmask(SIGSEGV))
                                     
/*********************************************************************************************************
  EXIT SIG
*********************************************************************************************************/

#define __SIGNO_MUST_EXIT           (__sigmask(SIGKILL) |       \
                                     __sigmask(SIGTERM) |       \
                                     __sigmask(SIGABRT) |       \
                                     __sigmask(SIGFPE)  |       \
                                     __sigmask(SIGILL)  |       \
                                     __sigmask(SIGBUS)  |       \
                                     __sigmask(SIGSEGV))
                                     
/*********************************************************************************************************
  Is si_code is stop
*********************************************************************************************************/

#define __SI_CODE_STOP(code)        (((code) == CLD_STOPPED) || \
                                     ((code) == CLD_CONTINUED))

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
#endif                                                                  /*  __SIGNALPRIVATE_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
