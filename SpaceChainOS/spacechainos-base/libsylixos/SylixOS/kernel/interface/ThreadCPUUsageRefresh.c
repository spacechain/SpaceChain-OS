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
** 文   件   名: ThreadCPUUsageRefresh.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 刷新线程CPU利用率

** BUG
2007.07.18  加入了 _DebugHandle() 功能
2007.11.13  使用链表库对链表操作进行完全封装.
2008.03.30  从关闭调度器改为关闭中断. 因为这些计数器是在中断中被修改的. 以此来保证准确性.
2009.09.18  使用新的算法, 极大的减少中断屏蔽时间.
2009.12.14  加入对内核时间的清零.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadCPUUsageRefresh
** 功能描述: 刷新线程CPU利用率
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0

LW_API  
ULONG  API_ThreadCPUUsageRefresh (VOID)
{
    REGISTER PLW_CLASS_TCB         ptcb;
             PLW_LIST_LINE         plineList;
             BOOL                  bNeedOn = LW_FALSE;
             
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统必须没有启动            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel is not running.\r\n");
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (__LW_TICK_CPUUSAGE_ISENABLE()) {
        __LW_TICK_CPUUSAGE_DISABLE();                                   /*  关闭测量                    */
        bNeedOn = LW_TRUE;
    }
    
    for (plineList  = _K_plineTCBHeader;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {
         
         ptcb = _LIST_ENTRY(plineList, LW_CLASS_TCB, TCB_lineManage);
         ptcb->TCB_ulCPUUsageTicks       = 0ul;
         ptcb->TCB_ulCPUUsageKernelTicks = 0ul;
    }
    
    _K_ulCPUUsageTicks       = 1ul;                                     /*  避免除 0 错误               */
    _K_ulCPUUsageKernelTicks = 0ul;
    
    if (bNeedOn) {
        __LW_TICK_CPUUSAGE_ENABLE();                                    /*  重新打开测量                */
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CPU_...       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
