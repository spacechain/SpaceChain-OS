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
** 文   件   名: hotplugLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 09 日
**
** 描        述: 热插拔支持.

** BUG:
2010.09.18  支持热插拔消息方式.
2011.03.06  修正 gcc 4.5.1 相关 warning.
2012.09.01  加入对 hotplug 线程句柄的获取.
2012.12.08  加入资源回收的功能.
2012.12.13  热插拔消息不能重复安装.
2013.10.03  系统删除 message 方式的热插拔回调, 支持新的 /dev/hotplug 热插拔通知应用的方法.
2013.12.01  不再使用消息队列, 使用内核提供的工作队列模型.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_HOTPLUG_EN > 0
#if LW_CFG_DEVICE_EN > 0
#include "hotplugDev.h"
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  事件回调节点
*********************************************************************************************************/
static LW_JOB_QUEUE            _G_jobqHotplug;
static LW_JOB_MSG              _G_jobmsgHotplug[LW_CFG_HOTPLUG_MAX_MSGS];
/*********************************************************************************************************
  事件轮询节点
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE               HPPN_lineManage;                         /*  节点链表                    */
    VOIDFUNCPTR                HPPN_pfunc;                              /*  函数指针                    */
    PVOID                      HPPN_pvArg;                              /*  函数参数                    */
    LW_RESOURCE_RAW            HPPN_resraw;                             /*  资源管理节点                */
} LW_HOTPLUG_POLLNODE;
typedef LW_HOTPLUG_POLLNODE   *PLW_HOTPLUG_POLLNODE;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE        _G_hHotplugLock     = 0;                 /*  循环检测连锁                */
static LW_OBJECT_HANDLE        _G_hHotplug         = 0;                 /*  线程句柄                    */
static LW_LIST_LINE_HEADER     _G_plineHotplugPoll = LW_NULL;           /*  热插拔循环检测链            */
/*********************************************************************************************************
  热插拔锁
*********************************************************************************************************/
#define __HOTPLUG_LOCK()       API_SemaphoreMPend(_G_hHotplugLock, LW_OPTION_WAIT_INFINITE)
#define __HOTPLUG_UNLOCK()     API_SemaphoreMPost(_G_hHotplugLock)
/*********************************************************************************************************
  INTERNAL FUNC
*********************************************************************************************************/
static VOID  _hotplugThread(VOID);                                      /*  作业处理程序                */
/*********************************************************************************************************
** 函数名称: _hotplugInit
** 功能描述: 初始化 hotplug 库
** 输　入  : NONE
** 输　出  : 是否初始化成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _hotplugInit (VOID)
{
    LW_OBJECT_HANDLE    hHotplugThread;
    LW_CLASS_THREADATTR threadattr;
    
    if (_jobQueueInit(&_G_jobqHotplug, &_G_jobmsgHotplug[0], 
                      LW_CFG_HOTPLUG_MAX_MSGS, LW_FALSE)) {
        return  (PX_ERROR);
    }

    _G_hHotplugLock = API_SemaphoreMCreate("hotplug_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_INHERIT_PRIORITY | 
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_WAIT_PRIORITY | 
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);                    /*  建立 poll 链操作锁          */
    if (!_G_hHotplugLock) {
        _jobQueueFinit(&_G_jobqHotplug);
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, LW_CFG_THREAD_HOTPLUG_STK_SIZE, 
                        LW_PRIO_T_SYSMSG, 
                        (LW_CFG_HOTPLUG_OPTION | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL),
                        LW_NULL);
                        
    hHotplugThread = API_ThreadCreate("t_hotplug",
                                      (PTHREAD_START_ROUTINE)_hotplugThread,
                                      (PLW_CLASS_THREADATTR)&threadattr,
                                      LW_NULL);                         /*  建立 job 处理线程           */
    if (!hHotplugThread) {
        API_SemaphoreMDelete(&_G_hHotplugLock);
        _jobQueueFinit(&_G_jobqHotplug);
        return  (PX_ERROR);
    }
    
    _G_hHotplug = hHotplugThread;
    
