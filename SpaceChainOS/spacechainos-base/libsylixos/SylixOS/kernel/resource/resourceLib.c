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
** 文   件   名: resourceLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 06 日
**
** 描        述: 资源管理器 (功耗管理器不在这里管理, 初始化前的一些全局对象可以不用管理)
**
** 注        意: 资源回收的原则是: 进程号不为 0, 且不是 GLOBAL 的对象需要进行回收.

** BUG:
2012.12.21  1.0.0.rc36 版以后的 SylixOS 实现了进程独立文件描述符, 这里不再使用 hook 回收文件描述符.
2013.09.04  加入对没有 global 属性的 powerm 节点回收功能.
2014.05.20  删除 __resPidCanExit() 函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "resourceReclaim.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  资源类型
  分为: 内核句柄, I/O文件, 原始资源, 其中前两种资源靠系统 hook 自动调用, 原始资源需要相关的管理器自行调用
*********************************************************************************************************/
typedef struct {
    LW_OBJECT_HANDLE        RESH_ulHandle;                              /*  资源句柄                    */
    BOOL                    RESH_bIsGlobal;                             /*  是否是全局资源              */
    pid_t                   RESH_pid;                                   /*  进程号                      */
} LW_RESOURCE_H;
typedef LW_RESOURCE_H      *PLW_RESOURCE_H;                             /*  内核对象类资源              */
/*********************************************************************************************************
  资源缓冲
*********************************************************************************************************/                                   
static LW_RESOURCE_H        _G_reshEventBuffer[LW_CFG_MAX_EVENTS];
static LW_RESOURCE_H        _G_reshEventsetBuffer[LW_CFG_MAX_EVENTSETS];
static LW_RESOURCE_H        _G_reshPartitionBuffer[LW_CFG_MAX_PARTITIONS];
static LW_RESOURCE_H        _G_reshRegionBuffer[LW_CFG_MAX_REGIONS];
static LW_RESOURCE_H        _G_reshTimerBuffer[LW_CFG_MAX_TIMERS];
static LW_RESOURCE_H        _G_reshThreadBuffer[LW_CFG_MAX_THREADS];
static LW_RESOURCE_H        _G_reshRmsBuffer[LW_CFG_MAX_RMSS];
static LW_RESOURCE_H        _G_reshThreadPoolBuffer[LW_CFG_MAX_THREAD_POOLS];
/*********************************************************************************************************
  原始资源缓冲
*********************************************************************************************************/                                   
static LW_LIST_LINE_HEADER  _G_plineResRaw;
/*********************************************************************************************************
  资源表操作锁
*********************************************************************************************************/                                   
static LW_OBJECT_HANDLE     _G_ulResHLock;
static LW_OBJECT_HANDLE     _G_ulResRawLock;

#define __LW_RESH_LOCK()        API_SemaphoreMPend(_G_ulResHLock, LW_OPTION_WAIT_INFINITE)
#define __LW_RESH_UNLOCK()      API_SemaphoreMPost(_G_ulResHLock)

