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
** 文   件   名: monitorBuffer.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器缓冲区管理.
*********************************************************************************************************/

#ifndef __MONITOR_BUFFER_H
#define __MONITOR_BUFFER_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

/*********************************************************************************************************
  缓冲区管理
*********************************************************************************************************/

PVOID   __monitorBufferCreate(size_t  stSize);

ULONG   __monitorBufferDelete(PVOID  pvMonitorBuffer);

ssize_t __monitorBufferPut(PVOID  pvMonitorBuffer, CPVOID pvEvent);

ssize_t __monitorBufferGet(PVOID  pvMonitorBuffer, PVOID pvEvent, size_t stBufferSize, ULONG ulTimeout);

ULONG   __monitorBufferFlush(PVOID  pvMonitorBuffer);

ULONG   __monitorBufferStatus(PVOID  pvMonitorBuffer, size_t *pstLeft, size_t *pstNextSize);

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
#endif                                                                  /*  __MONITOR_BUFFER_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
