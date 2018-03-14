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
** 文   件   名: InterTarget.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 05 月 26 日
**
** 描        述: 中断目标 CPU 设置.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0
/*********************************************************************************************************
** 函数名称: API_InterSetTarget
** 功能描述: 设置指定中断向量目标 CPU
** 输　入  : ulVector      中断向量号
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterSetTarget (ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    INTREG  iregInterLevel;
    ULONG   ulError;
    
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slVectorTable, &iregInterLevel);
    ulError = __ARCH_INT_VECTOR_SETTARGET(ulVector, stSize, pcpuset);
    LW_SPIN_UNLOCK_QUICK(&_K_slVectorTable, iregInterLevel);
    
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_InterGetTarget
** 功能描述: 获取指定中断向量目标 CPU
** 输　入  : ulVector      中断向量号
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterGetTarget (ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    INTREG  iregInterLevel;
    ULONG   ulError;
    
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slVectorTable, &iregInterLevel);
    ulError = __ARCH_INT_VECTOR_GETTARGET(ulVector, stSize, pcpuset);
    LW_SPIN_UNLOCK_QUICK(&_K_slVectorTable, iregInterLevel);
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