#define __LW_RESRAW_LOCK()      API_SemaphoreMPend(_G_ulResRawLock, LW_OPTION_WAIT_INFINITE)
#define __LW_RESRAW_UNLOCK()    API_SemaphoreMPost(_G_ulResRawLock)
/*********************************************************************************************************
** 函数名称: __resGetHandleBuffer
** 功能描述: 通过句柄获得对应资源记录器的位置
** 输　入  : ulHandle      资源句柄
** 输　出  : 资源节点
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_INLINE PLW_RESOURCE_H  __resGetHandleBuffer (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh = LW_NULL;
    UINT16          usIndex = _ObjectGetIndex(ulHandle);
    ULONG           ulClass = _ObjectGetClass(ulHandle);
    
    switch (ulClass) {
    
    case _OBJECT_THREAD:
        if (usIndex < LW_CFG_MAX_THREADS) {
            presh = &_G_reshThreadBuffer[usIndex];
        }
        break;
        
    case _OBJECT_THREAD_POOL:
        if (usIndex < LW_CFG_MAX_THREAD_POOLS) {
            presh = &_G_reshThreadPoolBuffer[usIndex];
        }
        break;
        
    case _OBJECT_SEM_C:
    case _OBJECT_SEM_B:
    case _OBJECT_SEM_M:
    case _OBJECT_SEM_RW:
    case _OBJECT_MSGQUEUE:
        if (usIndex < LW_CFG_MAX_EVENTS) {
            presh = &_G_reshEventBuffer[usIndex];
        }
        break;
    
    case _OBJECT_EVENT_SET:
        if (usIndex < LW_CFG_MAX_EVENTSETS) {
            presh = &_G_reshEventsetBuffer[usIndex];
        }
        break;
    
    case _OBJECT_TIMER:
        if (usIndex < LW_CFG_MAX_TIMERS) {
            presh = &_G_reshTimerBuffer[usIndex];
        }
        break;
    
    case _OBJECT_PARTITION:
        if (usIndex < LW_CFG_MAX_PARTITIONS) {
            presh = &_G_reshPartitionBuffer[usIndex];
        }
        break;
    
    case _OBJECT_REGION:
        if (usIndex < LW_CFG_MAX_REGIONS) {
            presh = &_G_reshRegionBuffer[usIndex];
        }
        break;
    
    case _OBJECT_RMS:
        if (usIndex < LW_CFG_MAX_RMSS) {
            presh = &_G_reshRmsBuffer[usIndex];
        }
        break;
        
    default:
        break;
    }
    
    return  (presh);
}
/*********************************************************************************************************
** 函数名称: __resAddHandleHook
** 功能描述: 资源管理器加入一个内核资源
** 输　入  : ulHandle      资源句柄
**           ulOption      创建选项
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __resAddHandleHook (LW_OBJECT_HANDLE  ulHandle, ULONG  ulOption)
{
    PLW_RESOURCE_H  presh;
    pid_t           pid = getpid();
    
    if (!_ObjectClassOK(ulHandle, _OBJECT_THREAD)) {                    /*  不是线程                    */
        if ((ulOption & LW_OPTION_OBJECT_GLOBAL) || (pid == 0)) {       /*  全局对象或内核任务建立      */
            return;                                                     /*  不记录                      */
        }
    }

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        presh->RESH_ulHandle = ulHandle;
        presh->RESH_pid      = pid;
        if (ulOption & LW_OPTION_OBJECT_GLOBAL) {
            presh->RESH_bIsGlobal = LW_TRUE;
        } else {
            presh->RESH_bIsGlobal = LW_FALSE;
        }
    }
    __LW_RESH_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __resDelHandleHook
** 功能描述: 从资源管理器删除一个内核资源
** 输　入  : ulHandle      资源句柄
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __resDelHandleHook (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;
    
    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        presh->RESH_ulHandle  = LW_OBJECT_HANDLE_INVALID;
        presh->RESH_bIsGlobal = LW_FALSE;
        presh->RESH_pid       = 0;
    }
    __LW_RESH_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __resAddRawHook
