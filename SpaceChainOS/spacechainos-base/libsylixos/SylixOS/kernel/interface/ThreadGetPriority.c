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
** 文   件   名: ThreadGetPriority.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 获得线程优先级。

** BUG
2007.07.18  加入 _DebugHandle() 功能
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.03.10  因为修正了 errno 系统, 一些 API 需要升级.
2012.08.28  允许获取 idle 线程
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadGetPriority
** 功能描述: 获得线程优先级。              中断中不可能发生优先级的转变，所以这里不必关中断
** 输　入  : 
**           ulId            线程ID
**           pucPriority     优先级
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_THREAD_CHANGE_PRIO_EN > 0

LW_API
ULONG  API_ThreadGetPriority (LW_OBJECT_HANDLE    ulId, UINT8  *pucPriority)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (pucPriority) {
        *pucPriority = ptcb->TCB_ucPriority;
    }
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CHANGE_PRIO_EN*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
