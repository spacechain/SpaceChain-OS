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
** 文   件   名: ThreadVarDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 删除线程私有变量。(ULONG)

** BUG
2007.07.19  加入 _DebugHandle() 功能
2007.11.13  使用链表库对链表操作进行完全封装.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadVarDelete
** 功能描述: 删除线程私有变量。
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
ULONG  API_ThreadVarDelete (LW_OBJECT_HANDLE  ulId, ULONG  *pulAddr)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_LINE         plineVar;
    REGISTER PLW_CLASS_THREADVAR   pthreadvar;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
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
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    for (plineVar  = ptcb->TCB_plinePrivateVars;                        /*  查找                        */
         plineVar != LW_NULL;
         plineVar  = _list_line_get_next(plineVar)) {
         
        pthreadvar = _LIST_ENTRY(plineVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
        if (pthreadvar->PRIVATEVAR_pulAddress == pulAddr) {
            if (ptcb == ptcbCur) {
                *pulAddr = pthreadvar->PRIVATEVAR_ulValueSave;
            }
            _List_Line_Del(plineVar, &ptcb->TCB_plinePrivateVars);      /*  从 TCB 中解链               */
            _Free_ThreadVar_Object(pthreadvar);                         /*  释放控制块                  */
        
            __KERNEL_EXIT();                                            /*  退出内核                    */
            return  (ERROR_NONE);
        }
    }
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    _DebugHandle(__ERRORMESSAGE_LEVEL, "var is not in thread context.\r\n");
    _ErrorHandle(ERROR_THREAD_VAR_NOT_EXIST);
    return  (ERROR_THREAD_VAR_NOT_EXIST);
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VA...*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
