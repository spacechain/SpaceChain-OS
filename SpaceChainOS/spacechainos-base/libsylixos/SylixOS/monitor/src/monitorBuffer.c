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
** 文   件   名: monitorBuffer.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器缓冲区管理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0
/*********************************************************************************************************
  事件缓冲区宏定义
*********************************************************************************************************/
#define MONITOR_BUFFER_MIN_SIZE             (8 * 1024)                  /*  最少大小                    */
#define MONITOR_BUFFER_UPDATE_SIZE          (4 * 1024)                  /*  空闲空间小于此将上传        */
/*********************************************************************************************************
  事件缓冲区结构
*********************************************************************************************************/
typedef struct {
    PUCHAR              MB_pucBuffer;                                   /*  缓冲区首地址                */
    size_t              MB_stSize;                                      /*  缓冲区大小                  */
    size_t              MB_stLeft;                                      /*  剩余空间大小                */
    
    PUCHAR              MB_pucPut;                                      /*  写入指针                    */
    PUCHAR              MB_pucGet;                                      /*  读出指针                    */
    
    LW_SPINLOCK_DEFINE (MB_slLock);                                     /*  spinlock                    */
    
    LW_OBJECT_HANDLE    MB_hReadSync;                                   /*  读同步                      */
} MONITOR_BUFFER;
typedef MONITOR_BUFFER *PMONITOR_BUFFER;
/*********************************************************************************************************
** 函数名称: __monitorBufferCreate
** 功能描述: 创建一个监控跟踪节点事件缓冲区
** 输　入  : stSize            缓冲区大小
** 输　出  : 事件缓冲区句柄
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  __monitorBufferCreate (size_t  stSize)
{
    PMONITOR_BUFFER  pmb;
    
    if (stSize < MONITOR_BUFFER_MIN_SIZE) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    pmb = (PMONITOR_BUFFER)__SHEAP_ALLOC(sizeof(MONITOR_BUFFER) + stSize);
    if (!pmb) {
        _ErrorHandle(ERROR_MONITOR_ENOMEM);
        return  (LW_NULL);
    }
    
    pmb->MB_hReadSync = API_SemaphoreBCreate("mb_sync", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pmb->MB_hReadSync == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pmb);
        return  (LW_NULL);
    }
    
    pmb->MB_pucBuffer = (PUCHAR)pmb + sizeof(MONITOR_BUFFER);
    pmb->MB_stSize    = stSize;
    pmb->MB_stLeft    = stSize;
    
    pmb->MB_pucPut    = pmb->MB_pucBuffer;
    pmb->MB_pucGet    = pmb->MB_pucBuffer;
    
    LW_SPIN_INIT(&pmb->MB_slLock);
    
    return  ((PVOID)pmb);
}
/*********************************************************************************************************
** 函数名称: __monitorBufferDelete
** 功能描述: 删除一个监控跟踪节点事件缓冲区
** 输　入  : pvMonitorBuffer   事件缓冲区
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __monitorBufferDelete (PVOID  pvMonitorBuffer)
{
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;

    if (!pmb) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_KERNEL_JOB_DEL(2, (VOIDFUNCPTR)API_SemaphoreBPost, (PVOID)pmb->MB_hReadSync,
                      LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    
    API_SemaphoreBDelete(&pmb->MB_hReadSync);
    
    __SHEAP_FREE(pmb);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __monitorBufferPut
** 功能描述: 向监控跟踪节点事件缓冲区插入一个事件信息
** 输　入  : pvMonitorBuffer   事件缓冲区
**           pvEvent           事件信息
** 输　出  : 写入的长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  __monitorBufferPut (PVOID      pvMonitorBuffer,
                             CPVOID     pvEvent)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;
    PUCHAR           pucLen = (PUCHAR)pvEvent;
    
    size_t           stLen;
    size_t           stSize;
    
    BOOL             bUpdate;
    
    stSize = (size_t)((pucLen[0] << 8) + pucLen[1]);                    /*  前两字节为大端长度信息      */
    if (stSize > MONITOR_EVENT_MAX_SIZE) {
        return  (0);
    }

    LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
    if (pmb->MB_stLeft < stSize) {
        LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
        _ErrorHandle(ERROR_MONITOR_ENOSPC);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "buffer full.\r\n");
        return  (0);
    }
    
    if ((pmb->MB_stLeft > MONITOR_BUFFER_UPDATE_SIZE) &&
        ((pmb->MB_stLeft - stSize) < MONITOR_BUFFER_UPDATE_SIZE)) {
        bUpdate = LW_TRUE;
    } else {
        bUpdate = LW_FALSE;
    }
    
    stLen = (pmb->MB_pucBuffer + pmb->MB_stSize) - pmb->MB_pucPut;
    if (stLen >= stSize) {
        lib_memcpy(pmb->MB_pucPut, pvEvent, stSize);
        
        if (stLen > stSize) {
            pmb->MB_pucPut += stSize;
        
        } else {
            pmb->MB_pucPut = pmb->MB_pucBuffer;
        }
        
    } else {
        lib_memcpy(pmb->MB_pucPut, pvEvent, stLen);
        pmb->MB_pucPut  = pmb->MB_pucBuffer;
        
        lib_memcpy(pmb->MB_pucPut, (PUCHAR)pvEvent + stLen, stSize - stLen);
        pmb->MB_pucPut += (stSize - stLen);
    }
    
    pmb->MB_stLeft -= stSize;
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
    
    if (bUpdate) {
        LW_KERNEL_JOB_ADD((VOIDFUNCPTR)API_SemaphoreBPost, (PVOID)pmb->MB_hReadSync,
                          LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    }

    return  ((ssize_t)stSize);
}
/*********************************************************************************************************
** 函数名称: __monitorBufferGet
** 功能描述: 从监控跟踪节点事件缓冲区获取一个事件信息
** 输　入  : pvMonitorBuffer   事件缓冲区
**           pvEvent           事件信息
**           stBufferSize      缓冲区长度
**           ulTimeout         超时时间
** 输　出  : 获取的事件实际长度
** 全局变量: 
** 调用模块: 
** 注  意  : 从缓冲区获取事件时首先查看缓冲区是否含有信息, 如果没有, 则等待信号量. 因为缓冲区装入事件时并
             不是立即释放信号量通知读取线程, 而是等到缓冲区快满时才通知, 如果事件较少的情况下可能会引起
             事件上传延迟较久的情况, 所以这里在等待信号量超时后, 再一次试图获取事件信息, 加快对事件的上传
             速度.
*********************************************************************************************************/
ssize_t  __monitorBufferGet (PVOID        pvMonitorBuffer,
                             PVOID        pvEvent,
                             size_t       stBufferSize,
                             ULONG        ulTimeout)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;
    
    size_t           stSize;
    size_t           stLen;
    
    ULONG            ulError;
    BOOL             bTimeout = LW_FALSE;
    
    do {
        LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
        stLen = pmb->MB_stSize - pmb->MB_stLeft;
        if (stLen) {
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
        
        if (bTimeout) {
            return  (0);
        }
        
        ulError = API_SemaphoreBPend(pmb->MB_hReadSync, ulTimeout);
        if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
            bTimeout = LW_TRUE;
        
        } else if (ulError) {
            return  (PX_ERROR);
        }
    } while (1);
    
    {
        UCHAR   ucHigh = pmb->MB_pucGet[0];
        UCHAR   ucLow;
        
        if ((pmb->MB_pucGet - pmb->MB_pucBuffer) >= pmb->MB_stSize) {
            ucLow = pmb->MB_pucBuffer[0];
        } else {
            ucLow = pmb->MB_pucGet[1];
        }
        
        stSize = (size_t)((ucHigh << 8) + ucLow);
    }
    
    if (stSize > stBufferSize) {
        LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);
        _ErrorHandle(ERROR_MONITOR_EMSGSIZE);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "message too big.\r\n");
        return  (0);
    }
    
    stLen = (pmb->MB_pucBuffer + pmb->MB_stSize) - pmb->MB_pucGet;
    if (stLen >= stSize) {
        lib_memcpy(pvEvent, pmb->MB_pucGet, stSize);
        
        if (stLen > stSize) {
            pmb->MB_pucGet += stSize;
        
        } else {
            pmb->MB_pucGet = pmb->MB_pucBuffer;
        }
        
    } else {
        lib_memcpy(pvEvent, pmb->MB_pucGet, stLen);
        pmb->MB_pucGet  = pmb->MB_pucBuffer;
        
        lib_memcpy((PUCHAR)pvEvent + stLen, pmb->MB_pucGet, stSize - stLen);
        pmb->MB_pucGet += (stSize - stLen);
    }
    
    pmb->MB_stLeft += stSize;
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);

    return  ((ssize_t)stSize);
}
/*********************************************************************************************************
** 函数名称: __monitorBufferFlush
** 功能描述: 清空监控跟踪节点事件缓冲区
** 输　入  : pvMonitorBuffer   事件缓冲区
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __monitorBufferFlush (PVOID  pvMonitorBuffer)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;

    API_SemaphoreBClear(pmb->MB_hReadSync);
    
    LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
    
    pmb->MB_stLeft = pmb->MB_stSize;
    pmb->MB_pucPut = pmb->MB_pucBuffer;
    pmb->MB_pucGet = pmb->MB_pucBuffer;
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __monitorBufferStatus
** 功能描述: 获取监控跟踪节点事件缓冲区状态
** 输　入  : pvMonitorBuffer   事件缓冲区
**           pstLeft           剩余空间大小
**           pstNextSize       下一个有效事件长度
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __monitorBufferStatus (PVOID  pvMonitorBuffer,
                              size_t *pstLeft,
                              size_t *pstNextSize)
{
    INTREG           iregInterLevel;
    PMONITOR_BUFFER  pmb = (PMONITOR_BUFFER)pvMonitorBuffer;
    
    LW_SPIN_LOCK_QUICK(&pmb->MB_slLock, &iregInterLevel);
    
    if (pstLeft) {
        *pstLeft = pmb->MB_stLeft;
    }
    
    if (pstNextSize) {
        if (pmb->MB_stLeft != pmb->MB_stSize) {
            UCHAR   ucHigh = pmb->MB_pucGet[0];
            UCHAR   ucLow;
            
            if ((pmb->MB_pucGet - pmb->MB_pucBuffer) >= pmb->MB_stSize) {
                ucLow = pmb->MB_pucBuffer[0];
            } else {
                ucLow = pmb->MB_pucGet[1];
            }
            
            *pstNextSize = (size_t)((ucHigh << 8) + ucLow);
        
        } else {
            *pstNextSize = 0;
        }
    }
    
    LW_SPIN_UNLOCK_QUICK(&pmb->MB_slLock, iregInterLevel);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
