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
** 文   件   名: aio_lib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 26 日
**
** 描        述: posix 异步I/O内部库. (用于进程模式)
** 注        意: 在 mutex 锁定 aiorc 后, 不能再锁定 aioqueue, 否则可能产生死锁.

** BUG:
2013.02.22  __aioThread() 不再使用 lseek 操作, 而是使用 pread 和 pwrite 进行读写.
2017.08.11  加入 AIO_REQ_BUSY 标识, 表明文件正在被操作.
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
  链表操作宏
*********************************************************************************************************/
#ifndef _LIST_OFFSETOF
#define _LIST_OFFSETOF(type, member)                          \
        ((size_t)&((type *)0)->member)
#define _LIST_CONTAINER_OF(ptr, type, member)                 \
        ((type *)((size_t)ptr - _LIST_OFFSETOF(type, member)))
#define _LIST_ENTRY(ptr, type, member)                        \
        _LIST_CONTAINER_OF(ptr, type, member)
#define _list_line_get_next(pline)  ((pline)->LINE_plistNext)
#endif
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
  aio 代理线程配置
*********************************************************************************************************/
#define __PX_AIO_TIMEOUT    (3 * CLOCKS_PER_SEC)
/*********************************************************************************************************
  aio 代理线程创建参数
*********************************************************************************************************/
static LW_CLASS_THREADATTR  _G_threadattrAio = {
    LW_NULL, LW_CFG_THREAD_DEFAULT_GUARD_SIZE, LW_CFG_POSIX_AIO_STK_SIZE, LW_PRIO_NORMAL, 
    LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_DETACHED,
    LW_NULL, LW_NULL
};
/*********************************************************************************************************
  aio 全局变量
*********************************************************************************************************/
AIO_QUEUE                   _G_aioqueue;                                /*  aio 队列管理                */
/*********************************************************************************************************
  aio 内部函数声明
*********************************************************************************************************/
static INT  __aioRemoveAllAiocb(AIO_REQ_CHAIN  *paiorc, INT  iError, INT  iErrNo);
/*********************************************************************************************************
** 函数名称: aio_init
** 功能描述: 初始化 AIO 函数库
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static __attribute__((constructor)) VOID  aio_init (VOID)
{
    _G_aioqueue.aioq_mutex = API_SemaphoreMCreate("aio_mutex", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY |
                                                  LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY, LW_NULL);
    if (_G_aioqueue.aioq_mutex == 0) {
        perror("aio_init() error : can not initialize mutex.");
        return;
    }
    
    if (API_ThreadCondInit(&_G_aioqueue.aioq_newreq, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_aioqueue.aioq_mutex);
        perror("aio_init() error : can not initialize thread cond.");
        return;
    }
    
    _G_aioqueue.aioq_plinework = LW_NULL;
    _G_aioqueue.aioq_plineidle = LW_NULL;
    
    _G_aioqueue.aioq_idlethread = 0;
    _G_aioqueue.aioq_actthread  = 0;
}
/*********************************************************************************************************
** 函数名称: aio_setstacksize
** 功能描述: 设置 aio 代理线程的堆栈大小 (仅对未来启动的线程有效)
** 输　入  : newsize       新的大小
** 输　出  : ERROR_NONE or ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  aio_setstacksize (size_t  newsize)
{
    if (newsize < LW_CFG_POSIX_AIO_STK_SIZE) {
        errno = EINVAL;
        return  (EINVAL);
    }
    _G_threadattrAio.THREADATTR_stStackByteSize = newsize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: aio_getstacksize
** 功能描述: 获取 aio 代理线程的堆栈大小
** 输　入  : NONE
** 输　出  : 堆栈大小
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
size_t  aio_getstacksize (void)
{
    return  (_G_threadattrAio.THREADATTR_stStackByteSize);
}
/*********************************************************************************************************
** 函数名称: __aioSearchFd
** 功能描述: 从指定的 AIO_REQ_CHAIN 中搜索与指定文件描述符相同的 aio req chain 节点. 
             (这里必须锁定 _G_aioqueue)
** 输　入  : plineHeader       搜索链表头
**           iFd               文件描述符
** 输　出  : 查找到的 aio req chain 节点
** 全局变量: 
** 调用模块: 
** 注  意  : 这个函数 search 到的 aiorc 都是没有被 cancel 的. 如果被 cancel 了, 一定会有代理线程将 cancel
             的 aiorc 删除. 
*********************************************************************************************************/
AIO_REQ_CHAIN  *__aioSearchFd (LW_LIST_LINE_HEADER  plineHeader, int  iFd)
{
    AIO_REQ_CHAIN  *paiorc;
    PLW_LIST_LINE   plineTemp;
    
    for (plineTemp  = plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        paiorc = _LIST_ENTRY(plineTemp, AIO_REQ_CHAIN, aiorc_line);
        if ((paiorc->aiorc_fildes == iFd) &&
            (paiorc->aiorc_iscancel == 0)) {                            /*  注意, cancel的paiorc不返回  */
            break;
        }
    }
    
    if (plineTemp) {
        return  (paiorc);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __aioCreateFd
** 功能描述: 创建一个 AIO_REQ_CHAIN 节点, 并加入指定链表 (这里必须锁定 _G_aioqueue)
** 输　入  : pplineHeader      创建后加入的链表表头地址
**           iFd               文件描述符
** 输　出  : 创建出的 aio req chain 节点
** 全局变量: 
** 调用模块: 
** 注  意  : 创建出的 AIO_REQ_CHAIN 节点, 一定是由代理线程 delete 的. 
*********************************************************************************************************/
static AIO_REQ_CHAIN  *__aioCreateFd (LW_LIST_LINE_HEADER  *pplineHeader, int  iFd)
{
    AIO_REQ_CHAIN  *paiorc;
    
    paiorc = (AIO_REQ_CHAIN *)malloc(sizeof(AIO_REQ_CHAIN));
    if (paiorc == LW_NULL) {
        return  (LW_NULL);
    }
    
    paiorc->aiorc_mutex = API_SemaphoreMCreate("aiorc_mutex", LW_PRIO_DEF_CEILING, 
                                LW_OPTION_DELETE_SAFE | LW_OPTION_INHERIT_PRIORITY, LW_NULL);
    if (paiorc->aiorc_mutex == 0) {
        free(paiorc);
        return  (LW_NULL);
    }
    
    if (API_ThreadCondInit(&paiorc->aiorc_cond, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
        API_SemaphoreMDelete(&paiorc->aiorc_mutex);
        free(paiorc);
        return  (LW_NULL);
    }
    
    paiorc->aiorc_plineaiocb = LW_NULL;
    paiorc->aiorc_fildes     = iFd;
    paiorc->aiorc_iscancel   = 0;                                       /*  没有被 cancel               */
    
    _List_Line_Add_Ahead(&paiorc->aiorc_line, pplineHeader);
    
    return  (paiorc);
}
/*********************************************************************************************************
** 函数名称: __aioDeleteFd
** 功能描述: 从指定的 AIO_REQ_CHAIN 中删除一个 AIO_REQ_CHAIN 节点 (这里必须锁定 _G_aioqueue)
** 输　入  : paiorc            AIO_REQ_CHAIN 节点
**           pplineHeader      创建后加入的链表表头地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __aioDeleteFd (AIO_REQ_CHAIN  *paiorc, LW_LIST_LINE_HEADER  *pplineHeader)
{
    API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
    __aioRemoveAllAiocb(paiorc, PX_ERROR, ECANCELED);                   /*  删除所有同文件描述符节点    */
    API_SemaphoreMPost(paiorc->aiorc_mutex);
    
    _List_Line_Del(&paiorc->aiorc_line, pplineHeader);

    API_SemaphoreMDelete(&paiorc->aiorc_mutex);
    API_ThreadCondDestroy(&paiorc->aiorc_cond);
    
    free(paiorc);
}
/*********************************************************************************************************
** 函数名称: __aioCancelFd
** 功能描述: 取消一个 AIO_REQ_CHAIN 节点, 
** 输　入  : paiorc            AIO_REQ_CHAIN 节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __aioCancelFd (AIO_REQ_CHAIN  *paiorc)
{
    API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
    __aioRemoveAllAiocb(paiorc, PX_ERROR, ECANCELED);                   /*  删除所有同文件描述符节点    */
    paiorc->aiorc_iscancel = 1;
    API_ThreadCondSignal(&paiorc->aiorc_cond);                          /*  通知代理线程删除 aiorc      */
    API_SemaphoreMPost(paiorc->aiorc_mutex);
}
/*********************************************************************************************************
** 函数名称: __aioInsertAiocb
** 功能描述: 将指定的 aiocb 按优先级插入到 aio req chain 节点下属的队列中 (这里必须锁定 paiorc)
** 输　入  : paiorc            目标 aio req chain 节点
**           paiocb            指定的 aiocb 
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 优先级数值越高, 越靠近表头, 请求越先被执行.
*********************************************************************************************************/
static VOID  __aioInsertAiocb (AIO_REQ_CHAIN  *paiorc, struct aiocb *paiocb)
{
    PLW_LIST_LINE   plineTemp;
    struct aioreq  *paioreqTemp;
    struct aiocb   *paiocbTemp;
    
    if (paiorc->aiorc_plineaiocb == LW_NULL) {                          /*  链表空, 插入表头即可        */
        _List_Line_Add_Ahead(&paiocb->aio_req.aioreq_line, &paiorc->aiorc_plineaiocb);
        return;
    }
    
    plineTemp = paiorc->aiorc_plineaiocb;
    do {
        paioreqTemp = _LIST_ENTRY(plineTemp, struct aioreq, aioreq_line);
        paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
        if (paiocbTemp->aio_reqprio < paiocb->aio_reqprio) {            /*  是否可以插入                */
            if (plineTemp == paiorc->aiorc_plineaiocb) {                /*  是否为表头位置              */
                _List_Line_Add_Ahead(&paiocb->aio_req.aioreq_line, 
                                     &paiorc->aiorc_plineaiocb);        /*  插入表头即可                */
                break;
            } else {
                _List_Line_Add_Left(&paiocb->aio_req.aioreq_line, 
                                    plineTemp);                         /*  插入当前节点的左侧          */
            }
            break;
        }
        if (_list_line_get_next(plineTemp) == LW_NULL) {                /*  最后一项                    */
            _List_Line_Add_Right(&paiocb->aio_req.aioreq_line, 
                                 plineTemp);                            /*  插入当前节点的右侧          */
            break;
        } else {
            plineTemp = _list_line_get_next(plineTemp);                 /*  继续检查下一项              */
        }
    } while (1);
}
/*********************************************************************************************************
** 函数名称: __aioRemoveAiocb
** 功能描述: 将指定的 aiocb 从 aio req chain 节点下属的队列中移除 (这里必须锁定 _G_aioqueue 与 paiorc)
** 输　入  : paiorc            aio req chain 节点
**           paiocb            指定的 aiocb 
**           iError            保存的返回值
**           iErrNo            保存的错误号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __aioRemoveAiocb (AIO_REQ_CHAIN  *paiorc, struct aiocb *paiocb, INT  iError, INT  iErrNo)
{
    PLW_LIST_LINE   plineTemp;
    struct aioreq  *paioreqTemp;
    struct aiocb   *paiocbTemp;
    
    for (plineTemp  = paiorc->aiorc_plineaiocb;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        paioreqTemp = _LIST_ENTRY(plineTemp, struct aioreq, aioreq_line);
        paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
        
        if (paiocbTemp == paiocb) {
            if (paiocbTemp->aio_req.aioreq_flags & AIO_REQ_BUSY) {
                return  (AIO_NOTCANCELED);
            }
        
            _List_Line_Del(plineTemp, 
                           &paiorc->aiorc_plineaiocb);
            
            paioreqTemp->aioreq_error  = iErrNo;
            paioreqTemp->aioreq_return = iError;
            
            if (paiocbTemp->aio_pwait) {
                __aioDetachWait(paiocbTemp, LW_NULL);                   /*  detach wait 关联            */
            }
            
            return  (AIO_CANCELED);
        }
    }
    
    return  (AIO_ALLDONE);
}
/*********************************************************************************************************
** 函数名称: __aioRemoveAllAiocb
** 功能描述: 将指定的 aiocb 下属的所有 aio req chain 节点移除 (这里必须锁定 _G_aioqueue 与 paiorc)
** 输　入  : paiorc            aio req chain 节点
**           iError            保存的返回值
**           iErrNo            保存的错误号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __aioRemoveAllAiocb (AIO_REQ_CHAIN  *paiorc, INT  iError, INT  iErrNo)
{
    struct aioreq  *paioreqTemp;
    struct aiocb   *paiocbTemp;

    while (paiorc->aiorc_plineaiocb) {
        paioreqTemp = _LIST_ENTRY(paiorc->aiorc_plineaiocb, struct aioreq, aioreq_line);
        paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
        
        _List_Line_Del(paiorc->aiorc_plineaiocb, 
                       &paiorc->aiorc_plineaiocb);
        
        paioreqTemp->aioreq_error  = iErrNo;
        paioreqTemp->aioreq_return = iError;
        
        if (paiocbTemp->aio_pwait) {
            __aioDetachWait(paiocbTemp, LW_NULL);                       /*  detach wait 关联            */
        }
    }
    return  (AIO_CANCELED);
}
/*********************************************************************************************************
** 函数名称: __aioAttachWait
** 功能描述: 向指定的 aiocb 中连接 wait 信息 (这里必须锁定 _G_aioqueue)
** 输　入  : paiocb            指定的 aiocb 
**           paiowait          wait 信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __aioAttachWait (const struct aiocb  *paiocb, struct aiowait  *paiowait)
{
    PLW_LIST_LINE   plineTempAiorc;
    PLW_LIST_LINE   plineTempAiocb;
    
    AIO_REQ_CHAIN   *paiorc;
    struct aioreq   *paioreqTemp;
    struct aiocb    *paiocbTemp;
    
    BOOL             bIsOk = LW_FALSE;
    
    for (plineTempAiorc  = _G_aioqueue.aioq_plinework;
         plineTempAiorc != LW_NULL;
         plineTempAiorc  = _list_line_get_next(plineTempAiorc)) {
         
        paiorc = _LIST_ENTRY(plineTempAiorc, AIO_REQ_CHAIN, aiorc_line);
        if (paiorc->aiorc_fildes == paiocb->aio_fildes) {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            for (plineTempAiocb  = paiorc->aiorc_plineaiocb;
                 plineTempAiocb != LW_NULL;
                 plineTempAiocb  = _list_line_get_next(plineTempAiocb)) {
                 
                paioreqTemp = _LIST_ENTRY(plineTempAiocb, struct aioreq, aioreq_line);
                paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
                
                if (paiocbTemp == paiocb) {
                    if (paiocbTemp->aio_pwait) {                        /*  不允许重复保存              */
                        API_SemaphoreMPost(paiorc->aiorc_mutex);
                        return  (PX_ERROR);
                    }
                    paiocbTemp->aio_pwait  = paiowait;
                    paiowait->aiowt_paiocb = paiocbTemp;
                    bIsOk = LW_TRUE;
                    break;
                }
            }
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            break;
        }
    }
    
    if (bIsOk) {
        return  (ERROR_NONE);
    }
    
    for (plineTempAiorc  = _G_aioqueue.aioq_plineidle;
         plineTempAiorc != LW_NULL;
         plineTempAiorc  = _list_line_get_next(plineTempAiorc)) {
         
        paiorc = _LIST_ENTRY(plineTempAiorc, AIO_REQ_CHAIN, aiorc_line);
        if (paiorc->aiorc_fildes == paiocb->aio_fildes) {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            for (plineTempAiocb  = paiorc->aiorc_plineaiocb;
                 plineTempAiocb != LW_NULL;
                 plineTempAiocb  = _list_line_get_next(plineTempAiocb)) {
                 
                paioreqTemp = _LIST_ENTRY(plineTempAiocb, struct aioreq, aioreq_line);
                paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
                
                if (paiocbTemp == paiocb) {
                    if (paiocbTemp->aio_pwait) {                        /*  不允许重复保存              */
                        API_SemaphoreMPost(paiorc->aiorc_mutex);
                        return  (PX_ERROR);
                    }
                    paiocbTemp->aio_pwait  = paiowait;
                    paiowait->aiowt_paiocb = paiocbTemp;
                    bIsOk = LW_TRUE;
                    break;
                }
            }
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            break;
        }
    }
    
    if (bIsOk) {
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __aioAttachWaitNoCheck
** 功能描述: 向指定的 aiocb 中连接 wait 信息, 不检查 aiocb 是否在队列中 (这里必须锁定 _G_aioqueue)
** 输　入  : paiocb            指定的 aiocb 
**           paiowait          wait 信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __aioAttachWaitNoCheck (struct aiocb  *paiocb, struct aiowait  *paiowait)
{
    paiocb->aio_pwait = paiowait;
    paiowait->aiowt_paiocb = paiocb;
}
/*********************************************************************************************************
** 函数名称: __aioDetachWait
** 功能描述: 将指定的 aiocb 中与 wait 信息分离 (这里必须锁定 _G_aioqueue)
** 输　入  : paiocb            指定的 aiocb   (可以为 NULL)
**           paiowait          wait 信息      (可以为 NULL , 但与 paiocb 不可同时为 NULL)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __aioDetachWait (struct aiocb  *paiocb, struct aiowait  *paiowait)
{
    if (paiocb) {
        paiowait = paiocb->aio_pwait;
        if (paiowait) {
            paiocb->aio_pwait      = LW_NULL;
            paiowait->aiowt_paiocb = LW_NULL;
        }
        return  (ERROR_NONE);
    }
    
    if (paiowait) {
        paiocb = paiowait->aiowt_paiocb;
        if (paiocb) {
            paiocb->aio_pwait      = LW_NULL;
            paiowait->aiowt_paiocb = LW_NULL;
        }
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __aioCreateWaitChain
** 功能描述: 创建一个等待队列
** 输　入  : iNum          队列中 wait 节点的个数
**           bIsNeedCond   是否创建 cond
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
AIO_WAIT_CHAIN  *__aioCreateWaitChain (int  iNum, BOOL  bIsNeedCond)
{
    AIO_WAIT_CHAIN  *paiowt;
    
    paiowt = (AIO_WAIT_CHAIN *)malloc(sizeof(AIO_WAIT_CHAIN));
    if (paiowt == LW_NULL) {
        return  (LW_NULL);
    }
    lib_bzero(paiowt, sizeof(AIO_WAIT_CHAIN));
    
    paiowt->aiowc_paiowait = (struct aiowait *)malloc((ULONG)(sizeof(struct aiowait) * iNum));
    if (paiowt->aiowc_paiowait == LW_NULL) {
        free(paiowt);
        return  (LW_NULL);
    }
    lib_bzero(paiowt->aiowc_paiowait, sizeof(struct aiowait) * iNum);
    
    if (bIsNeedCond) {
        if (API_ThreadCondInit(&paiowt->aiowc_cond, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
            free(paiowt->aiowc_paiowait);
            free(paiowt);
            return  (LW_NULL);
        }
    }

    return  (paiowt);
}
/*********************************************************************************************************
** 函数名称: __aioDeleteWaitChain
** 功能描述: 删除一个等待队列   (这里必须锁定 _G_aioqueue)
** 输　入  : paiowt        等待队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __aioDeleteWaitChain (AIO_WAIT_CHAIN  *paiowt)
{
    struct aiowait     *paiowait;
    PLW_LIST_LINE       plineTemp;

    for (plineTemp  = paiowt->aiowc_pline;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        paiowait = _LIST_ENTRY(plineTemp, struct aiowait, aiowt_line);
        __aioDetachWait(LW_NULL, paiowait);
    }

    if (paiowt->aiowc_paiowait->aiowt_pcond) {
        API_ThreadCondDestroy(&paiowt->aiowc_cond);
    }
    
    free(paiowt->aiowc_paiowait);
    free(paiowt);
}
/*********************************************************************************************************
** 函数名称: __aioDeleteWaitChainByAgent
** 功能描述: 代理线程删除一个等待队列, 这里不再需要 __aioDetachWait.  (这里必须锁定 _G_aioqueue)
** 输　入  : paiowt        等待队列控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __aioDeleteWaitChainByAgent (AIO_WAIT_CHAIN  *paiowt)
{
    if (paiowt->aiowc_paiowait->aiowt_pcond) {
        API_ThreadCondDestroy(&paiowt->aiowc_cond);
    }
    
    free(paiowt->aiowc_paiowait);
    free(paiowt);
}
/*********************************************************************************************************
** 函数名称: __aioWaitChainSetCnt
** 功能描述: 设置一个等待队列所有成员的计数器地址 (这里必须锁定 _G_aioqueue)
** 输　入  : paiowt        等待队列控制块
**           piCnt         计数器地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __aioWaitChainSetCnt (AIO_WAIT_CHAIN  *paiowt, INT  *piCnt)
{
    struct aiowait     *paiowait;
    PLW_LIST_LINE       plineTemp;
    
    for (plineTemp  = paiowt->aiowc_pline;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        paiowait = _LIST_ENTRY(plineTemp, struct aiowait, aiowt_line);
        paiowait->aiowt_pcnt = piCnt;
    }
}
/*********************************************************************************************************
** 函数名称: __aioThread
** 功能描述: aio 代理线程
** 输　入  : pvArg         参数
** 输　出  : 无实际意义
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __aioThread (PVOID  pvArg)
{
    AIO_REQ_CHAIN   *paiorc = (AIO_REQ_CHAIN *)pvArg;
    struct aioreq   *paioreq;
    struct aiocb    *paiocb;
    ULONG            ulError;

    for (;;) {
        API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
        
        if ((paiorc->aiorc_plineaiocb) &&
            (paiorc->aiorc_iscancel == 0)) {                            /*  paiorc 是否有待处理的节点   */
            
            paioreq = _LIST_ENTRY(paiorc->aiorc_plineaiocb, 
                                  struct aioreq, aioreq_line);
            paiocb  = _LIST_ENTRY(paioreq, struct aiocb, aio_req);
            
            API_ThreadSetPriority(API_ThreadIdSelf(), 
                                  paioreq->aioreq_prio);                /*  设置任务优先级与请求任务相同*/
            _List_Line_Del(paiorc->aiorc_plineaiocb,
                           &paiorc->aiorc_plineaiocb);                  /*  将处理节点从 paiorc 中删除  */
            
            paioreq->aioreq_flags |= AIO_REQ_BUSY;                      /*  准备开始操作文件            */
            
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            
            /*
             *  实际操作文件
             */
            errno = 0;                                                  /*  注意: 一定要将 errno 清零   */
            if (paiocb->aio_lio_opcode == LIO_SYNC) {
                paioreq->aioreq_return =  fsync(paiocb->aio_fildes);

            } else {
                switch (paiocb->aio_lio_opcode) {
                
                case LIO_READ:
                    paioreq->aioreq_return = pread(paiocb->aio_fildes,
                                                   (void *)paiocb->aio_buf,
                                                   paiocb->aio_nbytes,
                                                   paiocb->aio_offset);
                    break;
                
                case LIO_WRITE:
                    paioreq->aioreq_return = pwrite(paiocb->aio_fildes,
                                                    (const void *)paiocb->aio_buf,
                                                    paiocb->aio_nbytes,
                                                    paiocb->aio_offset);
                    break;
                    
                default:
                    paioreq->aioreq_return = PX_ERROR;
                    errno = EINVAL;
                }
            }
            paioreq->aioreq_error = errno;
            
            if (__issig(paiocb->aio_sigevent.sigev_signo) &&
                (paiocb->aio_sigevent.sigev_notify != SIGEV_NONE)) {
                _doSigEvent(paioreq->aioreq_thread, 
                            &paiocb->aio_sigevent, SI_ASYNCIO);         /*  notify                      */
            }
            
            /*
             *  处理等待信息, 
             */
            if (paiocb->aio_pwait) {                                    /*  需要处理 wait 信息          */
                API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
                if (paiocb->aio_pwait) {                                /*  需要再检查一遍              */
                    struct aiowait  *paiowait = paiocb->aio_pwait;
                    
                    __aioDetachWait(paiocb, LW_NULL);                   /*  取消 paiowait 与 paiocb 关联*/
                    
                    if (paiowait->aiowt_pcnt) {
                        (*paiowait->aiowt_pcnt)--;                      /*  计数器--                    */
                    }
                    
                    if (paiowait->aiowt_pcond) {
                        API_ThreadCondSignal(paiowait->aiowt_pcond);    /*  这里不需要回收 wait         */
                    
                    } else {
                        if ((paiowait->aiowt_pcnt) &&
                            (*paiowait->aiowt_pcnt == 0)) {             /*  需要回收 wait 队列          */
                            
                            AIO_WAIT_CHAIN *paiowt = (AIO_WAIT_CHAIN *)paiowait->aiowt_paiowc;
                            
                            if (paiowait->aiowt_psigevent &&
                                __issig(paiowait->aiowt_psigevent->sigev_signo)) {
                                _doSigEvent(paioreq->aioreq_thread, 
                                            paiowait->aiowt_psigevent, SI_ASYNCIO);
                                                                        /*  notify                      */
                            }
                            
                            if (paiowt) {
                                __aioDeleteWaitChainByAgent(paiowt);    /*  释放 wait 队列              */
                            }
                        }
                    }
                }
                API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            }
            
            paioreq->aioreq_flags &= ~AIO_REQ_BUSY;                     /*  清除忙标志                  */
        
        } else if (paiorc->aiorc_iscancel) {                            /*  被 cancel 了                */
        
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            
            /*
             *  paiorc 整体被 cancel 了, 这里需要删除 paiorc.
             */
            API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
            if (paiorc->aiorc_isworkq) {
                __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plinework);
            } else {
                __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plineidle);
            }
            goto    __check_idle;                                       /*  检查 idle 队列有没有 aiorc  */
        
        } else {                                                        /*  此文件没有需要处理的请求    */
            if (API_ThreadCondWait(&paiorc->aiorc_cond, 
                                   paiorc->aiorc_mutex,
                                   __PX_AIO_TIMEOUT) == ERROR_NONE) {   /*  再等待一段时间              */
                API_SemaphoreMPost(paiorc->aiorc_mutex);
                continue;                                               /*  继续操作这个文件描述符      */

            } else {
                API_SemaphoreMPost(paiorc->aiorc_mutex);
            }
            
            API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
            if ((paiorc->aiorc_plineaiocb == LW_NULL) ||
                (paiorc->aiorc_iscancel)) {                             /*  需要删除 paiorc             */
                
                if (paiorc->aiorc_isworkq) {
                    __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plinework);
                } else {
                    __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plineidle);
                }
                
