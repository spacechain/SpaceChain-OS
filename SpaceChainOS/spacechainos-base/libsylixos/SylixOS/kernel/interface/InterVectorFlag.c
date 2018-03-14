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
** 文   件   名: InterVectorFlag.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 12 日
**
** 描        述: 设置/获取指定中断向量的特性.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_InterVectorSetFlag
** 功能描述: 设置指定中断向量的特性. 
** 输　入  : ulVector                      中断向量号
**           ulFlag                        特性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : LW_IRQ_FLAG_QUEUE 必须在安装任何一个驱动前设置, 且设置后不再能取消.
             最好放在 bspIntInit() 函数中完成设置.
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorSetFlag (ULONG  ulVector, ULONG  ulFlag)
{
    INTREG              iregInterLevel;
    PLW_CLASS_INTDESC   pidesc;

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    pidesc = LW_IVEC_GET_IDESC(ulVector);
    
    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  关闭中断同时锁住 spinlock   */
    
    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_QUEUE) {               /*  已经是 QUEUE 类型中断向量   */
        LW_IVEC_SET_FLAG(ulVector, ulFlag | LW_IRQ_FLAG_QUEUE);
    
    } else {
        LW_IVEC_SET_FLAG(ulVector, ulFlag);
    }
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  打开中断, 同时打开 spinlock */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_InterVectorGetFlag
** 功能描述: 获得指定中断向量的特性. 
** 输　入  : ulVector                      中断向量号
**           *pulFlag                      特性
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorGetFlag (ULONG  ulVector, ULONG  *pulFlag)
{
    INTREG              iregInterLevel;
    PLW_CLASS_INTDESC   pidesc;

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    if (!pulFlag) {
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (ERROR_KERNEL_MEMORY);
    }
    
    pidesc = LW_IVEC_GET_IDESC(ulVector);
    
    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  关闭中断同时锁住 spinlock   */
    
    *pulFlag = LW_IVEC_GET_FLAG(ulVector);
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  打开中断, 同时打开 spinlock */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
