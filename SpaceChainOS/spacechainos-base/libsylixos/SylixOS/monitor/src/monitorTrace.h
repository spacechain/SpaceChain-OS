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
** 文   件   名: monitorTrace.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器.
*********************************************************************************************************/

#ifndef __MONITORTRACE_H
#define __MONITORTRACE_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

/*********************************************************************************************************
  监控节点函数类型
*********************************************************************************************************/

typedef BOOL  (*MONITOR_FILTER_ROUTINE)(PVOID           pvArg, 
                                        UINT32          uiEventId, 
                                        UINT32          uiSubEvent);
                                        
typedef VOID  (*MONITOR_COLLECTOR_ROUTINE)(PVOID           pvArg, 
                                           UINT32          uiEventId, 
                                           UINT32          uiSubEvent,
                                           CPVOID          pvMsg,
                                           size_t          stSize,
                                           CPCHAR          pcAddtional);

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API PVOID  API_MonitorTraceCreate(MONITOR_FILTER_ROUTINE       pfuncFilter,
                                     MONITOR_COLLECTOR_ROUTINE    pfuncCollector,
                                     PVOID                        pvArg,
                                     BOOL                         bEnable);

LW_API ULONG  API_MonitorTraceDelete(PVOID  pvMonitor);

LW_API ULONG  API_MonitorTraceEnable(PVOID  pvMonitor);

LW_API ULONG  API_MonitorTraceDisable(PVOID  pvMonitor);

LW_API VOID   API_MonitorTraceEvent(UINT32         uiEventId, 
                                    UINT32         uiSubEvent, 
                                    CPVOID         pvMsg, 
                                    size_t         stSize,
                                    CPCHAR         pcAddtional);

#define MONITOR_TRACE_EVENT(a, b, c, d, e)  API_MonitorTraceEvent(a, b, c, d, e)

#else

#define MONITOR_TRACE_EVENT(a, b, c, d, e)

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

#endif                                                                  /*  __MONITORTRACE_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
