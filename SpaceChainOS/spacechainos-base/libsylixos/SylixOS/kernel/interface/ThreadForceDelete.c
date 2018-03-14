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
** 文   件   名: ThreadForceDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 16 日
**
** 描        述: 线程强制删除函数。谨慎使用, 不管目标线程是否设置了安全标识, 都将删除.

** BUG
2008.01.16  在删除安全模式任务时, 打印相关 LOG 信息.
2008.03.29  加入对新的 wake up 和 watch dog 机制的处理.
2008.03.30  使用新的就绪环操作.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.02.03  加入销毁 TCB 扩展区.
2009.05.08  加入 _CleanupPopTCBExt().
2009.05.24  将 TCB_bIsInDeleteProc 处理提前.
2009.07.31  _excJobAdd() 应该强制删除.
2011.02.24  不再使用线程深度睡眠的方法, 而是用僵死状态这种机制.
2012.12.08  加入资源管理器静态回调.
2012.12.23  更行 API_ThreadForceDelete() 算法, 
            如果是删除自身, 则将需要回调的函数运行结束后再使用 except 线程删除.
2013.08.23  任务删除回调执行更加安全.
            只能删除相同进程内的任务. 其他进程的任务需要使用信号删除.
2013.08.28  加入内核事件监控器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_THREAD_DEL_EN > 0
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern ULONG  __threadDelete(PLW_CLASS_TCB  ptcbDel, BOOL  bIsInSafe, 
                             PVOID  pvRetVal, BOOL  bIsAlreadyWaitDeath);
/*********************************************************************************************************
** 函数名称: API_ThreadForceDelete
** 功能描述: 线程强制删除函数
** 输　入  : 
**           pulId         句柄
**           pvRetVal      返回值   (返回给 JOIN 的线程)
** 输　出  :
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadForceDelete (LW_OBJECT_HANDLE  *pulId, PVOID  pvRetVal)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcbDel;
    REGISTER LW_OBJECT_HANDLE      ulId;
    REGISTER BOOL                  bIsInSafeMode;                       /*  检测线程是否在安全模式      */
             ULONG                 ulError;
             
#if LW_CFG_MODULELOADER_EN > 0
             LW_LD_VPROC          *pvprocCur;
             LW_LD_VPROC          *pvprocDel;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统没有启动不能调用        */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system not running.\r\n");
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
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
    
    ptcbDel = _K_ptcbTCBIdTable[usIndex];
    
#if LW_CFG_MODULELOADER_EN > 0
    pvprocCur = __LW_VP_GET_TCB_PROC(ptcbCur);
    pvprocDel = __LW_VP_GET_TCB_PROC(ptcbDel);
    if (pvprocCur != pvprocDel) {                                       /*  不属于一个进程              */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not in same process.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {
        if (ptcbCur != ptcbDel) {                                       /*  主线程只能自己删除自己      */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not delete main thread.\r\n");
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    if (ptcbDel->TCB_iDeleteProcStatus) {                               /*  检查是否在删除过程中        */
        if (ptcbDel == ptcbCur) {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            for (;;) {
                API_TimeSleep(__ARCH_ULONG_MAX);                        /*  等待被删除                  */
            }
            return  (ERROR_NONE);                                       /*  永远运行不到这里            */
        
        } else {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(ERROR_THREAD_OTHER_DELETE);
            return  (ERROR_THREAD_OTHER_DELETE);
        }
    }
    
    if (ptcbDel->TCB_ulThreadSafeCounter) {                             /*  检测是否处于安全模式        */
        bIsInSafeMode = LW_TRUE;
    } else {
        bIsInSafeMode = LW_FALSE;
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    if (pvprocDel && (pvprocDel->VP_ulMainThread == ulId)) {            /*  主线程自己删除自己          */
        if (pvprocDel->VP_iStatus != __LW_VP_EXIT) {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            vprocExit(pvprocDel, ulId, (INT)pvRetVal);                  /*  进程退出, 此函数不返回      */
            return  (ERROR_NONE);                                       /*  不会运行到这里              */
        }
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    ptcbDel->TCB_iDeleteProcStatus = LW_TCB_DELETE_PROC_DEL;            /*  进入删除过程                */
    
    _ObjectCloseId(pulId);                                              /*  关闭 ID                     */
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (ptcbDel == ptcbCur) {
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  处理 hook 相关操作          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
        
        _excJobAdd((VOIDFUNCPTR)__threadDelete,
                   ptcbDel, (PVOID)bIsInSafeMode, pvRetVal, 
                   (PVOID)LW_FALSE, 0, 0);                              /*  使用信号系统的异常处理      */
        for (;;) {
            API_TimeSleep(__ARCH_ULONG_MAX);                            /*  等待被删除                  */
        }
        ulError = ERROR_NONE;
    
    } else {
        _ThreadDeleteWaitDeath(ptcbDel);                                /*  将要删除的线程进入僵死状态  */
        
        _ThreadDeleteProcHook(ptcbDel, pvRetVal);                       /*  处理 hook 相关操作          */
        
        MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_DELETE, 
                          ptcbDel->TCB_ulId, pvRetVal, LW_NULL);
        
        ulError = __threadDelete(ptcbDel, bIsInSafeMode, pvRetVal, LW_TRUE);
    }
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_THREAD_DEL_EN        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
