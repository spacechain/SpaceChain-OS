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
** 文   件   名: ThreadCancelWatchDog.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 02 日
**
** 描        述: 取消线程自己的看门狗

** BUG
2008.03.29  开始加入新的扫描链机制.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadCancelWatchDog
** 功能描述: 取消线程自己的看门狗
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0

LW_API  
VOID  API_ThreadCancelWatchDog (VOID)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    if (ptcbCur->TCB_bWatchDogInQ) {
        __DEL_FROM_WATCHDOG_LINE(ptcbCur);                              /*  从扫描链表中删除            */
        ptcbCur->TCB_ulWatchDog = 0ul;
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_CANCELWD, 
                      ptcbCur->TCB_ulId, LW_NULL);
}

#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
/*********************************************************************************************************
  END
*********************************************************************************************************/
