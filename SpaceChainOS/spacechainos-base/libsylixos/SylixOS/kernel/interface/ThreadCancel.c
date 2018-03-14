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
** 文   件   名: ThreadCancel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 07 日
**
** 描        述: 取消一个指定的线程.

** BUG:
2011.02.24  不再对信号掩码集进行操作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#if LW_CFG_SIGNAL_EN > 0
#include "../SylixOS/system/signal/signal.h"
#endif
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
/*********************************************************************************************************
** 函数名称: API_ThreadCancel
** 功能描述: 取消一个指定的线程
** 输　入  : pulId     线程句柄
** 输　出  : ERROR_NONE or ESRCH
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCancel (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                iregInterLevel;
             LW_OBJECT_HANDLE      ulId;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbDel;

    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
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
    
    iregInterLevel = KN_INT_DISABLE();
    
    ptcbDel = _K_ptcbTCBIdTable[usIndex];
    if (ptcbDel->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE) {         /*  允许 CANCEL                 */
        if (ptcbDel->TCB_iCancelType == LW_THREAD_CANCEL_DEFERRED) {    /*  延迟 CANCEL                 */
            ptcbDel->TCB_bCancelRequest = LW_TRUE;                      /*  目标线程进入下一个 TEST 点  */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核并打开中断          */
        
        } else {                                                        /*  异步取消                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核并打开中断          */
#if LW_CFG_SIGNAL_EN > 0
            kill(ulId, SIGCANCEL);                                      /*  立即发送取消信号            */
#else
            return  (API_ThreadDelete(&ulId, LW_NULL));
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        }
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核并打开中断          */
        _ErrorHandle(ERROR_THREAD_DISCANCEL);                           /*  不允许 CACNCEL              */
        return  (ERROR_THREAD_DISCANCEL);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
