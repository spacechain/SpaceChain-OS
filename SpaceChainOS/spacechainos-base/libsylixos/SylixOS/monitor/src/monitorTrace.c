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
** 文   件   名: monitorTrace.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0
/*********************************************************************************************************
  监控点结构
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                MONITOR_lineManage;
    BOOL                        MONITOR_bEnable;
    MONITOR_FILTER_ROUTINE      MONITOR_pfuncFilter;
    MONITOR_COLLECTOR_ROUTINE   MONITOR_pfuncCollector;
    PVOID                       MONITOR_pvArg;
} MONITOR_TRACE;
typedef MONITOR_TRACE          *PMONITOR_TRACE;
/*********************************************************************************************************
  监控点全局变量
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE_CACHE_ALIGN(_G_slMonitor);
static LW_LIST_LINE_HEADER      _G_plineMonitor;
static LW_LIST_LINE_HEADER      _G_plineMonitorWalk;
/*********************************************************************************************************
** 函数名称: API_MonitorTraceCreate
** 功能描述: 创建一个监控跟踪节点
** 输　入  : pfuncFilter       事件滤波器
**           pfuncCollector    事件收集器
**           pvArg             回调首参数
**           bEnable           是否使能
** 输　出  : 监控跟踪节点
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorTraceCreate (MONITOR_FILTER_ROUTINE       pfuncFilter,
                               MONITOR_COLLECTOR_ROUTINE    pfuncCollector,
                               PVOID                        pvArg,
                               BOOL                         bEnable)
{
    static BOOL     bIsInit = LW_FALSE;
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt;
    
    if (bIsInit == LW_FALSE) {
        bIsInit =  LW_TRUE;
        LW_SPIN_INIT(&_G_slMonitor);
    }
    
    if (!pfuncCollector) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    pmt = (PMONITOR_TRACE)__KHEAP_ALLOC(sizeof(MONITOR_TRACE));
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_ENOMEM);
        return  (LW_NULL);
    }
    
    _LIST_LINE_INIT_IN_CODE(pmt->MONITOR_lineManage);
    pmt->MONITOR_bEnable        = bEnable;
    pmt->MONITOR_pfuncFilter    = pfuncFilter;
    pmt->MONITOR_pfuncCollector = pfuncCollector;
    pmt->MONITOR_pvArg          = pvArg;
    
    if (bEnable) {
        LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
        _List_Line_Add_Ahead(&pmt->MONITOR_lineManage, &_G_plineMonitor);
        LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    }
    
    return  ((PVOID)pmt);
}
/*********************************************************************************************************
** 函数名称: API_MonitorTraceDelete
** 功能描述: 删除一个监控跟踪节点
** 输　入  : pvMonitor     监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorTraceDelete (PVOID  pvMonitor)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt = (PMONITOR_TRACE)pvMonitor;
    
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    if (pmt->MONITOR_bEnable) {
        _List_Line_Del(&pmt->MONITOR_lineManage, &_G_plineMonitor);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    
    __KHEAP_FREE(pmt);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorTraceEnable
** 功能描述: 使能一个监控跟踪节点
** 输　入  : pvMonitor     监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorTraceEnable (PVOID  pvMonitor)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt = (PMONITOR_TRACE)pvMonitor;
    
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    if (!pmt->MONITOR_bEnable) {
        pmt->MONITOR_bEnable = LW_TRUE;
        _List_Line_Add_Ahead(&pmt->MONITOR_lineManage, &_G_plineMonitor);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorTraceDisable
** 功能描述: 禁能一个监控跟踪节点
** 输　入  : pvMonitor     监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorTraceDisable (PVOID  pvMonitor)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt = (PMONITOR_TRACE)pvMonitor;
    
    if (!pmt) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    if (pmt->MONITOR_bEnable) {
        pmt->MONITOR_bEnable = LW_FALSE;
        if (_G_plineMonitorWalk == &pmt->MONITOR_lineManage) {
            _G_plineMonitorWalk =  _list_line_get_next(_G_plineMonitorWalk);
        }
        _List_Line_Del(&pmt->MONITOR_lineManage, &_G_plineMonitor);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __monitorTraceEvent
** 功能描述: 产生一个监控事件
** 输　入  : uiEventId         事件 ID
**           uiSubEvent        子事件
**           pvMsg             对应事件信息
**           stSize            事件信息长度
**           pcAddtional       追加字串信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_MonitorTraceEvent (UINT32         uiEventId, 
                             UINT32         uiSubEvent, 
                             CPVOID         pvMsg, 
                             size_t         stSize,
                             CPCHAR         pcAddtional)
{
    INTREG          iregInterLevel;
    PMONITOR_TRACE  pmt;
    
    if (!_G_plineMonitor) {                                             /*  不存在监控节点              */
        return;
    }
    
    LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    _G_plineMonitorWalk = _G_plineMonitor;
    while (_G_plineMonitorWalk) {
        pmt = _LIST_ENTRY(_G_plineMonitorWalk, MONITOR_TRACE, MONITOR_lineManage);
        _G_plineMonitorWalk = _list_line_get_next(_G_plineMonitorWalk);
        
        LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
        if (pmt->MONITOR_pfuncFilter == LW_NULL ||
            pmt->MONITOR_pfuncFilter(pmt->MONITOR_pvArg, uiEventId, uiSubEvent)) {
            pmt->MONITOR_pfuncCollector(pmt->MONITOR_pvArg, uiEventId, uiSubEvent, 
                                        pvMsg, stSize, pcAddtional);
        }
        LW_SPIN_LOCK_QUICK(&_G_slMonitor, &iregInterLevel);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slMonitor, iregInterLevel);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
