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
** 文   件   名: selectLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 11 月 07 日
**
** 描        述:  IO 系统 select 子系统 API.

** BUG
2007.11.11  当设置节点出现错误时. 一定要退出.
2007.11.20  将 pselctx->SELCTX_bPendedOnSelect = LW_TRUE; 提前, 以使 FIOSELECT 进行到中途线程被删除时
            可以使 delete hook 发挥作用, 将产生的 WAKE NODE 删除.
2009.07.17  保存最大文件号操作应该提前, 保证 delete hook 能够正常的删除产生的节点.
2009.09.03  检查全空文件集, 修正注释.
2009.11.25  select() 在判断 ERROR_IO_UNKNOWN_REQUEST 时也要判断 ENOSYS. 这是两个等价的错误类型.
2011.03.11  确保超时可以返回对应的 errno 编号.
2011.08.15  重大决定: 将所有 posix 定义的函数以函数方式(非宏)引出.
2013.03.15  加入 pselect api.
2017.08.18  等待时间更加精确.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
INT             __selDoIoctls(fd_set               *pfdset,
                              INT                   iFdSetWidth,
                              INT                   iFunc,
                              PLW_SEL_WAKEUPNODE    pselwunNode,
                              BOOL                  bStopOnErr);
/*********************************************************************************************************
** 函数名称: __selIsAllFdsetEmpty
** 功能描述: 检测是否所有的文件集都为空 (2010.01.21 此函数无用, 很多软件使用 select 进行延迟...)
** 输　入  : iWidthInBytes     文件集中的最大文件号的偏移量占了多少字节, 例如: fd = 10, 就占了 2 个 Byte
**           pfdsetRead        用户关心的可读文件集.
**           pfdsetWrite       用户关心的可写文件集.
**           pfdsetExcept      用户关心的异常文件集.
** 输　出  : LW_TRUE  为空
**           LW_FALSE 不为空
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __selIsAllFdsetEmpty (INT               iWidthInBytes,
                                   fd_set           *pfdsetRead,
                                   fd_set           *pfdsetWrite,
                                   fd_set           *pfdsetExcept)
{
    (VOID)iWidthInBytes;
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __selContextOrigFdsetInit
** 功能描述: 初始化 select context 中的原始文件集
** 输　入  : iWidthInBytes     文件集中的最大文件号的偏移量占了多少字节, 例如: fd = 10, 就占了 2 个 Byte
**           pfdsetRead        用户关心的可读文件集.
**           pfdsetWrite       用户关心的可写文件集.
**           pfdsetExcept      用户关心的异常文件集.
**           pselctx           select context
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __selFdsetInit (INT               iWidthInBytes,
                      fd_set           *pfdsetRead,
                      fd_set           *pfdsetWrite,
                      fd_set           *pfdsetExcept,
                      LW_SEL_CONTEXT   *pselctx)
{
    /*
     *  不同的 SylixOS 版本, FD_SETSIZE 可能不同, 所以 fd_set 大小可能不同, 
     *  所以这里只能操作 iWidthInBytes 位以内
     */
    if (pfdsetRead) {                                                   /*  拷贝等待读的文件集          */
        lib_bcopy((CPVOID)pfdsetRead, (PVOID)&pselctx->SELCTX_fdsetOrigReadFds,
                  iWidthInBytes);
    } else {
        lib_bzero((PVOID)&pselctx->SELCTX_fdsetOrigReadFds, iWidthInBytes);
    }

    if (pfdsetWrite) {                                                  /*  拷贝等待写的文件集          */
        lib_bcopy((CPVOID)pfdsetWrite, (PVOID)&pselctx->SELCTX_fdsetOrigWriteFds,
                  iWidthInBytes);
    } else {
        lib_bzero((PVOID)&pselctx->SELCTX_fdsetOrigWriteFds, iWidthInBytes);
    }

    if (pfdsetExcept) {                                                 /*  拷贝等待异常的文件集        */
        lib_bcopy((CPVOID)pfdsetExcept, (PVOID)&pselctx->SELCTX_fdsetOrigExceptFds,
                  iWidthInBytes);
    } else {
        lib_bzero((PVOID)&pselctx->SELCTX_fdsetOrigExceptFds, iWidthInBytes);
    }
}
/*********************************************************************************************************
** 函数名称: __selFdsetClear
** 功能描述: 清除文件集中的所有位
** 输　入  : iWidthInBytes     文件集中的最大文件号的偏移量占了多少字节, 例如: fd = 10, 就占了 2 个 Byte
**           pfdsetRead        用户关心的可读文件集.
**           pfdsetWrite       用户关心的可写文件集.
**           pfdsetExcept      用户关心的异常文件集.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __selFdsetClear (INT               iWidthInBytes,
                              fd_set           *pfdsetRead,
                              fd_set           *pfdsetWrite,
                              fd_set           *pfdsetExcept)
{
    /*
     *  不同的 SylixOS 版本, FD_SETSIZE 可能不同, 所以 fd_set 大小可能不同, 
     *  所以这里只能操作 iWidthInBytes 位以内
     */
    if (pfdsetRead) {                                                   /*  清空当前等待的读文件集      */
        lib_bzero((PVOID)pfdsetRead, iWidthInBytes);
    }

    if (pfdsetWrite) {                                                  /*  清空当前等待的写文件集      */
        lib_bzero((PVOID)pfdsetWrite, iWidthInBytes);
    }

    if (pfdsetExcept) {                                                 /*  清空当前等待的异常文件集    */
        lib_bzero((PVOID)pfdsetExcept, iWidthInBytes);
    }
}
/*********************************************************************************************************
** 函数名称: __selFdsetGetFileConter
** 功能描述: 获得指定文件集中文件的数量
** 输　入  : iWidth            文件集中,最大的文件号.
**           pfdsetRead        用户关心的可读文件集.
**           pfdsetWrite       用户关心的可写文件集.
**           pfdsetExcept      用户关心的异常文件集.
** 输　出  : 文件集中文件的数量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT   __selFdsetGetFileConter (INT               iWidth, 
                                      fd_set           *pfdsetRead,
                                      fd_set           *pfdsetWrite,
                                      fd_set           *pfdsetExcept)
{
    REGISTER INT    iFd;                                                /*  临时文件描述符              */
    REGISTER ULONG  ulPartMask;                                         /*  区域掩码                    */
    REGISTER INT    iFoundCounter = 0;
    
    /*
     *  不同的 SylixOS 版本, FD_SETSIZE 可能不同, 所以 fd_set 大小可能不同, 
     *  所以这里只能操作 iWidthInBytes 位以内
     */
    if (pfdsetRead) {                                                   /*  获得可读文件数量            */
        for (iFd = 0; iFd < iWidth; iFd++) {
            ulPartMask = pfdsetRead->fds_bits[((unsigned)iFd) / NFDBITS];
            if (ulPartMask == 0) {
                iFd += NFDBITS - 1;
            } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFd) % NFDBITS))) {
                iFoundCounter++;
            }
        }
    }
    
    if (pfdsetWrite) {                                                  /*  获得可写文件数量            */
        for (iFd = 0; iFd < iWidth; iFd++) {
            ulPartMask = pfdsetWrite->fds_bits[((unsigned)iFd) / NFDBITS];
            if (ulPartMask == 0) {
                iFd += NFDBITS - 1;
            } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFd) % NFDBITS))) {
                iFoundCounter++;
            }
        }
    }
    
    if (pfdsetExcept) {                                                 /*  获得异常文件数量            */
        for (iFd = 0; iFd < iWidth; iFd++) {
            ulPartMask = pfdsetExcept->fds_bits[((unsigned)iFd) / NFDBITS];
            if (ulPartMask == 0) {
                iFd += NFDBITS - 1;
            } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFd) % NFDBITS))) {
                iFoundCounter++;
            }
        }
    }
    
    return  (iFoundCounter);
}
/*********************************************************************************************************
** 函数名称: pselect
** 功能描述: BSD pselect() 多路 I/O 复用.
** 输　入  : iWidth            设置的文件集中,最大的文件号 + 1.
**           pfdsetRead        用户关心的可读文件集.
**           pfdsetWrite       用户关心的可写文件集.
**           pfdsetExcept      用户关心的异常文件集.
**           ptmspecTO         等待超时时间, LW_NULL 表示永远等待.
**           sigsetMask        等待时阻塞的信号
** 输　出  : 正常返回等待到的文件数量 ,错误返回 PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       驱动程序不支持
**           errno == ERROR_IO_SELECT_CONTEXT                   线程不存在 context
**           errno == ERROR_IO_SELECT_WIDTH                     最大文件号错误
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 等待超时
**           errno == ERROR_KERNEL_IN_ISR                       在中断中调用
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT     pselect (INT                     iWidth, 
                 fd_set                 *pfdsetRead,
                 fd_set                 *pfdsetWrite,
                 fd_set                 *pfdsetExcept,
                 const struct timespec  *ptmspecTO,
                 const sigset_t         *sigsetMask)
{
    REGISTER INT                 iIsOk = ERROR_NONE;                    /*  初始化为没有错误            */
    REGISTER INT                 iWidthInBytes;                         /*  需要检测的位数占多少字节    */
    REGISTER ULONG               ulWaitTime;                            /*  等待时间                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             
#if LW_CFG_SIGNAL_EN > 0
             sigset_t            sigsetMaskOld;
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
             LW_SEL_WAKEUPNODE   selwunNode;                            /*  生成的 NODE 模板            */
             ULONG               ulError;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  不能在中断中调用            */
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  没有 select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }
    
    if ((iWidth < 0) || (iWidth > FD_SETSIZE)) {                        /*  iWidth 参数错误             */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iWidth out of range.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_WIDTH);
        return  (PX_ERROR);
    }
    
    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  需要检测的位数占多少字节    */
    
    if (__selIsAllFdsetEmpty(iWidthInBytes, pfdsetRead, pfdsetWrite, 
                             pfdsetExcept)) {                           /*  检测输入文件集是否有效      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fdset is empty.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_FDSET_NULL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    __selFdsetInit(iWidthInBytes, pfdsetRead, pfdsetWrite, 
                   pfdsetExcept, pselctx);                              /*  初始化 select context       */
                                                                        /*  中的原始文件集              */
    __selFdsetClear(iWidthInBytes, pfdsetRead, pfdsetWrite, 
                    pfdsetExcept);                                      /*  清空参数文件集              */
                                                                        
    if (!ptmspecTO) {                                                   /*  计算等待时间                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  无限等待                    */

    } else {
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, ptmspecTO);
    }
    
    pselctx->SELCTX_pfdsetReadFds   = pfdsetRead;
    pselctx->SELCTX_pfdsetWriteFds  = pfdsetWrite;                      /*  保存用户参数地址            */
    pselctx->SELCTX_pfdsetExceptFds = pfdsetExcept;
    
    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  清除信号量                  */
    
    selwunNode.SELWUN_hThreadId = API_ThreadIdSelf();
    
    pselctx->SELCTX_iWidth = iWidth;                                    /*  保存最大文件号              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  需要 delete hook 清除 NODE  */
    
    if (pfdsetRead) {                                                   /*  检查读文件集                */
        selwunNode.SELWUN_seltypType = SELREAD;
        iIsOk = __selDoIoctls(&pselctx->SELCTX_fdsetOrigReadFds, 
                              iWidth, FIOSELECT, &selwunNode,
                              LW_TRUE);                                 /*  遇到错误,立即退出           */
        if (iIsOk != ERROR_NONE) {
            iIsOk  = PX_ERROR;
        }
    }
    
    if (iIsOk != PX_ERROR && pfdsetWrite) {                             /*  检查写文件集                */
        selwunNode.SELWUN_seltypType = SELWRITE;
        iIsOk = __selDoIoctls(&pselctx->SELCTX_fdsetOrigWriteFds, 
                              iWidth, FIOSELECT, &selwunNode,
                              LW_TRUE);                                 /*  遇到错误,立即退出           */
        if (iIsOk != ERROR_NONE) {
            iIsOk  = PX_ERROR;
        }
    }
    
    if (iIsOk != PX_ERROR && pfdsetExcept) {                            /*  检查异常文件集              */
        selwunNode.SELWUN_seltypType = SELEXCEPT;
        iIsOk = __selDoIoctls(&pselctx->SELCTX_fdsetOrigExceptFds, 
                              iWidth, FIOSELECT, &selwunNode,
                              LW_TRUE);                                 /*  遇到错误,立即退出           */
        if (iIsOk != ERROR_NONE) {
            iIsOk  = PX_ERROR;
        }
    }
    
    if (iIsOk != ERROR_NONE) {                                          /*  出现了错误                  */
        ulError = API_GetLastError();
        
        selwunNode.SELWUN_seltypType = SELREAD;
        __selDoIoctls(&pselctx->SELCTX_fdsetOrigReadFds, 
                      iWidth, FIOUNSELECT, &selwunNode,
                      LW_FALSE);                                        /*  释放节点                    */
                      
        selwunNode.SELWUN_seltypType = SELWRITE;
        __selDoIoctls(&pselctx->SELCTX_fdsetOrigWriteFds, 
                      iWidth, FIOUNSELECT, &selwunNode,
                      LW_FALSE);                                        /*  释放节点                    */
                      
        selwunNode.SELWUN_seltypType = SELEXCEPT;
        __selDoIoctls(&pselctx->SELCTX_fdsetOrigExceptFds, 
                      iWidth, FIOUNSELECT, &selwunNode,
                      LW_FALSE);                                        /*  释放节点                    */
        
        if (ulError == ENOSYS) {
            _ErrorHandle(ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER);          /*  驱动程序不支持              */
        }
        
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  自行清除 NODE 完成          */
        
        return  (PX_ERROR);
    }
    
