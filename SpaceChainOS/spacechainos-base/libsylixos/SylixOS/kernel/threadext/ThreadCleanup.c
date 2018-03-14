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
** 文   件   名: ThreadCleanup.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 04 日
**
** 描        述: 这是系统类 pthread_cleanup_??? 支持.

** BUG:
2011.04.23  内核分配内存失败, 将使用 _DebugHandle 打印错误信息.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadCleanupPush
** 功能描述: 将一个清除函数压入函数堆栈.
** 输　入  : pfuncRoutine      需要压栈的函数.
**           pvArg             压栈函数参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_THREAD_EXT_EN > 0

LW_API  
ULONG  API_ThreadCleanupPush (VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg)
{
             INTREG                 iregInterLevel;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER __PLW_CLEANUP_ROUTINE  pcurNode;
    REGISTER __PLW_THREAD_EXT       ptex;
    
    if (pfuncRoutine == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_HOOK_NULL);                           /*  cleanup hook null           */
        return  (ERROR_KERNEL_HOOK_NULL);
    }
    
    pcurNode = (__PLW_CLEANUP_ROUTINE)__KHEAP_ALLOC(sizeof(__LW_CLEANUP_ROUTINE));
    if (pcurNode == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  内核内存不足                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;
    
    pcurNode->CUR_pfuncClean = pfuncRoutine;
    pcurNode->CUR_pvArg      = pvArg;
    
    _LIST_MONO_LINK(&pcurNode->CUR_monoNext, ptex->TEX_pmonoCurHeader);
    ptex->TEX_pmonoCurHeader = &pcurNode->CUR_monoNext;
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadCleanupPushEx
** 功能描述: 将一个清除函数压入函数堆栈.
** 输　入  : ulId              线程句柄
**           pfuncRoutine      需要压栈的函数.
**           pvArg             压栈函数参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCleanupPushEx (LW_OBJECT_HANDLE  ulId, VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg)
{
             INTREG                 iregInterLevel;
    REGISTER UINT16                 usIndex;
    REGISTER PLW_CLASS_TCB          ptcb;
    
    REGISTER __PLW_CLEANUP_ROUTINE  pcurNode;
    REGISTER __PLW_THREAD_EXT       ptex;
    
    pcurNode = (__PLW_CLEANUP_ROUTINE)__KHEAP_ALLOC(sizeof(__LW_CLEANUP_ROUTINE));
    if (pcurNode == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  内核内存不足                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (pfuncRoutine == LW_NULL) {
        __KHEAP_FREE(pcurNode);
        _ErrorHandle(ERROR_KERNEL_HOOK_NULL);                           /*  cleanup hook null           */
        return  (ERROR_KERNEL_HOOK_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        __KHEAP_FREE(pcurNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_FALSE);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        __KHEAP_FREE(pcurNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (LW_FALSE);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        __KHEAP_FREE(pcurNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (LW_FALSE);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ptex = &ptcb->TCB_texExt;
    
    pcurNode->CUR_pfuncClean = pfuncRoutine;
    pcurNode->CUR_pvArg      = pvArg;
    
    _LIST_MONO_LINK(&pcurNode->CUR_monoNext, ptex->TEX_pmonoCurHeader);
    ptex->TEX_pmonoCurHeader = &pcurNode->CUR_monoNext;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核并打开中断          */
        
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadCleanupPop
** 功能描述: 将一个压栈函数运行并释放
** 输　入  : bRun          是否需要运行
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_ThreadCleanupPop (BOOL  bRun)
{
             INTREG                 iregInterLevel;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER __PLW_THREAD_EXT       ptex;
    REGISTER __PLW_CLEANUP_ROUTINE  pcurNode;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;
    
    pcurNode = (__PLW_CLEANUP_ROUTINE)ptex->TEX_pmonoCurHeader;
    if (pcurNode) {
        _list_mono_next(&ptex->TEX_pmonoCurHeader);
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        if (bRun) {
            LW_SOFUNC_PREPARE(pcurNode->CUR_pfuncClean);
            pcurNode->CUR_pfuncClean(pcurNode->CUR_pvArg);
        }
        __KHEAP_FREE(pcurNode);
    
    } else {
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
    }
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
