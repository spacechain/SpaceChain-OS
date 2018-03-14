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
** 文   件   名: ThreadStop.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 02 日
**
** 描        述: 停止/恢复一个线程。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadStop
** 功能描述: 停止一个线程
** 输　入  : ulId            线程 ID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_ThreadStop (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             ULONG          ulError;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  允许获得空闲线程堆栈信息    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (ptcb->TCB_iDeleteProcStatus) {                                  /*  在删除和重启的过程中        */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ulError = _ThreadStop(ptcb);
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
#if LW_CFG_SMP_EN > 0
    {
        PLW_CLASS_TCB  ptcbCur;
        
        LW_TCB_GET_CUR_SAFE(ptcbCur);
        if (ptcbCur->TCB_uiStatusChangeReq) {                           /*  状态改变是否成功            */
            ptcbCur->TCB_uiStatusChangeReq = 0;
            ulError = ERROR_THREAD_NULL;                                /*  目标线程已经被删除或者重启  */
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** 函数名称: API_ThreadContinue
** 功能描述: 恢复一个被停止的线程
** 输　入  : ulId            线程 ID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_ThreadContinue (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             ULONG          ulError;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  允许获得空闲线程堆栈信息    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ulError = _ThreadContinue(ptcb, LW_FALSE);
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
