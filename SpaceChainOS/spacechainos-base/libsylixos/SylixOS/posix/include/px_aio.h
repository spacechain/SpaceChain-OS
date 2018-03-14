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
** 文   件   名: px_aio.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 26 日
**
** 描        述: posix 异步I/O兼容库.
*********************************************************************************************************/

#ifndef __PX_AIO_H
#define __PX_AIO_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  aiowait (user do not use this struct)
*********************************************************************************************************/

struct aiocb;
struct aiowait {
    LW_LIST_LINE            aiowt_line;                                 /*  wait line                   */
    void                   *aiowt_paiowc;                               /*  wait chain control block    */
    PLW_THREAD_COND         aiowt_pcond;                                /*  wait pcond                  */
    int                    *aiowt_pcnt;                                 /*  wait complete counter       */
    struct sigevent        *aiowt_psigevent;                            /*  wait sigevent               */
    struct aiocb           *aiowt_paiocb;                               /*  wait aiocb                  */
};

/*********************************************************************************************************
  aioreq (user do not use this struct)
*********************************************************************************************************/

struct aioreq {
    LW_LIST_LINE            aioreq_line;                                /*  req list                    */
    LW_OBJECT_HANDLE        aioreq_thread;                              /*  req thread                  */
    uint8_t                 aioreq_prio;                                /*  req thread prio             */
    uint8_t                 aioreq_flags;                               /*  req flags                   */
    int                     aioreq_error;                               /*  error code                  */
    ssize_t                 aioreq_return;                              /*  return value                */
};

/*********************************************************************************************************
  aiocb
*********************************************************************************************************/

struct aiocb {
    int                     aio_fildes;                                 /*  file descriptor             */
    off_t                   aio_offset;                                 /*  file offset                 */
    volatile void          *aio_buf;                                    /*  Location of buffer.         */
    size_t                  aio_nbytes;                                 /*  Length of transfer.         */
    int                     aio_reqprio;                                /*  Request priority offset.    */
    struct sigevent         aio_sigevent;                               /*  Signal number and value.    */
    int                     aio_lio_opcode;                             /*  Operation to be performed.  */
    
    struct aioreq           aio_req;                                    /*  sylixos extern (do not use) */
    struct aiowait         *aio_pwait;                                  /*  sylixos extern (do not use) */
};

/*********************************************************************************************************
  aio_cancel() return values
*********************************************************************************************************/
/*********************************************************************************************************
  all requested operations have been canceled
*********************************************************************************************************/
#define AIO_CANCELED        0
/*********************************************************************************************************
  some of the operations could not be canceled since they are in progress
*********************************************************************************************************/
#define AIO_NOTCANCELED     1
/*********************************************************************************************************
  none of the requested operations could be canceled since they are already complete
*********************************************************************************************************/
#define AIO_ALLDONE         2

/*********************************************************************************************************
  lio_listio() mode
*********************************************************************************************************/
/*********************************************************************************************************
  calling process is to suspend until the operation is complete
*********************************************************************************************************/
#define LIO_WAIT            0 
/*********************************************************************************************************
  calling process is to continue execution while the operation is performed and no notification shall be
  given when the operation is completed
*********************************************************************************************************/
#define LIO_NOWAIT          1 

/*********************************************************************************************************
  lio_listio() opcode
*********************************************************************************************************/
#define LIO_NOP             0                                           /*  no transfer is requested    */
#define LIO_READ            1                                           /*  request a read()            */
#define LIO_WRITE           2                                           /*  request a write()           */
#define LIO_SYNC            3                                           /*  needed by aio_fsync()       */

/*********************************************************************************************************
  aio api (注意: POSIX 定义的有些函数参数不是 const 类型, 证明函数内有些成员会被改动)
*********************************************************************************************************/

LW_API int          aio_cancel(int fildes, struct aiocb *paiocb);
LW_API int          aio_error(const struct aiocb *paiocb);
LW_API int          aio_fsync(int op, struct aiocb *paiocb);
LW_API int          aio_read(struct aiocb *paiocb);
LW_API int          aio_write(struct aiocb *paiocb);
LW_API ssize_t      aio_return(struct aiocb *paiocb);
LW_API int          aio_suspend(const struct aiocb * const list[], int nent,
                                const struct timespec *timeout);
LW_API int          lio_listio(int mode, struct aiocb * const list[], int nent,
                               struct sigevent *sig);

/*********************************************************************************************************
  aio api extern
*********************************************************************************************************/

LW_API int          aio_setstacksize(size_t  newsize);
LW_API size_t       aio_getstacksize(void);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_AIO_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
