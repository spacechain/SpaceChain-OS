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
** 文   件   名: ThreadGetCPUUsage.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 获得线程CPU利用率

** BUG
2007.07.18  加入 _DebugHandle() 功能
2009.12.14  加入总的内核使用利用率测算.
2012.08.28  加入 API_ThreadGetCPUUsageAll() 函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadCPUUsageOn
** 功能描述: 启动 CPU 利用率测算
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0

LW_API
VOID  API_ThreadCPUUsageOn (VOID)
{
    __LW_TICK_CPUUSAGE_ENABLE();                                        /*  重新打开测量                */
}
/*********************************************************************************************************
** 函数名称: API_ThreadCPUUsageOff
** 功能描述: 关闭 CPU 利用率测算
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_ThreadCPUUsageOff (VOID)
{
    __LW_TICK_CPUUSAGE_DISABLE();
}
/*********************************************************************************************************
** 函数名称: API_ThreadCPUUsageIsOn
** 功能描述: 查看 CPU 利用率测算是否打开
** 输　入  : NONE
** 输　出  : 是否打开
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
BOOL  API_ThreadCPUUsageIsOn (VOID)
{
    return  (__LW_TICK_CPUUSAGE_ISENABLE());
}
/*********************************************************************************************************
** 函数名称: API_ThreadGetCPUUsage
** 功能描述: 获得线程CPU利用率 (千分率)
** 输　入  : ulId                          要检查的线程ID
**           puiThreadUsage                返回的指定线程的 CPU 利用率
**           puiCPUUsage                   CPU总利用率
**           puiKernelUsage                内核利用的 CPU 利用率
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetCPUUsage (LW_OBJECT_HANDLE  ulId, 
                              UINT             *puiThreadUsage,
                              UINT             *puiCPUUsage,
                              UINT             *puiKernelUsage)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER UINT                  uiTemp;
	
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
	
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
                                                                        /*  避免除 0 错误               */
    _K_ulCPUUsageTicks = (_K_ulCPUUsageTicks == 0) ? 1 : _K_ulCPUUsageTicks; 
    
    if (puiThreadUsage) {
        *puiThreadUsage = (UINT8)(((ptcb->TCB_ulCPUUsageTicks) * 1000) / _K_ulCPUUsageTicks);
    }
    
    if (puiCPUUsage) {                                                  /*  空闲线程利用率              */
        ptcb = _K_ptcbTCBIdTable[0];
        uiTemp = (UINT8)(((ptcb->TCB_ulCPUUsageTicks) * 1000) / _K_ulCPUUsageTicks);
        *puiCPUUsage = (UINT8)(1000 - uiTemp);
    }
    
    if (puiKernelUsage) {                                               /*  内核执行时间占总时间的利用率*/
        *puiKernelUsage = (UINT8)(((_K_ulCPUUsageKernelTicks) * 1000) / _K_ulCPUUsageTicks);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核并打开中断          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadGetCPUUsageAll
** 功能描述: 获得所有线程 CPU 利用率 (千分率)
** 输　入  : ulId[]                        要检查的线程ID
**           uiThreadUsage[]               返回的指定线程的 CPU 利用率
**           uiKernelUsage[]               内核利用的 CPU 利用率
**           iSize                         表格大小
** 输　出  : 获得的个数
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
INT  API_ThreadGetCPUUsageAll (LW_OBJECT_HANDLE  ulId[], 
                               UINT              uiThreadUsage[],
                               UINT              uiKernelUsage[],
                               INT               iSize)
{
    INT              i;
    INT              iIndex = 0;
    
    ULONG            ulCPUAllTicks;
    PLW_CLASS_TCB    ptcb;
    ULONG            ulCPUUsageTicks;
    ULONG            ulCPUUsageKernelTicks;
    
    REGISTER UINT    uiUsage;
    REGISTER UINT    uiUsageKernel;
    
    if (iSize == 0) {
        return  (iIndex);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    
    ulCPUAllTicks  = (_K_ulCPUUsageTicks == 0) ? 1 : _K_ulCPUUsageTicks;
    for (i = 0 ; i < LW_CFG_MAX_THREADS ; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  获得 TCB 控制块             */
        if (ptcb == LW_NULL) {                                          /*  线程不存在                  */
            continue;
        }
        
        ulCPUUsageTicks       = ptcb->TCB_ulCPUUsageTicks;
        ulCPUUsageKernelTicks = ptcb->TCB_ulCPUUsageKernelTicks;
        
        /*
         *  计算线程总的利用率, 保留一位小数
         */
        uiUsage = (UINT)((ulCPUUsageTicks * 10000) / ulCPUAllTicks);
        if ((uiUsage % 10) >= 5) {
            uiUsage = (uiUsage / 10) + 1;                               /*  五入                        */
        } else {
            uiUsage = (uiUsage / 10);                                   /*  四舍                        */
        }
        
        /*
         *  计算线程内核中的利用率, 保留一位小数
         */
        uiUsageKernel = (UINT)((ulCPUUsageKernelTicks * 10000) / ulCPUAllTicks);
        if ((uiUsageKernel % 10) >= 5) {
            uiUsageKernel = (uiUsageKernel / 10) + 1;                   /*  五入                        */
        } else {
            uiUsageKernel = (uiUsageKernel / 10);                       /*  四舍                        */
        }
        
        ulId[iIndex] = ptcb->TCB_ulId;
        uiThreadUsage[iIndex] = uiUsage;
        uiKernelUsage[iIndex] = uiUsageKernel;
        
        iIndex++;
        if (iIndex >= iSize) {
            break;
        }
    }
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (iIndex);
}

#endif                                                                  /*  LW_CFG_THREAD_CPU_USAGE...  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