** 功能描述: 资源管理器加入一个原始资源
** 输　入  : presraw       原始资源缓冲
**           pvfunc        释放函数
**           pvArg0~5      释放函数参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __resAddRawHook (PLW_RESOURCE_RAW    presraw, 
                       VOIDFUNCPTR         pvfunc,
                       PVOID               pvArg0,
                       PVOID               pvArg1,
                       PVOID               pvArg2,
                       PVOID               pvArg3,
                       PVOID               pvArg4,
                       PVOID               pvArg5)
{
    if (!presraw) {
        return;
    }
    
    presraw->RESRAW_pid = getpid();
    if (presraw->RESRAW_pid == 0) {                                     /*  不保存内核任务资源          */
        presraw->RESRAW_bIsInstall = LW_FALSE;                          /*  没有安装成功                */
        return;
    }
    
    presraw->RESRAW_pvArg[0] = pvArg0;
    presraw->RESRAW_pvArg[1] = pvArg1;
    presraw->RESRAW_pvArg[2] = pvArg2;
    presraw->RESRAW_pvArg[3] = pvArg3;
    presraw->RESRAW_pvArg[4] = pvArg4;
    presraw->RESRAW_pvArg[5] = pvArg5;
    
    presraw->RESRAW_pfuncFree = pvfunc;
    
    __LW_RESRAW_LOCK();
    _List_Line_Add_Ahead(&presraw->RESRAW_lineManage, &_G_plineResRaw);
    presraw->RESRAW_bIsInstall = LW_TRUE;                               /*  安装成功                    */
    __LW_RESRAW_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __resDelRawHook
** 功能描述: 从资源管理器去除一个原始资源
** 输　入  : presraw       原始资源缓冲
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __resDelRawHook (PLW_RESOURCE_RAW    presraw)
{
    if (!presraw) {
        return;
    }
    
    if (presraw->RESRAW_bIsInstall == LW_FALSE) {                       /*  必须是安装成功的            */
        return;
    }
    
    __LW_RESRAW_LOCK();
    _List_Line_Del(&presraw->RESRAW_lineManage, &_G_plineResRaw);
    presraw->RESRAW_bIsInstall = LW_FALSE;
    __LW_RESRAW_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __resThreadDelHook
** 功能描述: 线程删除完毕时将调用此函数
** 输　入  : pvVProc       线程保存的进程信息
**           ulId          被删除的线程 ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __resThreadDelHook (PVOID  pvVProc, LW_OBJECT_HANDLE  ulId)
{
    if (pvVProc) {
        vprocThreadExitHook(pvVProc, ulId);
    }
}
/*********************************************************************************************************
** 函数名称: __resPidReclaim
** 功能描述: 从资源管理器删除对应进程的全部资源 (需要等待所有进程内线程结束)
** 输　入  : pid           进程号
** 输　出  : 是否成功, 如果不成功证明这个进程还有线程在运行.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __resPidReclaim (pid_t  pid)
{
    INT                 i;
    PLW_RESOURCE_H      presh;
    PLW_RESOURCE_RAW    presraw;
    PLW_LIST_LINE       plineTemp;
    
    if (pid == 0) {
        return  (ERROR_NONE);                                           /*  内核, 不执行回收操作        */
    }
    
    __LW_RESRAW_LOCK();
    plineTemp = _G_plineResRaw;
    while (plineTemp) {
        presraw   = (PLW_RESOURCE_RAW)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        if (presraw->RESRAW_pid == pid) {
            presraw->RESRAW_pfuncFree(presraw->RESRAW_pvArg[0],
                                      presraw->RESRAW_pvArg[1],
                                      presraw->RESRAW_pvArg[2],
                                      presraw->RESRAW_pvArg[3],
                                      presraw->RESRAW_pvArg[4],
                                      presraw->RESRAW_pvArg[5]);        /*  释放原始资源                */
        }
    }
    __LW_RESRAW_UNLOCK();
    
    vprocIoReclaim(pid, LW_FALSE);                                      /*  回收进程所有打开的文件      */
    
#if LW_CFG_VMM_EN > 0
    API_VmmMmapReclaim(pid);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    
#if (LW_CFG_THREAD_POOL_EN > 0) && (LW_CFG_MAX_THREAD_POOLS > 0)
    for (i = 0; i < LW_CFG_MAX_THREAD_POOLS; i++) {
        presh = &_G_reshThreadPoolBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_ThreadPoolDelete(&presh->RESH_ulHandle);
        }
    }
#endif                                                                  /*  LW_CFG_THREAD_POOL_EN > 0   */
                                                                        /*  LW_CFG_MAX_THREAD_POOLS > 0 */
    for (i = 0; i < LW_CFG_MAX_EVENTS; i++) {                           /*  处理事件                    */
        presh = &_G_reshEventBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            if (_ObjectGetClass(presh->RESH_ulHandle) == _OBJECT_MSGQUEUE) {
                API_MsgQueueDelete(&presh->RESH_ulHandle);
            } else {
                API_SemaphoreDelete(&presh->RESH_ulHandle);
            }
        }
    }
    
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
    for (i = 0; i < LW_CFG_MAX_EVENTSETS; i++) {                        /*  处理事件组                  */
        presh = &_G_reshEventsetBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_EventSetDelete(&presh->RESH_ulHandle);
        }
    }
#endif
    
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS > 0)
    for (i = 0; i < LW_CFG_MAX_TIMERS; i++) {                           /*  处理定时器                  */
        presh = &_G_reshTimerBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_TimerDelete(&presh->RESH_ulHandle);
        }
    }
#endif

#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
    for (i = 0; i < LW_CFG_MAX_RMSS; i++) {                             /*  处理 RMS                    */
        presh = &_G_reshRmsBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_RmsDeleteEx(&presh->RESH_ulHandle, LW_TRUE);
        }
    }
