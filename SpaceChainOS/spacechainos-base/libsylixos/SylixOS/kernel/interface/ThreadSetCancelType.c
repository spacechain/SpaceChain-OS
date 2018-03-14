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
** 文   件   名: ThreadSetCancelType.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 07 日
**
** 描        述: 设置线程取消的类型. 可以是异步或者是同步.

** BUG:
2011.02.24  不再对信号掩码集进行操作.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.01,10  首先检查如果有删除请求则删除.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
/*********************************************************************************************************
** 函数名称: API_ThreadSetCancelType
** 功能描述: 设置当前线程被动取消时的动作, 
**           可以为 PTHREAD_CANCEL_ASYNCHRONOUS or PTHREAD_CANCEL_DEFERRED
** 输　入  : 
**           iNewType           新类型
**           piOldType          先早类型
** 输　出  : ERRNO
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_ThreadSetCancelType (INT  iNewType, INT  *piOldType)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;

    if (iNewType != LW_THREAD_CANCEL_ASYNCHRONOUS &&
        iNewType != LW_THREAD_CANCEL_DEFERRED) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    if (ptcbCur->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE   &&
        ptcbCur->TCB_iCancelType  == LW_THREAD_CANCEL_DEFERRED &&
        (ptcbCur->TCB_bCancelRequest)) {                                /*  需要删除                    */
        LW_OBJECT_HANDLE    ulId = ptcbCur->TCB_ulId;
        
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadDelete(&ulId, LW_THREAD_CANCELED);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
        return  (ERROR_NONE);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    if (piOldType) {
        *piOldType = ptcbCur->TCB_iCancelType;
    }
    ptcbCur->TCB_iCancelType = iNewType;
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