#if LW_CFG_DEVICE_EN > 0
    _hotplugDrvInstall();                                               /*  初始化热插拔消息设备        */
    _hotplugDevCreate();
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_HotplugContext
** 功能描述: 是否在 hotplug 处理线程中
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
BOOL  API_HotplugContext (VOID)
{
    if (API_ThreadIdSelf() == _G_hHotplug) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: API_HotplugEvent
** 功能描述: 将需要处理的 hotplug 事件加入处理队列
** 输　入  : pfunc                      函数指针
**           pvArg0                     函数参数
**           pvArg1                     函数参数
**           pvArg2                     函数参数
**           pvArg3                     函数参数
**           pvArg4                     函数参数
**           pvArg5                     函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_HotplugEvent (VOIDFUNCPTR  pfunc, 
                       PVOID        pvArg0,
                       PVOID        pvArg1,
                       PVOID        pvArg2,
                       PVOID        pvArg3,
                       PVOID        pvArg4,
                       PVOID        pvArg5)
{
    if (!pfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_jobQueueAdd(&_G_jobqHotplug, pfunc, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
        _ErrorHandle(ERROR_EXCE_LOST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_HotplugEventMessage
** 功能描述: 产生一个 hotplug 消息事件, 由 t_hotplug 决定执行的函数. (大端格式)
** 输　入  : iMsg                       消息号
**           bInsert                    是否为插入, 否则为拔出
**           pcPath                     设备路径或名称
**           uiArg0                     附加参数
**           uiArg1                     附加参数
**           uiArg2                     附加参数
**           uiArg3                     附加参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

LW_API  
INT      API_HotplugEventMessage (INT       iMsg,
                                  BOOL      bInsert,
                                  CPCHAR    pcPath,
                                  UINT32    uiArg0,
                                  UINT32    uiArg1,
                                  UINT32    uiArg2,
                                  UINT32    uiArg3)
{
    size_t  i;
    UCHAR   ucBuffer[LW_HOTPLUG_DEV_MAX_MSGSIZE];
    
    if (pcPath == LW_NULL) {
        i = 0;
    
    } else {
        i = lib_strlen(pcPath);
        if (i > PATH_MAX) {
            _ErrorHandle(ENAMETOOLONG);
            return  (PX_ERROR);
        }
    }
    
    ucBuffer[0] = (UCHAR)((iMsg >> 24) & 0xff);
    ucBuffer[1] = (UCHAR)((iMsg >> 16) & 0xff);
    ucBuffer[2] = (UCHAR)((iMsg >>  8) & 0xff);
    ucBuffer[3] = (UCHAR)((iMsg)       & 0xff);
    ucBuffer[4] = (UCHAR)bInsert;
    
    if (pcPath) {
        lib_strcpy((PCHAR)&ucBuffer[5], pcPath);
    } else {
        ucBuffer[5] = PX_EOS;
    }
    
    i += 6;                                                             /*  包含最后的 \0               */
    
    ucBuffer[i++] = (UCHAR)((uiArg0 >> 24) & 0xff);                     /*  MSB                         */
    ucBuffer[i++] = (UCHAR)((uiArg0 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg0 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg0)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg1 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg2 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg3 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3)       & 0xff);
    
    _hotplugDevPutMsg(iMsg, ucBuffer, i);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: API_HotplugPollAdd
** 功能描述: 在 hotplug 事件处理上下文中, 加入一个循环检测函数.
** 输　入  : pfunc                      函数指针
**           pvArg                      函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_HotplugPollAdd (VOIDFUNCPTR   pfunc, PVOID  pvArg)
{
    PLW_HOTPLUG_POLLNODE        phppn;
    
    phppn = (PLW_HOTPLUG_POLLNODE)__SHEAP_ALLOC(sizeof(LW_HOTPLUG_POLLNODE));
                                                                        /*  申请控制块内存              */
    if (!phppn) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                          /*  缺少内存                    */
        return  (PX_ERROR);
    }
    
    phppn->HPPN_pfunc = pfunc;
    phppn->HPPN_pvArg = pvArg;
    
    __HOTPLUG_LOCK();                                                   /*  lock hotplug poll list      */
    _List_Line_Add_Ahead(&phppn->HPPN_lineManage, &_G_plineHotplugPoll);
    __HOTPLUG_UNLOCK();                                                 /*  unlock hotplug poll list    */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (__PROC_GET_PID_CUR() && vprocFindProc((PVOID)pfunc)) {
        __resAddRawHook(&phppn->HPPN_resraw, (VOIDFUNCPTR)API_HotplugPollDelete,
                        (PVOID)pfunc, pvArg, 0, 0, 0, 0);
    } else {
        phppn->HPPN_resraw.RESRAW_bIsInstall = LW_FALSE;                /*  不需要回收操作              */
    }
#else
    __resAddRawHook(&phppn->HPPN_resraw, (VOIDFUNCPTR)API_HotplugPollDelete,
                    (PVOID)pfunc, pvArg, 0, 0, 0, 0);
#endif
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_HotplugPollDelete
** 功能描述: 从 hotplug 事件处理上下文中, 删除一个循环检测函数.
** 输　入  : pfunc                      函数指针
**           pvArg                      函数参数
** 输　出  : 操作是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_HotplugPollDelete (VOIDFUNCPTR   pfunc, PVOID  pvArg)
{
    PLW_HOTPLUG_POLLNODE        phppn = LW_NULL;
    PLW_LIST_LINE               plineTemp;
    
    __HOTPLUG_LOCK();                                                   /*  lock hotplug poll list      */
    for (plineTemp  = _G_plineHotplugPoll;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phppn = _LIST_ENTRY(plineTemp, LW_HOTPLUG_POLLNODE, HPPN_lineManage);
        if ((phppn->HPPN_pfunc == pfunc) &&
            (phppn->HPPN_pvArg == pvArg)) {
            _List_Line_Del(&phppn->HPPN_lineManage, &_G_plineHotplugPoll);
            break;
        }
    }
    __HOTPLUG_UNLOCK();                                                 /*  unlock hotplug poll list    */
    
    if (plineTemp != LW_NULL) {
        __resDelRawHook(&phppn->HPPN_resraw);
        __SHEAP_FREE(phppn);
        return  (ERROR_NONE);
    }
    
    _ErrorHandle(ERROR_HOTPLUG_POLL_NODE_NULL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _hotplugThread
** 功能描述: hotplug 事件处理线程
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _hotplugThread (VOID)
{
    ULONG  ulError;
    
    for (;;) {
        ulError = _jobQueueExec(&_G_jobqHotplug, LW_HOTPLUG_SEC * LW_TICK_HZ);
        if (ulError) {                                                  /*  需要处理轮询事件            */
            PLW_HOTPLUG_POLLNODE    phppn;
            PLW_LIST_LINE           plineTemp;
            
            __HOTPLUG_LOCK();                                           /*  lock hotplug poll list      */
            for (plineTemp  = _G_plineHotplugPoll;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                
                phppn = _LIST_ENTRY(plineTemp, LW_HOTPLUG_POLLNODE, HPPN_lineManage);
                if (phppn->HPPN_pfunc) {
                    phppn->HPPN_pfunc(phppn->HPPN_pvArg);               /*  调用先前安装的循环检测函数  */
                }
            }
            __HOTPLUG_UNLOCK();                                         /*  unlock hotplug poll list    */
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_NetJobGetLost
** 功能描述: 获得 hotplug 消息丢失的数量
** 输　入  : NONE
** 输　出  : 消息丢失的数量
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
size_t  API_HotplugGetLost (VOID)
{
    return  (_jobQueueLost(&_G_jobqHotplug));
}

#endif                                                                  /*  LW_CFG_HOTPLUG_EN           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
