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
** 文   件   名: signalEvent.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 01 日
**
** 描        述: 发送 sigevent 参数信号.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0
#include "signalPrivate.h"
#if LW_CFG_POSIX_EN > 0
#include "../SylixOS/posix/include/px_pthread.h"
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  进程相关处理
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#define __tcb_pid(ptcb)     vprocGetPidByTcbNoLock(ptcb)
#else
#define __tcb_pid(ptcb)     0
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  内部函数声明
*********************************************************************************************************/
extern VOID         _sigPendInit(PLW_CLASS_SIGPEND  psigpend);
extern LW_SEND_VAL  _doSignal(PLW_CLASS_TCB  ptcb, PLW_CLASS_SIGPEND   psigpend);
/*********************************************************************************************************
  工作队列发送参数类型
*********************************************************************************************************/
typedef union {
    LW_LIST_MONO            SE_monoLink;
    struct {
        struct sigevent     SE_sigevent;
        struct siginfo      SE_siginfo;
    } SE_event;
} SIGNAL_EVENT_ARG;
typedef SIGNAL_EVENT_ARG   *PSIGNAL_EVENT_ARG;
/*********************************************************************************************************
  工作队列发送参数池
*********************************************************************************************************/
#define SIGNAL_EVENT_POOL_SIZE  (NSIG + LW_CFG_MAX_EXCEMSGS)
static SIGNAL_EVENT_ARG         _G_sigeventargPool[SIGNAL_EVENT_POOL_SIZE];
static LW_LIST_MONO_HEADER      _G_pmonoSigEventArg;
/*********************************************************************************************************
** 函数名称: __sigEventArgInit
** 功能描述: 初始化 sig event 参数缓冲
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __sigEventArgInit (VOID)
{
    INT                 i;
    PSIGNAL_EVENT_ARG   psigea1;
    PSIGNAL_EVENT_ARG   psigea2;
    
    psigea1 = &_G_sigeventargPool[0];
    psigea2 = &_G_sigeventargPool[1];
    
    for (i = 0; i < (SIGNAL_EVENT_POOL_SIZE - 1); i++) {
        _LIST_MONO_LINK(&psigea1->SE_monoLink, &psigea2->SE_monoLink);
        psigea1++;
        psigea2++;
    }
    
    _INIT_LIST_MONO_HEAD(&psigea1->SE_monoLink);
    
    _G_pmonoSigEventArg = &_G_sigeventargPool[0].SE_monoLink;
}
/*********************************************************************************************************
** 函数名称: __sigEventArgAlloc
** 功能描述: 获取一个 sig event 参数缓冲
** 输　入  : NONE
** 输　出  : sig event 参数缓冲
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PSIGNAL_EVENT_ARG  __sigEventArgAlloc (VOID)
{
    return  ((PSIGNAL_EVENT_ARG)_list_mono_allocate(&_G_pmonoSigEventArg));
}
/*********************************************************************************************************
** 函数名称: __sigEventArgFree
** 功能描述: 释放一个 sig event 参数缓冲
** 输　入  : sigea     sig event 参数缓冲
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __sigEventArgFree (PSIGNAL_EVENT_ARG  psigea)
{
    _list_mono_free(&_G_pmonoSigEventArg, &psigea->SE_monoLink);
}
/*********************************************************************************************************
** 函数名称: _doSigEventInternal
** 功能描述: 用 sigevent 发送一个信号内部接口
** 输　入  : ulId                   目标线程 ID
**           psigea                 信号信息
**           bNeedFree              信号信息是否需要释放
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _doSigEventInternal (LW_OBJECT_HANDLE  ulId, PSIGNAL_EVENT_ARG   psigea, BOOL  bNeedFree)
{
    REGISTER UINT16              usIndex;
    REGISTER PLW_CLASS_TCB       ptcb;
             LW_CLASS_SIGPEND    sigpend;
             INT                 iNotify;
             INT                 iError = PX_ERROR;
             
#if LW_CFG_SIGNALFD_EN > 0
             LW_SEND_VAL         sendval;
#endif
             
    struct sigevent  *psigevent = &psigea->SE_event.SE_sigevent;
    struct siginfo   *psiginfo  = &psigea->SE_event.SE_siginfo;

#if LW_CFG_POSIX_EN > 0
    iNotify = psigevent->sigev_notify & (~SIGEV_THREAD_ID);
    if (psigevent->sigev_notify & SIGEV_THREAD_ID) {
        ulId = psigevent->sigev_notify_thread_id;                       /*  向指定线程发送信号          */
    }
