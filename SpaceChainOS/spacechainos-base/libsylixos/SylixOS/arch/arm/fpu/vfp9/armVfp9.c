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
** 文   件   名: armVfp9.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系架构 VFP9 (VFPv2 for ARM9) 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARM9 体系构架
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../armFpu.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static ARM_FPU_OP   _G_fpuopVfp9;
static INT          _G_iVfp9DNum;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern ULONG armVfp9Sid(VOID);
extern VOID  armVfp9Enable(VOID);
extern VOID  armVfp9Disable(VOID);
extern BOOL  armVfp9IsEnable(VOID);
extern VOID  armVfp9Save16(PVOID pvFpuCtx);
extern VOID  armVfp9Restore16(PVOID pvFpuCtx);
extern VOID  armVfp9Save32(PVOID pvFpuCtx);
extern VOID  armVfp9Restore32(PVOID pvFpuCtx);
/*********************************************************************************************************
** 函数名称: armVfp9CtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armVfp9CtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    INT i;
    
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    
    fdprintf(iFd, "FPSID   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpsid);
    fdprintf(iFd, "FPSCR   = 0x%08x  ", pcpufpuCtx->FPUCTX_uiFpscr);
    fdprintf(iFd, "FPEXC   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpexc);
    fdprintf(iFd, "FPINST  = 0x%08x  ", pcpufpuCtx->FPUCTX_uiFpinst);
    fdprintf(iFd, "FPINST2 = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpinst2);
    fdprintf(iFd, "MFVFR0  = 0x%08x  ", pcpufpuCtx->FPUCTX_uiMfvfr0);
    fdprintf(iFd, "MFVFR1  = 0x%08x\n", pcpufpuCtx->FPUCTX_uiMfvfr1);

    for (i = 0; i < _G_iVfp9DNum; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: armVfp9PrimaryInit
** 功能描述: 初始化并获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : 操作函数集
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PARM_FPU_OP  armVfp9PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    if (lib_strcmp(pcFpuName, ARM_FPU_VFP9_D16) == 0) {
        _G_iVfp9DNum = 32;
        _G_fpuopVfp9.AFPU_pfuncSave    = armVfp9Save16;
        _G_fpuopVfp9.AFPU_pfuncRestore = armVfp9Restore16;
    
    } else {
        _G_iVfp9DNum = 64;
        _G_fpuopVfp9.AFPU_pfuncSave    = armVfp9Save32;
        _G_fpuopVfp9.AFPU_pfuncRestore = armVfp9Restore32;
    }

    _G_fpuopVfp9.AFPU_pfuncHwSid    = armVfp9Sid;
    _G_fpuopVfp9.AFPU_pfuncEnable   = armVfp9Enable;
    _G_fpuopVfp9.AFPU_pfuncDisable  = armVfp9Disable;
    _G_fpuopVfp9.AFPU_pfuncIsEnable = armVfp9IsEnable;
    _G_fpuopVfp9.AFPU_pfuncCtxShow  = armVfp9CtxShow;

    return  (&_G_fpuopVfp9);
}
/*********************************************************************************************************
** 函数名称: armVfp9SecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armVfp9SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
