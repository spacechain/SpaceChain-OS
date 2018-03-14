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
** 文   件   名: signal.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 06 月 03 日
**
** 描        述: 系统信号处理函数库。 

** BUG
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.08.11  API_kill 加入对句柄类型的判断, 由于 shell 的一次误操作导致发送信号将 idle 删除了, 就是因为
            没有检查句柄类型.
2009.01.13  修改相关注释.
2009.05.24  不能向 TCB_bIsInDeleteProc 为 TRUE 的线程发送信号.
2011.08.15  重大决定: 将所有 posix 定义的函数以函数方式(非宏)引出.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2012.08.24  发送信号函数支持进程号, 相应信号将发送到对方主线程.
2012.12.12  sigprocmask 设置信号屏蔽时, 有些信号是不可屏蔽的.
2013.01.15  sigaction 安装的信号屏蔽字, 要允许不可屏蔽的信号.
2014.05.21  将 killTrap 改为 sigTrap 可以发送参数.
2014.10.31  加入诸多 POSIX 规定的信号函数.
2015.11.16  trap 信号使用 kill 发送.
2016.08.11  进程内线程设置信号处理句柄, 需要同步到所有进程内的线程.
2017.06.06  修正 sigaltstack() 错误.
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
  内部函数声明
*********************************************************************************************************/
extern PLW_CLASS_SIGCONTEXT  _signalGetCtx(PLW_CLASS_TCB  ptcb);
extern VOID                  _sigPendFree(PLW_CLASS_SIGPEND  psigpendFree);
extern BOOL                  _sigPendRun(PLW_CLASS_TCB  ptcb);
extern INT                   _sigPendGet(PLW_CLASS_SIGCONTEXT  psigctx, 
                                         const sigset_t  *psigset, struct siginfo *psiginfo);
/*********************************************************************************************************
  内部发送函数声明
*********************************************************************************************************/
extern LW_SEND_VAL           _doKill(PLW_CLASS_TCB  ptcb, INT  iSigNo);
extern LW_SEND_VAL           _doSigQueue(PLW_CLASS_TCB  ptcb, INT  iSigNo, const union sigval  sigvalue);
/*********************************************************************************************************
  内部进程相关函数声明
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: sigemptyset
** 功能描述: 初始化一个空的信号集
** 输　入  : 
**           psigset                 信号集
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigemptyset (sigset_t    *psigset)
{
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset = 0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigfillset
** 功能描述: 初始化一个满的信号集
** 输　入  : psigset                 信号集
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigfillset (sigset_t	*psigset)
{
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset = ~0;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigaddset
** 功能描述: 向一个信号集添加一个信号
** 输　入  : psigset                 信号集
**           iSigNo                  信号
** 输　出  : ERROR_NONE , EINVAL
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigaddset (sigset_t  *psigset, INT  iSigNo)
{
    if (!psigset || !__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset |= __sigmask(iSigNo);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigdelset
** 功能描述: 从一个信号集中删除一个信号
** 输　入  : psigset                 信号集
**           iSigNo                  信号
** 输　出  : ERROR_NONE , EINVAL
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigdelset (sigset_t  *psigset, INT  iSigNo)
{
    if (!psigset || !__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *psigset &= ~__sigmask(iSigNo);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigismember
** 功能描述: 检查一个信号是否属于一个信号集
** 输　入  : 
**           psigset                 信号集
**           iSigNo                  信号
** 输　出  : 0 or 1 or -1
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigismember (const sigset_t  *psigset, INT  iSigNo)
{
    if (!psigset || !__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (*psigset & __sigmask(iSigNo)) {
        return  (1);
    
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** 函数名称: __sigaction
** 功能描述: 进程所有线程设置一个指定信号的服务向量
** 输　入  : ptcb          线程控制块
**           iSigIndex     TCB_sigaction 下标
**           psigactionNew 新的处理结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  __sigaction (PLW_CLASS_TCB  ptcb, INT  iSigIndex, const struct sigaction  *psigactionNew)
{
    struct sigaction       *psigaction;
    PLW_CLASS_SIGCONTEXT    psigctx;
    
    psigctx    = _signalGetCtx(ptcb);
    psigaction = &psigctx->SIGCTX_sigaction[iSigIndex];
    
    *psigaction = *psigactionNew;
    psigaction->sa_mask &= ~__SIGNO_UNMASK;                             /*  有些信号不可屏蔽            */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: sigaction
