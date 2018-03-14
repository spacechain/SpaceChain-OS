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
** 文   件   名: ThreadVarAdd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 加入线程私有变量。(ULONG)

** BUG
2007.07.19  加入 _DebugHandle() 功能
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadVarAdd
** 功能描述: 加入线程私有变量, (多处理器无效)
** 输　入  : 
**           ulId            线程ID
**           pulAddr         私有变量地址
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

LW_API
ULONG  API_ThreadVarAdd (LW_OBJECT_HANDLE  ulId, ULONG  *pulAddr)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_THREADVAR   pthreadvar;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
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
    
    pthreadvar = _Allocate_ThreadVar_Object();                          /*  分配一个控制块              */
    
    if (!pthreadvar) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_VAR_FULL);
        return  (ERROR_THREAD_VAR_FULL);
    }
    
    pthreadvar->PRIVATEVAR_pulAddress  =  pulAddr;
    pthreadvar->PRIVATEVAR_ulValueSave = *pulAddr;
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
                                                                        /*  加入TCB上下文               */
    _List_Line_Add_Ahead(&pthreadvar->PRIVATEVAR_lineVarList, &ptcb->TCB_plinePrivateVars);
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VARS */
/*********************************************************************************************************
  END
*********************************************************************************************************/
