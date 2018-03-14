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
** 文   件   名: armFpu.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "armFpu.h"
#include "vfpnone/armVfpNone.h"
#if defined(__SYLIXOS_ARM_ARCH_M__)
#include "v7m/armVfpV7M.h"
#else
#include "vfp9/armVfp9.h"
#include "vfp11/armVfp11.h"
#include "vfpv3/armVfpV3.h"
#include "vfpv4/armVfpV4.h"
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PARM_FPU_OP      _G_pfpuop;
/*********************************************************************************************************
** 函数名称: archFpuPrimaryInit
** 功能描述: 主核 Fpu 控制器初始化
** 输　入  : pcMachineName 机器名称
**           pcFpuName     fpu 名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU pri-core initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

#if defined(__SYLIXOS_ARM_ARCH_M__)
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  选择 VFP 架构               */
        _G_pfpuop = armVfpNonePrimaryInit(pcMachineName, pcFpuName);

    } else {
        _G_pfpuop = armVfpV7MPrimaryInit(pcMachineName, pcFpuName);
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));

    ARM_VFP_ENABLE(_G_pfpuop);

    ARM_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpscr = 0x01000000;     /*  Set FZ bit in VFP           */

    ARM_VFP_DISABLE(_G_pfpuop);

#else
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  选择 VFP 架构               */
        _G_pfpuop = armVfpNonePrimaryInit(pcMachineName, pcFpuName);
    
    } else if ((lib_strcmp(pcFpuName, ARM_FPU_VFP9_D16) == 0) ||
               (lib_strcmp(pcFpuName, ARM_FPU_VFP9_D32) == 0)) {
        _G_pfpuop = armVfp9PrimaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFP11) == 0) {
        _G_pfpuop = armVfp11PrimaryInit(pcMachineName, pcFpuName);
        
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv3) == 0) {
        _G_pfpuop = armVfpV3PrimaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4) == 0) {
        _G_pfpuop = armVfpV4PrimaryInit(pcMachineName, pcFpuName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
    
    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
    
    ARM_VFP_ENABLE(_G_pfpuop);
    
    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpsid = (UINT32)ARM_VFP_HW_SID(_G_pfpuop);
    ARM_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);
    
    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpexc = 0x00000000;     /*  disable VFP                 */
    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpscr = 0x01000000;     /*  Set FZ bit in VFP           */
                                                                        /*  Do not enable FPU           */
    ARM_VFP_DISABLE(_G_pfpuop);
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
}
/*********************************************************************************************************
** 函数名称: archFpuSecondaryInit
** 功能描述: 从核 Fpu 控制器初始化
** 输　入  : pcMachineName 机器名称
**           pcFpuName     fpu 名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  archFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU sec-core initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

#if defined(__SYLIXOS_ARM_ARCH_M__)
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  选择 VFP 架构               */
        armVfpNoneSecondaryInit(pcMachineName, pcFpuName);

    } else {
        armVfpV7MSecondaryInit(pcMachineName, pcFpuName);
    }

#else
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  选择 VFP 架构               */
        armVfpNoneSecondaryInit(pcMachineName, pcFpuName);
    
    } else if ((lib_strcmp(pcFpuName, ARM_FPU_VFP9_D16) == 0) ||
               (lib_strcmp(pcFpuName, ARM_FPU_VFP9_D32) == 0)) {
        armVfp9SecondaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFP11) == 0) {
        armVfp11SecondaryInit(pcMachineName, pcFpuName);
        
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv3) == 0) {
        armVfpV3SecondaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4) == 0) {
        armVfpV4SecondaryInit(pcMachineName, pcFpuName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: OSFpuInitCtx
** 功能描述: 初始化一个 Fpu 上下文控制块 (这里并没有使能 FPU)
** 输　入  : pvFpuCtx   FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuCtxInit (PVOID pvFpuCtx)
{
    lib_memcpy(pvFpuCtx, &_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
}
/*********************************************************************************************************
** 函数名称: archFpuEnable
** 功能描述: 使能 FPU.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuEnable (VOID)
{
    ARM_VFP_ENABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuDisable
** 功能描述: 禁能 FPU.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuDisable (VOID)
{
    ARM_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** 函数名称: archFpuSave
** 功能描述: 保存 FPU 上下文.
** 输　入  : pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuSave (PVOID pvFpuCtx)
{
    ARM_VFP_SAVE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuRestore
** 功能描述: 回复 FPU 上下文.
** 输　入  : pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuRestore (PVOID pvFpuCtx)
{
    ARM_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuCtxShow
** 功能描述: 显示 FPU 上下文.
** 输　入  : iFd       文件描述符
**           pvFpuCtx  FPU 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archFpuCtxShow (INT  iFd, PVOID pvFpuCtx)
{
    ARM_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
}
/*********************************************************************************************************
** 函数名称: archFpuUndHandle
** 功能描述: 系统发生 undef 异常时, 调用此函数. 
**           只有某个任务或者中断, 真正使用浮点运算时 (即运行到浮点运算指令产生异常)
**           这时才可以打开浮点运算库.
** 输　入  : ptcbCur   当前任务 TCB
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archFpuUndHandle (PLW_CLASS_TCB  ptcbCur)
{
    if (ARM_VFP_ISENABLE(_G_pfpuop)) {                                  /*  如果当前上下文 FPU 使能     */
        return  (PX_ERROR);                                             /*  此未定义指令与 FPU 无关     */
    }
    
    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    ARM_VFP_ENABLE(_G_pfpuop);                                          /*  使能 FPU                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
