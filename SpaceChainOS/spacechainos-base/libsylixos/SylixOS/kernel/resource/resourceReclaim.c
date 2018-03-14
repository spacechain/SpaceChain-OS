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
** 文   件   名: resourceReclaim.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 09 日
**
** 描        述: 为了避免僵尸线程, SylixOS 使用统一的资源回收器进行资源回收.
**
** 注        意: 资源回收的原则是: 进程号不为 0, 且不是 GLOBAL 的对象需要进行回收.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  资源回收队列
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_ulResReclaimQueue;
/*********************************************************************************************************
** 函数名称: __resReclaimReq
** 功能描述: 请求回收进程资源
** 输　入  : pvVProc       进程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __resReclaimReq (PVOID  pvVProc)
{
    ULONG        ulError;
    LW_LD_VPROC *pvproc = (LW_LD_VPROC *)pvVProc;

    if (pvproc) {
        ulError = API_MsgQueueSend(_G_ulResReclaimQueue, (PVOID)&pvproc, sizeof(PVOID));
        if (ulError) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not req reclaim.\r\n");
        }
    }
}
/*********************************************************************************************************
** 函数名称: __resReclaimThread
** 功能描述: 资源回收器系统线程
** 输　入  : pvArg         暂时没有使用
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __resReclaimThread (PVOID  pvArg)
{
    LW_LD_VPROC *pvproc;
    ULONG        ulError;

    (VOID)pvArg;
    
    for (;;) {
        ulError = API_MsgQueueReceive(_G_ulResReclaimQueue, (PVOID)&pvproc, sizeof(PVOID), 
                                      LW_NULL, LW_OPTION_WAIT_INFINITE);
        if (ulError == ERROR_NONE) {
            vprocReclaim(pvproc, LW_TRUE);                              /*  回收进程资源                */
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __resReclaimInit
** 功能描述: 资源回收器初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __resReclaimInit (VOID)
{
    LW_CLASS_THREADATTR     threadattr;

    _G_ulResReclaimQueue = API_MsgQueueCreate("res_reclaim", LW_CFG_MAX_THREADS, 
                                              sizeof(PVOID), LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_ulResReclaimQueue == LW_OBJECT_HANDLE_INVALID) {
        return;
    }
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_THREAD_RECLAIM_STK_SIZE,
                        LW_PRIO_T_RECLAIM,
                        LW_CFG_RECLAIM_OPTION | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);
    
    API_ThreadCreate("t_reclaim", __resReclaimThread, &threadattr, LW_NULL);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
