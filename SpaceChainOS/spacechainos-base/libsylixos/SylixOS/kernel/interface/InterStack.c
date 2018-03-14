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
** 文   件   名: InterStack.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 02 日
**
** 描        述: 获得中断堆栈操作

** BUG
2007.04.12  LINE45 最大下标索引应该为：(LW_CFG_INT_STK_SIZE / __STACK_ALIGN_DIV) - 1
2009.04.11  加入多核的参数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  中断堆栈定义
*********************************************************************************************************/
extern LW_STACK     _K_stkInterruptStack[LW_CFG_MAX_PROCESSORS][LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)];
/*********************************************************************************************************
** 函数名称: API_InterStackBaseGet
** 功能描述: 获得中断堆栈栈顶 (当前调用者所在的 CPU, 关中断模式下被调用)
** 输　入  : NONE
** 输　出  : 中断栈顶
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID  API_InterStackBaseGet (VOID)
{
    return  (LW_CPU_GET_CUR()->CPU_pstkInterBase);
}
/*********************************************************************************************************
** 函数名称: API_InterStackCheck
** 功能描述: 获得中断堆栈使用量
** 输　入  : ulCPUId                       CPU 号
**           pstFreeByteSize               空闲堆栈大小   (可为 LW_NULL)
**           pstUsedByteSize               使用堆栈大小   (可为 LW_NULL)
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_InterStackCheck (ULONG   ulCPUId,
                           size_t *pstFreeByteSize,
                           size_t *pstUsedByteSize)
{
    REGISTER size_t                stFree = 0;
    REGISTER PLW_STACK             pstkButtom;
    
    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(ERROR_KERNEL_CPU_NULL);
        return;
    }
    
#if CPU_STK_GROWTH == 0
    for (pstkButtom = &_K_stkInterruptStack[ulCPUId][(LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)) - 1];
         ((*pstkButtom == _K_stkFreeFlag) && (stFree < LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)));
         pstkButtom--,
         stFree++);
#else
    for (pstkButtom = &_K_stkInterruptStack[ulCPUId][0];
         ((*pstkButtom == _K_stkFreeFlag) && (stFree < LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)));
         pstkButtom++,
         stFree++);
#endif

    if (pstFreeByteSize) {
        *pstFreeByteSize = stFree * sizeof(LW_STACK);
    }
    
    if (pstUsedByteSize) {
        *pstUsedByteSize = LW_CFG_INT_STK_SIZE - (stFree * sizeof(LW_STACK));
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
