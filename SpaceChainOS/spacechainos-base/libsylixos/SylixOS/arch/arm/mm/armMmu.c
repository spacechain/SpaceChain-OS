/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: armMmu.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "mmu/v4/armMmuV4.h"
#include "mmu/v7/armMmuV7.h"
/*********************************************************************************************************
** 函数名称: archMmuInit
** 功能描述: 初始化 CACHE 
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archMmuInit (CPCHAR  pcMachineName)
{
    LW_MMU_OP *pmmuop = API_VmmGetLibBlock();
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_920)  == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_926)  == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_1136) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_1176) == 0)) {           /* ARMv4/v5/v6 兼容             */
        armMmuV4Init(pmmuop, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A7)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A8)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {     /* ARMv7/v8 兼容                */
        if (__SYLIXOS_ARM_ARCH__ < 7) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "machine name is NOT fix with "
                                               "compiler -mcpu or -march parameter.\r\n");
        }
        armMmuV7Init(pmmuop, pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
