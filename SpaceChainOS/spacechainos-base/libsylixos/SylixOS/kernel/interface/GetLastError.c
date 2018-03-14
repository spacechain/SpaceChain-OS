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
** 文   件   名: GetLastError.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 用户可以调用这个 API 获得系统最近一个错误

** BUG
2007.03.22  加入系统没有启动时的错误处理机制
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2014.06.04  加入对 last error 的设置功能.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_GetLastError
** 功能描述: 获得系统最近一个错误
** 输　入  : 
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_GetLastError (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    ULONG           ulLastError;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断, 防止调度到其他 CPU*/
    
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        ulLastError = pcpuCur->CPU_ulInterError[pcpuCur->CPU_ulInterNesting];
    
    } else {
        ptcbCur = pcpuCur->CPU_ptcbTCBCur;
        if (ptcbCur) {
            ulLastError = ptcbCur->TCB_ulLastError;
        } else {
            ulLastError = _K_ulNotRunError;
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);

    return  (ulLastError);
}
/*********************************************************************************************************
** 函数名称: API_GetLastErrorEx
** 功能描述: 获得指定任务的 errno
** 输　入  : ulId      任务 ID
**           pulError  错误号
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_GetLastErrorEx (LW_OBJECT_HANDLE  ulId, ULONG  *pulError)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    
    if (!pulError) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    *pulError = ptcb->TCB_ulLastError;
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SetLastError
** 功能描述: 设置当前的 LastError
** 输　入  : ulError       当前错误号
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_SetLastError (ULONG  ulError)
{
    errno = (INT)ulError;
}
/*********************************************************************************************************
** 函数名称: API_SetLastErrorEx
** 功能描述: 设置指定任务的 errno
** 输　入  : ulId      任务 ID
**           ulError   错误号
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_SetLastErrorEx (LW_OBJECT_HANDLE  ulId, ULONG  ulError)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcb->TCB_ulLastError = ulError;
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
