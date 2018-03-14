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
** 文   件   名: ThreadFeedWatchDog.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 02 日
**
** 描        述: 设置线程线程自己的看门狗定时器

** BUG
2008.03.29  开始加入新的扫描链机制.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadFeedWatchDog
** 功能描述: 设置线程自己的看门狗定时器
** 输　入  : 
**           ulWatchDogTicks               看门狗定时器定时长短值
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0

LW_API  
ULONG  API_ThreadFeedWatchDog (ULONG  ulWatchDogTicks)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    if (ulWatchDogTicks == 0) {                                         /*  需要停止看门狗              */
        API_ThreadCancelWatchDog();
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核同时关闭中断        */
    
    if (ptcbCur->TCB_bWatchDogInQ) {                                    /*  已经在扫描链中              */
        __DEL_FROM_WATCHDOG_LINE(ptcbCur);
    }
    
    ptcbCur->TCB_ulWatchDog = ulWatchDogTicks;
    
    __ADD_TO_WATCHDOG_LINE(ptcbCur);                                    /*  加入扫描链表                */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核同时打开中断        */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_FEEDWD, 
                      ptcbCur->TCB_ulId, ulWatchDogTicks, LW_NULL);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
/*********************************************************************************************************
  END
*********************************************************************************************************/
