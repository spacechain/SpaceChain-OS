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
** 文   件   名: KernelFpu.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 19 日
**
** 描        述: 初始化内核浮点运算器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_KernelFpuPrimaryInit
** 功能描述: 初始化浮点运算器 (必须在 bsp 初始化回调中调用)
** 输　入  : pcMachineName 使用的处理器名称
**           pcFpuName     使用的 FPU 名称.
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

LW_API
VOID  API_KernelFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
#if LW_CFG_INTER_FPU > 0
    INT     i, j;
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
    
    if (LW_SYS_STATUS_GET()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel already running.\r\n");
        return;
    }

    archFpuPrimaryInit(pcMachineName, pcFpuName);                       /*  初始化 FPU 单元             */
    
#if LW_CFG_INTER_FPU > 0
    if (LW_KERN_FPU_EN_GET()) {                                         /*  中断状态允许使用 FPU        */
        __ARCH_FPU_ENABLE();                                            /*  这里需要在当前 FPU 上下文中 */
                                                                        /*  使能 FPU, 操作系统内核需要  */
    } else 
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
    {
        __ARCH_FPU_DISABLE();
    }
    
#if LW_CFG_INTER_FPU > 0
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        for (j = 0; j < LW_CFG_MAX_INTER_SRC; j++) {                    /*  初始化, 但是不使能 FPU      */
            __ARCH_FPU_CTX_INIT((PVOID)&(LW_CPU_GET(i)->CPU_fpuctxContext[j]));
        }
    }
#endif                                                                  /*  LW_CFG_INTER_FPU > 0        */
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "FPU initilaized.\r\n");
}
/*********************************************************************************************************
** 函数名称: API_KernelFpuSecondaryInit
** 功能描述: 初始化浮点运算器 (必须在 bsp 初始化回调中调用)
** 输　入  : pcMachineName 使用的处理器名称
**           pcFpuName     使用的 FPU 名称.
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API
VOID  API_KernelFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    archFpuSecondaryInit(pcMachineName, pcFpuName);
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "secondary FPU initilaized.\r\n");
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