__check_idle:
                if (_G_aioqueue.aioq_plineidle == LW_NULL) {            /*  aioq_plineidle 需要执行     */
                    
                    _G_aioqueue.aioq_idlethread++;                      /*  进入 idle 模式              */
                    ulError = API_ThreadCondWait(&_G_aioqueue.aioq_newreq,
                                                 _G_aioqueue.aioq_mutex,
                                                 __PX_AIO_TIMEOUT);
                    _G_aioqueue.aioq_idlethread--;
                    
                    if ((ulError != ERROR_NONE) || 
                        (_G_aioqueue.aioq_plineidle == LW_NULL)) {      /*  没有需要执行的 I/O 操作     */
                        _G_aioqueue.aioq_actthread--;
                        API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                        return  (LW_NULL);
                    }
                        
                    paiorc = _LIST_ENTRY(_G_aioqueue.aioq_plineidle, 
                                         AIO_REQ_CHAIN, 
                                         aiorc_line);                   /*  获取新的 paiorc             */
                    
                    _List_Line_Del(&paiorc->aiorc_line, 
                                   &_G_aioqueue.aioq_plineidle);        /*  从 idle 中删除              */
                    
                    paiorc->aiorc_isworkq = 1;                          /*  整体加入 aioq_plinework     */
                    _List_Line_Add_Ahead(&paiorc->aiorc_line, 
                                         &_G_aioqueue.aioq_plinework);
                }
            }
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __aioEnqueue
** 功能描述: 将 aio 一个请求节点打入执行队列 (这里必须锁定 _G_aioqueue)
** 输　入  : paiocb            aio 请求参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  __aioEnqueue (struct aiocb *paiocb)
{
    LW_CLASS_THREADATTR     threadattr;
    AIO_REQ_CHAIN          *paiorc;
    ULONG                   ulError;
    
    API_ThreadGetPriority(API_ThreadIdSelf(), &paiocb->aio_req.aioreq_prio);
    paiocb->aio_req.aioreq_flags  = 0;
    paiocb->aio_req.aioreq_error  = EINPROGRESS;
    paiocb->aio_req.aioreq_return = 0;
    
    if ((_G_aioqueue.aioq_idlethread == 0) && 
        (_G_aioqueue.aioq_actthread < LW_CFG_POSIX_AIO_MAX_THREAD)) {   /*  没有空闲任务, 且能创建新任务*/
        
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               paiocb->aio_fildes);                     /*  搜索工作队列中有没有相同的  */
        if (paiorc == LW_NULL) {
            paiorc =  __aioCreateFd(&_G_aioqueue.aioq_plinework, 
                                    paiocb->aio_fildes);                /*  创建节点                    */
            if (paiorc == LW_NULL) {
                paiocb->aio_req.aioreq_error  = EAGAIN;
                paiocb->aio_req.aioreq_return = PX_ERROR;
                errno = ENOMEM;                                         /*  系统缺少内存退出            */
                return  (PX_ERROR);
            }
            
            paiorc->aiorc_isworkq = 1;                                  /*  在 aioq_plinework 中        */
            __aioInsertAiocb(paiorc, paiocb);                           /*  插入文件描述符队列          */
            
            threadattr = _G_threadattrAio;
            threadattr.THREADATTR_ucPriority = paiocb->aio_req.aioreq_prio;
            threadattr.THREADATTR_pvArg      = (PVOID)paiorc;
            
            ulError = API_ThreadCreate("t_aio", __aioThread, &threadattr, LW_NULL);
            if (ulError == LW_OBJECT_HANDLE_INVALID) {
                return  (PX_ERROR);                                     /*  系统无法创建新的线程        */
            }
            _G_aioqueue.aioq_actthread++;
        
        } else {                                                        /*  已经存在相应文件描述符队列  */
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            __aioInsertAiocb(paiorc, paiocb);                           /*  插入文件描述符队列          */
            API_ThreadCondSignal(&paiorc->aiorc_cond);                  /*  通知线程                    */
            API_SemaphoreMPost(paiorc->aiorc_mutex);
        }
    
    } else {                                                            /*  存在空闲任务或线程已满      */
        
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               paiocb->aio_fildes);                     /*  搜索工作队列中有没有相同的  */
        if (paiorc == LW_NULL) {
            paiorc = __aioSearchFd(_G_aioqueue.aioq_plineidle,
                                   paiocb->aio_fildes);                 /*  从 idlethread 中查找        */
            if (paiorc == LW_NULL) {
                paiorc =  __aioCreateFd(&_G_aioqueue.aioq_plineidle,
                                        paiocb->aio_fildes);            /*  在 idlethread 创建          */
                if (paiorc == LW_NULL) {
                    paiocb->aio_req.aioreq_error  = EAGAIN;
                    paiocb->aio_req.aioreq_return = PX_ERROR;
                    errno = ENOMEM;                                     /*  系统缺少内存退出            */
                    return  (PX_ERROR);
                }
                
                paiorc->aiorc_isworkq = 0;                              /*  没有在 aioq_plinework 中    */
                __aioInsertAiocb(paiorc, paiocb);                       /*  插入文件描述符队列          */
                
                API_ThreadCondSignal(&_G_aioqueue.aioq_newreq);         /*  通知 idle 的线程去执行      */
            
            } else {                                                    /*  idlethread 中有对应的 fd    */
                API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
                __aioInsertAiocb(paiorc, paiocb);                       /*  插入文件描述符队列          */
                API_SemaphoreMPost(paiorc->aiorc_mutex);
            }
        
        } else {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            __aioInsertAiocb(paiorc, paiocb);                           /*  插入文件描述符队列          */
            API_ThreadCondSignal(&paiorc->aiorc_cond);                  /*  通知线程                    */
            API_SemaphoreMPost(paiorc->aiorc_mutex);
        }
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
