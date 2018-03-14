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
** 文   件   名: aio_lib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 26 日
**
** 描        述: posix 异步I/O内部头文件.
*********************************************************************************************************/

#ifndef __AIO_LIB_H
#define __AIO_LIB_H

/*********************************************************************************************************
  aio module
  
    +-------------+
    |  aio_queue  |
    +-------------+
           |
           | aioq_plinework
           |
    +-------------+
    |aio_req_chain|  aiorc_plineaiocb   +-------+       +-------+
    |   eg.fd=1   |-------------------->| aiocb |------>| aiocb |------>....
    +-------------+                     |  fd=1 |       |  fd=1 |
           |                            +-------+       +-------+
           |
           |
    +-------------+
    |aio_req_chain|  aiorc_plineaiocb   +-------+       +-------+
    |   eg.fd=2   |-------------------->| aiocb |------>| aiocb |------>....
    +-------------+                     |  fd=2 |       |  fd=2 |
           |                            +-------+       +-------+
           |
           |
    +-------------+
    |aio_req_chain|  aiorc_plineaiocb   +-------+       
    |   eg.fd=3   |-------------------->| aiocb |------> NULL
    +-------------+                     |  fd=3 |
           |                            +-------+
           |
           |
    +-------------+
    |aio_req_chain|  aiorc_plineaiocb   +-------+       
    |   eg.fd=4   |-------------------->| aiocb |------> NULL
    +-------------+                     |  fd=4 |
           |                            +-------+
           |
           |
          
          ...
          
*********************************************************************************************************/
/*********************************************************************************************************
  aioreq_flags
*********************************************************************************************************/

#define AIO_REQ_BUSY    1
#define AIO_REQ_FREE    2

/*********************************************************************************************************
  aio_queue
*********************************************************************************************************/

typedef struct aio_queue {
    LW_OBJECT_HANDLE            aioq_mutex;                             /*  mutex                       */
    LW_THREAD_COND              aioq_newreq;                            /*  cond newreq                 */
    
    LW_LIST_LINE_HEADER         aioq_plinework;                         /*  work queue                  */
    LW_LIST_LINE_HEADER         aioq_plineidle;                         /*  idle queue                  */
    
    int                         aioq_idlethread;                        /*  idle thread number          */
    int                         aioq_actthread;                         /*  active thread number        */
} AIO_QUEUE;

/*********************************************************************************************************
  aio_req_chain
*********************************************************************************************************/

typedef struct aio_req_chain {
    LW_LIST_LINE                aiorc_line;                             /*  aio req chain list          */
    LW_LIST_LINE_HEADER         aiorc_plineaiocb;                       /*  aiocb list header           */
    
    int                         aiorc_fildes;                           /*  file descriptor             */
    int                         aiorc_isworkq;                          /*  is in work queue            */
    int                         aiorc_iscancel;                         /*  is need cancel              */
    
    LW_OBJECT_HANDLE            aiorc_mutex;                            /*  mutex                       */
    LW_THREAD_COND              aiorc_cond;                             /*  cond                        */
} AIO_REQ_CHAIN;

/*********************************************************************************************************
  aio_wait_chain
*********************************************************************************************************/

typedef struct aio_wait_chain {
    LW_LIST_LINE_HEADER         aiowc_pline;                            /*  wait list header            */
    LW_THREAD_COND              aiowc_cond;                             /*  wait thread cond            */
    int                         aiowc_icnt;                             /*  wait counter                */
    struct aiowait             *aiowc_paiowait;                         /*  wait list buffer            */
    struct sigevent             aiowc_sigevent;                         /*  lio_listio sigevent         */
} AIO_WAIT_CHAIN;

/*********************************************************************************************************
  global var
*********************************************************************************************************/

extern AIO_QUEUE                _G_aioqueue;                            /*  aio queue                   */

/*********************************************************************************************************
  internal function
*********************************************************************************************************/

int             __aioEnqueue(struct aiocb *paiocb);

AIO_REQ_CHAIN  *__aioSearchFd(LW_LIST_LINE_HEADER  plineHeader, int  iFd);
VOID            __aioCancelFd(AIO_REQ_CHAIN  *paiorc);

INT             __aioRemoveAiocb(AIO_REQ_CHAIN  *paiorc, struct aiocb *paiocb, INT  iError, INT  iErrNo);

INT             __aioAttachWait(const struct aiocb  *paiocb, struct aiowait  *paiowait);
VOID            __aioAttachWaitNoCheck(struct aiocb  *paiocb, struct aiowait  *paiowait);
INT             __aioDetachWait(struct aiocb  *paiocb, struct aiowait  *paiowait);

AIO_WAIT_CHAIN *__aioCreateWaitChain(int  iNum, BOOL  bIsNeedCond);
VOID            __aioDeleteWaitChain(AIO_WAIT_CHAIN  *paiowt);
VOID            __aioWaitChainSetCnt(AIO_WAIT_CHAIN  *paiowt, INT  *piCnt);

#endif                                                                  /*  __AIO_LIB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
