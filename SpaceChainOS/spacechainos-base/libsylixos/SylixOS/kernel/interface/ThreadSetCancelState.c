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
** 文   件   名: ThreadSetCancelState.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 07 日
**
** 描        述: 设置取消线程是否使能

** BUG:
2011.02.24  不再对信号掩码集进行操作.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
/*********************************************************************************************************
** 函数名称: API_ThreadSetCancelState
** 功能描述: 设置取消线程是否使能, 
** 输　入  : iNewState                     更新的状态
**           piOldState                    先早的状态
** 输　出  : EINVAL                        参数错误
**           0
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadSetCancelState (INT  iNewState, INT  *piOldState)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;

    if ((iNewState != LW_THREAD_CANCEL_DISABLE) && 
        (iNewState != LW_THREAD_CANCEL_ENABLE)) {
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    if (piOldState) {
        *piOldState = ptcbCur->TCB_iCancelState;
    }
    ptcbCur->TCB_iCancelState = iNewState;
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
        
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