** 功能描述: 设置一个指定信号的服务向量, 同时可获取原始服务向量. 
**           (由于与 struct sigaction 重名, 所以这里直接使用 sigaction 函数名)
** 输　入  : iSigNo        信号
**           psigactionNew 新的处理结构
**           psigactionOld 先早的处理结构
** 输　出  : ERROR_NONE , EINVAL
** 全局变量: 
** 调用模块: 
** 注  意  : SIGKILL 允许 gdbserver 进行捕获操作.

                                           API 函数
*********************************************************************************************************/
LW_API  
INT   sigaction (INT                      iSigNo, 
                 const struct sigaction  *psigactionNew,
                 struct sigaction        *psigactionOld)
{
    struct sigaction               *psigaction;
    PLW_CLASS_SIGCONTEXT            psigctx;
    PLW_CLASS_TCB                   ptcbCur;
    REGISTER PLW_CLASS_SIGPEND      psigpend;
    REGISTER INT                    iSigIndex = __sigindex(iSigNo);     /*  TCB_sigaction 下标          */
    
    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (iSigNo == SIGSTOP) {                                            /*  不能捕获和忽略              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx    = _signalGetCtx(ptcbCur);
    psigaction = &psigctx->SIGCTX_sigaction[iSigIndex];
    
    if (psigactionOld) {
        *psigactionOld = *psigaction;                                   /*  保存先早信息                */
    }
    
    if (psigactionNew == LW_NULL) {
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();
    *psigaction = *psigactionNew;                                       /*  拷贝新的处理控制块          */
    psigaction->sa_mask &= ~__SIGNO_UNMASK;                             /*  有些信号不可屏蔽            */
    __KERNEL_EXIT();

#if LW_CFG_MODULELOADER_EN > 0                                          /*  进程所有线程设置新的处理句柄*/
    vprocThreadSigaction(vprocGetCur(), __sigaction, iSigIndex, psigactionNew);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (psigaction->sa_handler == SIG_IGN) {                            /*  设置为忽略该信号            */
        __KERNEL_ENTER();                                               /*  进入内核                    */
        psigctx->SIGCTX_sigsetPending &= ~__sigmask(iSigNo);            /*  没有等待 unmask 后执行的信号*/
        psigctx->SIGCTX_sigsetKill    &= ~__sigmask(iSigNo);            /*  没有在屏蔽状态 kill 这个信号*/
        
        {                                                               /*  删除队列中的相关信号节点    */
                     PLW_LIST_RING  pringHead = psigctx->SIGCTX_pringSigQ[iSigIndex];
            REGISTER PLW_LIST_RING  pringSigP = pringHead;
            
            if (pringHead) {                                            /*  唤醒队列中存在节点          */
                do {
                    psigpend  = _LIST_ENTRY(pringSigP, 
                                            LW_CLASS_SIGPEND, 
                                            SIGPEND_ringSigQ);          /*  获得 sigpend 控制块地址     */
                    
                    pringSigP = _list_ring_get_next(pringSigP);         /*  下一个节点                  */
                    
                    if ((psigpend->SIGPEND_siginfo.si_code != SI_KILL) &&
                        (psigpend->SIGPEND_iNotify         == SIGEV_SIGNAL)) {
                        _sigPendFree(psigpend);                         /*  需要交换空闲队列            */
                    }
                } while (pringSigP != pringHead);
            }
        }
        __KERNEL_EXIT();                                                /*  退出内核                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: bsd_signal
** 功能描述: 设置一个信号的处理句柄
** 输　入  : iSigNo        信号
**           pfuncHandler  处理句柄
** 输　出  : 先早的处理句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
sighandler_t bsd_signal (INT   iSigNo, PSIGNAL_HANDLE  pfuncHandler)
{
    return  (signal(iSigNo, pfuncHandler));
}
/*********************************************************************************************************
** 函数名称: signal
** 功能描述: 设置一个信号的处理句柄
** 输　入  : iSigNo        信号
**           pfuncHandler  处理句柄
** 输　出  : 先早的处理句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PSIGNAL_HANDLE signal (INT  iSigNo, PSIGNAL_HANDLE  pfuncHandler)
{
    INT               iError;
    struct sigaction  sigactionNew;
    struct sigaction  sigactionOld;
    
    sigactionNew.sa_handler = pfuncHandler;
    sigactionNew.sa_flags   = 0;                                        /*  不允许同类型信号嵌套        */
    
    sigemptyset(&sigactionNew.sa_mask);
    
    iError = sigaction(iSigNo, &sigactionNew, &sigactionOld);
    if (iError) {
        return  (SIG_ERR);
    
    } else {
        return  (sigactionOld.sa_handler);
    }
}
/*********************************************************************************************************
** 函数名称: sigvec
** 功能描述: 安装信号向量句柄 (BSD 兼容)
** 输　入  : iBlock                   新的阻塞信号掩码
** 输　出  : 先早的掩码
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigvec (INT  iSigNo, const struct sigvec *pvec, struct sigvec *pvecOld)
{
    struct sigaction sigactionNew;
    struct sigaction sigactionOld;
    
    INT              iSave  = 0;
    INT              iError = PX_ERROR;
    
    if (pvec) {                                                         /*  设置新的信号向量            */
        sigactionNew.sa_handler = pvec->sv_handler;
        sigactionNew.sa_mask    = pvec->sv_mask;
        sigactionNew.sa_flags   = pvec->sv_flags;
        if (pvecOld) {
            iError = sigaction(iSigNo, &sigactionNew, &sigactionOld);
            iSave  = 1;
        
        } else {
            iError = sigaction(iSigNo, &sigactionNew, LW_NULL);
        }
    
    } else if (pvecOld) {
        iError = sigaction(iSigNo, LW_NULL, &sigactionOld);             /*  获取                        */
        iSave  = 1;
    }
    