#if LW_CFG_SIGNAL_EN > 0
    if (sigsetMask) {
        sigprocmask(SIG_SETMASK, sigsetMask, &sigsetMaskOld);
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    
    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  开始等待                    */
    
#if LW_CFG_SIGNAL_EN > 0
    if (sigsetMask) {
        sigprocmask(SIG_SETMASK, &sigsetMaskOld, LW_NULL);
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    
    if (pfdsetRead) {                                                   /*  检查读文件集                */
        REGISTER INT    iTemp;
        selwunNode.SELWUN_seltypType = SELREAD;
        iTemp = __selDoIoctls(&pselctx->SELCTX_fdsetOrigReadFds, 
                              iWidth, FIOUNSELECT, &selwunNode,
                              LW_FALSE);                                /*  如果存在节点,删除节点       */
        if (iTemp != ERROR_NONE) {
            iIsOk  = PX_ERROR;
        }
    }

    if (pfdsetWrite) {                                                  /*  检查写文件集                */                
        REGISTER INT    iTemp;
        selwunNode.SELWUN_seltypType = SELWRITE;
        iTemp = __selDoIoctls(&pselctx->SELCTX_fdsetOrigWriteFds, 
                              iWidth, FIOUNSELECT, &selwunNode,
                              LW_FALSE);                                /*  如果存在节点,删除节点       */
        if (iTemp != ERROR_NONE) {
            iIsOk  = PX_ERROR;
        }
    }
    
    if (pfdsetExcept) {                                                 /*  检查异常文件集              */
        REGISTER INT    iTemp;
        selwunNode.SELWUN_seltypType = SELEXCEPT;
        iTemp = __selDoIoctls(&pselctx->SELCTX_fdsetOrigExceptFds, 
                              iWidth, FIOUNSELECT, &selwunNode,
                              LW_FALSE);                                /*  遇到错误,立即退出           */
        if (iTemp != ERROR_NONE) {
            iIsOk  = PX_ERROR;
        }
    }
    
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;
    
    if (iIsOk == PX_ERROR) {                                            /*  出现错误                    */
        return  (PX_ERROR);
    
    } else {
        _ErrorHandle(ulError);
    }
    
    return  (__selFdsetGetFileConter(iWidth, pfdsetRead, pfdsetWrite, pfdsetExcept));
}
/*********************************************************************************************************
** 函数名称: select
** 功能描述: BSD select() 多路 I/O 复用.
** 输　入  : iWidth            设置的文件集中,最大的文件号 + 1.
**           pfdsetRead        用户关心的可读文件集.
**           pfdsetWrite       用户关心的可写文件集.
**           pfdsetExcept      用户关心的异常文件集.
**           ptmvalTO          等待超时时间, LW_NULL 表示永远等待.
** 输　出  : 正常返回等待到的文件数量 ,错误返回 PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       驱动程序不支持
**           errno == ERROR_IO_SELECT_CONTEXT                   线程不存在 context
**           errno == ERROR_IO_SELECT_WIDTH                     最大文件号错误
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 等待超时
**           errno == ERROR_KERNEL_IN_ISR                       在中断中调用
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT     select (INT               iWidth, 
                fd_set           *pfdsetRead,
                fd_set           *pfdsetWrite,
                fd_set           *pfdsetExcept,
                struct timeval   *ptmvalTO)
{
    struct timespec   tmspec;
    struct timespec  *ptmspec;
           INT        iRet;

    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  不能在中断中调用            */
        return  (PX_ERROR);
    }
    
    if (ptmvalTO) {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tmspec);
        ptmspec = &tmspec;

    } else {
        ptmspec = LW_NULL;
    }
    
    iRet = pselect(iWidth, pfdsetRead, pfdsetWrite, pfdsetExcept, ptmspec, LW_NULL);
    
    /*
     *  当前暂时没有修改 ptmvalTO 的值.
     */
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
