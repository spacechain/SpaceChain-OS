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
** 文   件   名: armCache.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#if defined(__SYLIXOS_ARM_ARCH_M__)
#include "cache/v7m/armCacheV7M.h"
#else
#include "cache/v4/armCacheV4.h"
#include "cache/v5/armCacheV5.h"
#include "cache/v6/armCacheV6.h"
#include "cache/v7/armCacheV7.h"
#include "cache/v8/armCacheV8.h"
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
** 函数名称: archCacheInit
** 功能描述: 初始化 CACHE 
** 输　入  : uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archCacheInit (CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName)
{
    LW_CACHE_OP *pcacheop = API_CacheGetLibBlock();
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L1 cache controller initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

#if defined(__SYLIXOS_ARM_ARCH_M__)
    armCacheV7MInit(pcacheop, uiInstruction, uiData, pcMachineName);

#else
    if (lib_strcmp(pcMachineName, ARM_MACHINE_920) == 0) {
        armCacheV4Init(pcacheop, uiInstruction, uiData, pcMachineName);
        
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_926) == 0) {
        armCacheV5Init(pcacheop, uiInstruction, uiData, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_1136) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_1176) == 0)) {
        armCacheV6Init(pcacheop, uiInstruction, uiData, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A7)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A8)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0)) {
        if (__SYLIXOS_ARM_ARCH__ < 7) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "machine name is NOT fix with "
                                               "compiler -mcpu or -march parameter.\r\n");
        }
        armCacheV7Init(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        if (__SYLIXOS_ARM_ARCH__ < 8) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "machine name is NOT fix with "
                                               "compiler -mcpu or -march parameter.\r\n");
        }
        armCacheV8Init(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_R4) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R5) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R7) == 0)) {
        armCacheV7Init(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
}
/*********************************************************************************************************
** 函数名称: archCacheReset
** 功能描述: 复位 CACHE, MMU 初始化时需要调用此函数
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archCacheReset (CPCHAR  pcMachineName)
{
#if defined(__SYLIXOS_ARM_ARCH_M__)
    armCacheV7MReset(pcMachineName);

#else
    if (lib_strcmp(pcMachineName, ARM_MACHINE_920) == 0) {
        armCacheV4Reset(pcMachineName);
        
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_926) == 0) {
        armCacheV5Reset(pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_1136) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_1176) == 0)) {
        armCacheV6Reset(pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A7)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A8)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0)) {
        armCacheV7Reset(pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        armCacheV8Reset(pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_R4) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R5) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R7) == 0)) {
        armCacheV7Reset(pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