#endif
    
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
    for (i = 0; i < LW_CFG_MAX_PARTITIONS; i++) {                       /*  处理 PATITIONS              */
        presh = &_G_reshPartitionBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_PartitionDeleteEx(&presh->RESH_ulHandle, LW_TRUE);
        }
    }
#endif
    
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)
    for (i = 0; i < LW_CFG_MAX_REGIONS; i++) {                          /*  最后处理可变内存            */
        presh = &_G_reshRegionBuffer[i];
        if ((presh->RESH_pid == pid) && !presh->RESH_bIsGlobal) {
            API_RegionDeleteEx(&presh->RESH_ulHandle, LW_TRUE);
        }
    }
#endif
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __resPidReclaimOnlyRaw
** 功能描述: 从资源管理器删除对应进程的原始资源 (内核对象和文件不回收)
** 输　入  : pid           进程号
** 输　出  : 是否成功, 如果不成功证明这个进程还有线程在运行.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __resPidReclaimOnlyRaw (pid_t  pid)
{
    PLW_RESOURCE_RAW    presraw;
    PLW_LIST_LINE       plineTemp;
    
    if (pid == 0) {
        return  (ERROR_NONE);                                           /*  内核, 不执行回收操作        */
    }
    
    __LW_RESRAW_LOCK();
    plineTemp = _G_plineResRaw;
    while (plineTemp) {
        presraw   = (PLW_RESOURCE_RAW)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        if (presraw->RESRAW_pid == pid) {
            presraw->RESRAW_pfuncFree(presraw->RESRAW_pvArg[0],
                                      presraw->RESRAW_pvArg[1],
                                      presraw->RESRAW_pvArg[2],
                                      presraw->RESRAW_pvArg[3],
                                      presraw->RESRAW_pvArg[4],
                                      presraw->RESRAW_pvArg[5]);        /*  释放原始资源                */
        }
    }
    __LW_RESRAW_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __resHandleIsGlobal
** 功能描述: 判断一个内核资源是否为全局资源
** 输　入  : ulHandle      句柄
** 输　出  : 是否是全局资源
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  __resHandleIsGlobal (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        if ((presh->RESH_ulHandle == ulHandle) && !presh->RESH_bIsGlobal) {
            __LW_RESH_UNLOCK();
            return  (LW_FALSE);
        }
    }
    __LW_RESH_UNLOCK();
    
    return  (LW_TRUE);                                                  /*  默认都是全局对象            */
}
/*********************************************************************************************************
** 函数名称: __resHandleMakeGlobal
** 功能描述: 将一个资源设置为内核全局资源
** 输　入  : ulHandle      句柄
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __resHandleMakeGlobal (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        if (presh->RESH_ulHandle == ulHandle) {
            presh->RESH_bIsGlobal = LW_TRUE;
            __LW_RESH_UNLOCK();
            return  (ERROR_NONE);
        }
    }
    __LW_RESH_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __resHandleMakeLocal
** 功能描述: 将一个资源设置为进程内资源 (这里可能已经改变了进程号)
** 输　入  : ulHandle      句柄
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __resHandleMakeLocal (LW_OBJECT_HANDLE  ulHandle)
{
    PLW_RESOURCE_H  presh;
    pid_t           pid = getpid();

    __LW_RESH_LOCK();
    presh = __resGetHandleBuffer(ulHandle);
    if (presh) {
        presh->RESH_ulHandle  = ulHandle;
        presh->RESH_pid       = pid;                                    /*  重新确定 pid                */
        presh->RESH_bIsGlobal = LW_FALSE;
        __LW_RESH_UNLOCK();
        return  (ERROR_NONE);
    }
    __LW_RESH_UNLOCK();
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _resInit
** 功能描述: 资源管理器初始化 (此初始化放在内核初始化完毕后, 内核的一些基本资源这里不需要记录)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _resInit (VOID)
{
    __resReclaimInit();                                                 /*  资源回收器初始化            */

    _G_ulResHLock   = API_SemaphoreMCreate("resh_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _G_ulResRawLock = API_SemaphoreMCreate("resraw_lock", LW_PRIO_DEF_CEILING, LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    
    API_SystemHookAdd(__resAddHandleHook, LW_OPTION_OBJECT_CREATE_HOOK);
    API_SystemHookAdd(__resDelHandleHook, LW_OPTION_OBJECT_DELETE_HOOK);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
