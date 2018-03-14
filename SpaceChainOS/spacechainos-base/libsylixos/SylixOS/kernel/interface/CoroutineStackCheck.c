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
** 文   件   名: CoroutineStackCheck.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 19 日
**
** 描        述: 这是协程管理库(协程是一个轻量级的并发执行单位). 
                 检测一个协程的堆栈使用量!!, 该协程的父系线程建立时必须使用 STK_CHK 选项.
** BUG:
2009.04.06  使用新的可配置标志来判断空闲堆栈区, 力求更加准确.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** 函数名称: API_CoroutineStackCheck
** 功能描述: 检测一个协程的堆栈使用量!!, 该协程的父系线程建立时必须使用 STK_CHK 选项.
** 输　入  : pvCrcb                        协程句柄
**           pstFreeByteSize               空闲堆栈大小   (可为 LW_NULL)
**           pstUsedByteSize               使用堆栈大小   (可为 LW_NULL)
**           pstCrcbByteSize               协程控制块大小 (可为 LW_NULL)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_CoroutineStackCheck (PVOID      pvCrcb,
                                 size_t    *pstFreeByteSize,
                                 size_t    *pstUsedByteSize,
                                 size_t    *pstCrcbByteSize)
{
    REGISTER PLW_CLASS_COROUTINE   pcrcb = (PLW_CLASS_COROUTINE)pvCrcb;
    REGISTER size_t                stTotal;
    REGISTER size_t                stFree = 0;
    
    REGISTER PLW_STACK             pstkButtom;

    if (!pcrcb) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine handle invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    stTotal = pcrcb->COROUTINE_stStackSize;                             /*  总大小                      */
    
#if CPU_STK_GROWTH == 0                                                 /*  寻找堆栈头尾                */
    for (pstkButtom = pcrcb->COROUTINE_pstkStackBottom;
         *pstkButtom == _K_stkFreeFlag;
         pstkButtom--,
         stFree++);
#else
    for (pstkButtom = pcrcb->COROUTINE_pstkStackBottom;
         *pstkButtom == _K_stkFreeFlag;
         pstkButtom++,
         stFree++);
#endif
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (pstFreeByteSize) {
        *pstFreeByteSize = stFree * sizeof(LW_STACK);
    }
    
    if (pstUsedByteSize) {
        *pstUsedByteSize = (stTotal - stFree) * sizeof(LW_STACK);
    }
    
    if (pstCrcbByteSize) {
        *pstCrcbByteSize = sizeof(LW_CLASS_COROUTINE);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
