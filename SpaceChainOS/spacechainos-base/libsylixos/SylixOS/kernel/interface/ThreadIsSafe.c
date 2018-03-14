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
** 文   件   名: ThreadIsSafe.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 16 日
**
** 描        述: 获得线程的安全性状态.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadIsSafe
** 功能描述: 检测目标线程是否处于安全模式.
** 输　入  : 
**           ulId         线程句柄
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
BOOL    API_ThreadIsSafe (LW_OBJECT_HANDLE    ulId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	REGISTER BOOL                  bIsInSafeMode;
	
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_FALSE);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (LW_FALSE);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (LW_FALSE);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];                                  /*  获得 TCB 指针               */
    
    if (ptcb->TCB_ulThreadSafeCounter) {
        bIsInSafeMode = LW_TRUE;
    
    } else {
        bIsInSafeMode = LW_FALSE;
    }
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (bIsInSafeMode);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
