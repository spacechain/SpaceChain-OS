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
** 文   件   名: ThreadGetSlice.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 获得指定线程时间片

** BUG
2007.07.18  加入 _DebugHandle() 功能
2009.12.30  加入获得时间片扩展接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadGetSliceEx
** 功能描述: 获得指定线程时间片(扩展接口)
** 输　入  : 
**           ulId            线程ID
**           pusSliceTemp    时间片
**           pusCounter      剩余时间片
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetSliceEx (LW_OBJECT_HANDLE  ulId, UINT16  *pusSliceTemp, UINT16  *pusCounter)
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
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
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

    if (pusSliceTemp) {
        *pusSliceTemp = ptcb->TCB_usSchedSlice;
    }
    if (pusCounter) {
        *pusCounter = ptcb->TCB_usSchedCounter;
    }
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadGetSlice
** 功能描述: 获得指定线程时间片
** 输　入  : 
**           ulId            线程ID
**           pusSliceTemp    时间片
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetSlice (LW_OBJECT_HANDLE  ulId, UINT16  *pusSliceTemp)
{
    return  (API_ThreadGetSliceEx(ulId, pusSliceTemp, LW_NULL));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
