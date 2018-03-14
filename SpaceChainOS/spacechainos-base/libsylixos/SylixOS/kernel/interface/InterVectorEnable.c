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
** 文   件   名: InterVectorEnable.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 02 日
**
** 描        述: 使能指定向量的中断, 系统将响应该向量中断.

** BUG:
2013.08.28  加入内核事件监控器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_InterVectorEnable
** 功能描述: 使能指定向量的中断
** 输　入  : ulVector                      中断向量号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_InterVectorEnable (ULONG  ulVector)
{
    INTREG  iregInterLevel;

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slVectorTable, &iregInterLevel);
    __ARCH_INT_VECTOR_ENABLE(ulVector);
    LW_SPIN_UNLOCK_QUICK(&_K_slVectorTable, iregInterLevel);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_INT, MONITOR_EVENT_INT_VECT_EN, ulVector, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_InterVectorDisable
** 功能描述: 禁能指定向量的中断
** 输　入  : ulVector                      中断向量号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 如果 ulVector 为可嵌套中断, 则在中断上下文中禁能本中断, 退出中断时会自动打开. 
             所以禁能操作需要在任务状态下操作.

                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_InterVectorDisable (ULONG  ulVector)
{
    INTREG  iregInterLevel;
    
    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slVectorTable, &iregInterLevel);
    __ARCH_INT_VECTOR_DISABLE(ulVector);
    LW_SPIN_UNLOCK_QUICK(&_K_slVectorTable, iregInterLevel);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_INT, MONITOR_EVENT_INT_VECT_DIS, ulVector, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_InterVectorIsEnable
** 功能描述: 获得系统对指定向量中断响应状态
** 输　入  : ulVector                      中断向量号
**           pbIsEnable                    是否使能了相关中断
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_InterVectorIsEnable (ULONG  ulVector, BOOL  *pbIsEnable)
{
    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    if (!pbIsEnable) {
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (ERROR_KERNEL_MEMORY);
    }

    *pbIsEnable = __ARCH_INT_VECTOR_ISENABLE(ulVector);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
