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
** 文   件   名: ThreadStackCheck.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 线程主堆栈使用量检查

** BUG
2007.04.12  允许检查空闲线程堆栈
2007.07.19  加入 _DebugHandle() 功能
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.06  使用新的可配置标志来判断空闲堆栈区, 力求更加准确.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadStackCheck
** 功能描述: 线程主堆栈使用量检查
** 输　入  : 
**           ulId                          线程ID
**           pstFreeByteSize               空闲堆栈大小   (可为 LW_NULL)
**           pstUsedByteSize               使用堆栈大小   (可为 LW_NULL)
**           pstTcbByteSize                线程控制块大小 (可为 LW_NULL)
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadStackCheck (LW_OBJECT_HANDLE  ulId,
                             size_t           *pstFreeByteSize,
                             size_t           *pstUsedByteSize,
                             size_t           *pstTcbByteSize)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
    REGISTER size_t                stTotal;
    REGISTER size_t                stFree = 0;
    
    REGISTER PLW_STACK             pstkButtom;
	
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  允许获得空闲线程堆栈信息    */
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
    
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_STK_CHK) {
        stTotal = ptcb->TCB_stStackSize;                                /*  总大小                      */
        
#if CPU_STK_GROWTH == 0                                                 /*  寻找堆栈头尾                */
        for (pstkButtom = ptcb->TCB_pstkStackBottom;
             *pstkButtom == _K_stkFreeFlag;
             pstkButtom--,
             stFree++);
#else
        for (pstkButtom = ptcb->TCB_pstkStackBottom;
             *pstkButtom == _K_stkFreeFlag;
             pstkButtom++,
             stFree++);
#endif
        __KERNEL_EXIT();                                                /*  退出内核                    */
        
        if (pstFreeByteSize) {
            *pstFreeByteSize = stFree * sizeof(LW_STACK);
        }
        
        if (pstUsedByteSize) {
            *pstUsedByteSize = (stTotal - stFree) * sizeof(LW_STACK);
        }
        
        if (pstTcbByteSize) {
            *pstTcbByteSize = sizeof(LW_CLASS_TCB);
        }
        return  (ERROR_NONE);
    
    } else {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not has this opt.\r\n");
        _ErrorHandle(ERROR_THREAD_OPTION);
        return  (ERROR_THREAD_OPTION);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