#else
    iNotify = psigevent->sigev_notify;
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
    
    if ((iNotify != SIGEV_SIGNAL) &&
        (iNotify != SIGEV_THREAD)) {
        goto    __out;
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (ulId <= LW_CFG_MAX_THREADS) {                                   /*  进程号                      */
        ulId  = vprocMainThread((pid_t)ulId);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        goto    __out;
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        goto    __out;
    }
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_SIGNAL, MONITOR_EVENT_SIGNAL_SIGEVT, 
                      ulId, 
                      psigevent->sigev_signo, 
                      psigevent->sigev_notify,
                      psigevent->sigev_value.sival_ptr,
                      LW_NULL);
                      
    if ((iNotify == SIGEV_THREAD) &&
        (psigevent->sigev_notify_function)) {
#if LW_CFG_POSIX_EN > 0
        pthread_t   tid;
        
        if (pthread_create(&tid, psigevent->sigev_notify_attributes,
                           (void *(*)(void*))(psigevent->sigev_notify_function), 
                           psigevent->sigev_value.sival_ptr) < 0) {
            goto    __out;
        
        } else {
            iError = ERROR_NONE;
            goto    __out;
        }
#else
        _ErrorHandle(ENOSYS);
        goto    __out;
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
    }
    
    _sigPendInit(&sigpend);
    
    sigpend.SIGPEND_siginfo          = *psiginfo;
    sigpend.SIGPEND_siginfo.si_signo = psigevent->sigev_signo;          /*  以 sigevent 为主            */
    sigpend.SIGPEND_siginfo.si_errno = errno;
    sigpend.SIGPEND_siginfo.si_value = psigevent->sigev_value;
    sigpend.SIGPEND_iNotify          = iNotify;
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  正工作在 SMP 多核模式       */
        if (API_ThreadStop(ulId)) {
            goto    __out;
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(EINVAL);
        goto    __out;
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
        goto    __out;
    }
        
#if LW_CFG_SIGNALFD_EN > 0
    sendval = _doSignal(ptcb, &sigpend);
#else
    _doSignal(ptcb, &sigpend);
#endif
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {                                                 /*  正工作在 SMP 多核模式       */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  在内核状态下唤醒被停止线程  */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
#if LW_CFG_SIGNALFD_EN > 0
    if (sendval == SEND_BLOCK) {
        _sigfdReadUnblock(ulId, sigpend.SIGPEND_siginfo.si_signo);
    }
#endif                                                                  /*  LW_CFG_SIGNALFD_EN > 0      */

    iError = ERROR_NONE;
    
__out:
    if (bNeedFree) {
        __sigEventArgFree(psigea);
    }
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: _doSigEvent
** 功能描述: 用 sigevent 发送一个信号
** 输　入  : ulId                   目标线程 ID
**           psigevent              信号 event
**           iSigCode               si_code
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _doSigEvent (LW_OBJECT_HANDLE  ulId, struct sigevent *psigevent, INT  iSigCode)
{
    struct siginfo     *psiginfo;
    PLW_CLASS_TCB       ptcbCur;
    SIGNAL_EVENT_ARG    sigea;
    PSIGNAL_EVENT_ARG   psigea;
    BOOL                bExc;
    
    if (!psigevent) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (psigevent->sigev_notify == SIGEV_NONE) {                        /*  不发送信号                  */
        return  (ERROR_NONE);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (LW_CPU_GET_CUR_NESTING() || (ulId == ptcbCur->TCB_ulId)) {
        psigea = __sigEventArgAlloc();
        if (psigea == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        bExc   = LW_TRUE;
        
    } else {
        psigea = &sigea;
        bExc   = LW_FALSE;
    }
    
    psigea->SE_event.SE_sigevent = *psigevent;
    psiginfo = &psigea->SE_event.SE_siginfo;
    psiginfo->si_code = iSigCode;
    psiginfo->si_pid  = __tcb_pid(ptcbCur);
    psiginfo->si_uid  = ptcbCur->TCB_uid;
    
    if (bExc) {
        if (_excJobAdd((VOIDFUNCPTR)_doSigEventInternal, (PVOID)ulId, (PVOID)psigea, 
                       (PVOID)LW_TRUE, 0, 0, 0) < ERROR_NONE) {
            __sigEventArgFree(psigea);
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);
    
    } else {
        return  (_doSigEventInternal(ulId, psigea, LW_FALSE));
    }
}
/*********************************************************************************************************
** 函数名称: _doSigEventEx
** 功能描述: 用 sigevent 发送一个信号扩展接口
** 输　入  : ulId                   目标线程 ID
**           psigevent              信号 event
**           psiginfo               信号信息
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _doSigEventEx (LW_OBJECT_HANDLE  ulId, struct sigevent *psigevent, struct siginfo *psiginfo)
{
    PLW_CLASS_TCB       ptcbCur;
    SIGNAL_EVENT_ARG    sigea;
    PSIGNAL_EVENT_ARG   psigea;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!psigevent || !psiginfo) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (psigevent->sigev_notify == SIGEV_NONE) {                        /*  不发送信号                  */
        return  (ERROR_NONE);
    }
    
    if (LW_CPU_GET_CUR_NESTING() || (ulId == ptcbCur->TCB_ulId)) {
        psigea = __sigEventArgAlloc();
        if (psigea == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        psigea->SE_event.SE_sigevent = *psigevent;
        psigea->SE_event.SE_siginfo  = *psiginfo;
        
        if (_excJobAdd((VOIDFUNCPTR)_doSigEventInternal, (PVOID)ulId, (PVOID)psigea, 
                       (PVOID)LW_TRUE, 0, 0, 0) < ERROR_NONE) {
            __sigEventArgFree(psigea);
            return  (PX_ERROR);
        }
        return  (ERROR_NONE);
                   
    } else {
        psigea = &sigea;
        
        psigea->SE_event.SE_sigevent = *psigevent;
        psigea->SE_event.SE_siginfo  = *psiginfo;
        
        return  (_doSigEventInternal(ulId, psigea, LW_FALSE));
    }
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
