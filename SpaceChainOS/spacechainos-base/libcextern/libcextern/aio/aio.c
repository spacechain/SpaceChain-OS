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
** 文   件   名: aio.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 26 日
**
** 描        述: posix 异步I/O兼容库. (用于进程模式)
** 注        意: 在 mutex 锁定 aiorc 后, 不能再锁定 aioqueue, 否则可能产生死锁.

** BUG:
2013.02.22  由于不同的进程有不同的文件描述符空间, 用户进程需要使用 aio 必须连接 cextern 外部 aio 库. 
2013.11.28  aio_suspend() 加入测试取消点.
2015.04.08  timespecToTick() 必须使用动态接口获取 HZ.
*********************************************************************************************************/
#include <SylixOS.h>
#include <aio.h>
#include <string.h>
#include <stdlib.h>
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#include "posix/aio/aio_lib.h"
#ifndef __issig
#define __issig(m) (1 <= (m) && (m) <= NSIG)
#endif
/*********************************************************************************************************
  timespec to tick
*********************************************************************************************************/
static inline  ULONG   timespecToTick (const struct timespec  *ptv)
{
    ULONG   ulTicks;
    ULONG   ulHz = API_TimeGetFrequency();

    ulTicks  = (ULONG)(ptv->tv_sec * ulHz);
    ulTicks += (((((ptv->tv_nsec / 1000) * ulHz) / 100) / 100) / 100);

    return  (ulTicks);
}
/*********************************************************************************************************
  SylixOS list
*********************************************************************************************************/
extern VOID  _List_Ring_Add_Ahead(PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader);
extern VOID  _List_Ring_Add_Front(PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader);
extern VOID  _List_Ring_Add_Last(PLW_LIST_RING   pringNew, LW_LIST_RING_HEADER  *ppringHeader);
extern VOID  _List_Ring_Del(PLW_LIST_RING        pringDel, LW_LIST_RING_HEADER  *ppringHeader);
extern VOID  _List_Line_Add_Ahead(PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader);
extern VOID  _List_Line_Add_Tail( PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader);
extern VOID  _List_Line_Add_Left(PLW_LIST_LINE   plineNew, PLW_LIST_LINE  plineRight);
extern VOID  _List_Line_Add_Right(PLW_LIST_LINE  plineNew, PLW_LIST_LINE  plineLeft);
extern VOID  _List_Line_Del(PLW_LIST_LINE        plineDel, LW_LIST_LINE_HEADER  *pplineHeader);
/*********************************************************************************************************
  SylixOS send a signal
*********************************************************************************************************/
extern INT  _doSigEvent(LW_OBJECT_HANDLE  ulId, struct sigevent  *psigevent, INT  iSigCode);
/*********************************************************************************************************
** 函数名称: __aioWaitCleanup
** 功能描述: 清理 wait 队列
** 输　入  : paiowt            paiowait 链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __aioWaitCleanup (AIO_WAIT_CHAIN  *paiowt)
{
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    __aioDeleteWaitChain(paiowt);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
}
/*********************************************************************************************************
** 函数名称: aio_suspend
** 功能描述: 等待指定的一个或多个异步 I/O 请求操作完成
** 输　入  : list              aio 请求控制块数组 (如果其中有 NULL, 程序将忽略)
**           nent              数组元素个数
**           timeout           超时时间
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  aio_suspend (const struct aiocb * const list[], int nent, const struct timespec *timeout)
{
    AIO_WAIT_CHAIN     *paiowc;
    struct aiowait     *paiowait;
    ULONG               ulTimeout;
    
    BOOL                bNeedWait = LW_FALSE;
    
    int                 iWait = 0;                                      /*  paiowait 下标               */
    int                 iCnt;                                           /*  list 下标                   */
    int                 iRet = ERROR_NONE;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

	API_ThreadTestCancel();

    if ((nent <= 0) || (list == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (timeout == LW_NULL) {                                           /*  永久等待                    */
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTimeout = timespecToTick(timeout);
    }
    
    paiowc = __aioCreateWaitChain(nent, LW_TRUE);                       /*  创建等待队列                */
    if (paiowc == LW_NULL) {
        errno = ENOMEM;                                                 /*  should be EAGAIN ?          */
        return  (PX_ERROR);
    }
    
    paiowait = paiowc->aiowc_paiowait;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    
    for (iCnt = 0; iCnt < nent; iCnt++) {
        if (list[iCnt]) {
            if (list[iCnt]->aio_req.aioreq_error == EINPROGRESS) {      /*  必须为正在处理状态          */
                
                paiowait[iWait].aiowt_pcond     = &paiowc->aiowc_cond;
                paiowait[iWait].aiowt_pcnt      = LW_NULL;
                paiowait[iWait].aiowt_psigevent = LW_NULL;
                paiowait[iWait].aiowt_paiowc    = LW_NULL;              /*  不需要代理线程释放          */
                
                iRet = __aioAttachWait(list[iCnt], &paiowait[iWait]);
                if (iRet != ERROR_NONE) {
                    errno = EINVAL;
                    break;
                }
                
                _List_Line_Add_Tail(&paiowait[iWait].aiowt_line, 
                                    &paiowc->aiowc_pline);              /*  加入 wait 链表              */
                iWait++;
                
                bNeedWait = LW_TRUE;                                    /*  有 wait 链表节点连接,       */
            } else {
                break;
            }
        }
    }
    
    if (iRet != ERROR_NONE) {                                           /*  __aioAttachWait 失败        */
        __aioDeleteWaitChain(paiowc);
        API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((iCnt == nent) && bNeedWait) {                                  /*  是否需要等待                */
        API_ThreadCleanupPush(__aioWaitCleanup, paiowc);
        
        iRet = (INT)API_ThreadCondWait(&paiowc->aiowc_cond, _G_aioqueue.aioq_mutex, ulTimeout);
        
        API_ThreadCleanupPop(LW_FALSE);                                 /*  这里已经锁定 _G_aioqueue    */
                                                                        /*  不能运行 __aioWaitCleanup   */
        __aioDeleteWaitChain(paiowc);
        
        if (iRet != ERROR_NONE) {
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            errno = EAGAIN;
            return  (PX_ERROR);
        }
    
    } else {                                                            /*  不需要等待                  */
        __aioDeleteWaitChain(paiowc);
    }
    
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: lio_listio
** 功能描述: 同时发起多个传输 mode 参数可以是 LIO_WAIT 或 LIO_NOWAIT。LIO_WAIT 会阻塞这个调用,
             直到所有的 I/O 都完成为止. 在操作进行排队之后，LIO_NOWAIT 就会返回. 
             sigevent 引用定义了在所有 I/O 操作都完成时产生信号的方法。
** 输　入  : mode              LIO_WAIT or LIO_NOWAIT。LIO_WAIT
**           list              aio 请求控制块数组 (如果其中有 NULL, 程序将忽略)
**           nent              数组元素个数
**           sig               所有 I/O 操作都完成时产生信号的方法
                               LIO_WAIT 将会忽略此参数!
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  lio_listio (int mode, struct aiocb * const list[], int nent, struct sigevent *sig)
{
    AIO_WAIT_CHAIN     *paiowc;
    struct aiowait     *paiowait;
    
    int                 iNotify;
    int                 iWait = 0;                                      /*  paiowait 下标               */
    int                 iCnt;                                           /*  list 下标                   */
    int                 iRet = ERROR_NONE;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */
    
    if ((mode != LIO_WAIT) && (mode != LIO_NOWAIT)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((nent <= 0) || (list == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (mode == LIO_WAIT) {
        paiowc = __aioCreateWaitChain(nent, LW_TRUE);                   /*  创建等待队列                */
    } else {
        paiowc = __aioCreateWaitChain(nent, LW_FALSE);
    }
    if (paiowc == LW_NULL) {
        errno = ENOMEM;                                                 /*  should be EAGAIN ?          */
        return  (PX_ERROR);
    }
    
    if (sig) {
        paiowc->aiowc_sigevent = *sig;
    } else {
        paiowc->aiowc_sigevent.sigev_signo = 0;                         /*  无效信号不投递              */
    }
    
    paiowait = paiowc->aiowc_paiowait;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    
    for (iCnt = 0; iCnt < nent; iCnt++) {
        if (list[iCnt]) {
            if ((list[iCnt]->aio_lio_opcode != LIO_NOP) &&
                (list[iCnt]->aio_req.aioreq_error != EINPROGRESS)) {
                
                iNotify  = list[iCnt]->aio_sigevent.sigev_notify;
                iNotify &= ~SIGEV_THREAD_ID;
                if ((iNotify != SIGEV_SIGNAL) && 
                    (iNotify != SIGEV_THREAD)) {
                    list[iCnt]->aio_sigevent.sigev_notify = SIGEV_NONE;
                }
                
                list[iCnt]->aio_req.aioreq_thread = API_ThreadIdSelf(); /*  接收单个完成信号任务        */
                
                if (mode == LIO_WAIT) {
                    paiowait[iWait].aiowt_pcond     = &paiowc->aiowc_cond;
                    paiowait[iWait].aiowt_pcnt      = LW_NULL;
                    paiowait[iWait].aiowt_psigevent = LW_NULL;
                    paiowait[iWait].aiowt_paiowc    = LW_NULL;          /*  不需要代理线程释放          */
                
                } else {
                    paiowait[iWait].aiowt_pcond     = LW_NULL;
                    paiowait[iWait].aiowt_pcnt      = LW_NULL;
                    paiowait[iWait].aiowt_psigevent = &paiowc->aiowc_sigevent;
                    paiowait[iWait].aiowt_paiowc    = (void *)paiowc;
                }
                
                __aioAttachWaitNoCheck(list[iCnt], &paiowait[iWait]);   /*  aiocb and aiowait attach    */
                
                if (__aioEnqueue(list[iCnt]) == ERROR_NONE) {
                    _List_Line_Add_Tail(&paiowait[iWait].aiowt_line, 
                                        &paiowc->aiowc_pline);          /*  加入 wait 链表              */
                    iWait++;
                
                } else {
                    __aioDetachWait(LW_NULL, &paiowait[iWait]);         /*  删除关联关系                */
                    
                    iRet = PX_ERROR;
                }
            }
        }
    }
    
    if (iWait == 0) {                                                   /*  没有一个节点加入队列        */
        __aioDeleteWaitChain(paiowc);
        API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
        
        if (mode == LIO_NOWAIT) {
            if (sig && __issig(sig->sigev_signo)) {
                _doSigEvent(API_ThreadIdSelf(), sig, SI_ASYNCIO);
            }
        }
        return  (ERROR_NONE);
    
    } else if (mode == LIO_WAIT) {                                      /*  需要等待                    */
        __aioWaitChainSetCnt(paiowc, &iWait);                           /*  设置计数变量                */
        
        while (iWait > 0) {
            if (API_ThreadCondWait(&paiowc->aiowc_cond, 
                                   _G_aioqueue.aioq_mutex, 
                                   LW_OPTION_WAIT_INFINITE)) {          /*  等待所有请求执行完毕        */
                break;
            }
        }
        
        __aioDeleteWaitChain(paiowc);                                   /*  清除 wait 队列              */
        
    } else {                                                            /*  LIO_NOWAIT                  */
        paiowc->aiowc_icnt = iWait;                                     /*  必须使用全局变量            */
        
        __aioWaitChainSetCnt(paiowc, &paiowc->aiowc_icnt);              /*  设置计数变量                */
    }
    
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: aio_cancel
** 功能描述: 取消一个 aio 请求
** 输　入  : fildes            文件描述符
**           paiocb            aio 请求控制块 (可以为 NULL)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  aio_cancel (int fildes, struct aiocb *paiocb)
{
    AIO_REQ_CHAIN          *paiorc;
    INT                     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (iosFdGetName(fildes, LW_NULL, 0)) {                             /*  如出错 errno 自动设为 EBADF */
        return  (PX_ERROR);
    }

    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    
    if (paiocb == LW_NULL) {
        /*
         *  如果是 cancel 整个文件描述符, 那么这里调用 __aioCancelFd() 进行, __aioCancelFd() 内部将所有
         *  等待操作的节点释放, 然后通知代理线程删除 paiorc 结构, 这样是最安全的操作方式, 如果这里直接删除
         *  那么正在操作的 paiorc 可能会有危险, 
         *  从应用层角度是看不到这个过程的! 我们保证在函数返回前, 将所有的 aiocb 处理完毕!
         */
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               fildes);                                 /*  搜索工作队列中有没有相同的  */
        if (paiorc == LW_NULL) {
            if (_G_aioqueue.aioq_plineidle) {
                paiorc = __aioSearchFd(_G_aioqueue.aioq_plineidle, 
                                       fildes);                         /*  搜索空闲队列                */
                if (paiorc == LW_NULL) {
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (AIO_ALLDONE);
                
                } else {
                    __aioCancelFd(paiorc);                              /*  删除所有节点后通知任务删除  */
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (AIO_CANCELED);
                }
            
            } else {
                API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                return  (AIO_ALLDONE);
            }
        
        } else {
            __aioCancelFd(paiorc);                                      /*  删除所有节点后通知任务删除  */
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            return  (AIO_CANCELED);
        }
    
    } else {
        if (paiocb->aio_fildes != fildes) {
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            errno = EINVAL;
            return  (PX_ERROR);
        }
        
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               fildes);                                 /*  搜索工作队列中有没有相同的  */
        if (paiorc == LW_NULL) {
            if (_G_aioqueue.aioq_plineidle) {
                paiorc = __aioSearchFd(_G_aioqueue.aioq_plineidle, 
                                       fildes);                         /*  搜索空闲队列                */
                if (paiorc == LW_NULL) {
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (AIO_ALLDONE);
                
                } else {
                    API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
                    iRet = __aioRemoveAiocb(paiorc, paiocb, 
                                            PX_ERROR, ECANCELED);       /*  移除 aiocb                  */
                    API_SemaphoreMPost(paiorc->aiorc_mutex);
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (iRet);
                }
            
            } else {
                API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                return  (AIO_ALLDONE);
            }
        
        } else {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            iRet = __aioRemoveAiocb(paiorc, paiocb,
                                    PX_ERROR, ECANCELED);               /*  移除 aiocb                  */
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            return  (iRet);
        }
    }
    
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    return  (AIO_ALLDONE);
}
/*********************************************************************************************************
** 函数名称: aio_error
** 功能描述: 获得 aio 操作完成后的 errno
** 输　入  : paiocb            aio 请求控制块
** 输　出  : 操作返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  aio_error (const struct aiocb *paiocb)
{
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */
    
    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_ObjectGetClass(paiocb->aio_req.aioreq_thread) != _OBJECT_THREAD) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (paiocb->aio_req.aioreq_error);
}
/*********************************************************************************************************
** 函数名称: aio_return
** 功能描述: 获得 aio 操作完成后的返回值
** 输　入  : paiocb            aio 请求控制块
** 输　出  : 操作返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ssize_t  aio_return (struct aiocb *paiocb)
{
    ssize_t  ret;

#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */
    
    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_ObjectGetClass(paiocb->aio_req.aioreq_thread) != _OBJECT_THREAD) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (paiocb->aio_req.aioreq_flags & AIO_REQ_FREE) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (paiocb->aio_req.aioreq_error == EINPROGRESS) {
        errno = EINPROGRESS;
        return  (PX_ERROR);
    }
    
    ret = paiocb->aio_req.aioreq_return;
    
    paiocb->aio_req.aioreq_return = PX_ERROR;
    paiocb->aio_req.aioreq_flags |= AIO_REQ_FREE;
    
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: aio_read
** 功能描述: aio 读取文件内容
** 输　入  : paiocb            aio 请求控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  aio_read (struct aiocb *paiocb)
{
    INT     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (paiocb->aio_offset < 0) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iosFdGetName(paiocb->aio_fildes, LW_NULL, 0)) {                 /*  如出错 errno 自动设为 EBADF */
        return  (PX_ERROR);
    }
    
    paiocb->aio_lio_opcode = LIO_READ;
    
    paiocb->aio_req.aioreq_thread = API_ThreadIdSelf();
    paiocb->aio_pwait = LW_NULL;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    iRet = __aioEnqueue(paiocb);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: aio_wirte
** 功能描述: aio 写入文件内容
** 输　入  : paiocb            aio 请求控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  aio_write (struct aiocb *paiocb)
{
    INT     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (paiocb->aio_offset < 0) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iosFdGetName(paiocb->aio_fildes, LW_NULL, 0)) {                 /*  如出错 errno 自动设为 EBADF */
        return  (PX_ERROR);
    }
    
    paiocb->aio_lio_opcode = LIO_WRITE;
    
    paiocb->aio_req.aioreq_thread = API_ThreadIdSelf();
    paiocb->aio_pwait = LW_NULL;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    iRet = __aioEnqueue(paiocb);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: aio_fsync
** 功能描述: aio 同步文件的内容, 将缓冲内部的数据写入文件
** 输　入  : op                操作选项 (O_SYNC or O_DSYNC)
**           paiocb            aio 请求控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  aio_fsync (int op, struct aiocb *paiocb)
{
    INT     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  需要使用外部库提供的 aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iosFdGetName(paiocb->aio_fildes, LW_NULL, 0)) {                 /*  如出错 errno 自动设为 EBADF */
        return  (PX_ERROR);
    }
    
    if (API_ObjectGetClass(paiocb->aio_req.aioreq_thread) != _OBJECT_THREAD) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    paiocb->aio_lio_opcode = LIO_SYNC;
    
    paiocb->aio_req.aioreq_thread = API_ThreadIdSelf();
    paiocb->aio_pwait = LW_NULL;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    iRet = __aioEnqueue(paiocb);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