    if (iSave) {
        pvecOld->sv_handler = sigactionOld.sa_handler;
        pvecOld->sv_mask    = sigactionOld.sa_mask;
        pvecOld->sv_flags   = sigactionOld.sa_flags;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: sigpending
** 功能描述: 获得当前被暂时搁置未处理的信号 (有处理函数. 但是被屏蔽了)
** 输　入  : psigset                 信号集
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigpending (sigset_t  *psigset)
{
    PLW_CLASS_SIGCONTEXT  psigctx;
    PLW_CLASS_TCB         ptcbCur;
    
    if (psigset) {
        LW_TCB_GET_CUR_SAFE(ptcbCur);
        psigctx  = _signalGetCtx(ptcbCur);
        *psigset = psigctx->SIGCTX_sigsetPending;
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: sigprocmask
** 功能描述: 测试或改变当前线程的信号掩码
** 输　入  : iCmd                    命令
**           psigset                 新信号集
**           psigsetOld              先早信号集
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigprocmask (INT              iCmd, 
                  const sigset_t  *psigset, 
                        sigset_t  *psigsetOld)
{
    PLW_CLASS_TCB         ptcbCur;
    PLW_CLASS_SIGCONTEXT  psigctx;
    sigset_t              sigsetBlock;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    if (psigsetOld) {                                                   /*  保存古老的                  */
        *psigsetOld = psigctx->SIGCTX_sigsetMask;
    }
    
    if (!psigset) {                                                     /*  新的是否有效                */
        return  (ERROR_NONE);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    
    switch (iCmd) {
    
    case SIG_BLOCK:                                                     /*  添加阻塞                    */
        sigsetBlock  = *psigset;
        sigsetBlock &= ~__SIGNO_UNMASK;                                 /*  有些信号是不可屏蔽的        */
        psigctx->SIGCTX_sigsetMask |= sigsetBlock;
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_NONE);
        
    case SIG_UNBLOCK:                                                   /*  删除阻塞                    */
        psigctx->SIGCTX_sigsetMask &= ~(*psigset);
        break;
        
    case SIG_SETMASK:                                                   /*  设置阻塞                    */
        sigsetBlock  = *psigset;
        sigsetBlock &= ~__SIGNO_UNMASK;                                 /*  有些信号是不可屏蔽的        */
        psigctx->SIGCTX_sigsetMask  = sigsetBlock;
        break;
    
    default:                                                            /*  错误                        */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "command invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _sigPendRun(ptcbCur);                                               /*  可能有先前被阻塞信号需要运行*/
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigmask
** 功能描述: 通过信号的值获取一个信号掩码
** 输　入  : iSigNo                  信号
** 输　出  : 信号掩码
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigmask (INT  iSigNo)
{
    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    return  ((INT)__sigmask(iSigNo));
}
/*********************************************************************************************************
** 函数名称: siggetmask
** 功能描述: 获得当前线程信号掩码
** 输　入  : NONE
** 输　出  : 信号掩码
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  siggetmask (VOID)
{
    sigset_t    sigsetOld;
    INT         iMaskOld = 0;
    
    sigprocmask(SIG_BLOCK, LW_NULL, &sigsetOld);
    iMaskOld  = (INT)sigsetOld;
    
    return  (iMaskOld);
}
/*********************************************************************************************************
** 函数名称: sigsetmask
** 功能描述: 设置当前线程新的掩码 (BSD 兼容)
** 输　入  : iMask                   新的掩码
** 输　出  : 先早的掩码
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigsetmask (INT  iMask)
{
    sigset_t    sigsetNew;
    sigset_t    sigsetOld;
    INT         iMaskOld = 0;
    
    sigsetNew = (sigset_t)iMask;                                        /*  早期 BSD 接口仅 32 个信号   */
    sigprocmask(SIG_SETMASK, &sigsetNew, &sigsetOld);
    iMaskOld  = (INT)sigsetOld;
    
    return  (iMaskOld);
}
/*********************************************************************************************************
** 函数名称: sigsetblock
** 功能描述: 将新的需要阻塞的信号添加到当前线程 (BSD 兼容)
** 输　入  : iBlock                   新的阻塞信号掩码
** 输　出  : 先早的掩码
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigblock (INT  iMask)
{
    sigset_t    sigsetNew;
    sigset_t    sigsetOld;
    INT         iMaskOld = 0;
    
    sigsetNew = (sigset_t)iMask;                                        /*  早期 BSD 接口仅 32 个信号   */
    sigprocmask(SIG_BLOCK, &sigsetNew, &sigsetOld);
    iMaskOld  = (INT)sigsetOld;
    
    return  (iMaskOld);
}
/*********************************************************************************************************
** 函数名称: sighold
** 功能描述: 将新的需要阻塞的信号添加到当前线程
** 输　入  : iSigNo                   需要阻塞的信号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sighold (INT  iSigNo)
{
    sigset_t    sigset = 0;

    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    sigset = __sigmask(iSigNo);

    return  (sigprocmask(SIG_BLOCK, &sigset, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: sigignore
** 功能描述: 将指定的信号设置为 SIG_IGN
** 输　入  : iSigNo                   需要 SIG_IGN 的信号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  sigignore (INT  iSigNo)
{
    if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    signal(iSigNo, SIG_IGN);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigrelse
** 功能描述: 将指定的信号从掩码中删除
** 输　入  : iSigNo                   需要删除掩码的信号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  sigrelse (INT  iSigNo)
{
    sigset_t    sigset;
    
    if (sigprocmask(SIG_SETMASK, LW_NULL, &sigset) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (sigdelset(&sigset, iSigNo) < ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (sigprocmask(SIG_SETMASK, &sigset, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: sigpause
** 功能描述: 在当前掩码中清除指定的信号等待信号的到来, 然后返回先前的信号掩码. 
**           此 API 以被 sigsuspend 替代.
** 输　入  : iSigMask                 掩码
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigpause (INT  iSigMask)
{
    sigset_t    sigset;
    
    sigset = (sigset_t)iSigMask;
    
    return  (sigsuspend(&sigset));
}
/*********************************************************************************************************
** 函数名称: sigset
** 功能描述: 此函数暂不支持
** 输　入  : iSigNo                   需要删除掩码的信号
**           disp
** 输　出  : disp
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
sighandler_t  sigset (INT  iSigNo, sighandler_t  disp)
{
    _ErrorHandle(ENOSYS);
    return  ((sighandler_t)PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: siginterrupt
** 功能描述: 改变信号 SA_RESTART 选项
** 输　入  : iSigNo                   信号
**           iFlag                    是否使能 SA_RESTART 选项
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  siginterrupt (INT  iSigNo, INT  iFlag)
{
    struct sigaction    sigact;
    
    if (sigaction(iSigNo, LW_NULL, &sigact)) {
        return  (PX_ERROR);
    }
    
    if (iFlag) {
        sigact.sa_flags &= ~SA_RESTART;
    } else {
        sigact.sa_flags |= SA_RESTART;
    }
    
    return  (sigaction(iSigNo, &sigact, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: sigstack
** 功能描述: 设置信号上下文的堆栈
** 输　入  : ss                       新的堆栈信息
**           oss                      老的堆栈信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  sigstack (struct sigstack *ss, struct sigstack *oss)
{
    stack_t     stackNew, stackOld;
    
    if (ss) {
        stackNew.ss_sp = ss->ss_sp;
        if (ss->ss_sp) {
            stackNew.ss_flags = 0;
            stackNew.ss_size  = SIGSTKSZ;                               /*  默认大小                    */
        
        } else {
            stackNew.ss_flags = SS_DISABLE;
            stackNew.ss_size  = 0;
        }
        if (sigaltstack(&stackNew, &stackOld)) {
            return  (PX_ERROR);
        }
    
    } else if (oss) {
        sigaltstack(LW_NULL, &stackOld);
    }
        
    if (oss) {
        oss->ss_sp = stackOld.ss_sp;
        if (stackOld.ss_flags == SS_ONSTACK) {
            oss->ss_onstack = 1;
        } else {
            oss->ss_onstack = 0;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigaltstack
** 功能描述: 设置信号上下文的堆栈
** 输　入  : ss                       新的堆栈信息
**           oss                      老的堆栈信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  sigaltstack (const stack_t *ss, stack_t *oss)
{
    PLW_CLASS_TCB           ptcbCur;
    PLW_CLASS_SIGCONTEXT    psigctx;
    stack_t                *pstack;
    
    if (ss) {
        if ((ss->ss_flags != 0) && 
            (ss->ss_flags != SS_DISABLE)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        if (ss->ss_flags == 0) {
            if (!ss->ss_sp || 
                !ALIGNED(ss->ss_sp, LW_CFG_HEAP_ALIGNMENT)) {           /*  必须满足对齐关系            */
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            if (ss->ss_size < MINSIGSTKSZ) {
                _ErrorHandle(ENOMEM);
                return  (PX_ERROR);
            }
        }
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    psigctx = _signalGetCtx(ptcbCur);
    pstack  = &psigctx->SIGCTX_stack;
    
    if (oss) {
        if (pstack->ss_flags == 0) {
            if ((ptcbCur->TCB_pstkStackNow >= (PLW_STACK)pstack->ss_sp) && 
                (ptcbCur->TCB_pstkStackNow <  (PLW_STACK)((size_t)pstack->ss_sp + 
                                                          pstack->ss_size))) {
                oss->ss_flags = SS_ONSTACK;
            } else {
                oss->ss_flags = 0;
            }
            oss->ss_sp   = pstack->ss_sp;
            oss->ss_size = pstack->ss_size;
        
        } else {
            oss->ss_sp    = LW_NULL;
            oss->ss_size  = 0;
            oss->ss_flags = SS_DISABLE;
        }
    }
    
    if (ss) {
        if ((ptcbCur->TCB_pstkStackNow >= (PLW_STACK)pstack->ss_sp) && 
            (ptcbCur->TCB_pstkStackNow <  (PLW_STACK)((size_t)pstack->ss_sp + 
                                                      pstack->ss_size))) {
            _ErrorHandle(EPERM);                                        /*  正在信号上下文中使用此堆栈  */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            return  (PX_ERROR);
        }
        if (ss->ss_flags == 0) {
            pstack->ss_sp    = ss->ss_sp;
            pstack->ss_size  = ROUND_DOWN(ss->ss_size, LW_CFG_HEAP_ALIGNMENT);
            pstack->ss_flags = 0;
            
        } else {
            pstack->ss_sp    = LW_NULL;
            pstack->ss_size  = 0;
            pstack->ss_flags = SS_DISABLE;
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: kill
** 功能描述: 向指定的线程发送信号, 如果是进程, 将发送给其主线程.
** 输　入  : ulId                    线程 id 或者 进程号
**           iSigNo                  信号
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  kill (LW_OBJECT_HANDLE  ulId, INT  iSigNo)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    
#if LW_CFG_SIGNALFD_EN > 0
             LW_SEND_VAL        sendval;
#endif
    
#if LW_CFG_MODULELOADER_EN > 0
    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  进程号                      */
        ulId  = vprocMainThread((pid_t)ulId);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if (iSigNo == 0) {                                                  /*  测试目标是否存在            */
        return  (ERROR_NONE);
    
    } else if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING() || (ulId == API_ThreadIdSelf())) {
        _excJobAdd((VOIDFUNCPTR)kill, (PVOID)ulId, (PVOID)iSigNo, 0, 0, 0, 0);
        return  (ERROR_NONE);
    }

#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  正工作在 SMP 多核模式       */
        if (API_ThreadStop(ulId)) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (ptcb->TCB_iDeleteProcStatus) {
#if LW_CFG_SMP_EN > 0
        if (LW_NCPUS > 1) {                                             /*  正工作在 SMP 多核模式       */
            _ThreadContinue(ptcb, LW_FALSE);                            /*  在内核状态下唤醒被停止线程  */
        }
#endif                                                                  /*  LW_CFG_SMP_EN               */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
        return  (PX_ERROR);
    }
    
#if LW_CFG_SIGNALFD_EN > 0
    sendval = _doKill(ptcb, iSigNo);                                    /*  产生信号                    */
#else
    _doKill(ptcb, iSigNo);                                              /*  产生信号                    */
#endif
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  正工作在 SMP 多核模式       */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  在内核状态下唤醒被停止线程  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (sendval == SEND_BLOCK) {
        _sigfdReadUnblock(ulId, iSigNo);
    }
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: raise
** 功能描述: 向自己发送信号
** 输　入  : iSigNo                  信号
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  raise (INT  iSigNo)
{
    return  (kill(API_ThreadIdSelf(), iSigNo));
}
/*********************************************************************************************************
** 函数名称: __sigqueue
** 功能描述: 发送队列类型信号, 如果是进程, 将发送给其主线程.
** 输　入  : ulId                    线程 id 或者 进程号
**           iSigNo                  信号
**           psigvalue               信号 value
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __sigqueue (LW_OBJECT_HANDLE  ulId, INT   iSigNo, const union sigval *psigvalue)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    union    sigval             sigvalue = *psigvalue;
    
#if LW_CFG_SIGNALFD_EN > 0
             LW_SEND_VAL        sendval;
#endif
             
#if LW_CFG_MODULELOADER_EN > 0
    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  进程号                      */
        ulId  = vprocMainThread((pid_t)ulId);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (iSigNo == 0) {                                                  /*  测试目标是否存在            */
        return  (ERROR_NONE);
    
    } else if (!__issig(iSigNo)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  正工作在 SMP 多核模式       */
        if (API_ThreadStop(ulId)) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (ptcb->TCB_iDeleteProcStatus) {
#if LW_CFG_SMP_EN > 0
        if (LW_NCPUS > 1) {                                             /*  正工作在 SMP 多核模式       */
            _ThreadContinue(ptcb, LW_FALSE);                            /*  在内核状态下唤醒被停止线程  */
        }
#endif                                                                  /*  LW_CFG_SMP_EN               */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
        return  (PX_ERROR);
    }
    
#if LW_CFG_SIGNALFD_EN > 0
    sendval = _doSigQueue(ptcb, iSigNo, sigvalue);
#else
    _doSigQueue(ptcb, iSigNo, sigvalue);
#endif

#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  正工作在 SMP 多核模式       */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  在内核状态下唤醒被停止线程  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (sendval == SEND_BLOCK) {
        _sigfdReadUnblock(ulId, iSigNo);
    }
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigqueue
** 功能描述: 发送队列类型信号, 如果是进程, 将发送给其主线程.
** 输　入  : ulId                    线程 id 或者 进程号
**           iSigNo                  信号
**           sigvalue                信号 value
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigqueue (LW_OBJECT_HANDLE  ulId, INT   iSigNo, const union sigval  sigvalue)
{
    REGISTER UINT16  usIndex;

#if LW_CFG_MODULELOADER_EN > 0
    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  进程号                      */
        ulId  = vprocMainThread((pid_t)ulId);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING() || (ulId == API_ThreadIdSelf())) {
        _excJobAdd((VOIDFUNCPTR)__sigqueue, (PVOID)ulId, (PVOID)iSigNo, (PVOID)&sigvalue, 0, 0, 0);
        return  (ERROR_NONE);
    }
    
    return  (__sigqueue(ulId, iSigNo, &sigvalue));
}
/*********************************************************************************************************
** 函数名称: sigTrap
** 功能描述: 向指定任务发送信号, 同时停止自己. (本程序在异常上下文中执行)
** 输　入  : ulIdSig                  线程 id (不允许为进程号)
**           ulIdStop                 需要等待结束的线程
**           pvSigValue               信号参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __sig_trap (LW_OBJECT_HANDLE  ulIdSig, LW_OBJECT_HANDLE  ulIdStop, PVOID  pvSigValue)
{
#if LW_CFG_SMP_EN > 0
    BOOL  bIsRunning;
    
    for (;;) {
        if (API_ThreadIsRunning(ulIdStop, &bIsRunning)) {
            return;
        }
        if (!bIsRunning) {
            break;
        }
        LW_SPINLOCK_DELAY();
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    
    kill(ulIdSig, SIGTRAP);
}
/*********************************************************************************************************
** 函数名称: sigTrap
** 功能描述: 向指定任务发送信号, 同时停止自己. (本程序在异常上下文中执行)
** 输　入  : ulId                     线程 id (不允许为进程号)
**           sigvalue                 信号参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigTrap (LW_OBJECT_HANDLE  ulId, const union sigval  sigvalue)
{
    REGISTER PLW_CLASS_TCB  ptcbCur;
    
    if (!LW_CPU_GET_CUR_NESTING()) {                                    /*  必须在异常中                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "not in exception mode.\r\n");
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    _ThreadStop(ptcbCur);
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    _excJobAdd(__sig_trap, (PVOID)ulId, (PVOID)ptcbCur->TCB_ulId, sigvalue.sival_ptr, 0, 0, 0);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pause
** 功能描述: 等待一个信号的到来
** 输　入  : NONE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  pause (VOID)
{
             INTREG         iregInterLevel;
             PLW_CLASS_TCB  ptcbCur;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */

    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_PAUSE, 
                      ptcbCur->TCB_ulId, LW_NULL);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  等待信号                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    _ErrorHandle(EINTR);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: sigsuspend
** 功能描述: 使用指定的掩码等待一个有效信号的到来, 然后返回先前的信号掩码.
** 输　入  : psigsetMask        指定的信号掩码
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigsuspend (const sigset_t  *psigsetMask)
{
             INTREG         iregInterLevel;
             PLW_CLASS_TCB  ptcbCur;
    REGISTER PLW_CLASS_PCB  ppcb;
             BOOL           bIsRun;
    
             PLW_CLASS_SIGCONTEXT   psigctx;
             sigset_t               sigsetOld;
             
    if (!psigsetMask) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGSUSPEND, 
                   ptcbCur->TCB_ulId, *psigsetMask, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    sigsetOld = psigctx->SIGCTX_sigsetMask;                             /*  记录先前的掩码              */
    psigctx->SIGCTX_sigsetMask = *psigsetMask & (~__SIGNO_UNMASK);
    
    bIsRun = _sigPendRun(ptcbCur);
    if (bIsRun) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        sigprocmask(SIG_SETMASK, &sigsetOld, LW_NULL);                  /*  设置为原先的 mask           */
        
        _ErrorHandle(EINTR);
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  等待信号                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    sigprocmask(SIG_SETMASK, &sigsetOld, NULL);
    
    _ErrorHandle(EINTR);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: sigwait
** 功能描述: 等待 sigset 内信号的到来，以串行的方式从信号队列中取出信号进行处理, 信号将不再被执行.
** 输　入  : psigset       指定的信号集
**           psiginfo      获取的信号信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigwait (const sigset_t  *psigset, INT  *piSig)
{
             INTREG             iregInterLevel;
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
    
             INT                    iSigNo;
             PLW_CLASS_SIGCONTEXT   psigctx;
             struct siginfo         siginfo;
             LW_CLASS_SIGWAIT       sigwt;
    
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGWAIT, 
                   ptcbCur->TCB_ulId, *psigset, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    iSigNo = _sigPendGet(psigctx, psigset, &siginfo);                   /*  检查当前是否有等待的信号    */
    if (__issig(iSigNo)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (piSig) {
            *piSig = iSigNo;
        }
        return  (ERROR_NONE);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  等待信号                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    sigwt.SIGWT_sigset = *psigset;
    psigctx->SIGCTX_sigwait = &sigwt;                                   /*  保存等待信息                */
    
    if (__KERNEL_EXIT()) {                                              /*  是否其他信号激活            */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EINTR);                                            /*  SA_RESTART 也退出           */
        return  (PX_ERROR);
    }
    
    psigctx->SIGCTX_sigwait = LW_NULL;
    if (piSig) {
        *piSig = sigwt.SIGWT_siginfo.si_signo;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sigwaitinfo
** 功能描述: 等待 sigset 内信号的到来，以串行的方式从信号队列中取出信号进行处理, 信号将不再被执行.
** 输　入  : psigset       指定的信号集
**           psiginfo      获取的信号信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigwaitinfo (const sigset_t *psigset, struct  siginfo  *psiginfo)
{
             INTREG             iregInterLevel;
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
    
             INT                    iSigNo;
             PLW_CLASS_SIGCONTEXT   psigctx;
             struct siginfo         siginfo;
             LW_CLASS_SIGWAIT       sigwt;
    
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGWAIT, 
                   ptcbCur->TCB_ulId, *psigset, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    iSigNo = _sigPendGet(psigctx, psigset, &siginfo);                   /*  检查当前是否有等待的信号    */
    if (__issig(iSigNo)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (psiginfo) {
            *psiginfo = siginfo;
        }
        return  (siginfo.si_signo);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;                   /*  等待信号                    */
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    sigwt.SIGWT_sigset = *psigset;
    psigctx->SIGCTX_sigwait = &sigwt;                                   /*  保存等待信息                */
    
    if (__KERNEL_EXIT()) {                                              /*  是否其他信号激活            */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EINTR);                                            /*  SA_RESTART 也退出           */
        return  (PX_ERROR);
    }
    
    psigctx->SIGCTX_sigwait = LW_NULL;
    if (psiginfo) {
        *psiginfo = sigwt.SIGWT_siginfo;
    }
    
    return  (sigwt.SIGWT_siginfo.si_signo);
}
/*********************************************************************************************************
** 函数名称: sigtimedwait
** 功能描述: 等待 sigset 内信号的到来，以串行的方式从信号队列中取出信号进行处理, 信号将不再被执行.
** 输　入  : psigset        指定的信号集
**           psiginfo      获取的信号信息
**           ptv           超时时间 (NULL 表示一直等待)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  sigtimedwait (const sigset_t *psigset, struct  siginfo  *psiginfo, const struct timespec *ptv)
{
             INTREG             iregInterLevel;
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
    
             INT                    iSigNo;
             PLW_CLASS_SIGCONTEXT   psigctx;
             struct siginfo         siginfo;
             LW_CLASS_SIGWAIT       sigwt;
             
             ULONG                  ulTimeout;
    
    if (!psigset) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (ptv == LW_NULL) {                                               /*  永久等待                    */
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTimeout = __timespecToTick(ptv);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_D2(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGWAIT, 
                   ptcbCur->TCB_ulId, *psigset, LW_NULL);
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    psigctx = _signalGetCtx(ptcbCur);
    
    iSigNo = _sigPendGet(psigctx, psigset, &siginfo);                   /*  检查当前是否有等待的信号    */
    if (__issig(iSigNo)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (psiginfo) {
            *psiginfo = siginfo;
        }
        return  (siginfo.si_signo);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  不进行等待                  */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(EAGAIN);
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_SIGNAL;              /*  等待信号                    */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  清空等待时间                */
    
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    
    if (ulTimeout != LW_OPTION_WAIT_INFINITE) {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  设置超时时间                */
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入等待扫描链              */
    } else {
        ptcbCur->TCB_ulDelay = 0ul;
    }
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    sigwt.SIGWT_sigset = *psigset;
    psigctx->SIGCTX_sigwait = &sigwt;                                   /*  保存等待信息                */
    
    if (__KERNEL_EXIT()) {                                              /*  是否其他信号激活            */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EINTR);                                            /*  SA_RESTART 也退出           */
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  等待超时                    */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        psigctx->SIGCTX_sigwait = LW_NULL;
        _ErrorHandle(EAGAIN);
        return  (PX_ERROR);
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    psigctx->SIGCTX_sigwait = LW_NULL;
    if (psiginfo) {
        *psiginfo = sigwt.SIGWT_siginfo;
    }
    
    return  (sigwt.SIGWT_siginfo.si_signo);
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
